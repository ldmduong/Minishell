#include "minishell/core/executor.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>

#include "minishell/builtins/scripting/scheduler.h"
#include "minishell/core/shell.h"

namespace minishell {
namespace {

bool redirect_fd(int source, int target) {
  if (dup2(source, target) < 0) {
    std::perror("dup2");
    return false;
  }
  return true;
}

bool apply_redirects(const Command &command) {
  if (!command.in_file.empty()) {
    int fd = open(command.in_file.c_str(), O_RDONLY);
    if (fd < 0) {
      std::perror(command.in_file.c_str());
      return false;
    }
    bool ok = redirect_fd(fd, STDIN_FILENO);
    close(fd);
    if (!ok)
      return false;
  }

  if (!command.out_file.empty()) {
    int flags = O_WRONLY | O_CREAT;
    flags |= command.append ? O_APPEND : O_TRUNC;

    int fd = open(command.out_file.c_str(), flags, 0644);
    if (fd < 0) {
      std::perror(command.out_file.c_str());
      return false;
    }
    bool ok = redirect_fd(fd, STDOUT_FILENO);
    close(fd);
    if (!ok)
      return false;
  }

  return true;
}

void close_all(const std::vector<int> &fds) {
  for (int fd : fds) {
    if (fd >= 0)
      close(fd);
  }
}

int exit_status(int status) {
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  return 0;
}

int wait_for(pid_t pid) {
  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno == EINTR)
      continue;
    std::perror("waitpid");
    return 1;
  }
  return exit_status(status);
}

int run_builtin_in_shell(Shell &shell, const Command &command) {
  if (command.in_file.empty() && command.out_file.empty())
    return shell.registry().run(shell, command.args);

  int saved_input = fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC, 3);
  int saved_output = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
  if (saved_input < 0 || saved_output < 0) {
    std::perror("fcntl");
    if (saved_input >= 0)
      close(saved_input);
    if (saved_output >= 0)
      close(saved_output);
    return 1;
  }

  std::cout.flush();
  int result = 1;
  if (apply_redirects(command)) {
    result = shell.registry().run(shell, command.args);
  }

  std::cout.flush();
  bool input_restored = dup2(saved_input, STDIN_FILENO) >= 0;
  bool output_restored = dup2(saved_output, STDOUT_FILENO) >= 0;
  if (!input_restored || !output_restored) {
    std::perror("dup2");
    result = 1;
  }
  close(saved_input);
  close(saved_output);
  return result;
}

void run_child(Shell &shell, const Pipeline &pipeline, size_t index,
               const std::vector<int> &fds) {
  signal(SIGINT, SIG_DFL);

  const Command &command = pipeline.commands[index];
  if (pipeline.background) {
    setpgid(0, 0);
    if (index == 0 && command.in_file.empty()) {
      int fd = open("/dev/null", O_RDONLY);
      if (fd < 0) {
        std::perror("/dev/null");
        _exit(1);
      }
      if (!redirect_fd(fd, STDIN_FILENO))
        _exit(1);
      close(fd);
    }
  }

  if (index > 0 && !redirect_fd(fds[(index - 1) * 2], STDIN_FILENO))
    _exit(1);
  if (index + 1 < pipeline.commands.size() &&
      !redirect_fd(fds[index * 2 + 1], STDOUT_FILENO)) {
    _exit(1);
  }
  close_all(fds);

  if (!apply_redirects(command))
    _exit(1);

  if (shell.registry().has(command.args[0])) {
    int result = shell.registry().run(shell, command.args);
    std::cout.flush();
    std::cerr.flush();
    _exit(result);
  }

  std::vector<char *> argv;
  for (const std::string &argument : command.args) {
    argv.push_back(const_cast<char *>(argument.c_str()));
  }
  argv.push_back(nullptr);
  execvp(argv[0], argv.data());

  int error = errno;
  if (error == ENOENT) {
    std::cerr << "minishell: " << command.args[0] << ": không tìm thấy lệnh\n";
  } else {
    std::cerr << "minishell: " << command.args[0] << ": "
              << std::strerror(error) << '\n';
  }
  std::cerr.flush();
  _exit(error == ENOENT ? 127 : 126);
}

} // namespace

int Executor::run(Shell &shell, const Pipeline &pipeline) {
  if (pipeline.commands.empty())
    return 0;
  for (const Command &command : pipeline.commands) {
    if (command.args.empty())
      return 2;
  }

  const Command &first = pipeline.commands.front();
  if (pipeline.commands.size() == 1 && !pipeline.background &&
      first.args[0] != "cat" && shell.registry().has(first.args[0])) {
    return run_builtin_in_shell(shell, first);
  }

  const size_t command_count = pipeline.commands.size();
  std::vector<int> fds(command_count > 1 ? 2 * (command_count - 1) : 0, -1);
  for (size_t i = 0; i + 1 < command_count; ++i) {
    if (pipe(&fds[2 * i]) < 0) {
      std::perror("pipe");
      close_all(fds);
      return 1;
    }
  }

  std::cout.flush();
  std::vector<pid_t> children;
  for (size_t i = 0; i < command_count; ++i) {
    pid_t child = fork();
    if (child == 0)
      run_child(shell, pipeline, i, fds);
    if (child < 0) {
      std::perror("fork");
      break;
    }
    children.push_back(child);
  }
  close_all(fds);

  if (pipeline.background) {
    if (!children.empty())
      std::cout << "[nền] pid " << children.back() << '\n';
    return children.size() == command_count ? 0 : 1;
  }

  int result = 0;
  for (size_t i = 0; i < children.size(); ++i) {
    int code = wait_for(children[i]);
    if (i + 1 == children.size())
      result = code;
  }
  return children.size() == command_count ? result : 1;
}

void Executor::reap_background() {
  int status = 0;
  pid_t child;
  while ((child = waitpid(-1, &status, WNOHANG)) > 0) {
    builtins::forget_scheduled(child);
    std::cout << "[xong] pid " << child << " (mã " << exit_status(status)
              << ")\n";
  }
}

} // namespace minishell

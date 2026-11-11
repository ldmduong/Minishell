#include "minishell/core/executor.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>
#include <vector>

#include "minishell/builtins/scripting/scheduler.h"
#include "minishell/core/shell.h"

namespace minishell {
namespace {

const pid_t shell_pid = getpid();

bool install_fd(int source, int target) {
  if (source < 0 || source == target)
    return true;
  bool installed = dup2(source, target) >= 0;
  if (!installed)
    std::perror("dup2");
  close(source);
  return installed;
}

int redirect_target(RedirectMode mode) {
  return mode == RedirectMode::read ? STDIN_FILENO : STDOUT_FILENO;
}

bool apply_redirects(const Command &command) {
  for (const Redirect &redirect : command.redirects) {
    int flags = O_RDONLY;
    if (redirect.mode != RedirectMode::read) {
      flags = O_WRONLY | O_CREAT |
              (redirect.mode == RedirectMode::append ? O_APPEND : O_TRUNC);
    }
    int fd = open(redirect.path.c_str(), flags, 0666);
    if (fd < 0) {
      std::perror(("minishell: '" + redirect.path + "'").c_str());
      return false;
    }
    if (!install_fd(fd, redirect_target(redirect.mode)))
      return false;
  }

  return true;
}

void close_fd(int fd) {
  if (fd >= 0)
    close(fd);
}

int exit_status(int status) {
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  if (WIFSIGNALED(status))
    return 128 + WTERMSIG(status);
  if (WIFSTOPPED(status))
    return 128 + WSTOPSIG(status);
  return 0;
}

void report_finished(pid_t child, int status) {
  builtins::forget_scheduled(child);
  std::cout << "[xong] pid " << child << " (mã " << exit_status(status)
            << ")\n";
}

pid_t wait_child(int &status, int options) {
  pid_t child;
  do {
    child = waitpid(-1, &status, options);
  } while (child < 0 && errno == EINTR);
  return child;
}

int wait_pipeline(std::set<pid_t> &children, pid_t last_child,
                  pid_t group, bool foreground_terminal, bool &stopped) {
  int result = 0;
  while (!children.empty()) {
    int status = 0;
    pid_t child = wait_child(status, WUNTRACED);
    if (child < 0) {
      std::perror("waitpid");
      return 1;
    }
    auto member = children.find(child);
    if (member == children.end()) {
      if (!WIFSTOPPED(status))
        report_finished(child, status);
      continue;
    }
    if (WIFSTOPPED(status)) {
      stopped = true;
      if (group > 0) {
        kill(-group, SIGSTOP);
      } else {
        for (pid_t pid : children)
          kill(pid, SIGSTOP);
      }
      std::cout << "[dừng] pid " << child << '\n';
      return exit_status(status);
    }
    if (foreground_terminal && WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)
      raise(SIGINT);
    if (child == last_child)
      result = exit_status(status);
    children.erase(member);
  }
  return result;
}

int flush_builtin(int result) {
  if (std::cout.flush())
    return result;
  std::cout.clear();
  std::clearerr(stdout);
  std::cerr << "minishell: lỗi ghi đầu ra\n";
  return result == 0 ? 1 : result;
}

int run_builtin_in_shell(Shell &shell, const Command &command) {
  if (command.redirects.empty())
    return flush_builtin(shell.registry().run(shell, command.args));

  int saved[] = {-1, -1};
  for (const Redirect &redirect : command.redirects) {
    int target = redirect_target(redirect.mode);
    if (saved[target] >= 0)
      continue;
    saved[target] = fcntl(target, F_DUPFD_CLOEXEC, 3);
    if (saved[target] < 0) {
      std::perror("fcntl");
      for (int fd : saved)
        close_fd(fd);
      return 1;
    }
  }

  int result = 1;
  if (apply_redirects(command)) {
    result = flush_builtin(shell.registry().run(shell, command.args));
  }

  for (int target : {STDIN_FILENO, STDOUT_FILENO}) {
    if (!install_fd(saved[target], target))
      result = 1;
  }
  return result;
}

[[noreturn]] void run_child(Shell &shell, const Command &command, int input,
                           const int next_pipe[2], bool background,
                           pid_t group, const int gate[2]) {
  signal(SIGINT, SIG_DFL);
  signal(SIGCHLD, SIG_DFL);
  signal(SIGTSTP, SIG_DFL);
  signal(SIGTTIN, SIG_DFL);
  signal(SIGTTOU, SIG_DFL);

  if (group >= 0 && setpgid(0, group) < 0) {
    std::perror("setpgid");
    _exit(1);
  }
  if (gate[0] >= 0) {
    close(gate[1]);
    char byte;
    ssize_t count;
    do {
      count = read(gate[0], &byte, 1);
    } while (count < 0 && errno == EINTR);
    close(gate[0]);
    if (count != 0)
      _exit(1);
  }

  close_fd(next_pipe[0]);
  if (input < 0 && background) {
    input = open("/dev/null", O_RDONLY);
    if (input < 0) {
      std::perror("/dev/null");
      _exit(1);
    }
  }
  if (!install_fd(input, STDIN_FILENO) ||
      !install_fd(next_pipe[1], STDOUT_FILENO) || !apply_redirects(command))
    _exit(1);

  if (shell.registry().has(command.args[0])) {
    int result = flush_builtin(shell.registry().run(shell, command.args));
    std::cerr.flush();
    _exit(result);
  }

  std::vector<char *> argv;
  argv.reserve(command.args.size() + 1);
  for (const std::string &argument : command.args) {
    argv.push_back(const_cast<char *>(argument.c_str()));
  }
  argv.push_back(nullptr);
  execvp(argv[0], argv.data());

  const int error = errno;
  const char *message =
      error == ENOENT ? "không tìm thấy lệnh" : std::strerror(error);
  std::cerr << "minishell: " << command.args[0] << ": " << message << '\n';
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

  const bool terminal = !pipeline.background && getpid() == shell_pid &&
                        tcgetpgrp(STDIN_FILENO) == getpgrp();
  termios terminal_modes{};
  bool saved_modes = terminal && tcgetattr(STDIN_FILENO, &terminal_modes) == 0;
  int gate[2] = {-1, -1};
  // Release foreground children only after their group owns the terminal.
  if (terminal && pipe2(gate, O_CLOEXEC) != 0) {
    std::perror("pipe");
    return 1;
  }

  std::cout.flush();
  std::set<pid_t> children;
  pid_t group = pipeline.background || terminal ? 0 : -1;
  pid_t last_child = 0;
  int input = -1;
  size_t stage = 0;
  for (; stage < pipeline.commands.size(); ++stage) {
    int next_pipe[2] = {-1, -1};
    if (stage + 1 < pipeline.commands.size() && pipe2(next_pipe, O_CLOEXEC) != 0) {
      std::perror("pipe");
      break;
    }
    pid_t child = fork();
    if (child == 0)
      run_child(shell, pipeline.commands[stage], input, next_pipe,
                pipeline.background, group, gate);
    close_fd(input);
    close_fd(next_pipe[1]);
    input = next_pipe[0];
    if (child < 0) {
      std::perror("fork");
      break;
    }
    children.insert(child);
    last_child = child;
    if (group == 0)
      group = child;
    if (group > 0 && setpgid(child, group) < 0 && errno != EACCES) {
      std::perror("setpgid");
      break;
    }
  }
  close_fd(input);
  close_fd(gate[0]);
  bool started = stage == pipeline.commands.size();
  if (started && terminal && tcsetpgrp(STDIN_FILENO, group) < 0) {
    std::perror("tcsetpgrp");
    started = false;
  }
  if (!started) {
    for (pid_t child : children)
      kill(child, SIGKILL);
  }
  close_fd(gate[1]);

  if (started && pipeline.background) {
    std::cout << "[nền] pid " << last_child << '\n';
    return 0;
  }

  bool stopped = false;
  int result = wait_pipeline(children, last_child, group, terminal, stopped);
  if (started && terminal) {
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
      std::perror("tcsetpgrp");
      return 1;
    }
    if (stopped && saved_modes)
      tcsetattr(STDIN_FILENO, TCSADRAIN, &terminal_modes);
  }
  return started ? result : 1;
}

bool Executor::reap_background(bool at_prompt) {
  int status = 0;
  bool reported = false;
  pid_t child;
  while ((child = wait_child(status, WNOHANG)) > 0) {
    if (at_prompt && !reported)
      std::cout << '\r' << "\033[K";
    report_finished(child, status);
    reported = true;
  }
  return reported;
}

} // namespace minishell

#include "minishell/builtins/scripting/scheduler.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

std::map<pid_t, std::time_t> scheduled;
const pid_t scheduler_owner = getpid();

int schedule_command(Shell &shell, const std::vector<std::string> &args) {
  if (getpid() != scheduler_owner) {
    std::cerr << "schedule: chỉ dùng như lệnh độc lập ở shell chính\n";
    return 1;
  }

  if (args.size() == 2 && args[1] == "list") {
    std::time_t now = std::time(nullptr);
    for (auto job = scheduled.begin(); job != scheduled.end();) {
      if (kill(job->first, 0) != 0 && errno == ESRCH) {
        job = scheduled.erase(job);
        continue;
      }
      std::cout << "PID " << job->first << " còn "
                << (job->second > now ? job->second - now : 0) << " giây\n";
      ++job;
    }
    return 0;
  }

  if (args.size() == 3 && args[1] == "cancel") {
    pid_t pid = 0;
    const char *begin = args[2].data();
    const char *end = begin + args[2].size();
    auto parsed = std::from_chars(begin, end, pid);
    auto job = parsed.ec == std::errc{} && parsed.ptr == end
                   ? scheduled.find(pid)
                   : scheduled.end();
    if (job == scheduled.end()) {
      std::cerr << "schedule: không tìm thấy lịch PID " << args[2] << '\n';
      return 1;
    }
    if (kill(pid, SIGTERM) != 0) {
      std::perror("schedule: cancel");
      return 1;
    }
    scheduled.erase(job);
    std::cout << "Đã hủy lịch PID " << pid << '\n';
    return 0;
  }

  if (args.size() < 3) {
    std::cerr << "cách dùng: schedule GIÂY LỆNH [ĐỐI_SỐ] | schedule list | "
                 "schedule cancel PID\n";
    return 1;
  }

  unsigned int delay = 0;
  const char *begin = args[1].data();
  const char *end = begin + args[1].size();
  auto parsed = std::from_chars(begin, end, delay);
  if (parsed.ec != std::errc{} || parsed.ptr != end || delay == 0 ||
      delay > 86400) {
    std::cerr << "schedule: thời gian phải từ 1 đến 86400 giây\n";
    return 1;
  }
  if (shell.registry().has(args[2])) {
    std::cerr << "schedule: chỉ hỗ trợ lệnh ngoài, không hỗ trợ builtin\n";
    return 1;
  }

  std::cout.flush();
  pid_t child = fork();
  if (child < 0) {
    std::perror("schedule: fork");
    return 1;
  }
  if (child == 0) {
    setpgid(0, 0);
    signal(SIGINT, SIG_IGN);
    int input = open("/dev/null", O_RDONLY);
    if (input < 0 || (input != STDIN_FILENO && dup2(input, STDIN_FILENO) < 0)) {
      std::perror("schedule: stdin");
      _exit(1);
    }
    if (input != STDIN_FILENO)
      close(input);
    unsigned int remaining = delay;
    while (remaining > 0)
      remaining = sleep(remaining);

    signal(SIGINT, SIG_DFL);
    std::vector<char *> command;
    for (size_t i = 2; i < args.size(); ++i)
      command.push_back(const_cast<char *>(args[i].c_str()));
    command.push_back(nullptr);
    execvp(command[0], command.data());
    int error = errno;
    std::cerr << "schedule: " << args[2] << ": " << std::strerror(error)
              << '\n';
    std::cerr.flush();
    _exit(error == ENOENT ? 127 : 126);
  }

  scheduled[child] = std::time(nullptr) + delay;
  std::cout << "[schedule] PID " << child << " sau " << delay << " giây\n";
  return 0;
}

} // namespace

void register_scheduler(Registry &registry) {
  registry.add("schedule", schedule_command,
               "schedule GIÂY LỆNH | schedule list | schedule cancel PID");
}

} // namespace minishell::builtins
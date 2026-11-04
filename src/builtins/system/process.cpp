#include "minishell/builtins/system/process.h"

#include <dirent.h>
#include <signal.h>
#include <sys/types.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

struct Process {
  int pid;
  char state;
  std::string name;
};

bool read_process(int pid, Process &process) {
  std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
  std::string line;
  if (!std::getline(file, line))
    return false;

  size_t name_start = line.find('(');
  size_t name_end = line.rfind(')');
  if (name_start == std::string::npos || name_end == std::string::npos ||
      name_end + 2 >= line.size()) {
    return false;
  }

  process = {pid, line[name_end + 2],
             line.substr(name_start + 1, name_end - name_start - 1)};
  return true;
}

std::string state_name(char state) {
  switch (state) {
  case 'R':
    return "chạy";
  case 'S':
    return "ngủ";
  case 'D':
    return "chờ I/O";
  case 'T':
    return "dừng";
  case 't':
    return "debug";
  case 'Z':
    return "zombie";
  case 'I':
    return "rỗi";
  default:
    return std::string(1, state);
  }
}

int procs_command(Shell &, const std::vector<std::string> &) {
  DIR *processes = opendir("/proc");
  if (processes == nullptr) {
    std::perror("procs");
    return 1;
  }

  std::cout << std::left << std::setw(8) << "PID" << std::setw(12)
            << "TRẠNG THÁI" << "TÊN\n";

  while (dirent *entry = readdir(processes)) {
    if (entry->d_name[0] < '0' || entry->d_name[0] > '9')
      continue;

    Process process{};
    if (!read_process(std::atoi(entry->d_name), process))
      continue;
    std::cout << std::left << std::setw(8) << process.pid << std::setw(12)
              << state_name(process.state) << process.name << '\n';
  }

  closedir(processes);
  return 0;
}

int signal_processes(const std::vector<std::string> &args, size_t first,
                     int signal_number, const char *command) {
  if (first == args.size()) {
    std::cerr << command << ": thiếu PID\n";
    return 1;
  }

  int result = 0;
  for (size_t i = first; i < args.size(); ++i) {
    char *end = nullptr;
    errno = 0;
    long pid = std::strtol(args[i].c_str(), &end, 10);
    if (errno != 0 || end == args[i].c_str() || *end != '\0' || pid <= 0 ||
        pid > std::numeric_limits<pid_t>::max()) {
      std::cerr << command << ": PID không hợp lệ: " << args[i] << '\n';
      result = 1;
      continue;
    }

    if (kill(static_cast<pid_t>(pid), signal_number) != 0) {
      std::cerr << command << ": " << pid << ": " << std::strerror(errno)
                << '\n';
      result = 1;
    }
  }
  return result;
}

int kill_command(Shell &, const std::vector<std::string> &args) {
  int signal_number = SIGTERM;
  size_t first_pid = 1;

  if (args.size() > 1 && args[1].size() > 1 && args[1][0] == '-') {
    char *end = nullptr;
    errno = 0;
    long value = std::strtol(args[1].c_str() + 1, &end, 10);
    if (errno != 0 || end == args[1].c_str() + 1 || *end != '\0' ||
        value <= 0 || value > 64) {
      std::cerr << "kill: tín hiệu không hợp lệ: " << args[1] << '\n';
      return 1;
    }
    signal_number = static_cast<int>(value);
    first_pid = 2;
  }

  return signal_processes(args, first_pid, signal_number, "kill");
}

int stop_command(Shell &, const std::vector<std::string> &args) {
  return signal_processes(args, 1, SIGSTOP, "stop");
}

int resume_command(Shell &, const std::vector<std::string> &args) {
  return signal_processes(args, 1, SIGCONT, "resume");
}

} // namespace

void register_process(Registry &registry) {
  registry.add("procs", procs_command, "procs  liệt kê tiến trình (đọc /proc)");
  registry.add("kill", kill_command,
               "kill [-SIG] PID...  gửi tín hiệu (mặc định SIGTERM)");
  registry.add("stop", stop_command,
               "stop PID...  tạm dừng tiến trình (SIGSTOP)");
  registry.add("resume", resume_command,
               "resume PID...  tiếp tục tiến trình (SIGCONT)");
}

} // namespace minishell::builtins

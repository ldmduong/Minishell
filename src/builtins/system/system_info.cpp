#include "minishell/builtins/system/system_info.h"

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

bool read_field(const std::string &path, const std::string &key,
                std::string &value) {
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    if (line.rfind(key, 0) != 0)
      continue;

    size_t start = key.size();
    while (start < line.size() &&
           (line[start] == ' ' || line[start] == '\t' || line[start] == ':')) {
      ++start;
    }
    value = line.substr(start);
    return true;
  }
  return false;
}

int ram_command(Shell &, const std::vector<std::string> &) {
  std::string total_text;
  std::string available_text;
  if (!read_field("/proc/meminfo", "MemTotal:", total_text) ||
      !read_field("/proc/meminfo", "MemAvailable:", available_text)) {
    std::cerr << "ram: không đọc được /proc/meminfo\n";
    return 1;
  }

  long long total = std::strtoll(total_text.c_str(), nullptr, 10) / 1024;
  long long available =
      std::strtoll(available_text.c_str(), nullptr, 10) / 1024;
  long long used = total - available;
  std::cout << "RAM tổng:      " << total << " MB\n"
            << "RAM đã dùng:   " << used << " MB\n"
            << "RAM còn trống: " << available << " MB\n";
  if (total > 0)
    std::cout << "Tỷ lệ dùng:    " << used * 100 / total << "%\n";
  return 0;
}

int cpu_command(Shell &, const std::vector<std::string> &) {
  std::string model;
  if (!read_field("/proc/cpuinfo", "model name", model))
    model = "không rõ";

  long cores = sysconf(_SC_NPROCESSORS_ONLN);
  double load[3] = {};
  getloadavg(load, 3);

  std::cout << "CPU:      " << model << '\n'
            << "Số nhân:  " << cores << '\n'
            << std::fixed << std::setprecision(2)
            << "Tải (1/5/15 phút): " << load[0] << ' ' << load[1] << ' '
            << load[2] << '\n';
  return 0;
}

int uptime_command(Shell &, const std::vector<std::string> &) {
  std::ifstream file("/proc/uptime");
  double seconds;
  if (!(file >> seconds)) {
    std::cerr << "uptime: không đọc được /proc/uptime\n";
    return 1;
  }

  long long total_seconds = static_cast<long long>(seconds);
  std::cout << "Đã chạy: " << total_seconds / 86400 << " ngày "
            << (total_seconds % 86400) / 3600 << " giờ "
            << (total_seconds % 3600) / 60 << " phút " << total_seconds % 60
            << " giây\n";
  return 0;
}

} // namespace

void register_system_info(Registry &registry) {
  registry.add("ram", ram_command, "ram  xem bộ nhớ RAM");
  registry.add("cpu", cpu_command, "cpu  xem CPU và tải hệ thống");
  registry.add("uptime", uptime_command, "uptime  thời gian hệ thống đã chạy");
}

} // namespace minishell::builtins

#include "minishell/builtins/system/system_info.h"

#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

// Tìm dòng bắt đầu bằng key trong file, lấy phần giá trị phía sau
// (bỏ qua khoảng trắng, tab và dấu ':' ngay sau key).
bool read_field(const std::string& file, const std::string& key, std::string& out) {
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind(key, 0) != 0) continue;

        size_t p = key.size();
        while (p < line.size() && (line[p] == ' ' || line[p] == '\t' || line[p] == ':')) ++p;
        out = line.substr(p);
        return true;
    }
    return false;
}

int cmd_ram(Shell&, const std::vector<std::string>&) {
    std::string total_s, avail_s;
    if (!read_field("/proc/meminfo", "MemTotal:", total_s) ||
        !read_field("/proc/meminfo", "MemAvailable:", avail_s)) {
        std::cerr << "ram: không đọc được /proc/meminfo\n";
        return 1;
    }

    // /proc/meminfo tính bằng kB, đổi sang MB
    long long total = std::strtoll(total_s.c_str(), nullptr, 10) / 1024;
    long long avail = std::strtoll(avail_s.c_str(), nullptr, 10) / 1024;
    long long used = total - avail;

    std::cout << "RAM tổng:      " << total << " MB\n"
              << "RAM đã dùng:   " << used << " MB\n"
              << "RAM còn trống: " << avail << " MB\n";
    if (total > 0) {
        std::cout << "Tỷ lệ dùng:    " << used * 100 / total << "%\n";
    }
    return 0;
}

int cmd_cpu(Shell&, const std::vector<std::string>&) {
    std::string model;
    if (!read_field("/proc/cpuinfo", "model name", model)) model = "không rõ";

    long cores = sysconf(_SC_NPROCESSORS_ONLN);

    double load[3] = {0, 0, 0};
    getloadavg(load, 3);

    std::cout << "CPU:      " << model << "\n"
              << "Số nhân:  " << cores << "\n"
              << std::fixed << std::setprecision(2)
              << "Tải (1/5/15 phút): " << load[0] << " " << load[1] << " " << load[2] << "\n";
    return 0;
}

int cmd_uptime(Shell&, const std::vector<std::string>&) {
    std::ifstream in("/proc/uptime");
    double secs = 0;
    if (!(in >> secs)) {
        std::cerr << "uptime: không đọc được /proc/uptime\n";
        return 1;
    }

    long long s = static_cast<long long>(secs);
    std::cout << "Đã chạy: " << s / 86400 << " ngày "
              << (s % 86400) / 3600 << " giờ "
              << (s % 3600) / 60 << " phút "
              << s % 60 << " giây\n";
    return 0;
}

}  // namespace

void register_system_info(Registry& registry) {
    registry.add("ram", cmd_ram, "ram  xem bộ nhớ RAM");
    registry.add("cpu", cmd_cpu, "cpu  xem CPU và tải hệ thống");
    registry.add("uptime", cmd_uptime, "uptime  thời gian hệ thống đã chạy");
}

}  // namespace minishell::builtins
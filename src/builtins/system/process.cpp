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
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

struct ProcInfo {
    int pid = 0;
    char state = '?';
    std::string name;
};

// Đọc /proc/<pid>/stat, có dạng: "pid (tên) trạng_thái ...".
// Tên có thể chứa khoảng trắng hoặc dấu ngoặc, nên tìm dấu ')' cuối cùng.
bool read_proc(int pid, ProcInfo& info) {
    std::ifstream in("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!std::getline(in, line)) return false;

    size_t open = line.find('(');
    size_t close = line.rfind(')');
    if (open == std::string::npos || close == std::string::npos || close + 2 >= line.size()) {
        return false;
    }

    info.pid = pid;
    info.name = line.substr(open + 1, close - open - 1);
    info.state = line[close + 2];
    return true;
}

std::string state_name(char s) {
    switch (s) {
        case 'R': return "chạy";
        case 'S': return "ngủ";
        case 'D': return "chờ I/O";
        case 'T': return "dừng";
        case 't': return "debug";
        case 'Z': return "zombie";
        case 'I': return "rỗi";
        default:  return std::string(1, s);
    }
}

int cmd_procs(Shell&, const std::vector<std::string>&) {
    DIR* dir = opendir("/proc");
    if (!dir) {
        std::perror("procs");
        return 1;
    }

    std::cout << std::left << std::setw(8) << "PID"
              << std::setw(12) << "TRẠNG THÁI" << "TÊN\n";

    while (dirent* ent = readdir(dir)) {
        if (ent->d_name[0] < '0' || ent->d_name[0] > '9') continue;  // chỉ lấy thư mục số

        ProcInfo info;
        if (!read_proc(std::atoi(ent->d_name), info)) continue;

        std::cout << std::left << std::setw(8) << info.pid
                  << std::setw(12) << state_name(info.state)
                  << info.name << "\n";
    }

    closedir(dir);
    return 0;
}

// Gửi tín hiệu tới các PID bắt đầu từ args[first].
int send_signal(const std::vector<std::string>& args, size_t first, int sig, const char* cmd) {
    if (first >= args.size()) {
        std::cerr << cmd << ": thiếu PID\n";
        return 1;
    }

    int code = 0;
    for (size_t i = first; i < args.size(); ++i) {
        char* end = nullptr;
        long pid = std::strtol(args[i].c_str(), &end, 10);
        if (*end != '\0' || pid <= 0) {
            std::cerr << cmd << ": PID không hợp lệ: " << args[i] << "\n";
            code = 1;
            continue;
        }
        if (kill(static_cast<pid_t>(pid), sig) != 0) {
            std::cerr << cmd << ": " << pid << ": " << std::strerror(errno) << "\n";
            code = 1;
        }
    }
    return code;
}

int cmd_kill(Shell&, const std::vector<std::string>& args) {
    // kill [-SIG] PID...  mặc định SIGTERM
    int sig = SIGTERM;
    size_t first = 1;

    if (args.size() > 1 && args[1].size() > 1 && args[1][0] == '-') {
        char* end = nullptr;
        long s = std::strtol(args[1].c_str() + 1, &end, 10);
        if (*end != '\0' || s <= 0 || s > 64) {
            std::cerr << "kill: tín hiệu không hợp lệ: " << args[1] << "\n";
            return 1;
        }
        sig = static_cast<int>(s);
        first = 2;
    }
    return send_signal(args, first, sig, "kill");
}

int cmd_stop(Shell&, const std::vector<std::string>& args) {
    return send_signal(args, 1, SIGSTOP, "stop");
}

int cmd_resume(Shell&, const std::vector<std::string>& args) {
    return send_signal(args, 1, SIGCONT, "resume");
}

}  // namespace

void register_process(Registry& registry) {
    registry.add("procs", cmd_procs, "procs  liệt kê tiến trình (đọc /proc)");
    registry.add("kill", cmd_kill, "kill [-SIG] PID...  gửi tín hiệu (mặc định SIGTERM)");
    registry.add("stop", cmd_stop, "stop PID...  tạm dừng tiến trình (SIGSTOP)");
    registry.add("resume", cmd_resume, "resume PID...  tiếp tục tiến trình (SIGCONT)");
}

}  // namespace minishell::builtins
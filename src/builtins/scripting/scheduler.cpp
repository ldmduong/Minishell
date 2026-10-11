#include "minishell/builtins/scripting/scheduler.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

using Clock = std::chrono::steady_clock;

// Một lịch: tiến trình con giữ lịch sẽ chạy command sau `delay` giây,
// rồi lặp lại mỗi `interval` giây (interval = 0 nghĩa là chỉ chạy một lần).
struct Task {
    int id;                    // ID để người dùng hủy lịch
    pid_t pid;                 // PID của tiến trình con
    std::string command;
    long delay;
    long interval;
    Clock::time_point created; // lúc tạo lịch
};

std::vector<Task> tasks;
int next_id = 1;

// Đọc chuỗi toàn chữ số thành số nguyên
bool parse_int(const std::string& s, long& out) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) {
        return false;
    }
    try {
        out = std::stol(s);
        return true;
    } catch (...) {
        return false;
    }
}

// Ghép các từ từ vị trí `from` thành một chuỗi
std::string join_from(const std::vector<std::string>& args, size_t from) {
    std::string s;
    for (size_t i = from; i < args.size(); ++i) {
        if (i > from) s += ' ';
        s += args[i];
    }
    return s;
}

// Bỏ các lịch mà tiến trình con không còn tồn tại
void prune() {
    tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                               [](const Task& t) { return kill(t.pid, 0) != 0; }),
                tasks.end());
}

// Số giây còn lại đến lần chạy kế tiếp
long seconds_left(const Task& t) {
    long elapsed =
        std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - t.created).count();
    if (elapsed < t.delay) return t.delay - elapsed;
    if (t.interval == 0) return 0;
    return t.interval - (elapsed - t.delay) % t.interval;
}

// Tạo tiến trình con giữ lịch
int add_task(Shell& shell, long delay, long interval, const std::string& command) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("schedule: fork");
        return 1;
    }

    if (pid == 0) {
        // Tiến trình con
        setpgid(0, 0);                     // không nhận Ctrl+C từ terminal
        prctl(PR_SET_PDEATHSIG, SIGTERM);  // tự kết thúc khi shell cha thoát

        int devnull = open("/dev/null", O_RDONLY);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            close(devnull);
        }

        sleep(static_cast<unsigned>(delay));
        shell.execute_line(command);
        while (interval > 0) {
            sleep(static_cast<unsigned>(interval));
            shell.execute_line(command);
        }
        _exit(0);
    }

    tasks.push_back(Task{next_id, pid, command, delay, interval, Clock::now()});

    std::cout << "[lịch #" << next_id << "] pid " << pid << ": ";
    if (interval > 0) {
        std::cout << "mỗi " << interval << " giây";
    } else {
        std::cout << "một lần sau " << delay << " giây";
    }
    std::cout << " -> " << command << "\n";

    ++next_id;
    return 0;
}

// schedule list
int list_tasks() {
    prune();
    if (tasks.empty()) {
        std::cout << "không có lịch nào\n";
        return 0;
    }

    std::cout << std::left << std::setw(6) << "ID" << std::setw(9) << "PID"
              << std::setw(12) << "LẶP" << std::setw(12) << "CÒN LẠI" << "LỆNH\n";
    for (const auto& t : tasks) {
        std::string repeat = t.interval > 0 ? "mỗi " + std::to_string(t.interval) + "s"
                                            : "một lần";
        std::string left = std::to_string(seconds_left(t)) + "s";
        std::cout << std::setw(6) << t.id << std::setw(9) << t.pid << std::setw(12) << repeat
                  << std::setw(12) << left << t.command << "\n";
    }
    return 0;
}

// schedule cancel ID
int cancel_task(const std::string& id_text) {
    long id = 0;
    if (!parse_int(id_text, id)) {
        std::cerr << "schedule cancel: ID phải là số\n";
        return 1;
    }

    auto it = std::find_if(tasks.begin(), tasks.end(),
                           [id](const Task& t) { return t.id == id; });
    if (it == tasks.end()) {
        std::cerr << "schedule cancel: không có lịch #" << id << "\n";
        return 1;
    }

    if (kill(it->pid, SIGTERM) != 0) {
        perror("schedule cancel");
    }
    tasks.erase(it);
    std::cout << "đã hủy lịch #" << id << "\n";
    return 0;
}

int cmd_schedule(Shell& shell, const std::vector<std::string>& args) {
    long n = 0;

    // schedule every SỐ_GIÂY LỆNH...
    if (args.size() >= 4 && args[1] == "every") {
        if (!parse_int(args[2], n) || n < 1) {
            std::cerr << "schedule every: số giây phải từ 1 trở lên\n";
            return 1;
        }
        return add_task(shell, n, n, join_from(args, 3));
    }

    // schedule SỐ_GIÂY LỆNH...
    if (args.size() >= 3 && parse_int(args[1], n)) {
        return add_task(shell, n, 0, join_from(args, 2));
    }

    if (args.size() == 2 && args[1] == "list") {
        return list_tasks();
    }
    if (args.size() == 3 && args[1] == "cancel") {
        return cancel_task(args[2]);
    }

    std::cerr << "cách dùng: schedule SỐ_GIÂY LỆNH | schedule every SỐ_GIÂY LỆNH"
                 " | schedule list | schedule cancel ID\n";
    return 1;
}

}  // namespace

void register_scheduler(Registry& registry) {
    registry.add("schedule", cmd_schedule,
                 "schedule SỐ_GIÂY LỆNH | schedule every SỐ_GIÂY LỆNH"
                 " | schedule list | schedule cancel ID");
}

}  // namespace minishell::builtins
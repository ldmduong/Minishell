#include "minishell/builtins/system/threads.h"

#include <chrono>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

// Khóa để các luồng không in lẫn dòng với nhau
std::mutex out_mutex;

// Kiểm tra chuỗi chỉ gồm chữ số (dùng cho PID)
bool is_number(const std::string& s) {
    return !s.empty() && s.find_first_not_of("0123456789") == std::string::npos;
}

// Đọc tên luồng từ /proc/<pid>/task/<tid>/comm
std::string thread_name(const std::string& dir) {
    std::ifstream f(dir + "/comm");
    std::string name;
    std::getline(f, name);
    return name;
}

// Liệt kê các luồng của một tiến trình
void list_threads(const std::string& pid) {
    std::string dir = "/proc/" + pid + "/task";

    DIR* d = opendir(dir.c_str());
    if (!d) {
        std::cerr << "threads: không tìm thấy tiến trình " << pid << "\n";
        return;
    }

    std::cout << "TID        TÊN\n";
    while (dirent* e = readdir(d)) {
        std::string tid = e->d_name;
        if (tid == "." || tid == "..") continue;  // bỏ qua 2 mục ảo
        std::cout << tid << "    " << thread_name(dir + "/" + tid) << "\n";
    }
    closedir(d);
}

// Công việc của mỗi luồng demo: in, ngủ, in
void worker(int id) {
    {
        std::lock_guard<std::mutex> lock(out_mutex);
        std::cout << "luồng " << id << " bắt đầu\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200 * id));

    {
        std::lock_guard<std::mutex> lock(out_mutex);
        std::cout << "luồng " << id << " kết thúc\n";
    }
}

// Tạo n luồng rồi đợi tất cả xong
void run_demo(int n) {
    std::vector<std::thread> threads;
    for (int i = 1; i <= n; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    std::cout << "demo xong\n";
}

int cmd_threads(Shell&, const std::vector<std::string>& args) {
    // threads demo N
    if (args.size() == 3 && args[1] == "demo") {
        int n = 0;
        try {
            n = std::stoi(args[2]);
        } catch (...) {
            n = 0;
        }
        if (n < 1 || n > 16) {
            std::cerr << "threads demo: N phải từ 1 đến 16\n";
            return 1;
        }
        run_demo(n);
        return 0;
    }

    // threads PID
    if (args.size() == 2) {
        if (!is_number(args[1])) {
            std::cerr << "threads: PID phải là số\n";
            return 1;
        }
        list_threads(args[1]);
        return 0;
    }

    std::cerr << "cách dùng: threads PID | threads demo N\n";
    return 1;
}

}  // namespace

void register_threads(Registry& registry) {
    registry.add("threads", cmd_threads, "threads PID | threads demo N  xem luồng / chạy demo");
}

}  // namespace minishell::builtins
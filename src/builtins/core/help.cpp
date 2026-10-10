#include "minishell/builtins/core/help.h"

#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

// Một mục trong help: tiêu đề và danh sách tên lệnh thuộc mục đó
struct Group {
    std::string title;
    std::vector<std::string> names;
};

// Sắp xếp lệnh theo nhóm. Thêm lệnh mới thì thêm tên vào đúng nhóm.
const std::vector<Group> kGroups = {
    {"Điều hướng",        {"cd", "pwd"}},
    {"Phiên làm việc",    {"exit", "clear", "help", "history"}},
    {"Thư mục & tệp",     {"mkdir", "rmdir", "touch", "rm", "cat"}},
    {"Môi trường & alias", {"env", "export", "unset", "alias", "unalias"}},
    {"Hệ thống",          {"ram", "cpu", "uptime"}},
    {"Tiến trình & luồng", {"procs", "kill", "stop", "resume", "threads"}},
    {"Lập lịch",          {"schedule"}},
};

int cmd_help(Shell& shell, const std::vector<std::string>&) {
    const auto& all = shell.registry().all();
    std::set<std::string> shown;  // các lệnh đã in theo nhóm

    std::cout << "Các lệnh builtin, chia theo nhóm:\n";

    // In từng nhóm, theo thứ tự đã khai báo ở kGroups
    for (const auto& group : kGroups) {
        std::vector<std::string> lines;
        for (const auto& name : group.names) {
            for (const auto& [key, info] : all) {
                if (key == name) {
                    lines.push_back(info.help.empty() ? key : info.help);
                    shown.insert(key);
                }
            }
        }
        if (lines.empty()) continue;

        std::cout << "\n[" << group.title << "]\n";
        for (const auto& line : lines) {
            std::cout << "  " << line << "\n";
        }
    }

    // In các lệnh chưa được xếp vào nhóm nào
    bool header = false;
    for (const auto& [key, info] : all) {
        if (shown.count(key)) continue;
        if (!header) {
            std::cout << "\n[Khác]\n";
            header = true;
        }
        std::cout << "  " << (info.help.empty() ? key : info.help) << "\n";
    }

    std::cout << "\nHỗ trợ: pipe |, chuyển hướng < > >>, chạy nền &\n";
    return 0;
}

}  // namespace

void register_help(Registry& registry) {
    registry.add("help", cmd_help, "help  liệt kê các lệnh");
}

}  // namespace minishell::builtins
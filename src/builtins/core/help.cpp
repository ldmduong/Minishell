#include "minishell/builtins/core/help.h"

#include <iostream>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

int cmd_help(Shell& shell, const std::vector<std::string>&) {
    std::cout << "Các lệnh builtin:\n";
    for (const auto& [name, info] : shell.registry().all()) {
        std::cout << "  " << (info.help.empty() ? name : info.help) << "\n";
    }
    std::cout << "Hỗ trợ: pipe |, chuyển hướng < > >>, chạy nền &\n";
    return 0;
}

}  // namespace

void register_help(Registry& registry) {
    registry.add("help", cmd_help, "help  liệt kê các lệnh");
}

}  // namespace minishell::builtins
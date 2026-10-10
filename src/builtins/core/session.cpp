#include "minishell/builtins/core/session.h"

#include <iostream>
#include <string>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

int cmd_exit(Shell& shell, const std::vector<std::string>& args) {
    int code = 0;
    if (args.size() > 1) {
        try {
            code = std::stoi(args[1]);
        } catch (...) {
            std::cerr << "exit: đối số phải là số nguyên\n";
            return 2;
        }
    }
    shell.request_exit(code);
    return code;
}

int cmd_clear(Shell&, const std::vector<std::string>&) {
    std::cout << "\033[H\033[2J" << std::flush;
    return 0;
}

}  // namespace

void register_session(Registry& registry) {
    registry.add("exit", cmd_exit, "exit [mã]  thoát shell");
    registry.add("clear", cmd_clear, "clear  xóa màn hình");
}

}  // namespace minishell::builtins
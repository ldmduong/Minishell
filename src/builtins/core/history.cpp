#include "minishell/builtins/core/history.h"

#include <iostream>
#include <iomanip>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

int cmd_history(Shell& shell, const std::vector<std::string>&) {
    const auto& h = shell.history();
    for (size_t i = 0; i < h.size(); ++i) {
        std::cout << std::setw(4) << (i + 1) << "  " << h[i] << "\n";
    }
    return 0;
}

}  // namespace

void register_history(Registry& registry) {
    registry.add("history", cmd_history, "history  in các lệnh đã nhập");
}

}  // namespace minishell::builtins
#include "minishell/builtins/core/history.h"

#include <iomanip>
#include <iostream>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

int history_command(Shell &shell, const std::vector<std::string> &) {
  const auto &history = shell.history();
  for (size_t i = 0; i < history.size(); ++i) {
    std::cout << std::setw(4) << i + 1 << "  " << history[i] << '\n';
  }
  return 0;
}

} // namespace

void register_history(Registry &registry) {
  registry.add("history", history_command, "history  in các lệnh đã nhập");
}

} // namespace minishell::builtins

#include "minishell/builtins/core/alias.h"

#include <iostream>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

void print_alias(const std::string &name, const std::string &value) {
  std::cout << "alias " << name << "='" << value << "'\n";
}

int alias_command(Shell &shell, const std::vector<std::string> &args) {
  auto &aliases = shell.aliases();
  if (args.size() == 1) {
    for (const auto &[name, value] : aliases)
      print_alias(name, value);
    return 0;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    size_t equals = args[i].find('=');
    if (equals == std::string::npos) {
      auto alias = aliases.find(args[i]);
      if (alias == aliases.end()) {
        std::cerr << "alias: không tồn tại: " << args[i] << '\n';
        result = 1;
      } else {
        print_alias(alias->first, alias->second);
      }
      continue;
    }

    std::string name = args[i].substr(0, equals);
    if (name.empty()) {
      std::cerr << "alias: tên không hợp lệ\n";
      result = 1;
      continue;
    }
    aliases[name] = args[i].substr(equals + 1);
  }
  return result;
}

int unalias_command(Shell &shell, const std::vector<std::string> &args) {
  if (args.size() < 2) {
    std::cerr << "unalias: thiếu tên alias\n";
    return 1;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    if (shell.aliases().erase(args[i]) == 0) {
      std::cerr << "unalias: không tồn tại: " << args[i] << '\n';
      result = 1;
    }
  }
  return result;
}

} // namespace

void register_alias(Registry &registry) {
  registry.add("alias", alias_command,
               "alias [TÊN='lệnh']  đặt hoặc xem alias");
  registry.add("unalias", unalias_command, "unalias TÊN...  xóa alias");
}

} // namespace minishell::builtins

#include "minishell/builtins/core/help.h"

#include <iostream>
#include <set>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

struct Group {
  const char *title;
  std::vector<std::string> commands;
};

const std::vector<Group> groups = {
    {"Điều hướng", {"cd", "pwd"}},
    {"Phiên làm việc", {"exit", "clear", "help", "history"}},
    {"Thư mục & tệp", {"mkdir", "rmdir", "touch", "rm", "cat"}},
    {"Môi trường & alias", {"env", "export", "unset", "alias", "unalias"}},
    {"Hệ thống", {"ram", "cpu", "uptime"}},
    {"Tiến trình & luồng", {"procs", "kill", "stop", "resume", "threads"}},
};

int help_command(Shell &shell, const std::vector<std::string> &) {
  const auto &commands = shell.registry().all();
  std::set<std::string> printed;
  std::cout << "Các lệnh builtin, chia theo nhóm:\n";

  for (const Group &group : groups) {
    std::vector<std::string> descriptions;
    for (const std::string &name : group.commands) {
      auto command = commands.find(name);
      if (command == commands.end())
        continue;
      descriptions.push_back(
          command->second.help.empty() ? name : command->second.help);
      printed.insert(name);
    }

    if (descriptions.empty())
      continue;
    std::cout << "\n[" << group.title << "]\n";
    for (const std::string &description : descriptions) {
      std::cout << "  " << description << '\n';
    }
  }

  bool has_other_commands = false;
  for (const auto &[name, command] : commands) {
    if (printed.count(name) != 0)
      continue;
    if (!has_other_commands) {
      std::cout << "\n[Khác]\n";
      has_other_commands = true;
    }
    std::cout << "  " << (command.help.empty() ? name : command.help) << '\n';
  }

  std::cout << "\nHỗ trợ: pipe |, chuyển hướng < > >>, chạy nền &\n";
  return 0;
}

} // namespace

void register_help(Registry &registry) {
  registry.add("help", help_command, "help  liệt kê các lệnh");
}

} // namespace minishell::builtins

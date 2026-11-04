#include "minishell/builtins/core/navigation.h"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

std::string current_directory() {
  char path[4096];
  return getcwd(path, sizeof(path)) == nullptr ? std::string()
                                               : std::string(path);
}

int cd_command(Shell &shell, const std::vector<std::string> &args) {
  if (args.size() > 2) {
    std::cerr << "cd: quá nhiều đối số\n";
    return 1;
  }

  std::string target;
  bool print_new_directory = false;
  if (args.size() == 1) {
    const char *home = std::getenv("HOME");
    if (home == nullptr) {
      std::cerr << "cd: chưa đặt biến HOME\n";
      return 1;
    }
    target = home;
  } else if (args[1] == "-") {
    if (shell.prev_dir.empty()) {
      std::cerr << "cd: chưa có thư mục trước đó\n";
      return 1;
    }
    target = shell.prev_dir;
    print_new_directory = true;
  } else {
    target = args[1];
  }

  std::string old_directory = current_directory();
  if (chdir(target.c_str()) != 0) {
    std::perror(("cd: " + target).c_str());
    return 1;
  }

  shell.prev_dir = old_directory;
  if (print_new_directory)
    std::cout << current_directory() << '\n';
  return 0;
}

int pwd_command(Shell &, const std::vector<std::string> &) {
  std::string path = current_directory();
  if (path.empty()) {
    std::perror("pwd");
    return 1;
  }
  std::cout << path << '\n';
  return 0;
}

} // namespace

void register_navigation(Registry &registry) {
  registry.add("cd", cd_command, "cd [thư_mục | -]  đổi thư mục làm việc");
  registry.add("pwd", pwd_command, "pwd  in thư mục hiện tại");
}

} // namespace minishell::builtins

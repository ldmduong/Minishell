#include "minishell/builtins/core/navigation.h"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "minishell/core/shell.h"

namespace minishell::builtins {

std::string current_directory() {
  char path[4096];
  return getcwd(path, sizeof(path)) == nullptr ? std::string()
                                               : std::string(path);
}

int change_directory(Shell &shell, const std::string &target,
                     const char *command, bool print) {
  std::string old_directory = current_directory();
  if (chdir(target.c_str()) != 0) {
    std::perror((std::string(command) + ": " + target).c_str());
    return 1;
  }

  shell.prev_dir = old_directory;
  const std::string directory = current_directory();
  int old_status = old_directory.empty() ? unsetenv("OLDPWD")
                                         : setenv("OLDPWD", old_directory.c_str(), 1);
  int status = directory.empty() ? unsetenv("PWD") : setenv("PWD", directory.c_str(), 1);
  if (status != 0 || old_status != 0) {
    std::perror(command);
    return 1;
  }
  if (print)
    std::cout << directory << '\n';
  return 0;
}

namespace {

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

  return change_directory(shell, target, "cd", print_new_directory);
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

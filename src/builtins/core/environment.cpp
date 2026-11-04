#include "minishell/builtins/core/environment.h"

#include <cctype>
#include <cstdlib>
#include <iostream>

#include "minishell/core/shell.h"

extern char **environ;

namespace minishell::builtins {
namespace {

bool valid_name(const std::string &name) {
  if (name.empty())
    return false;

  unsigned char first = static_cast<unsigned char>(name[0]);
  if (!std::isalpha(first) && first != '_')
    return false;

  for (char character : name) {
    unsigned char value = static_cast<unsigned char>(character);
    if (!std::isalnum(value) && value != '_')
      return false;
  }
  return true;
}

void print_environment() {
  for (char **item = environ; *item != nullptr; ++item) {
    std::cout << *item << '\n';
  }
}

int env_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() > 1) {
    std::cerr << "env: chưa hỗ trợ đối số\n";
    return 1;
  }
  print_environment();
  return 0;
}

int export_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() == 1) {
    print_environment();
    return 0;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    size_t equals = args[i].find('=');
    std::string name = args[i].substr(0, equals);
    if (!valid_name(name)) {
      std::cerr << "export: tên không hợp lệ: " << name << '\n';
      result = 1;
      continue;
    }

    if (equals != std::string::npos &&
        setenv(name.c_str(), args[i].substr(equals + 1).c_str(), 1) != 0) {
      std::perror("export");
      result = 1;
    }
  }
  return result;
}

int unset_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() < 2) {
    std::cerr << "unset: thiếu tên biến\n";
    return 1;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    if (!valid_name(args[i])) {
      std::cerr << "unset: tên không hợp lệ: " << args[i] << '\n';
      result = 1;
    } else if (unsetenv(args[i].c_str()) != 0) {
      std::perror("unset");
      result = 1;
    }
  }
  return result;
}

} // namespace

void register_environment(Registry &registry) {
  registry.add("env", env_command, "env  in các biến môi trường");
  registry.add("export", export_command,
               "export [TÊN=giá_trị]  đặt biến môi trường");
  registry.add("unset", unset_command, "unset TÊN...  xóa biến môi trường");
}

} // namespace minishell::builtins

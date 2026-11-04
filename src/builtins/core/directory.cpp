#include "minishell/builtins/core/directory.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

bool make_parents(const std::string &path) {
  if (path.empty()) {
    errno = ENOENT;
    return false;
  }

  std::string prefix;
  for (size_t i = 0; i < path.size(); ++i) {
    prefix += path[i];
    if (path[i] != '/' && i + 1 != path.size())
      continue;
    if (prefix == "/")
      continue;

    if (mkdir(prefix.c_str(), 0755) == 0)
      continue;
    if (errno == EEXIST) {
      struct stat info{};
      if (stat(prefix.c_str(), &info) == 0 && S_ISDIR(info.st_mode))
        continue;
    }
    return false;
  }
  return true;
}

int mkdir_command(Shell &, const std::vector<std::string> &args) {
  bool parents = false;
  std::vector<std::string> paths;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "-p") {
      parents = true;
    } else {
      paths.push_back(args[i]);
    }
  }

  if (paths.empty()) {
    std::cerr << "mkdir: thiếu tên thư mục\n";
    return 1;
  }

  int result = 0;
  for (const std::string &path : paths) {
    bool ok = parents ? make_parents(path) : mkdir(path.c_str(), 0755) == 0;
    if (!ok) {
      std::perror(("mkdir: " + path).c_str());
      result = 1;
    }
  }
  return result;
}

int rmdir_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() < 2) {
    std::cerr << "rmdir: thiếu tên thư mục\n";
    return 1;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    if (rmdir(args[i].c_str()) != 0) {
      std::perror(("rmdir: " + args[i]).c_str());
      result = 1;
    }
  }
  return result;
}

} // namespace

void register_directory(Registry &registry) {
  registry.add("mkdir", mkdir_command, "mkdir [-p] thư_mục...  tạo thư mục");
  registry.add("rmdir", rmdir_command, "rmdir thư_mục...  xóa thư mục rỗng");
}

} // namespace minishell::builtins

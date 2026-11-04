#include "minishell/builtins/extras/bookmark.h"

#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

std::string bookmark_file() {
  const char *home = std::getenv("HOME");
  return home == nullptr ? std::string()
                         : std::string(home) + "/.minishell_bookmarks";
}

bool load_bookmarks(std::map<std::string, std::string> &bookmarks,
                    const std::string &path) {
  std::ifstream file(path);
  if (!file) {
    if (access(path.c_str(), F_OK) != 0) {
      if (errno == ENOENT)
        return true;
      std::perror(("bookmark: " + path).c_str());
    } else {
      std::cerr << "bookmark: không đọc được " << path << '\n';
    }
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    size_t separator = line.find('\t');
    if (separator == std::string::npos) {
      std::cerr << "bookmark: dữ liệu không hợp lệ trong " << path << '\n';
      return false;
    }
    bookmarks[line.substr(0, separator)] = line.substr(separator + 1);
  }
  if (file.bad()) {
    std::cerr << "bookmark: lỗi khi đọc " << path << '\n';
    return false;
  }
  return true;
}

bool save_bookmarks(const std::map<std::string, std::string> &bookmarks,
                    const std::string &path) {
  std::string temporary = path + ".tmp";
  std::ofstream file(temporary, std::ios::trunc);
  if (!file) {
    std::cerr << "bookmark: không thể ghi " << temporary << '\n';
    return false;
  }

  for (const auto &[name, directory] : bookmarks)
    file << name << '\t' << directory << '\n';
  file.close();
  if (!file) {
    std::cerr << "bookmark: lỗi khi ghi " << temporary << '\n';
    return false;
  }
  if (rename(temporary.c_str(), path.c_str()) != 0) {
    std::perror("bookmark: rename");
    return false;
  }
  return true;
}

std::string current_directory() {
  char path[4096];
  return getcwd(path, sizeof(path)) == nullptr ? std::string()
                                               : std::string(path);
}

int bookmark_command(Shell &shell, const std::vector<std::string> &args) {
  if (args.size() == 1 || args[1] == "list") {
    std::string path = bookmark_file();
    if (path.empty()) {
      std::cerr << "bookmark: chưa đặt HOME\n";
      return 1;
    }
    std::map<std::string, std::string> bookmarks;
    if (!load_bookmarks(bookmarks, path))
      return 1;
    for (const auto &[name, directory] : bookmarks)
      std::cout << name << '\t' << directory << '\n';
    return 0;
  }

  if (args[1] == "add") {
    if (args.size() < 3 || args.size() > 4) {
      std::cerr << "cách dùng: bookmark add TÊN [THƯ_MỤC]\n";
      return 1;
    }
    std::string directory = args.size() == 4 ? args[3] : current_directory();
    if (args[2].empty() || args[2].find_first_of("\t\n") != std::string::npos ||
        directory.empty() ||
        directory.find_first_of("\t\n") != std::string::npos) {
      std::cerr << "bookmark: tên hoặc đường dẫn không hợp lệ\n";
      return 1;
    }
    std::string path = bookmark_file();
    if (path.empty()) {
      std::cerr << "bookmark: chưa đặt HOME\n";
      return 1;
    }
    std::map<std::string, std::string> bookmarks;
    if (!load_bookmarks(bookmarks, path))
      return 1;
    bookmarks[args[2]] = directory;
    return save_bookmarks(bookmarks, path) ? 0 : 1;
  }

  if (args[1] == "remove") {
    if (args.size() != 3) {
      std::cerr << "cách dùng: bookmark remove TÊN\n";
      return 1;
    }
    std::string path = bookmark_file();
    if (path.empty()) {
      std::cerr << "bookmark: chưa đặt HOME\n";
      return 1;
    }
    std::map<std::string, std::string> bookmarks;
    if (!load_bookmarks(bookmarks, path))
      return 1;
    if (bookmarks.erase(args[2]) == 0) {
      std::cerr << "bookmark: không tồn tại: " << args[2] << '\n';
      return 1;
    }
    return save_bookmarks(bookmarks, path) ? 0 : 1;
  }

  if (args[1] == "go") {
    if (args.size() != 3) {
      std::cerr << "cách dùng: bookmark go TÊN\n";
      return 1;
    }
    std::string path = bookmark_file();
    if (path.empty()) {
      std::cerr << "bookmark: chưa đặt HOME\n";
      return 1;
    }
    std::map<std::string, std::string> bookmarks;
    if (!load_bookmarks(bookmarks, path))
      return 1;
    auto bookmark = bookmarks.find(args[2]);
    if (bookmark == bookmarks.end()) {
      std::cerr << "bookmark: không tồn tại: " << args[2] << '\n';
      return 1;
    }

    std::string old_directory = current_directory();
    if (chdir(bookmark->second.c_str()) != 0) {
      std::perror(("bookmark: " + bookmark->second).c_str());
      return 1;
    }
    shell.prev_dir = old_directory;
    return 0;
  }

  std::cerr << "cách dùng: bookmark [list | add TÊN [THƯ_MỤC] | go TÊN | "
               "remove TÊN]\n";
  return 1;
}

} // namespace

void register_bookmark(Registry &registry) {
  registry.add("bookmark", bookmark_command,
               "bookmark [list|add TÊN [THƯ_MỤC]|go TÊN|remove TÊN]");
}

} // namespace minishell::builtins
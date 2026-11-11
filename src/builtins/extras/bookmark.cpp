#include "minishell/builtins/extras/bookmark.h"

#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "minishell/builtins/core/navigation.h"

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
  std::string temporary = path + ".XXXXXX";
  int fd = mkstemp(temporary.data());
  if (fd < 0) {
    std::perror("bookmark: mkstemp");
    return false;
  }

  FILE *file = fdopen(fd, "w");
  if (!file) {
    std::perror("bookmark: fdopen");
    close(fd);
    unlink(temporary.c_str());
    return false;
  }

  bool written = true;
  for (const auto &[name, directory] : bookmarks) {
    std::string line = name + '\t' + directory + '\n';
    if (fwrite(line.data(), 1, line.size(), file) != line.size()) {
      written = false;
      break;
    }
  }
  if (fclose(file) != 0 || !written) {
    std::cerr << "bookmark: lỗi khi ghi " << temporary << '\n';
    unlink(temporary.c_str());
    return false;
  }
  if (rename(temporary.c_str(), path.c_str()) != 0) {
    std::perror("bookmark: rename");
    unlink(temporary.c_str());
    return false;
  }
  return true;
}

bool update_bookmarks(const std::string &path, const std::string &name,
                      const std::string &directory) {
  // Lock a separate file because rename replaces the bookmarks inode.
  int lock = open((path + ".lock").c_str(),
                  O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
  if (lock < 0) {
    std::perror("bookmark: open lock");
    return false;
  }
  if (flock(lock, LOCK_EX) != 0) {
    std::perror("bookmark: flock");
    close(lock);
    return false;
  }

  std::map<std::string, std::string> bookmarks;
  bool ok = load_bookmarks(bookmarks, path);
  if (ok) {
    if (!directory.empty()) {
      bookmarks[name] = directory;
    } else if (bookmarks.erase(name) == 0) {
      std::cerr << "bookmark: không tồn tại: " << name << '\n';
      ok = false;
    }
    if (ok)
      ok = save_bookmarks(bookmarks, path);
  }
  close(lock);
  return ok;
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
    return update_bookmarks(path, args[2], directory) ? 0 : 1;
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
    return update_bookmarks(path, args[2], {}) ? 0 : 1;
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

    return change_directory(shell, bookmark->second, "bookmark");
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

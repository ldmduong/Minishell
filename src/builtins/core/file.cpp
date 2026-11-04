#include "minishell/builtins/core/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

bool copy_file(int input, int output) {
  char buffer[4096];
  while (true) {
    ssize_t bytes_read = read(input, buffer, sizeof(buffer));
    if (bytes_read < 0) {
      if (errno == EINTR)
        continue;
      std::perror("cat");
      return false;
    }
    if (bytes_read == 0)
      return true;

    ssize_t written = 0;
    while (written < bytes_read) {
      ssize_t count = write(output, buffer + written, bytes_read - written);
      if (count < 0) {
        if (errno == EINTR)
          continue;
        std::perror("cat");
        return false;
      }
      if (count == 0) {
        std::cerr << "cat: ghi dữ liệu thất bại\n";
        return false;
      }
      written += count;
    }
  }
}

int touch_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() < 2) {
    std::cerr << "touch: thiếu tên file\n";
    return 1;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    const std::string &path = args[i];
    if (access(path.c_str(), F_OK) != 0) {
      int fd = open(path.c_str(), O_WRONLY | O_CREAT, 0644);
      if (fd < 0) {
        std::perror(("touch: " + path).c_str());
        result = 1;
      } else if (close(fd) != 0) {
        std::perror(("touch: " + path).c_str());
        result = 1;
      }
    } else if (utimensat(AT_FDCWD, path.c_str(), nullptr, 0) != 0) {
      std::perror(("touch: " + path).c_str());
      result = 1;
    }
  }
  return result;
}

int rm_command(Shell &, const std::vector<std::string> &args) {
  bool force = false;
  std::vector<std::string> paths;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "-f") {
      force = true;
    } else {
      paths.push_back(args[i]);
    }
  }

  if (paths.empty()) {
    if (force)
      return 0;
    std::cerr << "rm: thiếu tên file\n";
    return 1;
  }

  int result = 0;
  for (const std::string &path : paths) {
    struct stat info{};
    if (lstat(path.c_str(), &info) != 0) {
      if (!force || errno != ENOENT) {
        std::perror(("rm: " + path).c_str());
        result = 1;
      }
      continue;
    }
    if (S_ISDIR(info.st_mode)) {
      std::cerr << "rm: " << path << ": là thư mục, hãy dùng rmdir\n";
      result = 1;
      continue;
    }
    if (unlink(path.c_str()) != 0) {
      std::perror(("rm: " + path).c_str());
      result = 1;
    }
  }
  return result;
}

int cat_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() == 1) {
    return copy_file(STDIN_FILENO, STDOUT_FILENO) ? 0 : 1;
  }

  int result = 0;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i] == "-") {
      if (!copy_file(STDIN_FILENO, STDOUT_FILENO))
        result = 1;
      continue;
    }

    int fd = open(args[i].c_str(), O_RDONLY);
    if (fd < 0) {
      std::perror(("cat: " + args[i]).c_str());
      result = 1;
      continue;
    }
    if (!copy_file(fd, STDOUT_FILENO))
      result = 1;
    if (close(fd) != 0) {
      std::perror(("cat: " + args[i]).c_str());
      result = 1;
    }
  }
  return result;
}

} // namespace

void register_file(Registry &registry) {
  registry.add("touch", touch_command,
               "touch file...  tạo file rỗng hoặc cập nhật thời gian");
  registry.add("rm", rm_command,
               "rm [-f] file...  xóa file (không xóa thư mục)");
  registry.add("cat", cat_command,
               "cat [file...]  in nội dung file hoặc đầu vào chuẩn");
}

} // namespace minishell::builtins

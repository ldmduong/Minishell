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
  struct stat source{}, destination{};
  if (fstat(input, &source) != 0 || fstat(output, &destination) != 0) {
    std::perror("cat: fstat");
    return false;
  }
  if (S_ISREG(source.st_mode) && source.st_dev == destination.st_dev &&
      source.st_ino == destination.st_ino) {
    std::cerr << "cat: đầu vào và đầu ra là cùng một tệp\n";
    return false;
  }

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
  bool options = true;
  std::vector<std::string> paths;
  for (size_t i = 1; i < args.size(); ++i) {
    if (options && args[i] == "--") {
      options = false;
    } else if (options && args[i] == "-f") {
      force = true;
    } else if (options && args[i].size() > 1 && args[i].front() == '-') {
      std::cerr << "rm: không hỗ trợ tùy chọn " << args[i] << '\n';
      return 1;
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
    if (unlink(path.c_str()) == 0 || (force && errno == ENOENT))
      continue;
    if (errno == EISDIR)
      std::cerr << "rm: " << path << ": là thư mục, hãy dùng rmdir\n";
    else
      std::perror(("rm: " + path).c_str());
    result = 1;
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
               "rm [-f] [--] file...  xóa file (không xóa thư mục)");
  registry.add("cat", cat_command,
               "cat [file...]  in nội dung file hoặc đầu vào chuẩn");
}

} // namespace minishell::builtins

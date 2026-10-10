#include "minishell/builtins/core/file.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

// Chép toàn bộ dữ liệu từ fd đầu vào sang fd đầu ra.
bool copy_fd(int in, int out) {
    char buf[4096];
    while (true) {
        ssize_t n = read(in, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EINTR) continue;
            std::perror("cat");
            return false;
        }
        if (n == 0) return true;

        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, n - off);
            if (w < 0) {
                if (errno == EINTR) continue;
                std::perror("cat");
                return false;
            }
            off += w;
        }
    }
}

int cmd_touch(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "touch: thiếu tên file\n";
        return 1;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& path = args[i];
        if (access(path.c_str(), F_OK) != 0) {
            // File chưa có: tạo mới rỗng
            int fd = open(path.c_str(), O_WRONLY | O_CREAT, 0644);
            if (fd < 0) {
                std::perror(("touch: " + path).c_str());
                code = 1;
                continue;
            }
            close(fd);
        } else if (utimensat(AT_FDCWD, path.c_str(), nullptr, 0) != 0) {
            // File đã có: cập nhật thời gian sửa đổi về hiện tại
            std::perror(("touch: " + path).c_str());
            code = 1;
        }
    }
    return code;
}

int cmd_rm(Shell&, const std::vector<std::string>& args) {
    bool force = false;
    std::vector<std::string> files;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-f") {
            force = true;
        } else {
            files.push_back(args[i]);
        }
    }

    if (files.empty()) {
        if (force) return 0;
        std::cerr << "rm: thiếu tên file\n";
        return 1;
    }

    int code = 0;
    for (const auto& f : files) {
        struct stat st;
        if (lstat(f.c_str(), &st) != 0) {
            if (!force) {
                std::perror(("rm: " + f).c_str());
                code = 1;
            }
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            std::cerr << "rm: " << f << ": là thư mục, hãy dùng rmdir\n";
            code = 1;
            continue;
        }
        if (unlink(f.c_str()) != 0) {
            std::perror(("rm: " + f).c_str());
            code = 1;
        }
    }
    return code;
}

int cmd_cat(Shell&, const std::vector<std::string>& args) {
    // Không có đối số: đọc từ đầu vào chuẩn (ví dụ sau pipe hoặc <)
    if (args.size() == 1) {
        return copy_fd(STDIN_FILENO, STDOUT_FILENO) ? 0 : 1;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "-") {
            if (!copy_fd(STDIN_FILENO, STDOUT_FILENO)) code = 1;
            continue;
        }
        int fd = open(args[i].c_str(), O_RDONLY);
        if (fd < 0) {
            std::perror(("cat: " + args[i]).c_str());
            code = 1;
            continue;
        }
        if (!copy_fd(fd, STDOUT_FILENO)) code = 1;
        close(fd);
    }
    return code;
}

}  // namespace

void register_file(Registry& registry) {
    registry.add("touch", cmd_touch, "touch file...  tạo file rỗng hoặc cập nhật thời gian");
    registry.add("rm", cmd_rm, "rm [-f] file...  xóa file (không xóa thư mục)");
    registry.add("cat", cmd_cat, "cat [file...]  in nội dung file hoặc đầu vào chuẩn");
}

}  // namespace minishell::builtins
#include "minishell/builtins/core/directory.h"

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

// Tạo từng cấp của đường dẫn, bỏ qua cấp đã tồn tại (giống mkdir -p).
bool make_parents(const std::string& path) {
    size_t i = 0;
    while (i <= path.size()) {
        size_t slash = path.find('/', i);
        if (slash == std::string::npos) slash = path.size();

        std::string part = path.substr(0, slash);
        if (!part.empty() && mkdir(part.c_str(), 0755) != 0 && errno != EEXIST) {
            return false;
        }
        i = slash + 1;
    }
    return true;
}

int cmd_mkdir(Shell&, const std::vector<std::string>& args) {
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

    int code = 0;
    for (const auto& p : paths) {
        bool ok = parents ? make_parents(p) : (mkdir(p.c_str(), 0755) == 0);
        if (!ok) {
            std::perror(("mkdir: " + p).c_str());
            code = 1;
        }
    }
    return code;
}

int cmd_rmdir(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "rmdir: thiếu tên thư mục\n";
        return 1;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        if (rmdir(args[i].c_str()) != 0) {
            std::perror(("rmdir: " + args[i]).c_str());
            code = 1;
        }
    }
    return code;
}

}  // namespace

void register_directory(Registry& registry) {
    registry.add("mkdir", cmd_mkdir, "mkdir [-p] thư_mục...  tạo thư mục");
    registry.add("rmdir", cmd_rmdir, "rmdir thư_mục...  xóa thư mục rỗng");
}

}  // namespace minishell::builtins
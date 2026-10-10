#include "minishell/builtins/core/alias.h"

#include <iostream>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

void print_alias(const std::string& name, const std::string& value) {
    std::cout << "alias " << name << "='" << value << "'\n";
}

int cmd_alias(Shell& shell, const std::vector<std::string>& args) {
    auto& aliases = shell.aliases();

    // Không có đối số: liệt kê tất cả alias
    if (args.size() == 1) {
        for (const auto& [name, value] : aliases) print_alias(name, value);
        return 0;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        size_t eq = arg.find('=');

        if (eq == std::string::npos) {
            // Chỉ có tên: in alias đó
            auto it = aliases.find(arg);
            if (it == aliases.end()) {
                std::cerr << "alias: không tồn tại: " << arg << "\n";
                code = 1;
            } else {
                print_alias(it->first, it->second);
            }
            continue;
        }

        // TÊN=lệnh: đặt alias
        std::string name = arg.substr(0, eq);
        if (name.empty()) {
            std::cerr << "alias: tên không hợp lệ\n";
            code = 1;
            continue;
        }
        aliases[name] = arg.substr(eq + 1);
    }
    return code;
}

int cmd_unalias(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "unalias: thiếu tên alias\n";
        return 1;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        if (shell.aliases().erase(args[i]) == 0) {
            std::cerr << "unalias: không tồn tại: " << args[i] << "\n";
            code = 1;
        }
    }
    return code;
}

}  // namespace

void register_alias(Registry& registry) {
    registry.add("alias", cmd_alias, "alias [TÊN='lệnh']  đặt hoặc xem alias");
    registry.add("unalias", cmd_unalias, "unalias TÊN...  xóa alias");
}

}  // namespace minishell::builtins
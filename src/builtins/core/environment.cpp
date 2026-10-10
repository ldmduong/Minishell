#include "minishell/builtins/core/environment.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

// Danh sách biến môi trường của tiến trình hiện tại.
extern char** environ;

namespace minishell::builtins {

namespace {

// Tên biến hợp lệ: bắt đầu bằng chữ hoặc _, còn lại là chữ, số hoặc _.
bool valid_name(const std::string& name) {
    if (name.empty()) return false;
    unsigned char first = static_cast<unsigned char>(name[0]);
    if (!std::isalpha(first) && first != '_') return false;
    for (char ch : name) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (!std::isalnum(c) && c != '_') return false;
    }
    return true;
}

void print_env() {
    for (char** e = environ; *e; ++e) {
        std::cout << *e << "\n";
    }
}

int cmd_env(Shell&, const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::cerr << "env: chưa hỗ trợ đối số\n";
        return 1;
    }
    print_env();
    return 0;
}

int cmd_export(Shell&, const std::vector<std::string>& args) {
    if (args.size() == 1) {
        print_env();
        return 0;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        size_t eq = arg.find('=');
        std::string name = arg.substr(0, eq);

        if (!valid_name(name)) {
            std::cerr << "export: tên không hợp lệ: " << name << "\n";
            code = 1;
            continue;
        }
        if (eq == std::string::npos) continue;  // chỉ có tên, không có '=': bỏ qua

        std::string value = arg.substr(eq + 1);
        if (setenv(name.c_str(), value.c_str(), 1) != 0) {
            std::perror("export");
            code = 1;
        }
    }
    return code;
}

int cmd_unset(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "unset: thiếu tên biến\n";
        return 1;
    }

    int code = 0;
    for (size_t i = 1; i < args.size(); ++i) {
        if (!valid_name(args[i])) {
            std::cerr << "unset: tên không hợp lệ: " << args[i] << "\n";
            code = 1;
            continue;
        }
        unsetenv(args[i].c_str());
    }
    return code;
}

}  // namespace

void register_environment(Registry& registry) {
    registry.add("env", cmd_env, "env  in các biến môi trường");
    registry.add("export", cmd_export, "export [TÊN=giá_trị]  đặt biến môi trường");
    registry.add("unset", cmd_unset, "unset TÊN...  xóa biến môi trường");
}

}  // namespace minishell::builtins
#include "minishell/builtins/core/navigation.h"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "minishell/core/shell.h"

namespace minishell::builtins {

namespace {

std::string current_dir() {
    char buf[4096];
    return getcwd(buf, sizeof(buf)) ? buf : "";
}

int cmd_cd(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() > 2) {
        std::cerr << "cd: quá nhiều đối số\n";
        return 1;
    }

    std::string target;
    bool print_dir = false;
    if (args.size() == 1) {
        const char* home = std::getenv("HOME");
        if (!home) {
            std::cerr << "cd: chưa đặt biến HOME\n";
            return 1;
        }
        target = home;
    } else if (args[1] == "-") {
        if (shell.prev_dir.empty()) {
            std::cerr << "cd: chưa có thư mục trước đó\n";
            return 1;
        }
        target = shell.prev_dir;
        print_dir = true;
    } else {
        target = args[1];
    }

    std::string before = current_dir();
    if (chdir(target.c_str()) != 0) {
        std::perror(("cd: " + target).c_str());
        return 1;
    }
    shell.prev_dir = before;
    if (print_dir) std::cout << current_dir() << "\n";
    return 0;
}

int cmd_pwd(Shell&, const std::vector<std::string>&) {
    std::cout << current_dir() << "\n";
    return 0;
}

}  // namespace

void register_navigation(Registry& registry) {
    registry.add("cd", cmd_cd, "cd [thư_mục | -]  đổi thư mục làm việc");
    registry.add("pwd", cmd_pwd, "pwd  in thư mục hiện tại");
}

}  // namespace minishell::builtins
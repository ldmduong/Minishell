#include "minishell/core/shell.h"

#include <csignal>
#include <iostream>

#include "minishell/builtins/core/help.h"
#include "minishell/builtins/core/navigation.h"
#include "minishell/builtins/core/session.h"
#include "minishell/core/executor.h"
#include "minishell/core/line_editor.h"
#include "minishell/core/parser.h"

namespace minishell {

Shell::Shell() {
    signal(SIGINT, SIG_IGN);  // Ctrl+C không làm shell tự thoát

    builtins::register_navigation(registry_);
    builtins::register_session(registry_);
    builtins::register_help(registry_);
}

std::string Shell::prompt() const {
    return "minishell$ ";
}

int Shell::execute_line(const std::string& line) {
    Pipeline pipeline;
    std::string error;
    if (!Parser::parse(line, pipeline, error)) {
        std::cerr << "minishell: lỗi cú pháp: " << error << "\n";
        return 2;
    }
    if (pipeline.commands.empty()) return 0;
    return Executor::run(*this, pipeline);
}

int Shell::run() {
    std::string line;

    while (running_) {
        Executor::reap_background();

        if (!read_line(prompt(), history_, line)) break;
        if (line.find_first_not_of(" \t") == std::string::npos) continue;

        history_.push_back(line);
        execute_line(line);
    }

    return exit_code_;
}

}  // namespace minishell
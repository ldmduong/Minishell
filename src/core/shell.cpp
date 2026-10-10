#include "minishell/core/shell.h"
#include "minishell/core/line_editor.h"
#include "minishell/core/parser.h"
#include "minishell/core/executor.h"

#include <iostream>
#include <string>
#include <csignal>
namespace minishell {

Shell::Shell() {
    signal(SIGINT, SIG_IGN);  // Ctrl+C không làm shell tự thoát
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

    return 0;
}
}  // namespace minishell
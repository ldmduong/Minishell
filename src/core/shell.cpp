#include "minishell/core/shell.h"
#include "minishell/core/line_editor.h"
#include "minishell/core/parser.h"
#include <iostream>
#include <string>

namespace minishell {

std::string Shell::prompt() const {
    return "minishell$ ";
}

static void debug_print_pipeline(const std::string& line) {
    minishell::Pipeline p;
    std::string error;

    if (!minishell::Parser::parse(line, p, error)) {
        std::cout << "  [lỗi] " << error << "\n";
        return;
    }

    std::cout << "  số lệnh: " << p.commands.size()
              << ", nền: " << (p.background ? "có" : "không") << "\n";

    for (size_t i = 0; i < p.commands.size(); ++i) {
        const auto& c = p.commands[i];
        std::cout << "  lệnh " << i << ": args=[";
        for (size_t j = 0; j < c.args.size(); ++j) {
            std::cout << (j ? ", " : "") << "\"" << c.args[j] << "\"";
        }
        std::cout << "]";
        if (!c.in_file.empty()) std::cout << " in=" << c.in_file;
        if (!c.out_file.empty()) {
            std::cout << (c.append ? " append=" : " out=") << c.out_file;
        }
        std::cout << "\n";
    }
}


int Shell::run() {
    std::string line;

    while (running_) {
        if (!read_line(prompt(), history_, line)) break;

        if (line.find_first_not_of(" \t") == std::string::npos) continue;

        history_.push_back(line);
        debug_print_pipeline(line);
        std::cout << line << "\n";  // tạm thời, sẽ thay bằng parser sau
    }

    return 0;
}
}  // namespace minishell
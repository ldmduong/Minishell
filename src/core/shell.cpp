#include "minishell/core/shell.h"
#include "minishell/core/line_editor.h"
#include "minishell/core/parser.h"
#include <iostream>
#include <string>

namespace minishell {

std::string Shell::prompt() const {
    return "minishell$ ";
}



int Shell::run() {
    std::string line;

    while (running_) {
        if (!read_line(prompt(), history_, line)) break;

        if (line.find_first_not_of(" \t") == std::string::npos) continue;

        history_.push_back(line);
        
        std::cout << line << "\n";  // tạm thời, sẽ thay bằng parser sau
    }

    return 0;
}
}  // namespace minishell
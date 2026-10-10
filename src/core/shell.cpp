#include "minishell/core/shell.h"

#include <csignal>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "minishell/builtins/core/alias.h"
#include "minishell/builtins/core/directory.h"
#include "minishell/builtins/core/environment.h"
#include "minishell/builtins/core/file.h"
#include "minishell/builtins/core/help.h"
#include "minishell/builtins/core/history.h"
#include "minishell/builtins/core/navigation.h"
#include "minishell/builtins/core/session.h"
#include "minishell/core/executor.h"
#include "minishell/core/line_editor.h"
#include "minishell/core/parser.h"

namespace minishell {

namespace {

// Tách chuỗi thành các từ, phân cách bằng khoảng trắng.
std::vector<std::string> split_words(const std::string& text) {
    std::istringstream in(text);
    std::vector<std::string> words;
    std::string w;
    while (in >> w) words.push_back(w);
    return words;
}

}  // namespace

Shell::Shell() {
    signal(SIGINT, SIG_IGN);  // Ctrl+C không làm shell tự thoát

    builtins::register_navigation(registry_);
    builtins::register_session(registry_);
    builtins::register_help(registry_);
    builtins::register_history(registry_);
    builtins::register_directory(registry_);
    builtins::register_file(registry_);
    builtins::register_environment(registry_);
    builtins::register_alias(registry_);
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

    // Thay alias ở từ đầu tiên của mỗi lệnh. Chỉ thay một cấp để tránh lặp vô hạn.
    for (auto& cmd : pipeline.commands) {
        if (cmd.args.empty()) continue;

        auto it = aliases_.find(cmd.args[0]);
        if (it == aliases_.end()) continue;

        std::vector<std::string> words = split_words(it->second);
        if (words.empty()) continue;

        cmd.args.erase(cmd.args.begin());
        cmd.args.insert(cmd.args.begin(), words.begin(), words.end());
    }

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
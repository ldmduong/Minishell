#pragma once
#include <string>
#include <vector>

#include "minishell/core/registry.h"

namespace minishell {

class Shell {
public:
    Shell();

    int run();

    // Phân tích và chạy một dòng lệnh, trả về mã thoát.
    int execute_line(const std::string& line);

    // Yêu cầu shell thoát sau lệnh hiện tại (dùng cho lệnh exit).
    void request_exit(int code = 0) {
        running_ = false;
        exit_code_ = code;
    }

    const std::vector<std::string>& history() const { return history_; }

    Registry& registry() { return registry_; }
    const Registry& registry() const { return registry_; }

    std::string prev_dir;  // thư mục trước đó, dùng cho "cd -"

private:
    std::string prompt() const;
    bool running_ = true;
    int exit_code_ = 0;
    std::vector<std::string> history_;
    Registry registry_;
};

}  // namespace minishell
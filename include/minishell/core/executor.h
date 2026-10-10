#pragma once
#include "minishell/core/parser.h"

namespace minishell {

class Shell;

class Executor {
public:
    // Chạy một pipeline, trả về mã thoát của lệnh cuối (job nền trả về 0).
    static int run(Shell& shell, const Pipeline& pipeline);

    // Thu dọn các tiến trình nền đã kết thúc.
    static void reap_background();
};

}  // namespace minishell
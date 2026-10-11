#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: schedule (lập lịch chạy lệnh sau một khoảng thời gian, hoặc lặp lại).
void register_scheduler(Registry& registry);

}  // namespace minishell::builtins
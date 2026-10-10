#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: threads (xem luồng của tiến trình, hoặc chạy demo luồng).
void register_threads(Registry& registry);

}  // namespace minishell::builtins
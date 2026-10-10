#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: threads (liệt kê luồng hoặc chạy demo).
void register_threads(Registry& registry);

}  // namespace minishell::builtins
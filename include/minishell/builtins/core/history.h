#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: history (in các lệnh đã nhập trong phiên).
void register_history(Registry& registry);

}  // namespace minishell::builtins
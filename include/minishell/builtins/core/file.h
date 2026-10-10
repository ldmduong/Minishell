#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: touch, rm [-f], cat.
void register_file(Registry& registry);

}  // namespace minishell::builtins
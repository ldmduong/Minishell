#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: env, export, unset.
void register_environment(Registry& registry);

}  // namespace minishell::builtins
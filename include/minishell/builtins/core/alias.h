#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: alias, unalias.
void register_alias(Registry& registry);

}  // namespace minishell::builtins
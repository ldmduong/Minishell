#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: mkdir [-p], rmdir.
void register_directory(Registry& registry);

}  // namespace minishell::builtins
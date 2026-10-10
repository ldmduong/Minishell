#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: procs, kill, stop, resume.
void register_process(Registry& registry);

}  // namespace minishell::builtins
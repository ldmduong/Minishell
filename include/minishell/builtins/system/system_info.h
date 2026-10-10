#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {

// Đăng ký: ram, cpu, uptime.
void register_system_info(Registry& registry);

}  // namespace minishell::builtins
#pragma once

#include <sys/types.h>

#include "minishell/core/registry.h"

namespace minishell::builtins {

void register_scheduler(Registry &registry);
void forget_scheduled(pid_t pid);

} // namespace minishell::builtins

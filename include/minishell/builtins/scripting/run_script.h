#pragma once

#include "minishell/core/registry.h"

namespace minishell::builtins {

void register_scripting(Registry &registry);

} // namespace minishell::builtins
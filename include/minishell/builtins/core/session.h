#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {
void register_session(Registry& registry);  // exit, clear
}
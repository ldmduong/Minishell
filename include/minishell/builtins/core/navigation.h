#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {
void register_navigation(Registry& registry);  // cd, pwd
}
#pragma once
#include "minishell/core/registry.h"

namespace minishell::builtins {
void register_navigation(Registry& registry);  // cd, pwd
std::string current_directory();
int change_directory(Shell &shell, const std::string &target,
                     const char *command, bool print = false);
}

#include "minishell/core/shell.h"
#include <iostream>

int main() {
    std::cout << "minishell OK" << std::endl;
    minishell::Shell shell;
    return shell.run();
}
#pragma once    
#include <string>
#include <vector> 

namespace minishell {
bool read_line(const std::string& prompt, const std::vector<std::string>& history, std::string& out);
}
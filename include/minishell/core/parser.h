#pragma once 
#include <string>
#include <vector>

namespace minishell {
struct Command
{
    std::vector<std::string> args;
    std::string in_file;
    std::string out_file;
    bool append = false;

};
struct Pipeline {
    std::vector<Command> commands;
    bool background = false;  
};
class Parser {
public:
    static bool parse(const std::string& line, Pipeline& out, std::string& error);

};




}
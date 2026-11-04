#include "minishell/core/parser.h"

#include <string>
#include <vector>

int main() {
  minishell::Pipeline pipeline;
  std::string error;

  if (!minishell::Parser::parse(
          "echo \"one two\" 'three' escaped\\ word # ignored", pipeline,
          error)) {
    return 1;
  }
  if (pipeline.commands.size() != 1 ||
      pipeline.commands[0].args != std::vector<std::string>{"echo", "one two",
                                                            "three",
                                                            "escaped word"}) {
    return 2;
  }

  if (!minishell::Parser::parse("cat < input | grep x >> output &", pipeline,
                                error)) {
    return 3;
  }
  if (!pipeline.background || pipeline.commands.size() != 2 ||
      pipeline.commands[0].in_file != "input" ||
      pipeline.commands[1].args[0] != "grep" ||
      pipeline.commands[1].out_file != "output" ||
      !pipeline.commands[1].append) {
    return 4;
  }

  for (const char *invalid :
       {"| echo", "echo |", "echo >", "echo & other", "echo 'unfinished"}) {
    if (minishell::Parser::parse(invalid, pipeline, error) || error.empty())
      return 5;
  }

  if (!minishell::Parser::parse("   # comment", pipeline, error) ||
      !pipeline.commands.empty() || !error.empty()) {
    return 6;
  }
  return 0;
}

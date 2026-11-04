#include "minishell/core/registry.h"

#include <utility>

namespace minishell {

void Registry::add(const std::string &name, Handler handler,
                   const std::string &help) {
  commands_[name] = {std::move(handler), help};
}

bool Registry::has(const std::string &name) const {
  return commands_.find(name) != commands_.end();
}

int Registry::run(Shell &shell, const std::vector<std::string> &args) const {
  if (args.empty())
    return 0;

  auto command = commands_.find(args[0]);
  if (command == commands_.end())
    return 127;
  return command->second.handler(shell, args);
}

} // namespace minishell

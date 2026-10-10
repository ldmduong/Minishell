#include "minishell/core/registry.h"

namespace minishell {

void Registry::add(const std::string& name, Handler handler, const std::string& help) {
    commands_[name] = CommandInfo{std::move(handler), help};
}

bool Registry::has(const std::string& name) const {
    return commands_.find(name) != commands_.end();
}

int Registry::run(Shell& shell, const std::vector<std::string>& args) const {
    if (args.empty()) return 0;
    auto it = commands_.find(args[0]);
    if (it == commands_.end()) return 127;  // không tìm thấy lệnh
    return it->second.handler(shell, args);
}

}  // namespace minishell
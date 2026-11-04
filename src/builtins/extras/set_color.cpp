#include "minishell/builtins/extras/set_color.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

int color_command(Shell &, const std::vector<std::string> &args) {
  static const std::map<std::string, std::string> colors = {
      {"reset", "\033[0m"},   {"red", "\033[31m"},   {"green", "\033[32m"},
      {"yellow", "\033[33m"}, {"blue", "\033[34m"},  {"magenta", "\033[35m"},
      {"cyan", "\033[36m"},   {"white", "\033[37m"},
  };

  if (args.size() != 2) {
    std::cerr
        << "cách dùng: color reset|red|green|yellow|blue|magenta|cyan|white\n";
    return 1;
  }
  auto color = colors.find(args[1]);
  if (color == colors.end()) {
    std::cerr << "color: màu không hỗ trợ: " << args[1] << '\n';
    return 1;
  }
  std::cout << color->second << std::flush;
  return 0;
}

} // namespace

void register_set_color(Registry &registry) {
  registry.add("color", color_command,
               "color reset|red|green|yellow|blue|magenta|cyan|white");
}

} // namespace minishell::builtins
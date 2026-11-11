#pragma once

#include <functional>
#include <string>
#include <vector>

namespace minishell {

enum class RedirectMode { read, write, append };

struct Redirect {
  RedirectMode mode;
  std::string path;
};

struct Command {
  std::vector<std::string> args;
  std::vector<Redirect> redirects;
};

struct Pipeline {
  std::vector<Command> commands;
  bool background = false;
};

class Parser {
public:
  using ExpandWord = std::function<std::string(const std::string &)>;

  static bool parse(const std::string &line, Pipeline &out, std::string &error,
                    const ExpandWord &expand = {},
                    std::vector<std::string> *literal_words = nullptr);
};

} // namespace minishell

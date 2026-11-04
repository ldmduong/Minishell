#pragma once

#include <map>
#include <string>
#include <vector>

#include "minishell/core/parser.h"
#include "minishell/core/registry.h"

namespace minishell {

class Shell {
public:
  Shell();

  int run();
  int execute_line(const std::string &line);
  int execute_pipeline(Pipeline pipeline);
  void request_exit(int code = 0);
  bool running() const { return running_; }

  const std::vector<std::string> &history() const { return history_; }

  Registry &registry() { return registry_; }
  const Registry &registry() const { return registry_; }

  std::map<std::string, std::string> &aliases() { return aliases_; }
  const std::map<std::string, std::string> &aliases() const { return aliases_; }

  std::string prev_dir;

private:
  std::string prompt() const;

  bool running_ = true;
  int exit_code_ = 0;
  std::vector<std::string> history_;
  std::map<std::string, std::string> aliases_;
  Registry registry_;
};

inline void Shell::request_exit(int code) {
  running_ = false;
  exit_code_ = code;
}

} // namespace minishell

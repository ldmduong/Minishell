#include "minishell/core/shell.h"

#include <csignal>
#include <iostream>
#include <utility>

#include "minishell/builtins/core/alias.h"
#include "minishell/builtins/core/directory.h"
#include "minishell/builtins/core/environment.h"
#include "minishell/builtins/core/file.h"
#include "minishell/builtins/core/help.h"
#include "minishell/builtins/core/history.h"
#include "minishell/builtins/core/navigation.h"
#include "minishell/builtins/core/session.h"
#include "minishell/builtins/extras/base_conversion.h"
#include "minishell/builtins/extras/bookmark.h"
#include "minishell/builtins/extras/dancing.h"
#include "minishell/builtins/extras/set_color.h"
#include "minishell/builtins/scripting/run_script.h"
#include "minishell/builtins/scripting/scheduler.h"
#include "minishell/builtins/system/process.h"
#include "minishell/builtins/system/system_info.h"
#include "minishell/builtins/system/threads.h"
#include "minishell/core/executor.h"
#include "minishell/core/line_editor.h"
#include "minishell/core/parser.h"

namespace minishell {

Shell::Shell() {
  signal(SIGINT, SIG_IGN);

  builtins::register_navigation(registry_);
  builtins::register_session(registry_);
  builtins::register_help(registry_);
  builtins::register_history(registry_);
  builtins::register_directory(registry_);
  builtins::register_file(registry_);
  builtins::register_environment(registry_);
  builtins::register_alias(registry_);
  builtins::register_system_info(registry_);
  builtins::register_process(registry_);
  builtins::register_threads(registry_);
  builtins::register_scripting(registry_);
  builtins::register_scheduler(registry_);
  builtins::register_base_conversion(registry_);
  builtins::register_bookmark(registry_);
  builtins::register_set_color(registry_);
  builtins::register_dancing(registry_);
}

std::string Shell::prompt() const { return "minishell$ "; }

int Shell::execute_line(const std::string &line) {
  Pipeline pipeline;
  std::string error;
  if (!Parser::parse(line, pipeline, error)) {
    std::cerr << "minishell: lỗi cú pháp: " << error << '\n';
    return 2;
  }

  return execute_pipeline(std::move(pipeline));
}

int Shell::execute_pipeline(Pipeline pipeline) {
  std::string error;
  for (Command &command : pipeline.commands) {
    if (command.args.empty())
      continue;

    auto alias = aliases_.find(command.args[0]);
    if (alias == aliases_.end())
      continue;

    Pipeline replacement;
    if (!Parser::parse(alias->second, replacement, error) ||
        replacement.background || replacement.commands.size() != 1) {
      continue;
    }

    const Command &alias_command = replacement.commands.front();
    if (!alias_command.in_file.empty() || !alias_command.out_file.empty() ||
        alias_command.args.empty()) {
      continue;
    }

    command.args.erase(command.args.begin());
    command.args.insert(command.args.begin(), alias_command.args.begin(),
                        alias_command.args.end());
  }

  if (pipeline.commands.empty())
    return 0;
  return Executor::run(*this, pipeline);
}

int Shell::run() {
  std::string line;
  while (running_) {
    Executor::reap_background();
    if (!read_line(prompt(), history_, line))
      break;

    if (line.find_first_not_of(" \t") == std::string::npos)
      continue;
    history_.push_back(line);
    execute_line(line);
  }

  return exit_code_;
}

} // namespace minishell

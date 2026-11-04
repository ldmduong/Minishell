#include "minishell/builtins/extras/dancing.h"

#include <chrono>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

volatile std::sig_atomic_t interrupted = 0;

void handle_interrupt(int) { interrupted = 1; }

int dance_command(Shell &, const std::vector<std::string> &args) {
  int cycles = 1;
  if (args.size() > 2) {
    std::cerr << "cách dùng: dance [SỐ_VÒNG 1-5]\n";
    return 1;
  }
  if (args.size() == 2) {
    try {
      size_t used = 0;
      cycles = std::stoi(args[1], &used);
      if (used != args[1].size())
        cycles = 0;
    } catch (const std::exception &) {
      cycles = 0;
    }
  }
  if (cycles < 1 || cycles > 5) {
    std::cerr << "dance: số vòng phải từ 1 đến 5\n";
    return 1;
  }

  static const char *frames[] = {"\\o/  ", " |   ", "/ \\  ", " |   "};
  struct sigaction action{};
  struct sigaction previous{};
  action.sa_handler = handle_interrupt;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGINT, &action, &previous) != 0) {
    std::perror("dance: sigaction");
    return 1;
  }
  interrupted = 0;
  for (int cycle = 0; cycle < cycles && !interrupted; ++cycle) {
    for (const char *frame : frames) {
      std::cout << "\r" << frame << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(120));
      if (interrupted)
        break;
    }
  }
  sigaction(SIGINT, &previous, nullptr);
  std::cout << '\n';
  return interrupted ? 130 : 0;
}

} // namespace

void register_dancing(Registry &registry) {
  registry.add("dance", dance_command,
               "dance [SỐ_VÒNG 1-5]  hiệu ứng vui trong terminal");
}

} // namespace minishell::builtins
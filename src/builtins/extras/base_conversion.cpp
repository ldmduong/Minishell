#include "minishell/builtins/extras/base_conversion.h"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

bool parse_base(const std::string &text, int &base) {
  try {
    size_t used = 0;
    int value = std::stoi(text, &used);
    if (used != text.size() || value < 2 || value > 36)
      return false;
    base = value;
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

int base_command(Shell &, const std::vector<std::string> &args) {
  if (args.size() != 4) {
    std::cerr << "cách dùng: base SỐ CƠ_SỐ_NGUỒN CƠ_SỐ_ĐÍCH (2-36)\n";
    return 1;
  }

  int source_base = 0;
  int target_base = 0;
  if (!parse_base(args[2], source_base) || !parse_base(args[3], target_base)) {
    std::cerr << "base: cơ số phải nằm trong khoảng 2-36\n";
    return 1;
  }

  long long value = 0;
  const char *begin = args[1].data();
  const char *end = begin + args[1].size();
  auto parsed = std::from_chars(begin, end, value, source_base);
  if (parsed.ec != std::errc{} || parsed.ptr != end) {
    std::cerr << "base: số không hợp lệ hoặc vượt quá giới hạn\n";
    return 1;
  }

  static constexpr char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  bool negative = value < 0;
  uint64_t remaining = negative ? uint64_t{0} - static_cast<uint64_t>(value)
                                : static_cast<uint64_t>(value);
  std::string output;
  do {
    output += digits[remaining % target_base];
    remaining /= target_base;
  } while (remaining != 0);

  if (negative)
    output += '-';
  for (size_t left = 0, right = output.size() - 1; left < right;
       ++left, --right) {
    std::swap(output[left], output[right]);
  }
  std::cout << output << '\n';
  return 0;
}

} // namespace

void register_base_conversion(Registry &registry) {
  registry.add("base", base_command, "base SỐ CƠ_SỐ_NGUỒN CƠ_SỐ_ĐÍCH (2-36)");
}

} // namespace minishell::builtins
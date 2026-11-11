#include "minishell/builtins/scripting/run_script.h"

#include <sys/stat.h>

#include <charconv>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "minishell/core/parser.h"
#include "minishell/core/shell.h"

namespace minishell::builtins {
namespace {

constexpr unsigned int max_script_depth = 32;
constexpr int interrupted_status = 128 + SIGINT;
volatile std::sig_atomic_t interrupted = 0;

void interrupt_script(int) { interrupted = 1; }

struct ScriptState {
  unsigned int depth = 0;
  bool failed = false;
};

bool valid_control_line(const Pipeline &pipeline, const std::string &keyword,
                        std::string &error) {
  if (pipeline.commands.size() == 1 && !pipeline.background &&
      pipeline.commands.front().redirects.empty())
    return true;
  if (keyword == "if" || keyword == "else" || keyword == "endif" ||
      keyword == "for" || keyword == "endfor" || keyword == "function" ||
      keyword == "endfunction" || keyword == "call") {
    error = "'" + keyword + "' không hỗ trợ pipe, chuyển hướng hoặc chạy nền";
    return false;
  }
  return true;
}

bool parse_words(const std::string &line, std::vector<std::string> &words,
                 std::string &error) {
  Pipeline pipeline;
  if (!Parser::parse(line, pipeline, error))
    return false;
  words = pipeline.commands.empty() ? std::vector<std::string>{}
                                    : pipeline.commands[0].args;
  return words.empty() || valid_control_line(pipeline, words[0], error);
}

std::string value_of(const std::string &name,
                     const std::map<std::string, std::string> &variables,
                     const std::vector<std::string> &arguments) {
  auto variable = variables.find(name);
  if (variable != variables.end())
    return variable->second;
  if (!name.empty() && name[0] >= '1' && name[0] <= '9') {
    unsigned long index = std::strtoul(name.c_str(), nullptr, 10);
    if (index <= arguments.size())
      return arguments[index - 1];
    return {};
  }
  if (name == "@") {
    std::string joined;
    for (const std::string &argument : arguments) {
      if (!joined.empty())
        joined += ' ';
      joined += argument;
    }
    return joined;
  }
  const char *environment = std::getenv(name.c_str());
  return environment == nullptr ? std::string() : std::string(environment);
}

std::string expand(const std::string &text,
                   const std::map<std::string, std::string> &variables,
                   const std::vector<std::string> &arguments) {
  std::string result;
  for (size_t i = 0; i < text.size();) {
    if (text[i] != '$' || i + 1 == text.size()) {
      result += text[i++];
      continue;
    }

    size_t start = i + 1;
    size_t end = start;
    if (text[start] == '{') {
      start++;
      end = text.find('}', start);
      if (end == std::string::npos) {
        result += text[i++];
        continue;
      }
      result += value_of(text.substr(start, end - start), variables, arguments);
      i = end + 1;
    } else if (text[start] == '@') {
      result += value_of("@", variables, arguments);
      i = start + 1;
    } else if (text[start] >= '0' && text[start] <= '9') {
      while (end < text.size() && text[end] >= '0' && text[end] <= '9')
        ++end;
      result += value_of(text.substr(start, end - start), variables, arguments);
      i = end;
    } else if ((text[start] >= 'A' && text[start] <= 'Z') ||
               (text[start] >= 'a' && text[start] <= 'z') ||
               text[start] == '_') {
      while (end < text.size() &&
             ((text[end] >= 'A' && text[end] <= 'Z') ||
              (text[end] >= 'a' && text[end] <= 'z') ||
              (text[end] >= '0' && text[end] <= '9') || text[end] == '_'))
        ++end;
      result += value_of(text.substr(start, end - start), variables, arguments);
      i = end;
    } else {
      result += '$';
      ++i;
    }
  }
  return result;
}

bool find_end(const std::vector<std::string> &lines, size_t start, size_t finish,
              const std::string &open, const std::string &close, size_t &end,
              std::string &error) {
  int nested = 0;
  for (size_t i = start + 1; i < finish; ++i) {
    std::vector<std::string> words;
    if (!parse_words(lines[i], words, error))
      return false;
    if (words.empty())
      continue;
    if (words[0] == open) {
      ++nested;
    } else if (words[0] == close) {
      if (nested == 0) {
        end = i;
        return true;
      }
      --nested;
    }
  }
  error = "thiếu '" + close + "'";
  return false;
}

bool find_if_parts(const std::vector<std::string> &lines, size_t start,
                   size_t finish, size_t &otherwise, size_t &end,
                   std::string &error) {
  int nested = 0;
  otherwise = finish;
  for (size_t i = start + 1; i < finish; ++i) {
    std::vector<std::string> words;
    if (!parse_words(lines[i], words, error))
      return false;
    if (words.empty())
      continue;
    if (words[0] == "if") {
      ++nested;
    } else if (words[0] == "endif") {
      if (nested == 0) {
        end = i;
        return true;
      }
      --nested;
    } else if (words[0] == "else" && nested == 0) {
      if (otherwise != finish) {
        error = "if chỉ được có một else";
        return false;
      }
      otherwise = i;
    }
  }
  error = "thiếu 'endif'";
  return false;
}

bool number(const std::string &text, long long &value) {
  const char *begin = text.data();
  const char *end = begin + text.size();
  auto parsed = std::from_chars(begin, end, value);
  return parsed.ec == std::errc{} && parsed.ptr == end;
}

bool evaluate(const std::vector<std::string> &words, bool &result,
              std::string &error) {
  if (words.size() == 2 && (words[0] == "-n" || words[0] == "-z")) {
    result = (words[1].empty()) == (words[0] == "-z");
    return true;
  }

  if (words.size() == 2 &&
      (words[0] == "-e" || words[0] == "-f" || words[0] == "-d")) {
    struct stat info{};
    if (stat(words[1].c_str(), &info) != 0) {
      result = false;
      return true;
    }
    if (words[0] == "-f")
      result = S_ISREG(info.st_mode);
    else if (words[0] == "-d")
      result = S_ISDIR(info.st_mode);
    else
      result = true;
    return true;
  }

  if (words.size() != 3) {
    error = "điều kiện không hợp lệ";
    return false;
  }
  if (words[1] == "==" || words[1] == "=") {
    result = words[0] == words[2];
    return true;
  }
  if (words[1] == "!=") {
    result = words[0] != words[2];
    return true;
  }

  long long left = 0;
  long long right = 0;
  if (words[1] == "-eq" || words[1] == "-ne" || words[1] == "-gt" ||
      words[1] == "-ge" || words[1] == "-lt" || words[1] == "-le") {
    if (!number(words[0], left) || !number(words[2], right)) {
      error = "so sánh số cần hai số nguyên hợp lệ";
      return false;
    }
    if (words[1] == "-eq")
      result = left == right;
    else if (words[1] == "-ne")
      result = left != right;
    else if (words[1] == "-gt")
      result = left > right;
    else if (words[1] == "-ge")
      result = left >= right;
    else if (words[1] == "-lt")
      result = left < right;
    else
      result = left <= right;
    return true;
  }

  error = "toán tử điều kiện không được hỗ trợ: " + words[1];
  return false;
}

bool valid_variable(const std::string &name) {
  if (name.empty() || !((name[0] >= 'A' && name[0] <= 'Z') ||
                        (name[0] >= 'a' && name[0] <= 'z') || name[0] == '_'))
    return false;
  for (char character : name) {
    if (!((character >= 'A' && character <= 'Z') ||
          (character >= 'a' && character <= 'z') ||
          (character >= '0' && character <= '9') || character == '_'))
      return false;
  }
  return true;
}

class Script {
public:
  Script(Shell &shell, ScriptState &state) : shell_(shell), state_(state) {}

  int run(const std::vector<std::string> &lines,
          const std::vector<std::string> &arguments) {
    return execute(lines, 0, lines.size(), arguments, 0);
  }

private:
  int execute(const std::vector<std::string> &lines, size_t start,
              size_t finish, const std::vector<std::string> &arguments,
              unsigned int depth) {
    if (depth > max_script_depth) {
      std::cerr << "run_script: vượt quá độ sâu lồng 32\n";
      state_.failed = true;
      return 1;
    }

    int result = 0;
    for (size_t i = start; i < finish && shell_.running() &&
                           !state_.failed && !interrupted &&
                           result != interrupted_status;) {
      Pipeline pipeline;
      std::string error;
      std::vector<std::string> literal_words;
      auto expand_word = [&](const std::string &word) {
        return expand(word, variables_, arguments);
      };
      if (!Parser::parse(lines[i], pipeline, error, expand_word, &literal_words)) {
        report(i, error);
        return 2;
      }
      if (pipeline.commands.empty()) {
        ++i;
        continue;
      }

      const auto &words = pipeline.commands.front().args;
      const std::string &keyword = literal_words[0];
      if (!valid_control_line(pipeline, keyword, error)) {
        report(i, error);
        return 2;
      }
      if (keyword == "if") {
        size_t otherwise = finish;
        size_t end = 0;
        if (!find_if_parts(lines, i, finish, otherwise, end, error)) {
          report(i, error);
          return 2;
        }
        std::vector<std::string> condition(words.begin() + 1, words.end());
        bool condition_result = false;
        if (!evaluate(condition, condition_result, error)) {
          report(i, error);
          return 2;
        }
        if (condition_result) {
          size_t block_end = otherwise == finish ? end : otherwise;
          result = execute(lines, i + 1, block_end, arguments, depth + 1);
        } else if (otherwise != finish) {
          result = execute(lines, otherwise + 1, end, arguments, depth + 1);
        }
        i = end + 1;
      } else if (keyword == "for") {
        size_t end = 0;
        if (words.size() < 4 || !valid_variable(literal_words[1]) ||
            literal_words[2] != "in" ||
            !find_end(lines, i, finish, "for", "endfor", end, error)) {
          if (error.empty())
            error = "cách dùng: for TÊN in GIÁ_TRỊ...";
          report(i, error);
          return 2;
        }
        const std::string &name = literal_words[1];
        auto old_value = variables_.find(name);
        bool had_old_value = old_value != variables_.end();
        std::string saved_value =
            had_old_value ? old_value->second : std::string();
        for (size_t value = 3; value < words.size(); ++value) {
          variables_[name] = words[value];
          result = execute(lines, i + 1, end, arguments, depth + 1);
          if (!shell_.running() || state_.failed || interrupted ||
              result == interrupted_status)
            break;
        }
        if (had_old_value)
          variables_[name] = saved_value;
        else
          variables_.erase(name);
        i = end + 1;
      } else if (keyword == "function") {
        size_t end = 0;
        if (words.size() != 2 ||
            !find_end(lines, i, finish, "function", "endfunction", end, error)) {
          if (error.empty())
            error = "cách dùng: function TÊN";
          report(i, error);
          return 2;
        }
        functions_[literal_words[1]] =
            std::vector<std::string>(lines.begin() + i + 1, lines.begin() + end);
        i = end + 1;
      } else if (keyword == "else" || keyword == "endif" ||
                 keyword == "endfor" || keyword == "endfunction") {
        report(i, "khối '" + keyword + "' không có lệnh mở tương ứng");
        return 2;
      } else if (keyword == "call") {
        if (words.size() < 2) {
          report(i, "cách dùng: call TÊN_HÀM [ĐỐI_SỐ]...");
          return 2;
        }
        auto function = functions_.find(words[1]);
        if (function == functions_.end()) {
          report(i, "không tìm thấy hàm: " + words[1]);
          return 1;
        }
        std::vector<std::string> function_arguments(words.begin() + 2, words.end());
        const auto body = function->second;
        result = execute(body, 0, body.size(), function_arguments, depth + 1);
        ++i;
      } else {
        result = shell_.execute_pipeline(std::move(pipeline));
        ++i;
      }
    }
    return result;
  }

  void report(size_t line, const std::string &error) {
    state_.failed = true;
    std::cerr << "run_script: dòng " << line + 1 << ": " << error << '\n';
  }

  Shell &shell_;
  ScriptState &state_;
  std::map<std::string, std::string> variables_;
  std::map<std::string, std::vector<std::string>> functions_;
};

int run_script_command(Shell &shell, const std::vector<std::string> &args) {
  static ScriptState state;
  if (state.depth >= max_script_depth) {
    std::cerr << "run_script: vượt quá độ sâu gọi tệp 32\n";
    state.failed = true;
    return 1;
  }
  if (args.size() < 2) {
    std::cerr << "cách dùng: run_script TỆP [ĐỐI_SỐ]...\n";
    return 1;
  }

  std::ifstream file(args[1]);
  if (!file) {
    std::cerr << "run_script: không mở được " << args[1] << '\n';
    return 1;
  }
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(file, line))
    lines.push_back(line);
  if (file.bad()) {
    std::cerr << "run_script: lỗi khi đọc " << args[1] << '\n';
    return 1;
  }
  file.close();

  std::vector<std::string> arguments(args.begin() + 2, args.end());
  struct sigaction previous{};
  if (state.depth == 0) {
    state.failed = false;
    interrupted = 0;
    struct sigaction action{};
    action.sa_handler = interrupt_script;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, &previous) != 0) {
      std::perror("run_script: sigaction");
      return 1;
    }
  }

  ++state.depth;
  Script script(shell, state);
  int result = script.run(lines, arguments);
  if (--state.depth == 0)
    sigaction(SIGINT, &previous, nullptr);
  return interrupted ? interrupted_status : result;
}

} // namespace

void register_scripting(Registry &registry) {
  registry.add("run_script", run_script_command,
               "run_script TỆP [ĐỐI_SỐ]...  chạy script (if/for/function)");
}

} // namespace minishell::builtins

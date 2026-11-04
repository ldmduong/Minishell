#include "minishell/core/parser.h"

#include <string>
#include <vector>

namespace minishell {
namespace {

struct Token {
  std::string text;
  bool is_operator;
};

bool tokenize(const std::string &line, std::vector<Token> &tokens,
              std::string &error) {
  std::string word;
  bool in_word = false;

  auto add_word = [&]() {
    if (in_word) {
      tokens.push_back({word, false});
      word.clear();
      in_word = false;
    }
  };

  for (size_t i = 0; i < line.size();) {
    char ch = line[i];

    if (ch == ' ' || ch == '\t') {
      add_word();
      ++i;
    } else if (ch == '#' && !in_word) {
      break;
    } else if (ch == '\'' || ch == '"') {
      const char quote = ch;
      in_word = true;
      ++i;

      while (i < line.size() && line[i] != quote) {
        if (quote == '"' && line[i] == '\\' && i + 1 < line.size() &&
            (line[i + 1] == '"' || line[i + 1] == '\\')) {
          ++i;
        }
        word += line[i++];
      }

      if (i == line.size()) {
        error = quote == '\'' ? "thiếu dấu nháy đơn đóng"
                              : "thiếu dấu nháy kép đóng";
        return false;
      }
      ++i;
    } else if (ch == '\\' && i + 1 < line.size()) {
      in_word = true;
      word += line[i + 1];
      i += 2;
    } else if (ch == '|' || ch == '<' || ch == '&' || ch == '>') {
      add_word();

      std::string op(1, ch);
      ++i;
      if (ch == '>' && i < line.size() && line[i] == '>') {
        op = ">>";
        ++i;
      }
      tokens.push_back({op, true});
    } else {
      in_word = true;
      word += ch;
      ++i;
    }
  }

  add_word();
  return true;
}

} // namespace

bool Parser::parse(const std::string &line, Pipeline &out, std::string &error) {
  out = Pipeline{};
  error.clear();

  std::vector<Token> tokens;
  if (!tokenize(line, tokens, error))
    return false;
  if (tokens.empty())
    return true;

  Command command;
  for (size_t i = 0; i < tokens.size(); ++i) {
    const Token &token = tokens[i];

    if (!token.is_operator) {
      command.args.push_back(token.text);
      continue;
    }

    if (token.text == "|") {
      if (command.args.empty()) {
        error = "thiếu lệnh trước dấu |";
        return false;
      }
      out.commands.push_back(command);
      command = Command{};
      continue;
    }

    if (token.text == "&") {
      if (i + 1 != tokens.size()) {
        error = "& chỉ được đặt ở cuối dòng";
        return false;
      }
      out.background = true;
      continue;
    }

    if (i + 1 == tokens.size() || tokens[i + 1].is_operator) {
      error = "thiếu tên file sau '" + token.text + "'";
      return false;
    }

    const std::string &file = tokens[++i].text;
    if (token.text == "<") {
      command.in_file = file;
    } else {
      command.out_file = file;
      command.append = token.text == ">>";
    }
  }

  if (command.args.empty()) {
    error = "thiếu lệnh sau dấu | hoặc trước &";
    return false;
  }

  out.commands.push_back(command);
  return true;
}

} // namespace minishell

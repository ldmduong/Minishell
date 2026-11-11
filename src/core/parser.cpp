#include "minishell/core/parser.h"

#include <string>
#include <utility>
#include <vector>

namespace minishell {
namespace {

struct Token {
  std::string text;
  std::string expanded;
  bool is_operator = false;
};

bool tokenize(const std::string &line, std::vector<Token> &tokens,
              std::string &error, const Parser::ExpandWord &expand) {
  Token word;
  std::string fragment;
  bool in_word = false;
  char quote = '\0';

  auto add_fragment = [&](bool literal) {
    if (fragment.empty())
      return;
    word.text += fragment;
    if (expand)
      word.expanded += literal ? fragment : expand(fragment);
    fragment.clear();
  };

  auto add_word = [&]() {
    if (in_word) {
      add_fragment(false);
      tokens.push_back(std::move(word));
      word = Token{};
      in_word = false;
    }
  };

  for (size_t i = 0; i < line.size();) {
    char ch = line[i];

    if (quote != '\0' && ch == quote) {
      add_fragment(quote == '\'');
      quote = '\0';
      ++i;
    } else if (ch == '\\' && i + 1 < line.size() &&
               (quote == '\0' ||
                (quote == '"' && (line[i + 1] == '"' ||
                                  line[i + 1] == '\\' ||
                                  (expand && line[i + 1] == '$'))))) {
      add_fragment(false);
      fragment += line[i + 1];
      add_fragment(true);
      in_word = true;
      i += 2;
    } else if (quote != '\0') {
      fragment += ch;
      ++i;
    } else if (ch == ' ' || ch == '\t') {
      add_word();
      ++i;
    } else if (ch == '#' && !in_word) {
      break;
    } else if (ch == '\'' || ch == '"') {
      add_fragment(false);
      quote = ch;
      in_word = true;
      ++i;
    } else if (ch == '|' || ch == '<' || ch == '&' || ch == '>') {
      add_word();

      std::string op(1, ch);
      ++i;
      if (ch == '>' && i < line.size() && line[i] == '>') {
        op = ">>";
        ++i;
      }
      tokens.push_back({op, {}, true});
    } else {
      in_word = true;
      fragment += ch;
      ++i;
    }
  }

  if (quote != '\0') {
    error = quote == '\'' ? "thiếu dấu nháy đơn đóng"
                          : "thiếu dấu nháy kép đóng";
    return false;
  }
  add_word();
  return true;
}

} // namespace

bool Parser::parse(const std::string &line, Pipeline &out, std::string &error,
                   const ExpandWord &expand,
                   std::vector<std::string> *literal_words) {
  out = Pipeline{};
  error.clear();
  if (literal_words)
    literal_words->clear();

  std::vector<Token> tokens;
  if (!tokenize(line, tokens, error, expand))
    return false;
  if (tokens.empty())
    return true;

  Command command;
  for (size_t i = 0; i < tokens.size(); ++i) {
    const Token &token = tokens[i];

    if (!token.is_operator) {
      if (literal_words && out.commands.empty())
        literal_words->push_back(token.text);
      command.args.push_back(expand ? token.expanded : token.text);
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

    const Token &file = tokens[++i];
    RedirectMode mode = RedirectMode::read;
    if (token.text != "<")
      mode = token.text == ">>" ? RedirectMode::append : RedirectMode::write;
    command.redirects.push_back({mode, expand ? file.expanded : file.text});
  }

  if (command.args.empty()) {
    error = "thiếu lệnh sau dấu | hoặc trước &";
    return false;
  }

  out.commands.push_back(command);
  return true;
}

} // namespace minishell

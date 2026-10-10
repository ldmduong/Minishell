#include "minishell/core/parser.h"
#include <iostream>

namespace minishell {

namespace {

// Token: từ thường hoặc toán tử (| < > >> &)
struct Token {
    std::string text;
    bool is_operator;
};

// Tách dòng thành token, xử lý nháy đơn, nháy kép, backslash và chú thích #.
bool tokenize(const std::string& line, std::vector<Token>& tokens, std::string& error) {
    std::string cur;
    bool in_word = false;
    size_t i = 0;
    const size_t n = line.size();

    auto flush = [&]() {
        if (in_word) {
            tokens.push_back({cur, false});
            cur.clear();
            in_word = false;
        }
    };

    while (i < n) {
        char c = line[i];

        if (c == ' ' || c == '\t') {
            flush();
            ++i;
        } else if (c == '#' && !in_word) {
            break;  // chú thích đến hết dòng
        } else if (c == '\'') {
            in_word = true;
            ++i;
            while (i < n && line[i] != '\'') cur += line[i++];
            if (i >= n) {
                error = "thiếu dấu nháy đơn đóng";
                return false;
            }
            ++i;
        } else if (c == '"') {
            in_word = true;
            ++i;
            while (i < n && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < n && (line[i + 1] == '"' || line[i + 1] == '\\')) {
                    ++i;
                }
                cur += line[i++];
            }
            if (i >= n) {
                error = "thiếu dấu nháy kép đóng";
                return false;
            }
            ++i;
        } else if (c == '\\' && i + 1 < n) {
            in_word = true;
            cur += line[i + 1];
            i += 2;
        } else if (c == '|' || c == '<' || c == '&') {
            flush();
            tokens.push_back({std::string(1, c), true});
            ++i;
        } else if (c == '>') {
            flush();
            if (i + 1 < n && line[i + 1] == '>') {
                tokens.push_back({">>", true});
                i += 2;
            } else {
                tokens.push_back({">", true});
                ++i;
            }
        } else {
            in_word = true;
            cur += c;
            ++i;
        }
    }
    flush();
    return true;
}

}  // namespace

bool Parser::parse(const std::string& line, Pipeline& out, std::string& error) {
    out = Pipeline{};

    std::vector<Token> tokens;
    if (!tokenize(line, tokens, error)) return false;
    if (tokens.empty()) return true;

    Command cur;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& t = tokens[i];

        if (!t.is_operator) {
            cur.args.push_back(t.text);
            continue;
        }

        if (t.text == "|") {
            if (cur.args.empty()) {
                error = "thiếu lệnh trước dấu |";
                return false;
            }
            out.commands.push_back(cur);
            cur = Command{};
        } else if (t.text == "&") {
            if (i + 1 != tokens.size()) {
                error = "& chỉ được đặt ở cuối dòng";
                return false;
            }
            out.background = true;
        } else {  // <, >, >>: token kế tiếp phải là tên file
            if (i + 1 >= tokens.size() || tokens[i + 1].is_operator) {
                error = "thiếu tên file sau '" + t.text + "'";
                return false;
            }
            const std::string& file = tokens[++i].text;
            if (t.text == "<") {
                cur.in_file = file;
            } else {
                cur.out_file = file;
                cur.append = (t.text == ">>");
            }
        }
    }

    if (cur.args.empty()) {
        error = "thiếu lệnh sau dấu | hoặc trước &";
        return false;
    }
    out.commands.push_back(cur);
    return true;
}

}  // namespace minishell
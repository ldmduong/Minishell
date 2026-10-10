#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace minishell {

class Shell;

// Hàm xử lý lệnh builtin: nhận shell và danh sách đối số (args[0] là tên lệnh),
// trả về mã thoát (0 là thành công).
using Handler = std::function<int(Shell&, const std::vector<std::string>&)>;

struct CommandInfo {
    Handler handler;
    std::string help;   // mô tả ngắn, hiện trong lệnh help
};

class Registry {
public:
    void add(const std::string& name, Handler handler, const std::string& help = "");
    bool has(const std::string& name) const;
    int run(Shell& shell, const std::vector<std::string>& args) const;
    const std::map<std::string, CommandInfo>& all() const { return commands_; }

private:
    std::map<std::string, CommandInfo> commands_;
};

}  // namespace minishell
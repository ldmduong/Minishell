#include "minishell/core/line_editor.h"

#include <termios.h>
#include <unistd.h>

#include <iostream>

namespace minishell {

namespace {

// Chuyển terminal sang chế độ raw khi còn trong phạm vi hàm, tự khôi phục khi thoát.
class RawMode {
public:
    RawMode() : ok_(tcgetattr(STDIN_FILENO, &orig_) == 0) {
        if (!ok_) return;
        termios raw = orig_;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    ~RawMode() {
        if (ok_) tcsetattr(STDIN_FILENO, TCSANOW, &orig_);
    }
    bool ok() const { return ok_; }

private:
    termios orig_{};
    bool ok_;
};

// Đọc một byte. Trả về -1 nếu lỗi hoặc hết dữ liệu.
int read_byte() {
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    return n == 1 ? c : -1;
}

// Vẽ lại cả dòng rồi đưa con trỏ về đúng vị trí cur.
void redraw(const std::string& prompt, const std::string& buf, size_t cur) {
    std::cout << "\r\033[K" << prompt << buf;
    size_t back = buf.size() - cur;  // số ô cần lùi từ cuối dòng về vị trí con trỏ
    if (back > 0) std::cout << "\033[" << back << "D";
    std::cout << std::flush;
}

}  // namespace

bool read_line(const std::string& prompt, const std::vector<std::string>& history, std::string& out) {
    RawMode raw;
    if (!raw.ok()) {  // stdin không phải terminal
        std::cout << prompt << std::flush;
        return static_cast<bool>(std::getline(std::cin, out));
    }

    std::string buf;
    size_t cur = 0;                // vị trí con trỏ trong buf
    std::string draft;             // nội dung đang gõ dở khi bắt đầu duyệt lịch sử
    size_t pos = history.size();   // history.size() nghĩa là dòng mới
    std::cout << prompt << std::flush;

    while (true) {
        int c = read_byte();
        if (c < 0) {
            std::cout << "\n";
            return false;
        }

        if (c == '\r' || c == '\n') {  // Enter
            std::cout << "\n";
            out = buf;
            return true;
        }

        if (c == 4) {  // Ctrl+D: thoát khi dòng rỗng
            if (buf.empty()) {
                std::cout << "\n";
                return false;
            }
            continue;
        }

        if (c == 127 || c == 8) {  // Backspace: xóa ký tự trước con trỏ
            if (cur > 0) {
                buf.erase(cur - 1, 1);
                --cur;
                redraw(prompt, buf, cur);
            }
            continue;
        }

        if (c == 27) {  // ESC: đầu chuỗi phím đặc biệt
            int b = read_byte();
            if (b != '[' && b != 'O') continue;
            int k = read_byte();

            if (k == 'A' && pos > 0) {  // ↑: lệnh cũ hơn
                if (pos == history.size()) draft = buf;
                buf = history[--pos];
                cur = buf.size();
                redraw(prompt, buf, cur);
            } else if (k == 'B' && pos < history.size()) {  // ↓: lệnh mới hơn
                ++pos;
                buf = (pos == history.size()) ? draft : history[pos];
                cur = buf.size();
                redraw(prompt, buf, cur);
            } else if (k == 'C' && cur < buf.size()) {  // →
                ++cur;
                redraw(prompt, buf, cur);
            } else if (k == 'D' && cur > 0) {  // ←
                --cur;
                redraw(prompt, buf, cur);
            } else if (k == 'H') {  // Home
                cur = 0;
                redraw(prompt, buf, cur);
            } else if (k == 'F') {  // End
                cur = buf.size();
                redraw(prompt, buf, cur);
            } else if (k >= '0' && k <= '9') {
                // Delete là ESC [ 3 ~. Các phím số khác: bỏ qua đến '~'
                int d = k;
                while (d >= 0 && d != '~') d = read_byte();
                if (k == '3' && cur < buf.size()) {  // Delete: xóa ký tự sau con trỏ
                    buf.erase(cur, 1);
                    redraw(prompt, buf, cur);
                }
            }
            continue;
        }

        if (c >= 32 && c < 127) {  // chỉ nhận ký tự ASCII in được
            buf.insert(cur, 1, static_cast<char>(c));
            ++cur;
            redraw(prompt, buf, cur);
        }
    }
}

}  // namespace minishell
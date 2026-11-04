#include "minishell/core/line_editor.h"

#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

namespace minishell {
namespace {

class RawMode {
public:
  RawMode() {
    if (tcgetattr(STDIN_FILENO, &original_) < 0)
      return;

    termios raw = original_;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VINTR] = _POSIX_VDISABLE;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    active_ = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
  }

  ~RawMode() {
    if (active_)
      tcsetattr(STDIN_FILENO, TCSANOW, &original_);
  }

  bool active() const { return active_; }

private:
  termios original_{};
  bool active_ = false;
};

int read_byte() {
  unsigned char byte;
  ssize_t count;
  do {
    count = read(STDIN_FILENO, &byte, 1);
  } while (count < 0 && errno == EINTR);
  return count == 1 ? byte : -1;
}

void redraw(const std::string &prompt, const std::string &text, size_t cursor) {
  std::cout << '\r' << "\033[K" << prompt << text;
  size_t distance_to_end = text.size() - cursor;
  if (distance_to_end > 0)
    std::cout << "\033[" << distance_to_end << 'D';
  std::cout << std::flush;
}

void move_history(int direction, const std::vector<std::string> &history,
                  size_t &position, std::string &draft, std::string &text,
                  size_t &cursor) {
  if (direction < 0 && position > 0) {
    if (position == history.size())
      draft = text;
    text = history[--position];
  } else if (direction > 0 && position < history.size()) {
    ++position;
    text = position == history.size() ? draft : history[position];
  } else {
    return;
  }

  cursor = text.size();
}

bool read_key(const std::string &prompt,
              const std::vector<std::string> &history, std::string &text) {
  size_t cursor = 0;
  size_t history_position = history.size();
  std::string draft;

  while (true) {
    int key = read_byte();
    if (key < 0) {
      std::cout << '\n';
      return false;
    }

    if (key == '\r' || key == '\n') {
      std::cout << '\n';
      return true;
    }

    if (key == 4) {
      if (text.empty()) {
        std::cout << '\n';
        return false;
      }
      continue;
    }

    if (key == 3) {
      text.clear();
      std::cout << "^C\n" << std::flush;
      return true;
    }

    if (key == 127 || key == 8) {
      if (cursor > 0) {
        text.erase(cursor - 1, 1);
        --cursor;
        redraw(prompt, text, cursor);
      }
      continue;
    }

    if (key == 27) {
      int introducer = read_byte();
      if (introducer != '[' && introducer != 'O')
        continue;
      int code = read_byte();

      if (code == 'A' || code == 'B') {
        move_history(code == 'A' ? -1 : 1, history, history_position, draft,
                     text, cursor);
        redraw(prompt, text, cursor);
      } else if (code == 'C' && cursor < text.size()) {
        ++cursor;
        redraw(prompt, text, cursor);
      } else if (code == 'D' && cursor > 0) {
        --cursor;
        redraw(prompt, text, cursor);
      } else if (code == 'H') {
        cursor = 0;
        redraw(prompt, text, cursor);
      } else if (code == 'F') {
        cursor = text.size();
        redraw(prompt, text, cursor);
      } else if (code == '3') {
        if (read_byte() == '~' && cursor < text.size()) {
          text.erase(cursor, 1);
          redraw(prompt, text, cursor);
        }
      }
      continue;
    }

    if (key >= 32 && key < 127) {
      text.insert(cursor, 1, static_cast<char>(key));
      ++cursor;
      redraw(prompt, text, cursor);
    }
  }
}

} // namespace

bool read_line(const std::string &prompt,
               const std::vector<std::string> &history, std::string &out) {
  RawMode raw;
  if (!raw.active()) {
    std::cout << prompt << std::flush;
    return static_cast<bool>(std::getline(std::cin, out));
  }

  out.clear();
  std::cout << prompt << std::flush;
  return read_key(prompt, history, out);
}

} // namespace minishell

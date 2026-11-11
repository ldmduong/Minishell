#include "minishell/core/line_editor.h"

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <iostream>

#include "minishell/core/executor.h"

namespace minishell {
namespace {

volatile std::sig_atomic_t child_event_writer = -1;

void notify_child_exit(int) {
  const int saved_errno = errno;
  const char byte = 0;
  // A full pipe already has a pending wakeup.
  [[maybe_unused]] const ssize_t written = write(child_event_writer, &byte, 1);
  errno = saved_errno;
}

class ChildEvents {
public:
  ChildEvents() {
    if (pipe2(fds_, O_NONBLOCK | O_CLOEXEC) != 0) {
      std::perror("pipe2");
      return;
    }
    child_event_writer = fds_[1];
    struct sigaction action{};
    action.sa_handler = notify_child_exit;
    action.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGCHLD, &action, &previous_) != 0) {
      std::perror("sigaction");
      close(fds_[0]);
      close(fds_[1]);
      fds_[0] = -1;
      child_event_writer = -1;
    }
  }

  ~ChildEvents() {
    if (fds_[0] >= 0) {
      sigaction(SIGCHLD, &previous_, nullptr);
      close(fds_[0]);
      close(fds_[1]);
      child_event_writer = -1;
    }
  }

  int fd() const { return fds_[0]; }

private:
  int fds_[2] = {-1, -1};
  struct sigaction previous_{};
};

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

void redraw(const std::string &prompt, const std::string &text, size_t cursor) {
  std::cout << '\r' << "\033[K" << prompt << text;
  size_t distance_to_end = text.size() - cursor;
  if (distance_to_end > 0)
    std::cout << "\033[" << distance_to_end << 'D';
  std::cout << std::flush;
}

int read_byte(const std::string &prompt, const std::string &text, size_t cursor) {
  static ChildEvents events;
  pollfd fds[] = {{STDIN_FILENO, POLLIN, 0}, {events.fd(), POLLIN, 0}};
  for (;;) {
    if (poll(fds, 2, -1) < 0) {
      if (errno == EINTR)
        continue;
      std::perror("poll");
      return -1;
    }
    if (fds[1].revents & POLLIN) {
      char bytes[128];
      while (read(events.fd(), bytes, sizeof(bytes)) > 0) {
      }
      if (Executor::reap_background(!prompt.empty())) {
        if (prompt.empty())
          std::cout << std::flush;
        else
          redraw(prompt, text, cursor);
      }
    }
    if (fds[0].revents == 0)
      continue;
    unsigned char byte;
    ssize_t count = read(STDIN_FILENO, &byte, 1);
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0)
      std::perror("read");
    return count == 1 ? byte : -1;
  }
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
    int key = read_byte(prompt, text, cursor);
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
      int introducer = read_byte(prompt, text, cursor);
      if (introducer != '[' && introducer != 'O')
        continue;
      int code = read_byte(prompt, text, cursor);

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
        if (read_byte(prompt, text, cursor) == '~' && cursor < text.size()) {
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
  out.clear();
  if (!prompt.empty()) {
    RawMode raw;
    std::cout << prompt << std::flush;
    if (raw.active())
      return read_key(prompt, history, out);
  }

  // Leave stdin at the first byte after the command for a child that reads it.
  for (;;) {
    int byte = read_byte(prompt, out, out.size());
    if (byte == '\n')
      return true;
    if (byte < 0)
      return !out.empty();
    out += static_cast<char>(byte);
  }
}

} // namespace minishell

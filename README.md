# 🐚 minishell

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus)
![CMake](https://img.shields.io/badge/CMake-%E2%89%A5%203.16-064F8C?logo=cmake)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20WSL-orange?logo=linux)
![Course](https://img.shields.io/badge/Môn-ET4291%20Hệ%20điều%20hành-green)

Một **shell mini** viết bằng C++17 cho môn **Hệ điều hành (ET4291)**. Shell hỗ trợ chạy lệnh ngoài, pipe, chuyển hướng, chạy nền, lịch sử lệnh có mũi tên, cùng nhiều lệnh tích hợp về tệp, tiến trình, luồng và hệ thống.

---

## ✨ Tính năng nổi bật

- **Chạy lệnh ngoài** qua `fork` + `execvp`, ví dụ `ls`, `grep`, `echo`.
- **Pipe** nhiều tầng: `cat file.txt | grep abc | wc -l`.
- **Chuyển hướng** đầu vào và đầu ra: `<`, `>`, `>>`.
- **Chạy nền** với `&`, tự thu hồi tiến trình đã xong.
- **Dòng lệnh tương tác**: mũi tên ↑ ↓ để xem lịch sử, ← → để di chuyển con trỏ, `Delete`, `Home`, `End`, `Ctrl+D` để thoát.
- **Alias** tùy biến: `alias ll='ls -l'`.
- **Biến môi trường**: `env`, `export`, `unset`.
- **Quản lý tiến trình**: xem danh sách, gửi tín hiệu, tạm dừng và tiếp tục.
- **Xem luồng** của một tiến trình và **chạy thử luồng** với `std::thread`.
- **Lập lịch** lệnh chạy sau một khoảng thời gian.

---

## 🧭 Danh sách lệnh tích hợp

Gõ `help` để xem danh sách đầy đủ theo nhóm.

| Nhóm | Lệnh | Mô tả |
|---|---|---|
| **Điều hướng** | `cd [-]`, `pwd` | Đổi thư mục, `cd -` quay lại thư mục trước |
| **Phiên làm việc** | `exit [mã]`, `clear`, `help`, `history` | Thoát, xóa màn hình, trợ giúp, xem lịch sử |
| **Thư mục & tệp** | `mkdir [-p]`, `rmdir`, `touch`, `rm [-f]`, `cat` | Quản lý thư mục và tệp |
| **Môi trường & alias** | `env`, `export`, `unset`, `alias`, `unalias` | Biến môi trường và bí danh lệnh |
| **Hệ thống** | `ram`, `cpu`, `uptime` | Thông tin RAM, CPU, thời gian hoạt động |
| **Tiến trình & luồng** | `procs`, `kill [-SIG] PID`, `stop PID`, `resume PID`, `threads PID`, `threads demo N` | Quản lý tiến trình và luồng |
| **Lập lịch** | `schedule SỐ_GIÂY LỆNH`, `schedule list`, `schedule cancel PID` | Chạy lệnh sau một khoảng thời gian |

---

## 🎬 Thử nhanh

```text
minishell$ mkdir -p demo/sub
minishell$ cd demo
minishell$ touch a.txt
minishell$ echo "xin chào" > a.txt
minishell$ cat a.txt | wc -c
minishell$ ram
minishell$ threads demo 3
minishell$ schedule 3 echo "hẹn 3 giây"
minishell$ alias ll='ls -l'
minishell$ history
minishell$ exit
```

---

## 🛠️ Yêu cầu

- Linux hoặc **WSL2 Ubuntu** (khuyến nghị Ubuntu 24.04)
- `g++` hỗ trợ C++17
- `cmake` ≥ 3.16
- `make`, `gdb`, `valgrind` (tùy chọn, để gỡ lỗi và kiểm tra bộ nhớ)

Cài đặt nhanh trên Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake gdb valgrind procps git
```

---

## 🚀 Build và chạy

Dùng Makefile (đơn giản nhất):

```bash
make            # cấu hình và build bằng CMake
make run        # build rồi chạy minishell
make clean      # xóa thư mục build/ và install/
make rebuild    # làm sạch rồi build lại
```

Hoặc dùng CMake trực tiếp:

```bash
cmake -S . -B build
cmake --build build -j
./build/minishell
```

Kiểm tra bộ nhớ với Valgrind:

```bash
valgrind --leak-check=full ./build/minishell
```

---

## 🐳 Chạy bằng Docker

```bash
docker build -f docker/Dockerfile -t minishell .
docker run -it --rm minishell
```

---

## 🗂️ Cấu trúc dự án

```text
minishell/
├── CMakeLists.txt
├── Makefile
├── requirements.txt
├── include/minishell/
│   ├── core/              # shell, parser, executor, line editor, registry
│   └── builtins/
│       ├── core/          # cd, pwd, exit, clear, help, history, file, env, alias...
│       ├── scripting/     # schedule (và các lệnh script trong kế hoạch)
│       ├── system/        # ram, cpu, uptime, procs, kill, threads
│       └── extras/        # tiện ích mở rộng (kế hoạch)
├── src/                   # mã nguồn tương ứng với include/
├── samples/               # chương trình mẫu
├── tests/                 # kiểm thử
└── docker/                # Dockerfile
```

---

## 🧠 Ý tưởng thiết kế

- **Registry**: mỗi lệnh tích hợp được đăng ký bằng tên, hàm xử lý và chuỗi trợ giúp. Shell không cần biết chi tiết của từng lệnh.
- **Executor**: lệnh đơn chạy trong shell khi cần, còn pipeline và lệnh ngoài chạy trong tiến trình con qua `fork`, `pipe`, `dup2` và `execvp`.
- **Tín hiệu**: shell bỏ qua `SIGINT`, còn tiến trình con khôi phục hành vi mặc định để Ctrl+C chỉ dừng lệnh đang chạy.
- **Line editor**: chuyển terminal sang chế độ raw bằng `termios`, đọc từng byte để xử lý mũi tên và chỉnh sửa dòng.
- **Thông tin hệ thống**: đọc trực tiếp từ `/proc` (`meminfo`, `cpuinfo`, `uptime`, `<pid>/stat`, `<pid>/task`).

---

## 🗺️ Kế hoạch tiếp theo

- [ ] Nhóm lệnh script: `run_script`, điều kiện, vòng lặp, hàm
- [ ] Nhóm tiện ích mở rộng: chuyển đổi cơ số, bookmark, đổi màu, hiệu ứng
- [ ] Bộ kiểm thử đơn vị và kịch bản tự động
- [ ] CI với GitHub Actions (build, test, valgrind)
- [ ] Job control: `jobs`, `fg`, `bg`

---

## 👤 Tác giả

- **Họ tên:** Lê Đình Minh Dương 
- **MSSV:** *20233355*\
- **Môn học:** ET4291 - Hệ điều hành

---

## 📄 Giấy phép

Dự án phục vụ mục đích học tập trong môn Hệ điều hành.
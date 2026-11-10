import os
from pathlib import Path
import pty
import re
import select
import signal
import sys
from tempfile import TemporaryDirectory
import time

pending = {}


def read_until(fd, marker, timeout=4):
    output = bytearray(pending.pop(fd, b""))
    deadline = time.monotonic() + timeout
    while True:
        position = output.find(marker)
        if position >= 0:
            end = position + len(marker)
            pending[fd] = bytes(output[end:])
            return bytes(output[:end])

        remaining = deadline - time.monotonic()
        if remaining <= 0 or not select.select([fd], [], [], remaining)[0]:
            raise RuntimeError(f"timed out waiting for {marker!r}; got {bytes(output)!r}")
        output.extend(os.read(fd, 4096))


shell_pid, terminal = pty.fork()
if shell_pid == 0:
    os.execl(sys.argv[1], sys.argv[1])

try:
    read_until(terminal, b"minishell$ ")

    os.write(terminal, b"echo discarded")
    time.sleep(0.1)
    os.write(terminal, b"\x03")
    read_until(terminal, b"^C\r\nminishell$ ")

    os.write(terminal, b"echo alive\n")
    read_until(terminal, b"alive\r\nminishell$ ")

    os.write(terminal, b"sleep 10\n")
    time.sleep(0.2)
    os.write(terminal, b"\x03")
    read_until(terminal, b"minishell$ ", timeout=3)

    os.write(terminal, b"echo survived\n")
    read_until(terminal, b"survived\r\nminishell$ ")

    os.write(terminal, b"cat\n")
    read_until(terminal, b"cat\r\n")
    time.sleep(0.1)
    os.write(terminal, b"\x03")
    read_until(terminal, b"minishell$ ")

    with TemporaryDirectory() as directory:
        script = Path(directory) / "interrupt.ms"
        script.write_text("echo script-ready\nsleep 10\necho should-not-run\n")
        os.write(terminal, f'run_script "{script}"\n'.encode())
        read_until(terminal, b"script-ready\r\n")
        time.sleep(0.1)
        os.write(terminal, b"\x03")
        output = read_until(terminal, b"minishell$ ")
        if b"should-not-run" in output:
            raise RuntimeError(f"script continued after Ctrl+C: {output!r}")

    os.write(terminal, b"dance 5\n")
    time.sleep(0.2)
    os.write(terminal, b"\x03")
    read_until(terminal, b"minishell$ ", timeout=3)

    os.write(terminal, b"schedule 30 echo cancelled\n")
    scheduled = read_until(terminal, b"[schedule] PID")
    scheduled += read_until(terminal, b"minishell$ ")
    match = re.search(rb"\[schedule\] PID (\d+)", scheduled)
    if match is None:
        raise RuntimeError(f"schedule did not report its PID: {scheduled!r}")
    pid = match.group(1)

    os.write(terminal, b"schedule list\n")
    listed = read_until(terminal, "còn ".encode())
    listed += read_until(terminal, b"minishell$ ")
    if pid not in listed:
        raise RuntimeError(f"schedule list omitted PID {pid!r}: {listed!r}")

    os.write(terminal, b"schedule cancel " + pid + b"\n")
    cancelled = read_until(terminal, "Đã hủy lịch PID".encode())
    cancelled += read_until(terminal, b"minishell$ ")
    if "Đã hủy lịch PID".encode() not in cancelled:
        raise RuntimeError(f"schedule cancel failed: {cancelled!r}")

    os.write(terminal, b"schedule 1 echo scheduled-ok\n")
    scheduled = read_until(terminal, b"[schedule] PID")
    scheduled += read_until(terminal, b"minishell$ ")
    match = re.search(rb"\[schedule\] PID (\d+)", scheduled)
    if match is None:
        raise RuntimeError(f"schedule did not report its PID: {scheduled!r}")
    os.write(terminal, b"sleep 2\n")
    completed = read_until(terminal, b"scheduled-ok\r\n", timeout=4)
    completed += read_until(terminal, b"minishell$ ", timeout=4)
    if b"scheduled-ok" not in completed:
        raise RuntimeError(f"scheduled command did not run: {completed!r}")

    os.write(terminal, b"schedule cancel " + match.group(1) + b"\n")
    read_until(terminal, "không tìm thấy lịch PID".encode())
    read_until(terminal, b"minishell$ ")

    os.write(terminal, b"exit\n")
    os.waitpid(shell_pid, 0)
except Exception:
    try:
        os.killpg(shell_pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    os.waitpid(shell_pid, 0)
    raise
finally:
    os.close(terminal)

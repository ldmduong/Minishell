import os
import pty
import select
import signal
import sys
import time


def read_until(fd, marker, timeout=4):
    output = bytearray()
    deadline = time.monotonic() + timeout
    while marker not in output:
        remaining = deadline - time.monotonic()
        if remaining <= 0 or not select.select([fd], [], [], remaining)[0]:
            raise RuntimeError(f"timed out waiting for {marker!r}; got {bytes(output)!r}")
        output.extend(os.read(fd, 4096))
    return bytes(output)


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

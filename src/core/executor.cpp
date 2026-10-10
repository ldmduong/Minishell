#include "minishell/core/executor.h"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#include "minishell/core/shell.h"

namespace minishell {

namespace {

bool apply_redirects(const Command& cmd) {
    if (!cmd.in_file.empty()) {
        int fd = open(cmd.in_file.c_str(), O_RDONLY);
        if (fd < 0) {
            std::perror(cmd.in_file.c_str());
            return false;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    if (!cmd.out_file.empty()) {
        int flags = O_WRONLY | O_CREAT | (cmd.append ? O_APPEND : O_TRUNC);
        int fd = open(cmd.out_file.c_str(), flags, 0644);
        if (fd < 0) {
            std::perror(cmd.out_file.c_str());
            return false;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
    return true;
}

int decode_status(int status) {
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 0;
}

int run_builtin_in_shell(Shell& shell, const Command& cmd) {
    std::cout.flush();
    int saved_in = dup(STDIN_FILENO);
    int saved_out = dup(STDOUT_FILENO);

    int code = 1;
    if (apply_redirects(cmd)) code = shell.registry().run(shell, cmd.args);

    std::cout.flush();
    dup2(saved_in, STDIN_FILENO);
    dup2(saved_out, STDOUT_FILENO);
    close(saved_in);
    close(saved_out);
    return code;
}

}  // namespace

int Executor::run(Shell& shell, const Pipeline& pipeline) {
    const auto& cmds = pipeline.commands;
    if (cmds.empty()) return 0;

    if (cmds.size() == 1 && !pipeline.background && shell.registry().has(cmds[0].args[0])) {
        return run_builtin_in_shell(shell, cmds[0]);
    }

    const size_t n = cmds.size();
    std::vector<int> fds(n > 1 ? 2 * (n - 1) : 0);
    for (size_t i = 0; i + 1 < n; ++i) {
        if (pipe(&fds[2 * i]) < 0) {
            std::perror("pipe");
            return 1;
        }
    }

    std::cout.flush();
    std::vector<pid_t> pids;
    for (size_t i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            std::perror("fork");
            break;
        }

        if (pid == 0) {  
            signal(SIGINT, SIG_DFL);  

            if (pipeline.background) {
                setpgid(0, 0);  // tách nhóm để Ctrl+C không giết job nền
                if (i == 0 && cmds[i].in_file.empty()) {
                    int dn = open("/dev/null", O_RDONLY);
                    if (dn >= 0) {
                        dup2(dn, STDIN_FILENO);
                        close(dn);
                    }
                }
            }

            if (i > 0) dup2(fds[2 * (i - 1)], STDIN_FILENO);
            if (i + 1 < n) dup2(fds[2 * i + 1], STDOUT_FILENO);
            for (int fd : fds) close(fd);

            if (!apply_redirects(cmds[i])) _exit(1);

            const auto& args = cmds[i].args;
            if (shell.registry().has(args[0])) {
                int code = shell.registry().run(shell, args);
                std::cout.flush();
                _exit(code);
            }

            std::vector<char*> argv;
            for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());

            if (errno == ENOENT)
                std::cerr << "minishell: " << args[0] << ": không tìm thấy lệnh\n";
            else
                std::cerr << "minishell: " << args[0] << ": " << std::strerror(errno) << "\n";
            _exit(errno == ENOENT ? 127 : 126);
        }

        pids.push_back(pid);  
    }

    for (int fd : fds) close(fd);

    if (pipeline.background) {
        if (!pids.empty()) std::cout << "[nền] pid " << pids.back() << "\n";
        return 0;
    }

    int last = 0;
    for (size_t i = 0; i < pids.size(); ++i) {
        int status = 0;
        waitpid(pids[i], &status, 0);
        if (i + 1 == pids.size()) last = decode_status(status);
    }
    return last;
}

void Executor::reap_background() {
    int status = 0;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        std::cout << "[xong] pid " << pid << " (mã " << decode_status(status) << ")\n";
    }
}

}  // namespace minishell
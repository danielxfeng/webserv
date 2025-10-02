#include "signalHandler.hpp"

void sigchld_handler(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

void setup_sigchld_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted syscalls
    sigaction(SIGCHLD, &sa, nullptr);
}
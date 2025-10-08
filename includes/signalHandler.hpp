#pragma once
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include "LogSys.hpp"

void sigchld_handler(int);
void setup_signal_handlers();
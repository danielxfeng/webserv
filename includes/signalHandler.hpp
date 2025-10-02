#pragma once
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

void sigchld_handler(int );
void setup_sigchld_handler() {
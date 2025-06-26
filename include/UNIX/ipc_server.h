#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <map>
#include <thread>

#include "constants.h"
#include "system.h"
#include "CLI.h"
#include "structs.h"

void cleanup();
bool is_server_running();
void run_server();

extern FileSystemInterface* fs;
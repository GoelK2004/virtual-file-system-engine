// #include "CLI.h"
// #include "fileSystemInterface.h"
// #include "system.h"
// #include "VFS.h"
// #include "mountManager.h"

#include <signal.h>

#include "fileSystemInterface.h"
#include "system.h"
#include "UNIX/ipc_server.h"
#include "UNIX/ipc_client.h"

void handle_sigint(int signal) {
    cleanup();
}

int main() {
    signal(SIGINT, handle_sigint);
    
    // pid_t pid = fork();
    // if (pid < 0) {
    //     perror("fork failed");
    //     return 1;
    // }
    // if (pid == 0) {
    //     sleep(1);
    //     run_client();
    // } else {
    //     run_server();
    // }
    if (!is_server_running())   run_server();
    else  run_client();
    return 0;
}

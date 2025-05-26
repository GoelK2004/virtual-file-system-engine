#include "CLI.h"
#include "fileSystemInterface.h"
#include "system.h"
#include "VFS.h"
#include "mountManager.h"

int main(){
	
    // Step 0: Creating a mount class
    MountManager mountManager;

    // Step 1: Creating a concrete file system
    FileSystemInterface* fs = new System("/disks/myDisk.img");

    // Step 2: Create and initialize VFS Manager
    VFSManager* vfsManager = new VFSManager();
    vfsManager->mount(fs); 
    mountManager.mount("/dir1", "/disks/myDisk.img", "rootFS", vfsManager);

    // Step 2: Pass this to the CLI
    std::cout << "-----------------------------------------------------\n";
    CommandLineInterface cli(mountManager.getCurrentVFS(), mountManager.getCurrentFSName());
    cli.runCLI();

    // Cleanup
    delete fs;
    return 0;
}
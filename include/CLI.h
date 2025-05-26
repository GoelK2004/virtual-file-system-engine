#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "VFS.h"


class CommandLineInterface {
private:
    VFSManager* vfs;

public:
    explicit CommandLineInterface(VFSManager* filesys);
    void setVFS(VFSManager* vfsS) {
        this->vfs = vfsS;
    }
	void runCLI();
};

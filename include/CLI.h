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
    CommandLineInterface(VFSManager* vfs);
    void setVFS(VFSManager* vfs) {
        this->vfs = vfs;
    }
	void runCLI();
};

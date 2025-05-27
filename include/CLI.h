#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "VFS.h"


class CommandLineInterface {
private:
    VFSManager* vfs;
    std::string FSName;

public:
    explicit CommandLineInterface(VFSManager* filesys, const std::string& fsName);
    void setVFS(VFSManager* vfsS) {
        this->vfs = vfsS;
    }
	void runCLI();
};

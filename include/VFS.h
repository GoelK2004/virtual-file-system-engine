#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "fileSystemInterface.h"
#include "structs.h"

class VFSManager {
private:
    FileSystemInterface* fs; // Mounted file system

public:
    VFSManager() = default;
	~VFSManager() {
		if (fs)	delete fs;
		fs = nullptr;
	};

	FileSystemInterface* getFS() {
		return fs;
	}
	
    void mount(FileSystemInterface* fileSystem);
    void unmount();
    bool isMounted() const;

	// Helper functions
	std::string createPath(ClientSession* session);
    // Exposed command functions
	void printHelp(ClientSession* session);
	// void format();
	// void load();
	bool create(const std::string& path, ClientSession* session);
	bool create(const std::string& path, const int& fileSize, ClientSession* session);
	std::string read(const std::string& path, ClientSession* session);
	bool write(const std::string& path, const std::string& data, ClientSession* session);
	bool append(const std::string& path, const std::string& data, ClientSession* session);
	bool remove(const std::string& path, ClientSession* session);
	bool rename(const std::string& oldName, const std::string& newName, ClientSession* session);
	bool mkdir(const std::string& path, ClientSession* session);
	bool rmdir(const std::string& path, ClientSession* session);
	bool rmrdir(const std::string& path, ClientSession* session);
	bool cd(const std::string& path, ClientSession* session);
	void ls(ClientSession* session);
	void stat(const std::string& path, ClientSession* session);
	bool chmod(const std::string& path, int mode, ClientSession* session);
	bool chown(const std::string& path, const std::string& uid, ClientSession* session);
	bool chgrp(const std::string& path, uint32_t gid, ClientSession* session);
	void whoami(ClientSession* session) const;
	void login(const std::string& user_id, const std::string& password, ClientSession* session);
	void userAdd(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id = -1);
	void showUsers(ClientSession* session);
	void showGroups(ClientSession* session);
	void tree(ClientSession* session, const std::string& path = "/", int depth = 0, const std::string& prefix = "");

	// LOGS
	void bTree();
	void show();

};

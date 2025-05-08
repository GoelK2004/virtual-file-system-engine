#ifndef VFS_MANAGER_H
#define VFS_MANAGER_H

#include <memory>
#include <string>
#include "fileSystemInterface.h"

class VFSManager {
private:
    FileSystemInterface* fs; // Mounted file system

public:
    VFSManager() = default;
	~VFSManager() {
		delete fs;
	};

	FileSystemInterface* getFS() {
		return fs;
	}
	
    void mount(FileSystemInterface* fileSystem);
    void unmount();
    bool isMounted() const;

	// Helper functions
	std::string createPath();
    // Exposed command functions
	void printHelp();
	// void format();
	// void load();
	bool create(const std::string& path);
	bool create(const std::string& path, const int& filSize);
	std::string read(const std::string& path);
	bool write(const std::string& path, const std::string& data);
	bool append(const std::string& path, const std::string& data);
	bool remove(const std::string& path);
	bool rename(const std::string& oldName, const std::string& newName);
	bool mkdir(const std::string& path);
	bool rmdir(const std::string& path);
	bool cd(const std::string& path);
	void ls();
	void stat(const std::string& path);
	bool chmod(const std::string& path, int mode);
	bool chown(const std::string& path, const std::string& uid);
	bool chgrp(const std::string& path, uint32_t gid);
	void whoami() const;
	void login(const std::string& user_id, const std::string& password);
	void userAdd(const std::string& userName, const std::string& password, uint32_t group_id = -1);
	void showUsers();
	void showGroups();
	void tree(const std::string& path = "/", int depth = 0, const std::string& prefix = "");

};

#endif
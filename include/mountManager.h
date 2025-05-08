#pragma once

#include <iostream>
#include "VFS.h"

struct MountedFs {
	std::string fsName;
	std::string diskpath;
	std::string mountPath;
	VFSManager* fs;
};

class MountManager {
	private:
		std::vector<MountedFs*> mountTable;
		MountedFs* current;

	public:
		MountManager() {
			current = nullptr;
		}
		void mount(const std::string& path, const std::string& diskPath, const std::string& fsName, VFSManager* fs);
		bool unmount(const std::string& fsName);
		bool switchTo(const std::string& fsName);
		VFSManager* getCurrentVFS();
		FileSystemInterface* getCurrentFS();
		std::string getCurrentFSName() const;
		std::string getMountPath() const;
		std::string getDiskPath() const;
		void listMounts() const;
		MountedFs* getCurrent();
};
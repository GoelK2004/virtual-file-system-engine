#include "mountManager.h"

void MountManager::mount(const std::string& path, const std::string& diskPath, const std::string& fsName, VFSManager* fs) {

	for (auto& entry : mountTable) {
		if (entry->fs == fs || entry->fsName == fsName) {
			std::cerr << "Error: File system already mounted.\n";
			return;
		}
		if (entry->mountPath == path) {
			std::cerr << "Error: Path already in use.\n";
			return;
		}
	}
	
	MountedFs* mountedFs = new MountedFs();
	if (!mountedFs) return;
	mountedFs->diskpath = diskPath;
	mountedFs->mountPath = path;
	mountedFs->fs = fs;
	mountedFs->fsName = fsName;

	mountTable.push_back(mountedFs);
	current = mountTable.back();
	std::cout << "Successfully mounted the file system: " << fsName << ".\n";
}

bool MountManager::unmount(const std::string& fsName) {
	for (auto entry = mountTable.begin(); entry != mountTable.end(); ++entry) {
		if ((*entry)->fsName == fsName) {
			MountedFs* tempEntry = *entry;
			mountTable.erase(entry);
			if (current == *entry) {
				current = nullptr;
			}
			delete tempEntry;
			std::cout << "Successfully unmounted: " << fsName << '\n';
			return true;
		}
	}

	std::cerr << "FS not found: " << fsName << std::endl;
    return false;
}

bool MountManager::switchTo(const std::string& fsName) {
	for (auto entry = mountTable.begin(); entry != mountTable.end(); ++entry) {
		if ((*entry)->fsName == fsName) {
			current = *entry;
			std::cout << "Successfully switched to: " << fsName << '\n';
			return true;
		}
	}
	
	std::cerr << "FS not found: " << fsName << std::endl;
	return false;
}

VFSManager* MountManager::getCurrentVFS() {
	return current ? current->fs : nullptr;
}

FileSystemInterface* MountManager::getCurrentFS() {
	return current ? current->fs->getFS() : nullptr;
}

std::string MountManager::getCurrentFSName() const {
	return current ? current->fsName : "";
}

std::string MountManager::getMountPath() const {
	return current ? current->mountPath : "";
}

std::string MountManager::getDiskPath() const {
	return current ? current->diskpath : "";
}

void MountManager::listMounts() const {
	for (auto entry = mountTable.begin(); entry != mountTable.end(); ++entry) {
		std::cout << (*entry)->fsName << ' ' << (*entry)->mountPath << '\n';
	}
}

MountedFs* MountManager::getCurrent() {
	return current ? current : nullptr;
}
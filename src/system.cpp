#include "system.h"

System::System(const std::string& diskPath) {
	journalManager = new JournalManager(this, "./journal/journal.log");
	Entries = new MetadataManager(this, ORDER);
	FATTABLE = std::vector<bool>(TOTAL_BLOCKS, false);
	this->DISK_PATH = diskPath;
	user = User();
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::in | std::ios::out);
	if (!disk) {
		formatFileSystem();
	} else {
		load();
	}
	std::cout << "FileSystem initialized.\n";
};  // Constructor for init

System::~System() {
	saveInDisk();
	for (auto& entry : metaDataTable) {
		delete entry;
	}
	std::cout << "Meta data freed.\n";
	for (auto& entry : userDatabase) {
		delete entry;
	}
	std::cout << "User data freed.\n";
	Entries->deleteBPlusTree();
	delete Entries;
	delete journalManager;
	std::cout << "Indexing data freed.\n";
    std::cout << "FileSystem cleaned up.\n";
}

std::string System::createPath() {
	return createPathM();
}

void System::printHelp() {
	printHelpM();
}

// void System::format() {
// 	System();
// }

void System::load() {
	loadFromDisk();
}

bool System::create(const std::string& path) {
	return createFiles(path);
}

bool System::create(const std::string& path, const int& fileSize) {
	return createFiles(path, fileSize);
}

std::string System::read(const std::string& path) {
	return readData(path);
}

bool System::write(const std::string& path, const std::string& data) {
	return writeData(path, data, false);
}

bool System::append(const std::string& path, const std::string& data) {
	return writeData(path, data, true);
}

bool System::remove(const std::string& path) {
	return deleteDataFile(path);
}

bool System::rename(const std::string& oldName, const std::string& newName) {
	return renameFiles(oldName, newName);
}

bool System::mkdir(const std::string& path) {
	return createDirectory(path);
}

bool System::rmdir(const std::string& path) {
	return deleteDataDir(path);
}

bool System::cd(const std::string& path) {
	return changeDirectory(path);
}

void System::ls() {
	list();
}

void System::stat(const std::string& path) {
	fileMetadata(path);
}

bool System::chmod(const std::string& path, int mode) {
	return chmodFileM(path, mode);
}

bool System::chown(const std::string& path, const std::string& uid) {
	return chownM(path, uid);
}

bool System::chgrp(const std::string& path, uint32_t gid) {
	return chgrpCommand(path, gid);
}

void System::whoami() const {
	whoamiM();
}

void System::login(const std::string& user_id, const std::string& password) {
	loginM(user_id, password);
}

void System::userAdd(const std::string& userName, const std::string& password, uint32_t group_id) {
	userAddM(userName, password, group_id);
}

void System::showUsers() {
	showUsersM();
}

void System::showGroups() {
	showGroupsM();
}

void System::tree(const std::string& path, int depth, const std::string& prefix) {
	treeM(path, depth, prefix);
}

// LOGS
void System::show() {
	for (int i = 0; i <static_cast<int>(metaDataTable.size()); i++) {
		std::cout << metaDataTable[i]->fileName << ' ' << metaDataTable[i]->owner_id << '\n';
	}
}
void System::bTree() {
	Entries->printMetadataTree();
}
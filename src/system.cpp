#include "system.h"

System::System(const std::string& diskPath) {
	bool check = true;
	journalManager = new JournalManager(this, "./journal/journal.log");
	Entries = new MetadataManager(this, ORDER);
	FATTABLE = std::vector<bool>(TOTAL_BLOCKS, false);
	this->DISK_PATH = diskPath;
	// user = User();
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::in | std::ios::out);
	if (!disk) {
		check = formatFileSystem();
	} else {
		check = load();
		if (!check)	exit(EXIT_FAILURE);
		ClientSession* session = new ClientSession();
		login("root", "0", session);

	}
	if (!check)	exit(EXIT_FAILURE);
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

std::string System::createPath(ClientSession* session) {
	session->msg.clear();
	return createPathM(session);
}

void System::printHelp(ClientSession* session) {
	session->msg.clear();
	printHelpM(session);
}

// void System::format() {
// 	System();
// }

bool System::load() {
	return loadFromDisk();
}

bool System::create(const std::string& path, ClientSession* session) {
	session->msg.clear();
	return createFiles(path, session);
}

bool System::create(const std::string& path, const int& fileSize, ClientSession* session) {
	session->msg.clear();
	return createFiles(path, session, fileSize);
}

std::string System::read(const std::string& path, ClientSession* session) {
	session->msg.clear();
	std::string content = readData(path, session);
	return content;
}

bool System::write(const std::string& path, const std::string& data, ClientSession* session) {
	session->msg.clear();
	return writeData(path, data, false, session);
}

bool System::append(const std::string& path, const std::string& data, ClientSession* session) {
	session->msg.clear();
	return writeData(path, data, true, session);
}

bool System::remove(const std::string& path, ClientSession* session) {
	session->msg.clear();
	return deleteDataFile(path, session);
}

bool System::rename(const std::string& oldName, const std::string& newName, ClientSession* session) {
	session->msg.clear();
	return renameFiles(oldName, newName, session);
}

bool System::mkdir(const std::string& path, ClientSession* session) {
	session->msg.clear();
	return createDirectory(path, session);
}

bool System::rmdir(const std::string& path, ClientSession* session) {
	session->msg.clear();
	return deleteDataDir(path, session);
}

bool System::rmrdir(const std::string& path, ClientSession* session) {
	session->msg.clear();
	return recursiveDelete(path, session);
}

bool System::cd(const std::string& path, ClientSession* session) {
	session->msg.clear();
	return changeDirectory(path, session);
}

void System::ls(ClientSession* session) {
	session->msg.clear();
	list(session);
}

void System::stat(const std::string& path, ClientSession* session) {
	session->msg.clear();
	fileMetadata(path, session);
}

bool System::chmod(const std::string& path, int mode, ClientSession* session) {
	session->msg.clear();
	return chmodFileM(path, mode, session);
}

bool System::chown(const std::string& path, const std::string& uid, ClientSession* session) {
	session->msg.clear();
	return chownM(path, uid, session);
}

bool System::chgrp(const std::string& path, uint32_t gid, ClientSession* session) {
	session->msg.clear();
	return chgrpCommand(path, gid, session);
}

void System::whoami(ClientSession* session) {
	session->msg.clear();
	whoamiM(session);
}

void System::login(const std::string& user_id, const std::string& password, ClientSession* session) {
	session->msg.clear();
	loginM(user_id, password, session);
}

void System::userAdd(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id) {
	session->msg.clear();
	userAddM(userName, password, session, group_id);
}

void System::showUsers(ClientSession* session) {
	session->msg.clear();
	showUsersM(session);
}

void System::showGroups(ClientSession* session) {
	session->msg.clear();
	showGroupsM(session);
}

void System::tree(ClientSession* session, const std::string& path, int depth, const std::string& prefix) {
	session->msg.clear();
	treeM(session, path, depth, prefix);
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
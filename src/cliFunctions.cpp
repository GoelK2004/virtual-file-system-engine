#include "cliFunctions.h"

void System::printHelpM() {
	std::cout << "Available commands:\n"
              << "  format\n"
              << "  ls\n"
              << "  cd <dir>\n"
              << "  mkdir <name>\n"
              << "  rmdir <name>\n"
              << "  create <filename> <fileSize(optional)>\n"
              << "  write <filename> <fileContent>\n"
              << "  append <filename> <fileContent>\n"
              << "  read <filename>\n"
              << "  rm <filename>\n"
              << "  rename <old> <new>\n"
              << "  stat <filename>\n"
			  << "  chmod <filename> <permissions>\n"
			  << "  chown <filename> <owner_id:group_id>\n"
			  << "  chgrp <filename> <group_id>\n"
			  << "  whoami\n"
			  << "  login <username> <password>\n"
			  << "  useradd <username> <password> <group_id(optional)>\n"
			  << "  showusr\n"
			  << "  showgrp\n"
			  << "  tree\n"	  
              << "  exit\n";
}

bool System::chmodFileM(const std::string& fileName, int newPermissions) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	std::string searchFile = std::to_string(currentDir) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	FileEntry* file = metaDataTable[fileIndex];
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != currentDir || file->isDirectory) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (user.user_id != 0){
		if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_WRITE)){
			std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
			return false;
		}
	}

	file->permissions = newPermissions;
	file->modified_at = std::time(nullptr);
	int save = saveDirectoryTable(disk, fileIndex);
	if (save == 0) {
		std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	std::cout << "Successfully updated the permissions.\n";
	disk.close();
	delete file;
	return true;
}

bool System::chownM(const std::string& fileName, std::string changeOwnership) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	std::string searchFile = std::to_string(currentDir) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	FileEntry* file = metaDataTable[fileIndex];
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != currentDir || file->isDirectory) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_WRITE)){
		std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
		return false;
	}

	int colonPos = changeOwnership.find(":");
	int owner_id = std::stoi(changeOwnership.substr(0, colonPos));
	int group_id = std::stoi(changeOwnership.substr(colonPos + 1));
	file->modified_at = std::time(nullptr);
	file->owner_id = owner_id;
	file->group_id = group_id;
	int save = saveDirectoryTable(disk, fileIndex);
	if (save == 0) {
		std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	std::cout << "Successfully updated the permissions.\n";
	disk.close();
	delete file;
	return true;
}

bool System::chgrpCommand(const std::string& fileName, uint32_t new_group_id) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	std::string searchFile = std::to_string(currentDir) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	FileEntry* file = metaDataTable[fileIndex];
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != currentDir || file->isDirectory) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_WRITE)){
		std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
		return false;
	}

	file->modified_at = std::time(nullptr);
	file->group_id = new_group_id;
	int save = saveDirectoryTable(disk, fileIndex);
	if (save == 0) {
		std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	std::cout << "Successfully updated the permissions.\n";
	disk.close();
	delete file;
	return true;
}

void System::whoamiM() const {
    auto it_user = userTable.find(user.user_id);
    auto it_group = groupTable.find(user.group_id);
    if (it_user != userTable.end())
        std::cout << "User: " << it_user->second << "\n";
    else
		std::cout << "Unknown user ID: " << user.user_id << "\n";
	if (it_group != groupTable.end())
		std::cout << "Group: " << it_group->second << "\n";
	else
		std::cout << "Unknown group ID: " << user.group_id << "\n";
}

void System::loginM(const std::string& username, const std::string& password) {
	if (username == "root" && std::stoi(password) == 0) {
		User* userOrg = new User();
		userOrg->group_id = 0;
		userOrg->user_id = 0;
		strncpy(userOrg->userName, "root", USER_NAME_LENGTH);
		userOrg->userName[USER_NAME_LENGTH - 1] = '\0';
		userOrg->password = 0;
		user = *userOrg;
		currentDir = 0;
		
		delete userOrg;
		std::cout << "Login successful! Welcome, " << username << ".\n";
		return;
	}
	for (const auto& entry : userDatabase) {
		if (entry->userName == username && entry->password == hashFileName(password)) {
			user = *entry;
			currentDir = 0;
			std::cout << "Login successful! Welcome, " << username << ".\n";
            return;
		}
	}
	std::cout << "Login failed: Invalid credentials.\n";
}

void System::userAddM(const std::string& userName, const std::string& password, uint32_t group_id) {
	for (const auto& entry : userDatabase) {
		if (entry->userName == userName) {
			std::cout << "Error: User already exists.\n";
            return;
		}
	}
	
	User* user = new User();
	strncpy(user->userName, userName.c_str(), USER_NAME_LENGTH);
	user->userName[USER_NAME_LENGTH - 1] = '\0';
	user->password = hashFileName(password);
	user->user_id = 1000 + (++totalUsers);
	user->group_id = group_id;
	userDatabase.push_back(user);

	userTable[user->user_id] = userName;
	if (group_id != -1U){
		auto group = groupTable.find(group_id);
		if (group == groupTable.end()) {
			std::string groupName = "GROUP" + std::to_string(group_id);
			groupTable.insert({group_id, groupName});
			totalGroups++;
		}
	}
	std::cout << "User added: " << userName << " (UID: " << user->user_id << ", GID: " << user->group_id << ")\n";
}

void System::showUsersM() {
	for (auto user : userTable) {
		std::cout << user.first << ' ' << user.second << '\n';
	}
}
void System::showGroupsM() {
	for (auto group : groupTable) {
		std::cout << group.first << ' ' << group.second << '\n';
	}
}

std::vector<FileEntry*> System::getDirectoryEntries(FileEntry* dir) {
	std::vector<FileEntry*> children;
	if (!dir) {
		for (const auto& entry : metaDataTable) {
			if (entry->parentIndex == 0 && entry->owner_id == user.user_id) {
				children.push_back(entry);
			}
		}
	}
	else {
		for (const auto& entry : metaDataTable) {
			if (entry->parentIndex == dir->dirID && entry->owner_id == dir->owner_id) {
				children.push_back(entry);
			}
		}
	}

	return children;
}

void System::treeM(const std::string& path, int depth, const std::string& prefix) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	FileEntry* dir = nullptr;
	if (path != "/"){
		dir = resolvePath(disk, path);
		if (!dir || !dir->isDirectory) {
			std::cerr << "Error: Invalid Directory: " << path << '\n';
			return;
		}
	}
	if (depth == 0){
		std::cout << path << '\n';
	}
	std::vector<FileEntry*> entries;
	entries = getDirectoryEntries(dir);
	for (size_t i = 0; i < entries.size(); i++) {
		bool lastEntry = (i == entries.size() - 1);
		FileEntry* entry = entries[i];

		std::string name(entry->fileName);
		int totalPos = static_cast<int>(std::to_string(user.user_id).length() + std::to_string(entry->parentIndex).length()) + 2;
		name = name.substr(totalPos);
		std::cout << prefix
				<< (lastEntry ? ":-- " : "|-- ")
				<< name
				<< (entry->isDirectory ? "/" : "")
				<< '\n';
		
		if (entry->isDirectory) {
			std::string name(entry->fileName);
			int totalPos = static_cast<int>(std::to_string(user.user_id).length() + std::to_string(entry->parentIndex).length()) + 2;
			name = name.substr(totalPos);
			std::string newPath = path + (path.back() == '/' ? "" : "/") + name;
			treeM(newPath, depth + 1, prefix + (lastEntry ? "    " : "|   "));
		}
	}
}

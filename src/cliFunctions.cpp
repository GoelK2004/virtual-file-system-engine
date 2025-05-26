#include "cliFunctions.h"

void System::printHelpM() {
	std::cout << "Available commands:\n"
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

	const std::string searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
	const int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory(index).\n";
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
	return true;
}

bool System::chownM(const std::string& fileName, const std::string& changeOwnership) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	std::string searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
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

	int colonPos = changeOwnership.find(':');
	int owner_id = std::stoi(changeOwnership.substr(0, colonPos));
	if (changeOwnership.substr(colonPos + 1) == "")	file->group_id = 0;
	else {
		int group_id = std::stoi(changeOwnership.substr(colonPos + 1));
		file->group_id = group_id;
	}
	file->modified_at = std::time(nullptr);
	file->owner_id = owner_id;
	
	Entries->removeFileEntry(searchFile);
	searchFile = std::to_string(owner_id) + std::to_string(0) + "F_" + fileName;
	Entries->insertFileEntry(searchFile, fileIndex);
	strncpy(file->fileName, searchFile.c_str(), FILE_NAME_LENGTH - 1);
	file->fileName[FILE_NAME_LENGTH - 1] = '\0';

	int save = saveDirectoryTable(disk, fileIndex);
	if (save == 0) {
		std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	std::cout << "Successfully updated the permissions.\n";
	disk.close();
	return true;
}

bool System::chgrpCommand(const std::string& fileName, uint32_t new_group_id) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	const std::string searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
	const int fileIndex = Entries->getFile(searchFile);
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

	return true;
}

void System::whoamiM() const {
    const auto it_user = userTable.find(user.user_id);
    const auto it_group = groupTable.find(user.group_id);
    if (it_user != userTable.end())  {
	    std::cout << "User: " << it_user->second << "\n";
	    std::cout << "User ID: " << it_user->first << "\n";
    }
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
	const int passwordHashed = hashFileName(password);
	for (const auto& entry : userDatabase) {
		if (entry->userName == username && entry->password == passwordHashed) {
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
	
	User* userN = new User();
	strncpy(userN->userName, userName.c_str(), USER_NAME_LENGTH);
	userN->userName[USER_NAME_LENGTH - 1] = '\0';
	userN->password = hashFileName(password);
	userN->user_id = 1000 + (++totalUsers);
	userN->group_id = group_id;
	userDatabase.push_back(userN);

	userTable[userN->user_id] = userName;
	if (group_id != -1U){
		auto group = groupTable.find(group_id);
		if (group == groupTable.end()) {
			std::string groupName = "GROUP" + std::to_string(group_id);
			groupTable.insert({group_id, groupName});
			totalGroups++;
		}
	}
	std::cout << "User added: " << userName << " (UID: " << userN->user_id << ", GID: " << userN->group_id << ")\n";
}

void System::showUsersM() {
	for (const auto& userP : userTable) {
		std::cout << userP.first << ' ' << userP.second << '\n';
	}
}
void System::showGroupsM() {
	for (const auto& group : groupTable) {
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
			// std::string name(entry->fileName);
			// int totalPos = static_cast<int>(std::to_string(user.user_id).length() + std::to_string(entry->parentIndex).length()) + 2;
			// name = name.substr(totalPos);
			std::string newPath = path + (path.back() == '/' ? "" : "/") + name;
			treeM(newPath, depth + 1, prefix + (lastEntry ? "    " : "|   "));
		}
	}
}

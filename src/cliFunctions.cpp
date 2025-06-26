#include "cliFunctions.h"

void System::printHelpM(ClientSession* session) {
	session->msg.clear();
	std::string msg("Available commands:\n"
              		 "help\n"
              		 "ls\n"
              		 "cd <dir>\n"
              		 "mkdir <name>\n"
              		 "rmdir <name>\n"
              		 "create <filename> <fileSize(optional)>\n"
              		 "write <filename> \"<fileContent(in quotes)>\"\n"
              		 "append <filename> \"<fileContent(in quotes)>\"\n"
              		 "read <filename>\n"
              		 "rm <filename>\n"
              		 "rename <old> <new>\n"
              		 "stat <filename>\n"
			  		 "chmod <filename> <permissions>\n"
			  		 "chown <filename> <owner_id:group_id>\n"
			  		 "chgrp <filename> <group_id>\n"
			  		 "whoami\n"
			  		 "login <username> <password>\n"
			  		 "useradd <username> <password> <group_id(optional)>\n"
			  		 "showusr\n"
			  		 "showgrp\n"
			  		 "tree\n"	  
              		 "exit\n");
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
}

bool System::chmodFileM(const std::string& fileName, int newPermissions, ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		session->oss << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	const std::string searchFile = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + fileName;
	const int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		session->oss << "Error: File '" << fileName << "' not found in the directory(index).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory(index).\n";
		return false;
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileIndex];
	}
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != session->currentDirectory || file->isDirectory) {
		session->oss << "Error: File '" << fileName << "' not found in the directory.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (session->user.user_id != 0){
		if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
			session->oss << "Error: Write permission denied for the file '" << fileName << "'.\n";
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			return false;
		}
	}

	file->permissions = newPermissions;
	file->modified_at = std::time(nullptr);
	int save = saveDirectoryTable(disk, fileIndex, session);
	if (save == 0) {
		session->oss << "Error: Could not update the permissions.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	// std::cout << "Successfully updated the permissions.\n";
	disk.close();
	return true;
}

bool System::chownM(const std::string& fileName, const std::string& changeOwnership, ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		session->oss << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	std::string searchFile = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		session->oss << "Error: File '" << fileName << "' not found in the directory.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileIndex];
	}
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != session->currentDirectory || file->isDirectory) {
		session->oss << "Error: File '" << fileName << "' not found in the directory.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
		session->oss << "Error: Write permission denied for the file '" << fileName << "'.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
		return false;
	}

	int colonPos = changeOwnership.find(':');
	int owner_id = std::stoi(changeOwnership.substr(0, colonPos));
	
	bool found = false;
	for (const auto& usr : userDatabase)	if (static_cast<int>(usr->user_id) == owner_id)	found = true;
	if (!found) {
		session->oss << "Error: Owner not found.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Owner not found.\n";
		return false;
	}
	
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

	int save = saveDirectoryTable(disk, fileIndex, session);
	if (save == 0) {
		session->oss << "Error: Could not update the permissions.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	// std::cout << "Successfully updated the permissions.\n";
	disk.close();
	return true;
}

bool System::chgrpCommand(const std::string& fileName, uint32_t new_group_id, ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		session->oss << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}

	const std::string searchFile = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + fileName;
	const int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		session->oss << "Error: File '" << fileName << "' not found in the directory.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileIndex];
	}
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != session->currentDirectory || file->isDirectory) {
		session->oss << "Error: File '" << fileName << "' not found in the directory.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
		session->oss << "Error: Write permission denied for the file '" << fileName << "'.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
		return false;
	}

	file->modified_at = std::time(nullptr);
	file->group_id = new_group_id;
	int save = saveDirectoryTable(disk, fileIndex, session);
	if (save == 0) {
		session->oss << "Error: Could not update the permissions.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Could not update the permissions.\n";
        return false;
    }
	// std::cout << "Successfully updated the permissions.\n";
	disk.close();

	return true;
}

void System::whoamiM(ClientSession* session) {
	std::shared_lock<std::shared_mutex> lock_userTable(userTableMutex);
	std::shared_lock<std::shared_mutex> lock_groupTable(groupTableMutex);
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
    const auto it_user = userTable.find(session->user.user_id);
    const auto it_group = groupTable.find(session->user.group_id);
    if (it_user != userTable.end())  {
	    session->oss << "User: " << it_user->second << "\n";
	    session->oss << "User ID: " << it_user->first << "\n";
	    // std::cout << "User: " << it_user->second << "\n";
	    // std::cout << "User ID: " << it_user->first << "\n";
    }
    else {
		session->oss << "Unknown user ID: " << session->user.user_id << "\n";
		// std::cout << "Unknown user ID: " << user.user_id << "\n";
	}
	if (it_group != groupTable.end()) {
		session->oss << "Group: " << it_group->second << "\n";
		// std::cout << "Group: " << it_group->second << "\n";
	}
	else {
		session->oss << "Unknown group ID: " << session->user.group_id << "\n";
		// std::cout << "Unknown group ID: " << user.group_id << "\n";
	}
	std::string msg = session->oss.str();
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
}

void System::loginM(const std::string& username, const std::string& password, ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	
	if (username == "root" && std::stoi(password) == 0) {
		User* userOrg = new User();
		userOrg->group_id = 0;
		userOrg->user_id = 0;
		strncpy(userOrg->userName, "root", USER_NAME_LENGTH);
		userOrg->userName[USER_NAME_LENGTH - 1] = '\0';
		userOrg->password = 0;
		session->user = *userOrg;
		session->currentDirectory = 0;
		
		delete userOrg;
		journalManager->recoverUncommitedOperations(session->user.userName, session);
		// std::cout << "Login successful! Welcome, " << username << ".\n";
		return;
	}
	const int passwordHashed = hashFileName(password);
	for (const auto& entry : userDatabase) {
		if (entry->userName == username && entry->password == passwordHashed) {
			session->user = *entry;
			session->currentDirectory = 0;
			journalManager->recoverUncommitedOperations(session->user.userName, session);
			// std::cout << "Login successful! Welcome, " << username << ".\n";
            return;
		}
	}
	session->oss << "Login failed: Invalid credentials.\n";
	std::string msg = session->oss.str();
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
}

void System::userAddM(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id) {
	std::unique_lock<std::shared_mutex> lock_userData(userDataMutex);
	std::unique_lock<std::shared_mutex> lock_userTable(userTableMutex);
	std::unique_lock<std::shared_mutex> lock_groupTable(groupTableMutex);
	std::unique_lock<std::shared_mutex> lock_userCount(userCountMutex);
	std::unique_lock<std::shared_mutex> lock_groupCount(groupCountMutex);
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	for (const auto& entry : userDatabase) {
		if (entry->userName == userName) {
			session->oss << "Error: User already exists.\n";
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
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
	// std::cout << "User added: " << userName << " (UID: " << userN->user_id << ", GID: " << userN->group_id << ")\n";
}

void System::showUsersM(ClientSession* session) {
	std::shared_lock<std::shared_mutex> lock(userTableMutex);
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	for (const auto& userP : userTable) {
		session->oss << userP.first << ' ' << userP.second << '\n';
	}
	std::string msg = session->oss.str();
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
	
}
void System::showGroupsM(ClientSession* session) {
	std::shared_lock<std::shared_mutex> lock(groupTableMutex);
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	for (const auto& group : groupTable) {
		session->oss << group.first << ' ' << group.second << '\n';
	}
	std::string msg = session->oss.str();
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
}

std::vector<FileEntry*> System::getDirectoryEntries(FileEntry* dir, ClientSession* session) {
	std::shared_lock<std::shared_mutex> lock(metaMutex);
	std::vector<FileEntry*> children;
	if (!dir) {
		for (const auto& entry : metaDataTable) {
			if (entry->parentIndex == 0 && entry->owner_id == session->user.user_id) {
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

void System::treeM(ClientSession* session, const std::string& path, int depth, const std::string& prefix) {
	session->oss.str("");
	session->oss.clear();
	
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	FileEntry* dir = nullptr;
	if (path != "/"){
		dir = resolvePath(disk, path, session);
		if (!dir || !dir->isDirectory) {
			session->oss << "Error: Invalid Directory: " << path << '\n';
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			return;
		}
	}
	if (depth == 0){
		session->oss << path << '\n';
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		session->oss.str("");
		session->oss.clear();
	}
	std::vector<FileEntry*> entries;
	entries = getDirectoryEntries(dir, session);
	for (size_t i = 0; i < entries.size(); i++) {
		bool lastEntry = (i == entries.size() - 1);
		FileEntry* entry = entries[i];

		std::string name(entry->fileName);
		int totalPos = static_cast<int>(std::to_string(session->user.user_id).length() + std::to_string(entry->parentIndex).length()) + 2;
		name = name.substr(totalPos);
		session->oss << prefix
				<< (lastEntry ? ":-- " : "|-- ")
				<< name
				<< (entry->isDirectory ? "/" : "")
				<< '\n';
		
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		session->oss.str("");
		session->oss.clear();

		if (entry->isDirectory) {
			// std::string name(entry->fileName);
			// int totalPos = static_cast<int>(std::to_string(user.user_id).length() + std::to_string(entry->parentIndex).length()) + 2;
			// name = name.substr(totalPos);
			std::string newPath = path + (path.back() == '/' ? "" : "/") + name;
			treeM(session, newPath, depth + 1, prefix + (lastEntry ? "    " : "|   "));
		}
	}
}

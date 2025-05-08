#include "directory.h"

bool System::createDirectory(const std::string &directoryName){
	std::cout << "Creating directory '" << directoryName << "'\n";
	std::string savedDir = std::to_string(user.user_id) + std::to_string(currentDir) + "D_" + directoryName;
	int dirID = Entries->getDir(savedDir);
	if (dirID != -1) {
		FileEntry* dir = metaDataTable[dirID];
		if (currentDir < 0 || currentDir >= MAX_FILES || (dir && dir->isDirectory && dir->owner_id == user.user_id)) {
			std::cerr << "\tError: Directory " << directoryName << " already exists.\n";
			return false;
		}
	}

	if (metaIndex >= MAX_FILES) {
		std::cerr << "\tError: Not enough storage space for creating new directory.\n";
		return false;
	}
			
	FileEntry* newDir = new FileEntry();
	strncpy(newDir->fileName, savedDir.c_str(), sizeof(newDir->fileName) - 1);
	newDir->fileSize = 0;
	newDir->isDirectory = true;
	newDir->owner_id = user.user_id;
	newDir->group_id = user.group_id;
	newDir->parentIndex = currentDir;
	newDir->dirID = availableDirEntry++;

	Entries->insertFileEntry(savedDir, metaIndex);
	metaDataTable.push_back(newDir);
	metaIndex++;

	std::cout << "\tDirectory created successfully.\n";
	return true;
}

FileEntry* System::resolvePath(std::fstream &disk, const std::string &path){
	if (!disk){
		std::cerr << "Error: Cannot access disk while resolving path.\n";
		return nullptr;
	}
	std::string cleanPath = path;
	if (!cleanPath.empty() && cleanPath.back() == '/'){
		cleanPath.pop_back();
	}

	if (cleanPath.empty())	return 0;

	int currentIndex = 0;
	std::istringstream ss(cleanPath);
	std::string token;
	FileEntry* dir;

	while (getline(ss, token, '/')){
		if (token.empty())	continue;

		std::string searchDir = std::to_string(user.user_id) + std::to_string(currentIndex) + "D_" + token;
		int dirIndex = Entries->getDir(searchDir);
		if (dirIndex == -1) {
			std::cerr << "Error: Path " << path << " could not be resolved(dir not found).\n";
			return nullptr;
		}
		dir = metaDataTable[dirIndex];
		if (dir == nullptr || dir->fileName[0] == '\0'){
			std::cerr << "Error: Path " << path << " could not be resolved(dir misplace).\n";
			return nullptr;
		}
		currentIndex = dir->dirID;
	}

	return dir;
}

std::string System::createPathM() {
	std::string path;
	std::string currentUser(user.userName);
	int scopedDir = currentDir;
	while (scopedDir != 0) {
		for (auto& file : metaDataTable) {
			if (file->dirID == scopedDir && file->owner_id == user.user_id) {
				std::string name(file->fileName);
				path = "/" + name.erase(0, 2 + static_cast<int>(std::to_string(currentDir).length() + std::to_string(user.user_id).length())) + path;
				scopedDir = file->parentIndex;
				break;
			}
		}
	}
	if (path == "")
		path = "/";
	return "(" + currentUser + ") " + path;
}

bool System::changeDirectory(const std::string &dirName){
	if (strcmp(dirName.c_str(), "..") == 0){
		if (currentDir == 0){
			std::cout << "Already at root directory.\n";
			return false;
		}
		for (const auto &entry : metaDataTable) {
			if (entry->dirID == currentDir && entry->isDirectory) {
				currentDir = entry->parentIndex;
				return false;
			}
		}
	}
	std::string searchDir = std::to_string(user.user_id) + std::to_string(currentDir) + "D_" + dirName;
	int dirIndex = Entries->getDir(searchDir);
	if (dirIndex == -1) {
		std::cerr << "Error: Directory '" << dirName << "' not found(dir not found).\n";
		return false;
	}
	FileEntry* dir = metaDataTable[dirIndex];
	if (!dir || (dir->owner_id != user.user_id)) {
		std::cerr << "Error: Directory '" << dirName << "' not found.\n";
		return false;
	}
	currentDir = dir->dirID;
	return true;
}

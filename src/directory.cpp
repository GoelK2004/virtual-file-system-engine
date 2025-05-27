#include "directory.h"

bool System::createDirectory(const std::string &directoryName){
	std::cout << "Creating directory '" << directoryName << "'\n";
	
	int lastDel, currentIndex = currentDir;
	std::string dirNameCal(directoryName);
	if (dirNameCal.back() == '/')	dirNameCal.pop_back();
	lastDel = dirNameCal.rfind('/');
	std::string newDirName(dirNameCal.substr(lastDel + 1));
	if (lastDel != -1) {
		FileEntry* dir = nullptr;
		std::stringstream ss(dirNameCal.substr(0, lastDel));
		std::string token;
		while (getline(ss, token, '/')) {
			if (token.empty())	continue;
			if (token == ".")	continue;
			if (token == "..") {
				if (currentIndex == 0)	currentDir = 0;
				else if (!dir) {
					for (const auto& entry : metaDataTable)	{
						if (entry->dirID == currentIndex) {
							currentIndex = entry->parentIndex;
							break;
						}
					}
					for (const auto& entry : metaDataTable) {
						if (entry->dirID == currentIndex) {
							dir = entry;
							break;
						}
					}
				} 
				else {
					currentIndex = dir->parentIndex;
					for (const auto& entry : metaDataTable) {
						if (entry->dirID == currentIndex && entry->owner_id == user.user_id && entry->isDirectory) {
							dir = entry;
							break;
						}
					}
				} 
			} else {
				std::string searchDir = std::to_string(user.user_id) + std::to_string(currentIndex) + "D_" + token;
				const int dirIndex = Entries->getDir(searchDir);
				if (dirIndex == -1) {
					std::cerr << "Error: Path could not be resolved(dir not found).\n";
					return false;
				}
				dir = metaDataTable[dirIndex];
				if (dir == nullptr || dir->fileName[0] == '\0'){
					std::cerr << "Error: Path could not be resolved(dir misplace).\n";
					return false;
				}
				currentIndex = dir->dirID;
			}
		}
	}
	
	const std::string savedDir = std::to_string(user.user_id) + std::to_string(currentIndex) + "D_" + newDirName;
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
	newDir->parentIndex = currentIndex;
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

	if (cleanPath.empty())	return nullptr;

	int currentIndex = 0;
	std::stringstream ss(cleanPath);
	std::string token;
	FileEntry* dir = nullptr;

	while (getline(ss, token, '/')){
		if (token.empty())	continue;

		std::string searchDir = std::to_string(user.user_id) + std::to_string(currentIndex) + "D_" + token;
		const int dirIndex = Entries->getDir(searchDir);
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
	const std::string currentUser(user.userName);
	int scopedDir = currentDir;
	while (scopedDir != 0) {
		for (const auto& file : metaDataTable) {
			if (file->dirID == scopedDir && file->owner_id == user.user_id) {
				std::string name(file->fileName);
				path = "/" + name.erase(0, 2 + static_cast<int>(std::to_string(currentDir).length() + std::to_string(user.user_id).length())) + path;
				scopedDir = file->parentIndex;
				break;
			}
		}
	}
	if (path.empty())
		path = "/";
	return "(" + currentUser + ") " + path;
}

bool System::changeDirectory(const std::string &dirName){
	
	std::string parsedDirName(dirName);
	int currentIndex = currentDir;
	if (parsedDirName.back() == '/')	parsedDirName.pop_back();
	currentIndex = extractPath(parsedDirName, currentIndex);

	if (currentIndex != -1)	currentDir = currentIndex;
	return true;
}

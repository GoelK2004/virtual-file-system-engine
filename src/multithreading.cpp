#include "multithreading.h"

static void openFile(FileEntry* file) {
	std::unique_lock<std::mutex> lock(file->lockMutex);

	file->openCount++;
	std::cout << "[System] File '" << file->fileName << "' opened. Open count: " << file->openCount << "\n";
}

static void closeFile(FileEntry* file) {
	std::unique_lock<std::mutex> lock(file->lockMutex);

	if (file->openCount > 0) {
		file->openCount--;
		std::cout << "[System] File '" << file->fileName << "' closed. Open count: " << file->openCount << "\n";
	} else {
		std::cerr << "[Warning] File '" << file->fileName << "' is not open.\n";
	}
}

static void acquireReadLock(FileEntry* file) {
	std::unique_lock<std::mutex> lock(file->lockMutex);

	
	while (file->isWriteLocked || file->writersWaiting > 0) {
		file->readerCV.wait(lock);
	}
	
	std::cout << "[Reader] acquired read lock for file: " << file->fileName << ".\n";
	file->readerCount++;
}

static void releaseReadLock(FileEntry* file) {
	std::unique_lock<std::mutex> lock(file->lockMutex);

	file->readerCount--;
	if (file->readerCount == 0) {
		file->writerCV.notify_one();
	}
	std::cout << "[Reader] released read lock for file: " << file->fileName << ".\n";
}

std::string System::readData(const std::string& fileName) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk){
		std::cerr << "Error: Disk not accessible while reading a file.\n";
		return "";
	}
	std::string searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		std::cerr << "Error: Cannot read file '" << fileName << "' (file not found: 1).\n";
		return "";
	}
	FileEntry* file = metaDataTable[fileIndex];
	if (file == nullptr || file->fileName[0] == '\0' || strncmp(file->fileName, searchFile.c_str(), FILE_NAME_LENGTH) != 0 || file->parentIndex != currentDir || file->isDirectory){
		std::cerr << "Error: Cannot read file '" << fileName << "' (file not found: 2).\n";
		return "";
	}

	if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_READ)){
		std::cerr << "Error: Permission denied to read file '" << fileName << "'.\n";
		return "";
	}
	file->accessed_at = static_cast<int>(std::time(nullptr));

	openFile(file);
	acquireReadLock(file);
	std::string content = readFileData(disk, file);
	releaseReadLock(file);
	closeFile(file);

	disk.close();
	return content;
}

static void acquireWriteLock(FileEntry* file) {
	std::unique_lock<std::mutex> lock(file->lockMutex);

	file->writersWaiting++;
	while (file->readerCount > 0 || file->isWriteLocked) {
		file->writerCV.wait(lock);
	}

	file->writersWaiting--;
	file->isWriteLocked = true;
	std::cout << "[Writer] acquired write lock for file: " << file->fileName << ".\n";
}

static void releaseWriteLock(FileEntry* file) {
	std::unique_lock<std::mutex> lock(file->lockMutex);
	
	file->isWriteLocked = false;

	if (file->writersWaiting > 0) {
		file->writerCV.notify_one();
	} else {
		file->readerCV.notify_all();
	}
	const char* str = file->fileName[0] == '\0' ? "[DELETED]" : file->fileName;
	std::cout << "[Writer] released write lock for file: " << str << ".\n";
}

bool System::writeData(const std::string &fileName, const std::string &fileContent, bool append, const bool check, uint64_t timestamp) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk.is_open()) {
		std::cerr << "Error: Disk file is not open for writing.(writing to '" << fileName << "')\n";
		return false;
	}
	if (fileContent.empty()) {
		std::cerr << "Error: No data provided to write for file '" << fileName << "'.\n";
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

	openFile(file);
	acquireWriteLock(file);
	uint64_t time;
	if (!check){
		if (append)
			time = journalManager->logOperation(OP_WRITE_APPEND, searchFile, fileContent, file->fileSize);
		else
			time = journalManager->logOperation(OP_WRITE, searchFile, fileContent, file->fileSize);
	}
	writeFileData(*this, disk, file, fileIndex, fileContent, append);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	releaseWriteLock(file);
	closeFile(file);
	
	disk.close();
	return true;
}

bool System::deleteDataFile(const std::string& fileName, const bool check, uint64_t timestamp) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	std::string searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
	int fileInd = Entries->getFile(searchFile);
	if (fileInd == -1) {
		std::cerr << "\tError: Cannot delete file '" << fileName << "' (file not present).\n";
		return false;
	}
	FileEntry* file = metaDataTable[fileInd];
	if (!file || file->parentIndex != currentDir) {
		std::cerr << "\tError: Cannot delete file '" << fileName << "' (file not found).\n";
		return false;
	}

	if (file->attributes & ATTRIBUTES_SYSTEM){
		std::cerr << "\tError: Cannot delete file, permission denied(system critical file).\n";
		return false;
	}
	if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_WRITE)){
		std::cerr << "\tError: Cannot delete file, permission denied.\n";
		return false;
	}

	acquireWriteLock(file);
	uint64_t time;
	if (!check)
		time = journalManager->logOperation(OP_DELETE_FILE, searchFile, "", file->fileSize);
	deleteFile(*this, disk, file, fileInd);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	file->fileName[0] = '\0';
	releaseWriteLock(file);
	delete file;
	
	disk.close();
	return true;
}

bool System::deleteDataDir(const std::string& fileName, const bool check, uint64_t timestamp) {
	int lastDel, currentIndex = currentDir;
	std::string dirNameCal(fileName);
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
	
	
	const std::string searchFile = std::to_string(user.user_id) + std::to_string(currentIndex) + "D_" + newDirName;
	int fileInd = Entries->getFile(searchFile);
	if (fileInd == -1) {
		std::cerr << "\tError: Cannot delete directory '" << fileName << "' (folder not present).\n";
		return false;
	}
	FileEntry* file = metaDataTable[fileInd];
	if (!file) {
		std::cerr << "\tError: Cannot delete directory '" << fileName << "' (directory not found).\n";
		return false;
	}
	if (file->attributes & ATTRIBUTES_SYSTEM){
		std::cerr << "\tError: Cannot delete directory, permission denied(system critical folder).\n";
		return false;
	}
	if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_WRITE)){
		std::cerr << "\tError: Cannot delete directory, permission denied.\n";
		return false;
	}

	acquireWriteLock(file);
	uint64_t time;
	if (!check)
		time = journalManager->logOperation(OP_DELETE_DIR, searchFile, "", file->fileSize);
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	deleteFile(*this, disk, file, fileInd);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	file->fileName[0] = '\0';
	releaseWriteLock(file);
	delete file;
	
	return true;
}

bool System::createFiles(const std::string& fileName, const int &fileSize, uint16_t permissions, const bool check, uint64_t timestamp) {
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk) {
		std::cerr << "\tError: Cannot access disk for creating file '" << fileName  << "'.\n";
		return false;
	}
	if (metaIndex >= MAX_FILES) {
		std::cerr << "\tError: File entry full.\n";
		return false;
	}
	int requiredBlocks = (fileSize + BLOCK_SIZE - 1)/BLOCK_SIZE;
	if (superblock.freeBlocks < requiredBlocks){
		std::cerr << "\tError: Not enough free blocks to create new file.\n";
		return false;
	}
	bool validParent = (currentDir == 0);
	if (!validParent){
		if (currentDir < 0 || currentDir >= MAX_FILES){
			std::cerr << "\tError: Parent index invalid.\n";
			return false;
		}
	}
	std::string savedName = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
	int searchFileIndex = Entries->getFile(savedName);
	if (searchFileIndex != -1) {
		FileEntry* searchFile = metaDataTable[searchFileIndex];
		if (searchFile && searchFile->parentIndex == currentDir && searchFile->owner_id == user.user_id) {
			std::cerr << "\tError: File exists in the directory.\n";
			return false;
		}
	}
	FileEntry* newFile = new FileEntry(savedName);

	acquireWriteLock(newFile);
	uint64_t time;
	if (!check)
		time = journalManager->logOperation(OP_CREATE, fileName, "", fileSize);
	createFile(*this, disk, fileName, fileSize, newFile, permissions);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	releaseWriteLock(newFile);
	
	disk.close();
	return true;
}

bool System::renameFiles(const std::string &fileName, const std::string &newName, const bool check, uint64_t timestamp) {
	std::string searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory(index).\n";
		return false;
	}
	FileEntry* file = metaDataTable[fileIndex];
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != currentDir) {
		std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (!hasPermission(*file, user.user_id, user.group_id, PERMISSION_WRITE)){
		std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
		return false;
	}
	
	for (const auto& fileT : metaDataTable) {
		std::string fileNameT(fileT->fileName);
		fileNameT = fileNameT.substr(static_cast<int>(std::to_string(fileT->owner_id).length() + std::to_string(fileT->parentIndex).length()) + 2);
		if (fileT->parentIndex == file->parentIndex && fileNameT == newName) {
			std::cerr << "\tError: File with same name already exists.\n";
			return false;
		}
	}
	
	openFile(file);
	acquireWriteLock(file);
	uint64_t time;
	if (!check){
		time = journalManager->logOperation(OP_RENAME, searchFile, "", file->fileSize);
	}
	renameFile(*this, file, newName);
	
	Entries->removeFileEntry(searchFile);
	searchFile = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + newName;
	Entries->insertFileEntry(searchFile, fileIndex);
	
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	releaseWriteLock(file);
	closeFile(file);
	return true;
}

void System::list() {
    bool found = false;

    for (const auto& entry : metaDataTable) {
        if (entry->parentIndex == currentDir && strlen(entry->fileName) > 0 && entry->owner_id == user.user_id) {
			std::string name(entry->fileName);
			std::cout << name.erase(0, 2 + static_cast<int>(std::to_string(currentDir).length() + std::to_string(user.user_id).length())) << "\t" << entry->fileSize << " bytes\n";
			found = true;
        }
    }

    if (!found) {
        std::cout << "Directory is empty.\n";
    }
}

void System::fileMetadata(const std::string& filename) {
    std::string fullName = std::to_string(user.user_id) + std::to_string(currentDir) + "F_" + filename;

    for (const auto& entry : metaDataTable) {
        if (entry->parentIndex == currentDir && fullName == entry->fileName) {
			time_t createdTime = static_cast<time_t>(entry->created_at);
			std::tm* createdTimeInfo = std::localtime(&createdTime);
			time_t modifiedTime = static_cast<time_t>(entry->modified_at);
			std::tm* modifiedTimeInfo = std::localtime(&modifiedTime);
			std::string name(entry->fileName);
			int pos = static_cast<int>(std::to_string(user.user_id).length() + std::to_string(currentDir).length()) + 2;
            std::cout << "Filename     : " << name.substr(pos) << "\n";
            std::cout << "Size         : " << entry->fileSize << " bytes\n";
            std::cout << "Created At   : " << std::asctime(createdTimeInfo);
            std::cout << "Modified At  : " << std::asctime(modifiedTimeInfo);
            std::cout << "Permissions  : " << permissionToString(entry) << "\n";
            std::cout << "Attributes   : " << std::oct << entry->attributes << std::dec << "\n";
            std::cout << "Ownder ID    : " << entry->owner_id << "\n";
            std::cout << "Group ID     : " << entry->group_id << "\n";
			
            return;
        }
    }

    std::cout << "File not found in current directory.\n";
}
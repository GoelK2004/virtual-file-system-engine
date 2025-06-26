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

std::string System::readData(const std::string& fileName, ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk){
		std::string msg("Error: Disk not accessible while reading a file.\n");
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return "";
	}
	std::string searchFile = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
	if (fileIndex == -1) {
		session->oss << "Error: Cannot read file '" << fileName << "' (file not found: 1).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Cannot read file '" << fileName << "' (file not found: 1).\n";
		return "";
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileIndex];
	}
	if (file == nullptr || file->fileName[0] == '\0' || strncmp(file->fileName, searchFile.c_str(), FILE_NAME_LENGTH) != 0 || file->parentIndex != session->currentDirectory || file->isDirectory){
		session->oss << "Error: Cannot read file '" << fileName << "' (file not found: 2).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: Cannot read file '" << fileName << "' (file not found: 2).\n";
		return "";
	}

	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_READ)){
		session->oss << "Error: Permission denied to read file '" << fileName << "'.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return "";
	}
	file->accessed_at = static_cast<int>(std::time(nullptr));

	openFile(file);
	acquireReadLock(file);
	std::string content = readFileData(disk, file, session);
	releaseReadLock(file);
	closeFile(file);

	disk.close();
	session->msg.insert(session->msg.end(), session->oss.str().begin(), session->oss.str().end());
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

bool System::writeData(const std::string &fileName, const std::string &fileContent, bool append, ClientSession* session, const bool check, uint64_t timestamp, FileJournaling* entry) {
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
	if (fileContent.empty()) {
		session->oss << "Error: No data provided to write for file '" << fileName << "'.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "Error: No data provided to write for file '" << fileName << "'.\n";
		return false;
	}

	if (entry)	session->currentDirectory = entry->directory;
	
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
		session->msg.insert(session->msg.end(), session->oss.str().begin(), session->oss.str().end());
		// std::cerr << "\tError: File '" << fileName << "' not found in the directory.\n";
		return false;
	}
	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
		session->oss << "Error: Write permission denied for the file '" << fileName << "'.\n";
		session->msg.insert(session->msg.end(), session->oss.str().begin(), session->oss.str().end());
		// std::cerr << "\tError: Write permission denied for the file '" << fileName << "'.\n";
		return false;
	}

	openFile(file);
	acquireWriteLock(file);
	uint64_t time;
	if (!check){
		if (append)
			time = journalManager->logOperation(std::string(session->user.userName), OP_WRITE_APPEND, searchFile, "", fileContent, file->fileSize, session->currentDirectory);
		else
			time = journalManager->logOperation(std::string(session->user.userName), OP_WRITE, searchFile, "", fileContent, file->fileSize, session->currentDirectory);
	}
	// std::sleep(10);
	// std::this_thread::sleep_for(std::chrono::seconds(10));
	writeFileData(*this, session, disk, file, fileIndex, fileContent, append);
	if (!check)
		journalManager->markCommitted(time);
	else {
		session->currentDirectory = 0;
		journalManager->markCommitted(timestamp);
	}
	releaseWriteLock(file);
	closeFile(file);
	
	disk.close();
	if (session->oss.str() != "")	{
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
	}
	return true;
}

bool System::deleteDataFile(const std::string& fileName, ClientSession* session, const bool check, uint64_t timestamp, FileJournaling* entry) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	
	int currentIndex = session->currentDirectory;
	std::string path(fileName);
	int lastDel = path.rfind('/');
	std::string newFileName(path.substr(lastDel + 1));
	if (lastDel != -1)	currentIndex = extractPath(path.substr(0, lastDel), currentIndex, session);
	
	if (entry && currentIndex != entry->directory)	currentIndex = entry->directory;
	
	std::string searchFile = std::to_string(session->user.user_id) + std::to_string(currentIndex) + "F_" + newFileName;
	int fileInd = Entries->getFile(searchFile);
	if (fileInd == -1) {
		session->oss << "Error: Cannot delete file '" << newFileName << "' (file not present).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete file '" << newFileName << "' (file not present).\n";
		return false;
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileInd];
	}
	if (!file || file->parentIndex != currentIndex) {
		session->oss << "Error: Cannot delete file '" << fileName << "' (file not found).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete file '" << fileName << "' (file not found).\n";
		return false;
	}

	if (file->attributes & ATTRIBUTES_SYSTEM){
		session->oss << "Error: Cannot delete file, permission denied(system critical file).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete file, permission denied(system critical file).\n";
		return false;
	}
	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
		session->oss << "Error: Cannot delete file, permission denied.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete file, permission denied.\n";
		return false;
	}

	acquireWriteLock(file);
	uint64_t time;
	if (!check)
		time = journalManager->logOperation(std::string(session->user.userName), OP_DELETE_FILE, searchFile, "", "", file->fileSize, currentIndex);
	deleteFile(*this, session, disk, file, fileInd);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	file->fileName[0] = '\0';
	releaseWriteLock(file);
	delete file;
	
	disk.close();
	if (session->oss.str() != "") {
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
	}
	return true;
}

bool System::deleteDataDir(const std::string& fileName, ClientSession* session, const bool check, uint64_t timestamp, FileJournaling* entry) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	int lastDel, currentIndex = session->currentDirectory;
	std::string dirNameCal(fileName);
	
	if (dirNameCal.back() == '/')	dirNameCal.pop_back();
	lastDel = dirNameCal.rfind('/');
	std::string newDirName(dirNameCal.substr(lastDel + 1));

	
	if (lastDel != -1)	currentIndex = extractPath(dirNameCal.substr(0, lastDel), currentIndex, session);
	
	
	if (entry && currentIndex != entry->directory)	currentIndex = entry->directory;
	
	const std::string searchFile = std::to_string(session->user.user_id) + std::to_string(currentIndex) + "D_" + newDirName;
	
	bool recursive = false;
	for (auto& file : metaDataTable) {
		if (strcmp(file->fileName, searchFile.c_str()) == 0)	continue;
		if (file->parentIndex == currentIndex) {
			recursive = true;
			break;
		}
	}

	if (recursive) {
		session->oss << "Error: Cannot delete directory '" << fileName << "'. It is not empty.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return false;
	}

	int fileInd = Entries->getFile(searchFile);
	if (fileInd == -1) {
		session->oss << "Error: Cannot delete directory '" << fileName << "' (folder not present).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cout << "\tError: Cannot delete directory '" << fileName << "' (folder not present).\n";
		return false;
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileInd];
	}
	if (!file) {
		session->oss << "Error: Cannot delete directory '" << fileName << "' (directory not found).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete directory '" << fileName << "' (directory not found).\n";
		return false;
	}
	if (file->attributes & ATTRIBUTES_SYSTEM){
		session->oss << "Error: Cannot delete directory, permission denied(system critical folder).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete directory, permission denied(system critical folder).\n";
		return false;
	}
	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
		session->oss << "Error: Cannot delete directory, permission denied.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Cannot delete directory, permission denied.\n";
		return false;
	}

	acquireWriteLock(file);
	uint64_t time;
	if (!check)
		time = journalManager->logOperation(std::string(session->user.userName), OP_DELETE_DIR, searchFile, "", "", file->fileSize, currentIndex);
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	deleteFile(*this, session, disk, file, fileInd);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	file->fileName[0] = '\0';
	releaseWriteLock(file);
	delete file;
	
	if (session->oss.str() != "") {
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
	}
	return true;
}

bool System::createFiles(const std::string& fileName, ClientSession* session, const int &fileSize, uint16_t permissions, const bool check, uint64_t timestamp, FileJournaling* entry) {
	std::unique_lock<std::shared_mutex> lock(metaIndexMutex);
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	std::fstream disk(DISK_PATH, std::ios::binary | std::ios::out | std::ios::in);
	if (!disk) {
		session->oss << "Error: Cannot access disk for creating file '" << fileName  << "'.\n";
		if (session->oss.str() != "") {
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		}
		// std::cerr << "\tError: Cannot access disk for creating file '" << fileName  << "'.\n";
		return false;
	}
	if (metaIndex >= MAX_FILES) {
		session->oss << "Error: File entry full.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: File entry full.\n";
		return false;
	}
	int requiredBlocks = (fileSize + BLOCK_SIZE - 1)/BLOCK_SIZE;
	if (superblock.freeBlocks < requiredBlocks){
		session->oss << "Error: Not enough free blocks to create new file.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		// std::cerr << "\tError: Not enough free blocks to create new file.\n";
		return false;
	}

	int currentIndex = session->currentDirectory;
	std::string path(fileName);
	int lastDel = path.rfind('/');
	std::string newFileName(path.substr(lastDel + 1));
	if (lastDel != -1)	currentIndex = extractPath(path.substr(0, lastDel), currentIndex, session);
	
	if (entry && currentIndex != entry->directory) {
		currentIndex = entry->directory;
		// newFileName = entryfileName.erase(0, 2 + static_cast<int>(std::to_string(currentIndex).length() + entry->user.length()));
	}
	
	bool validParent = (currentIndex == 0);
	if (!validParent){
		if (currentIndex < 0 || currentIndex >= MAX_FILES){
			session->oss << "Error: Parent index invalid.\n";
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			// std::cerr << "\tError: Parent index invalid.\n";
			return false;
		}
	}
	std::string savedName = std::to_string(session->user.user_id) + std::to_string(currentIndex) + "F_" + newFileName;
	int searchFileIndex = Entries->getFile(savedName);
	if (searchFileIndex != -1) {
		FileEntry* searchFile;
		{
			std::shared_lock<std::shared_mutex> lock(metaMutex);
			searchFile = metaDataTable[searchFileIndex];
		}
		if (searchFile && searchFile->parentIndex == currentIndex && searchFile->owner_id == session->user.user_id) {
			session->oss << "Error: File exists in the directory.\n";
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			// std::cerr << "\tError: File exists in the directory.\n";
			return false;
		}
	}
	FileEntry* newFile = new FileEntry(savedName);

	acquireWriteLock(newFile);
	uint64_t time;
	if (!check)
		time = journalManager->logOperation(std::string(session->user.userName), OP_CREATE, savedName, "", "", fileSize, currentIndex);
	createFile(*this, session, disk, newFileName, fileSize, newFile, currentIndex, permissions);
	if (!check)
		journalManager->markCommitted(time);
	else {
		journalManager->markCommitted(timestamp);
	}
	releaseWriteLock(newFile);
	
	disk.close();
	if (session->oss.str() != "") {
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
	}
	return true;
}

bool System::renameFiles(const std::string &fileName, const std::string &newName, ClientSession* session, const bool check, uint64_t timestamp, FileJournaling* entry) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	if (entry)	session->currentDirectory = entry->directory;
	
	std::string searchFile = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + fileName;
	int fileIndex = Entries->getFile(searchFile);
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
	if (file == nullptr || file->fileName[0] == '\0' || file->parentIndex != session->currentDirectory) {
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
	
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		for (const auto& fileT : metaDataTable) {
			std::string fileNameT(fileT->fileName);
			if (fileNameT != ""){
				fileNameT = fileNameT.substr(static_cast<int>(std::to_string(fileT->owner_id).length() + std::to_string(fileT->parentIndex).length()) + 2);
				if (fileT->parentIndex == file->parentIndex && fileNameT == newName) {
					session->oss << "Error: File with same name already exists.\n";
					std::string msg = session->oss.str();
					session->msg.insert(session->msg.end(), msg.begin(), msg.end());
					// std::cerr << "\tError: File with same name already exists.\n";
					return false;
				}
			}
		}
	}
	
	openFile(file);
	acquireWriteLock(file);
	uint64_t time;
	if (!check){
		time = journalManager->logOperation(std::string(session->user.userName), OP_RENAME, searchFile, newName, "", file->fileSize, session->currentDirectory);
	}
	renameFile(file, newName, session);
	
	Entries->removeFileEntry(searchFile);
	searchFile = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + newName;
	Entries->insertFileEntry(searchFile, fileIndex);
	
	if (!check)
		journalManager->markCommitted(time);
	else {
		session->currentDirectory = 0;
		journalManager->markCommitted(timestamp);
	}
	releaseWriteLock(file);
	closeFile(file);
	
	if (session->oss.str() != "") {
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
	}
	
	return true;
}

void System::list(ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
    bool found = false;

	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		for (const auto& entry : metaDataTable) {
			if (entry->parentIndex == session->currentDirectory && strlen(entry->fileName) > 0 && entry->owner_id == session->user.user_id) {
				std::string name(entry->fileName);
				session->oss << name.erase(0, 2 + static_cast<int>(std::to_string(session->currentDirectory).length() + std::to_string(session->user.user_id).length())) << "\t" << entry->fileSize << " bytes\n";
				found = true;
			}
		}
	}

    if (!found) {
        session->oss << "Directory is empty.\n";
    }

	std::string msg = session->oss.str();
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
}

void System::fileMetadata(const std::string& filename, ClientSession* session) {
	std::shared_lock<std::shared_mutex> lock(metaMutex);
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
    std::string fullName = std::to_string(session->user.user_id) + std::to_string(session->currentDirectory) + "F_" + filename;

    for (const auto& entry : metaDataTable) {
        if (entry->parentIndex == session->currentDirectory && fullName == entry->fileName) {
			time_t createdTime = static_cast<time_t>(entry->created_at);
			std::tm* createdTimeInfo = std::localtime(&createdTime);
			time_t modifiedTime = static_cast<time_t>(entry->modified_at);
			std::tm* modifiedTimeInfo = std::localtime(&modifiedTime);
			std::string name(entry->fileName);
			int pos = static_cast<int>(std::to_string(session->user.user_id).length() + std::to_string(session->currentDirectory).length()) + 2;
            session->oss << "Filename     : " << name.substr(pos) << "\n";
            session->oss << "Size         : " << entry->fileSize << " bytes\n";
            session->oss << "Created At   : " << std::asctime(createdTimeInfo);
            session->oss << "Modified At  : " << std::asctime(modifiedTimeInfo);
            session->oss << "Permissions  : " << permissionToString(entry) << "\n";
            session->oss << "Attributes   : " << std::oct << entry->attributes << std::dec << "\n";
            session->oss << "Ownder ID    : " << entry->owner_id << "\n";
            session->oss << "Group ID     : " << entry->group_id << "\n";
			
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			
            return;
        }
    }

    session->oss << "File not found in current directory.\n";
	std::string msg = session->oss.str();
	session->msg.insert(session->msg.end(), msg.begin(), msg.end());
}

bool System::recursiveDelete(const std::string& fileName, ClientSession* session) {
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	int lastDel, currentIndex = session->currentDirectory;
	std::string dirNameCal(fileName);
	
	if (dirNameCal.back() == '/')	dirNameCal.pop_back();
	lastDel = dirNameCal.rfind('/');
	std::string newDirName(dirNameCal.substr(lastDel + 1));
	if (lastDel != -1)	currentIndex = extractPath(dirNameCal.substr(0, lastDel), currentIndex, session);
	
	const std::string searchFile = std::to_string(session->user.user_id) + std::to_string(currentIndex) + "D_" + newDirName;
	int fileInd = Entries->getFile(searchFile);
	if (fileInd == -1) {
		session->oss << "Error: Cannot delete directory '" << fileName << "' (folder not present).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return false;
	}
	FileEntry* file;
	{
		std::shared_lock<std::shared_mutex> lock(metaMutex);
		file = metaDataTable[fileInd];
	}
	if (!file) {
		session->oss << "Error: Cannot delete directory '" << fileName << "' (directory not found).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return false;
	}
	if (file->attributes & ATTRIBUTES_SYSTEM){
		session->oss << "Error: Cannot delete directory, permission denied(system critical folder).\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return false;
	}
	if (!hasPermission(*file, session->user.user_id, session->user.group_id, PERMISSION_WRITE)){
		session->oss << "Error: Cannot delete directory, permission denied.\n";
		std::string msg = session->oss.str();
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return false;
	}

	for (auto& entry : metaDataTable) {
		if (strcmp(entry->fileName, searchFile.c_str()) == 0)	continue;
		else if (entry->parentIndex == file->dirID) {
			std::string toDelFile(entry->fileName);
			toDelFile = toDelFile.substr(2 + std::to_string(session->user.user_id).length() + std::to_string(currentIndex).length());
			if (entry->isDirectory) {
				int tempCurDir = session->currentDirectory;
				session->currentDirectory = file->dirID;
				recursiveDelete(toDelFile, session);
				session->currentDirectory = tempCurDir;
			} else {
				int tempCurDir = session->currentDirectory;
				session->currentDirectory = currentIndex;
				deleteDataFile(toDelFile, session);
				session->currentDirectory = tempCurDir;
			}
		}
	}
	deleteDataDir(fileName, session);

	return true;
}
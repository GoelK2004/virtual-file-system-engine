#include "directory.h"

bool System::createDirectory(const std::string &directoryName, ClientSession* session){
	// std::cout << "Creating directory '" << directoryName << "'\n";
	session->msg.clear();
	session->oss.str("");
	session->oss.clear();
	
	
	int lastDel, currentIndex = session->currentDirectory;
	std::string dirNameCal(directoryName);
	if (dirNameCal.back() == '/')	dirNameCal.pop_back();
	lastDel = dirNameCal.rfind('/');
	std::string newDirName(dirNameCal.substr(lastDel + 1));
	if (lastDel != -1)	currentIndex = extractPath(dirNameCal.substr(0, lastDel), currentIndex, session);
	const std::string savedDir = std::to_string(session->user.user_id) + std::to_string(currentIndex) + "D_" + newDirName;
	int dirID = Entries->getDir(savedDir);
	if (dirID != -1) {
		FileEntry* dir;
		{
			std::shared_lock<std::shared_mutex> lock(metaMutex);
			dir = metaDataTable[dirID];
		}
		if (currentIndex < 0 || currentIndex >= MAX_FILES || (dir && dir->isDirectory && dir->owner_id == session->user.user_id)) {
			session->oss << "Error: Directory " << directoryName << " already exists.\n";
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			// std::cerr << "\tError: Directory " << directoryName << " already exists.\n";
			return false;
		}
	}
	
	{
		std::shared_lock<std::shared_mutex> lock(metaIndexMutex);
		if (metaIndex >= MAX_FILES) {
			session->oss << "Error: Not enough storage space for creating new directory.\n";
			std::string msg = session->oss.str();
			session->msg.insert(session->msg.end(), msg.begin(), msg.end());
			// std::cerr << "\tError: Not enough storage space for creating new directory.\n";
			return false;
		}
	}

	FileEntry* newDir = new FileEntry();
	strncpy(newDir->fileName, savedDir.c_str(), sizeof(newDir->fileName) - 1);
	newDir->fileSize = 0;
	newDir->isDirectory = true;
	newDir->owner_id = session->user.user_id;
	newDir->group_id = session->user.group_id;
	newDir->parentIndex = currentIndex;
	{
		std::unique_lock<std::shared_mutex> lock(dirEntryMutex);
		newDir->dirID = availableDirEntry++;
	}

	{
		std::shared_lock<std::shared_mutex> lock(metaIndexMutex);
		Entries->insertFileEntry(savedDir, metaIndex);
	}
	{
		std::unique_lock<std::shared_mutex> lock(metaMutex);
		metaDataTable.push_back(newDir);
	}
	{
		std::unique_lock<std::shared_mutex> lock(metaIndexMutex);
		metaIndex++;
	}

	// std::cout << "\tDirectory created successfully.\n";
	return true;
}

FileEntry* System::resolvePath(std::fstream &disk, const std::string &path, ClientSession* session){
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

		std::string searchDir = std::to_string(session->user.user_id) + std::to_string(currentIndex) + "D_" + token;
		const int dirIndex = Entries->getDir(searchDir);
		if (dirIndex == -1) {
			std::cerr << "Error: Path " << path << " could not be resolved(dir not found).\n";
			return nullptr;
		}
		{
			std::shared_lock<std::shared_mutex> lock(metaMutex);
			dir = metaDataTable[dirIndex];
		}
		if (dir == nullptr || dir->fileName[0] == '\0'){
			std::cerr << "Error: Path " << path << " could not be resolved(dir misplace).\n";
			return nullptr;
		}
		currentIndex = dir->dirID;
	}

	return dir;
}

std::string System::createPathM(ClientSession* session) {
	std::shared_lock<std::shared_mutex> lock(metaMutex);
	std::string path;
	const std::string currentUser(session->user.userName);
	int scopedDir = session->currentDirectory;
	while (scopedDir != 0) {
		for (const auto& file : metaDataTable) {
			if (file->dirID == scopedDir && file->owner_id == session->user.user_id) {
				std::string name(file->fileName);
				path = "/" + name.erase(0, 2 + static_cast<int>(std::to_string(session->currentDirectory).length() + std::to_string(session->user.user_id).length())) + path;
				scopedDir = file->parentIndex;
				break;
			}
		}
	}
	if (path.empty())
		path = "/";
	return "(" + currentUser + ") " + path;
}

bool System::changeDirectory(const std::string &dirName, ClientSession* session){
	
	std::string parsedDirName(dirName);
	int currentIndex = session->currentDirectory;
	if (parsedDirName.back() == '/')	parsedDirName.pop_back();
	currentIndex = extractPath(parsedDirName, currentIndex, session);

	if (currentIndex != -1)	session->currentDirectory = currentIndex;
	std::cout << session->currentDirectory << '\n';
	return true;
}

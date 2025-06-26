#include "centralised.h"

// BITMAP FAT Table
// std::vector<bool> FATTABLE(TOTAL_BLOCKS, false);

bool System::loadBitMap(std::fstream &disk){
	std::unique_lock<std::shared_mutex> lock(FATMutex);
	std::vector<char> buffer(TOTAL_BLOCKS / 8, 0);
	disk.seekg(BITMAP_START * BLOCK_SIZE, std::ios::beg);
	disk.read(buffer.data(), static_cast<int>(buffer.size()));
	if (disk.fail())	return false;
	for (size_t i = 0; i < TOTAL_BLOCKS; i++)	FATTABLE[i] = (buffer[i / 8] >> (7 - (i % 8))) & 1;
	return true;
}
int System::saveBitMap(std::fstream &disk){
	std::unique_lock<std::shared_mutex> lock(FATMutex);
	std::vector<char> buffer(TOTAL_BLOCKS / 8, 0);
	for (int i = 0; i < TOTAL_BLOCKS; i++){
		buffer[i / 8] |= FATTABLE[i] << (7 - (i % 8));
	}

	disk.seekp(BITMAP_START * BLOCK_SIZE, std::ios::beg);
	disk.write(buffer.data(), buffer.size());
	if (disk.fail()){
		std::cerr << "\tError: Cannot save bitmap to disk.\n";
		disk.clear();
		return 0;
	}
	disk.flush();
	// loadBitMap(disk);
	std::cout << "\tSuccessfully saved bitmap to disk.\n";
	return 1;
}
std::vector<int> System::allocateBitMapBlocks(std::fstream &disk, int numBlocks, ClientSession* session){
	std::unique_lock<std::shared_mutex> lock(FATMutex);
	std::vector<int> allocatedBlocks;
	for (size_t i = DATA_START; i < TOTAL_BLOCKS; i++){
		if (!FATTABLE[i]){
			allocatedBlocks.push_back(i);
			if (static_cast<int>(allocatedBlocks.size()) == numBlocks){
				for (const int block : allocatedBlocks)	FATTABLE[block] = true;
				// std::cerr << "\tAllocated all requested blocks.\n";
				lock.unlock();
				int save = saveBitMap(disk);
				lock.lock();
				if (save == 0){
					for (const int block : allocatedBlocks)	FATTABLE[block] = false;
					return {};
				}
				const std::vector<char> buffer(BLOCK_SIZE, 0);
				for (const int block : allocatedBlocks) {
					disk.seekp(block * BLOCK_SIZE, std::ios::beg);
					disk.write(buffer.data(), BLOCK_SIZE);
				}
				return allocatedBlocks;
			}
		}
	}
	if (static_cast<int>(allocatedBlocks.size()) != numBlocks){
		for (const int block : allocatedBlocks)	FATTABLE[block] = false;
		std::string msg("Error: Cannot allocate all requested blocks.\n");
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());;
		return {};
	}
	return {};
}
void System::freeBitMapBlocks(std::fstream &disk, const std::vector<int> &blocks){
	{
		std::unique_lock<std::shared_mutex> lock(FATMutex);
		if (blocks.empty())	return;
		for (const int block : blocks){
			if (block >= 0 && block < static_cast<int>(FATTABLE.size()))	FATTABLE[block] = false;
		}
	}
	saveBitMap(disk);
	// std::cout << "\tBitmap blocks freed.\n";
}

// Directory Table
// std::vector<FileEntry*> metaDataTable;
// MetadataManager Entries = MetadataManager(ORDER);

bool System::loadDirectoryTable(std::fstream &disk){
	disk.clear();
	std::unique_lock<std::shared_mutex> lock_meta(metaMutex);
	std::unique_lock<std::shared_mutex> lock_dir(dirEntryMutex);
	std::unique_lock<std::shared_mutex> lock_metaIndex(metaIndexMutex);
	for (int i = 0; i < ROOT_DIR_BLOCKS; i++) {
		char buffer[BLOCK_SIZE];
		disk.seekg((ROOT_DIR_START + i) * BLOCK_SIZE, std::ios::beg);
		disk.read(buffer, BLOCK_SIZE);
		if (disk.fail())	return false;
		for (int j = 0; j < ORDER - 1; j++) {
			SerializableFileEntry entry;
			size_t offset = j * sizeof(SerializableFileEntry);
			if (offset + sizeof(SerializableFileEntry) <= BLOCK_SIZE) {
				memcpy(&entry, buffer + offset, sizeof(SerializableFileEntry));
				FileEntry* toBeSaved = new FileEntry(entry);
				if (toBeSaved->fileName[0] != '\0') {
					metaDataTable.push_back(toBeSaved);
					int index = Entries->getFile(toBeSaved->fileName);
					if (index != -1 && index != static_cast<int>(metaDataTable.size()) - 1) {
						Entries->updateIdx(toBeSaved->fileName, metaDataTable.size() - 1);
					}
					metaIndex++;
					if (toBeSaved->isDirectory)	availableDirEntry++;
				}
			}
		}
	}
	return true;
}
int System::saveDirectoryTable(std::fstream &disk, int index, ClientSession* session){
	if (index < 0 || index >= static_cast<int>(metaDataTable.size())) {
		// std::cerr << "\tError: Index out of bounds while saving FileEntry/rootDirectory to disk.\n";
		std::string msg("Error: Index out of bounds while saving FileEntry/rootDirectory to disk.\n");
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
		return 0;
	}
	std::unique_lock<std::shared_mutex> lock(metaMutex);
	const int blocksPassed = index / (ORDER - 1); // Since each block record stores only ORDER entries
	const int blockToModify = index % (ORDER - 1);
	auto entryToSave = SerializableFileEntry(*metaDataTable[index]);
	disk.seekp((ROOT_DIR_START + blocksPassed) * BLOCK_SIZE + (blockToModify * sizeof(FileEntry)), std::ios::beg);
	disk.write(reinterpret_cast<char*>(&entryToSave), sizeof(SerializableFileEntry));
	if (disk.fail()){
		std::string msg("Error: Failed to save root directory to disk.\n");
		session->msg.insert(session->msg.end(), msg.begin(), msg.end());
        // std::cerr << "\tError: Failed to save root directory to disk.\n";
		disk.clear();
		return 0;
    }
	disk.flush();
	return 1;
}
int System::saveDirectoryTableEntire(std::fstream &disk){
	std::unique_lock<std::shared_mutex> lock(metaMutex);
	int entryIndex = 0;
	const int totalEntries = static_cast<int>(metaDataTable.size());
	for (int i = 0; i < ROOT_DIR_BLOCKS && entryIndex < totalEntries; i++) {
		char buffer[BLOCK_SIZE] = {0};

		for (int j = 0; j < (ORDER - 1) && entryIndex < totalEntries; j++) {
			size_t offset = j * sizeof(SerializableFileEntry);
			if (entryIndex < totalEntries && (offset + sizeof(SerializableFileEntry) <= BLOCK_SIZE)){
				SerializableFileEntry toSave = SerializableFileEntry(*metaDataTable[entryIndex]);
				memcpy(buffer + offset, &toSave, sizeof(SerializableFileEntry));
				entryIndex++;
			}
		}

		disk.seekp((ROOT_DIR_START + i) * BLOCK_SIZE, std::ios::beg);
		disk.write(buffer, BLOCK_SIZE);
		if (disk.fail()){
			std::cerr << "\tError: Failed to save root directory to disk.\n";
			disk.clear();
			return 0;
		}
	}
	disk.flush();
	return 1;
}

// Superblock
// Superblock superblock;

bool System::loadSuperblock(std::fstream &disk){
	std::unique_lock<std::shared_mutex> lock(superblockMutex);
	disk.seekg(SUPER_BLOCK_START * BLOCK_SIZE, std::ios::beg);
	disk.read(reinterpret_cast<char*>(&superblock), sizeof(Superblock));
	return true;
}
int System::saveSuperblock(std::fstream &disk){
	std::unique_lock<std::shared_mutex> lock(superblockMutex);
	disk.seekp(SUPER_BLOCK_START * BLOCK_SIZE, std::ios::beg);
	disk.write(reinterpret_cast<char*>(&superblock), sizeof(Superblock));
	if (disk.fail()){
		std::cerr << "\tError: Failed to save superblock to disk.\n";
		disk.clear();
		return 0;
	}
	disk.flush();
	return 1;
}

// Users
int System::saveUsers(std::fstream& disk) {
	std::unique_lock<std::shared_mutex> lock(userDataMutex);
	disk.seekp(SUPER_BLOCK_START + sizeof(Superblock), std::ios::beg);
	disk.write(reinterpret_cast<char*>(&totalUsers), sizeof(int));
	for (int i = 0; i < totalUsers; i++){
		if (!userDatabase[i])	continue;
		User userTS = *userDatabase[i];
		disk.seekp(SUPER_BLOCK_START + sizeof(Superblock) + sizeof(int) + (sizeof(User) * i), std::ios::beg);
		disk.write(reinterpret_cast<char*>(&userTS), sizeof(User));
		if (disk.fail()){
			std::cerr << "\tError: Failed to save user details at index " << i << ".\n";
			disk.clear();
			return 0;
		}
	}
	disk.flush();
	// std::cout << "Successfully saved user information.\n";
	return 1;
}

bool System::loadUsers(std::fstream& disk) {
	std::shared_lock<std::shared_mutex> lock_userData(userDataMutex);
	std::shared_lock<std::shared_mutex> lock_userTable(userTableMutex);
	std::shared_lock<std::shared_mutex> lock_groupTable(groupTableMutex);
	std::shared_lock<std::shared_mutex> lock_userCount(userCountMutex);
	std::shared_lock<std::shared_mutex> lock_groupCount(groupCountMutex);
	userTable.clear();
	userDatabase.clear();
	groupTable.clear();
	totalUsers = 0;
	disk.clear();
	disk.seekg(SUPER_BLOCK_START + sizeof(Superblock), std::ios::beg);
	disk.read(reinterpret_cast<char*>(&totalUsers), sizeof(int));
	if (disk.fail()) {
		std::cerr << "Error: Failed to read total users.\n";
		disk.clear();
		return false;
	}
	for (int i = 0; i < totalUsers; i++) {
		User* userRet = new User();
		disk.seekg(SUPER_BLOCK_START + sizeof(Superblock) + sizeof(int) + sizeof(User) * i, std::ios::beg);
		disk.read(reinterpret_cast<char*>(userRet), sizeof(User));
		userDatabase.push_back(userRet);
		std::string name(userRet->userName);
		userTable[userRet->user_id] = name;
		if (groupTable.find(userRet->group_id) == groupTable.end()) {
			groupTable[userRet->group_id] = "GROUP" + std::to_string(userRet->group_id);
			totalGroups++;
		}
		// if (strncmp(userRet->userName, "root", USER_NAME_LENGTH) == 0 && userRet->password == 0) {
		// 	session->user = *userRet;
		// }
	}
	return true;
}
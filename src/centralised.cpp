#include "centralised.h"

// BITMAP FAT Table
// std::vector<bool> FATTABLE(TOTAL_BLOCKS, false);

void System::loadBitMap(std::fstream &disk){
	std::vector<char> buffer(TOTAL_BLOCKS / 8, 0);
	disk.seekg(BITMAP_START * BLOCK_SIZE, std::ios::beg);
	disk.read(buffer.data(), buffer.size());
	if (disk.fail())	return;
	for (size_t i = 0; i < TOTAL_BLOCKS; i++)	FATTABLE[i] = (buffer[i / 8] >> (7 - (i % 8))) & 1;
}
int System::saveBitMap(std::fstream &disk){
	std::vector<char> buffer(TOTAL_BLOCKS / 8, 0);
	for (size_t i = 0; i < TOTAL_BLOCKS; i++){
		buffer[i / 8] |= FATTABLE[i] << (7 - (i % 8));
	}

	disk.seekp(BITMAP_START * BLOCK_SIZE, std::ios::beg);
	disk.write(buffer.data(), buffer.size());
	if (disk.fail()){
		std::cerr << "\tError: Cannot save bitmap to disk.\n";
		return 0;
	}
	disk.flush();
	loadBitMap(disk);
	std::cout << "\tSuccessfully saved bitmap to disk.\n";
	return 1;
}
std::vector<int> System::allocateBitMapBlocks(std::fstream &disk, int numBlocks){
	std::vector<int> allocatedBlocks;
	for (size_t i = DATA_START; i < TOTAL_BLOCKS; i++){
		if (!FATTABLE[i]){
			allocatedBlocks.push_back(i);
			if (static_cast<int>(allocatedBlocks.size()) == numBlocks){
				for (int block : allocatedBlocks)	FATTABLE[block] = true;
				std::cerr << "\tAllocated all requested blocks.\n";
				int save = saveBitMap(disk);
				if (save == 0){
					for (int block : allocatedBlocks)	FATTABLE[block] = false;
					return {};
				}
				std::vector<char> buffer(BLOCK_SIZE, 0);
				for (int block : allocatedBlocks) {
					disk.seekp(block * BLOCK_SIZE, std::ios::beg);
					disk.write(buffer.data(), BLOCK_SIZE);
				}
				return allocatedBlocks;
			}
		}
	}
	if (static_cast<int>(allocatedBlocks.size()) != numBlocks){
		for (int block : allocatedBlocks)	FATTABLE[block] = false;
		std::cerr << "\tError: Cannot allocate all requested blocks.\n";
		return {};
	}
	return {};
}
void System::freeBitMapBlocks(std::fstream &disk, const std::vector<int> &blocks){
	if (blocks.empty())	return;
	for (int block : blocks){
		if (block >= 0 && block < static_cast<int>(FATTABLE.size()))	FATTABLE[block] = 0;
	}
	saveBitMap(disk);
	std::cout << "\tBitmap blocks freed.\n";
}

// Directory Table
// std::vector<FileEntry*> metaDataTable;
// MetadataManager Entries = MetadataManager(ORDER);

void System::loadDirectoryTable(std::fstream &disk){
	disk.clear();
	for (int i = 0; i < ROOT_DIR_BLOCKS; i++) {
		char buffer[BLOCK_SIZE];
		disk.seekg((ROOT_DIR_START + i) * BLOCK_SIZE, std::ios::beg);
		disk.read(buffer, BLOCK_SIZE);
		if (disk.fail())	return;
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
				}
			}
		}
	}
}
int System::saveDirectoryTable(std::fstream &disk, int index){
	if (index < 0 || index >= static_cast<int>(metaDataTable.size())) {
		std::cerr << "\tError: Index out of bounds while saving FileEntry/rootDirectory to disk.\n";
		return 0;
	}
	int blocksPassed = index / (ORDER - 1); // Since each block record stores only ORDER entries
	int blockToModify = index % (ORDER - 1);
	SerializableFileEntry entryToSave = SerializableFileEntry(*metaDataTable[index]);
	disk.seekp((ROOT_DIR_START + blocksPassed) * BLOCK_SIZE + (blockToModify * sizeof(FileEntry)), std::ios::beg);
	disk.write(reinterpret_cast<char*>(&entryToSave), sizeof(SerializableFileEntry));
	if (disk.fail()){
        std::cerr << "\tError: Failed to save root directory to disk.\n";
		disk.clear();
		return 0;
    }
	disk.flush();
	return 1;
}
int System::saveDirectoryTableEntire(std::fstream &disk){
	int entryIndex = 0;
	int totalEntries = metaDataTable.size();
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

void System::loadSuperblock(std::fstream &disk){
	disk.seekg(SUPER_BLOCK_START * BLOCK_SIZE, std::ios::beg);
	disk.read(reinterpret_cast<char*>(&superblock), sizeof(Superblock));
}
int System::saveSuperblock(std::fstream &disk){
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
	disk.seekp(SUPER_BLOCK_START + sizeof(Superblock), std::ios::beg);
	disk.write(reinterpret_cast<char*>(&totalUsers), sizeof(int));
	for (int i = 0; i < totalUsers; i++){
		if (!userDatabase[i])	continue;
		User user = *userDatabase[i];
		disk.seekp(SUPER_BLOCK_START + sizeof(Superblock) + sizeof(int) + (sizeof(User) * i), std::ios::beg);
		disk.write(reinterpret_cast<char*>(&user), sizeof(User));
		if (disk.fail()){
			std::cerr << "\tError: Failed to save user details at index " << i << ".\n";
			disk.clear();
			return 0;
		}
	}
	disk.flush();
	std::cout << "Successfully saved user information.\n";
	return 1;
}

void System::loadUsers(std::fstream& disk) {
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
		return;
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
		if (strncmp(userRet->userName, "root", USER_NAME_LENGTH) == 0 && userRet->password == 0) {
			user = *userRet;
		}
	}
}
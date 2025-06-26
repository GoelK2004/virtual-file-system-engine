#include "disk.h"

bool initialiseDisk(const std::string& diskName){
	std::ofstream disk(diskName, std::ios::binary);
	disk.clear();
	if (!disk){
		std::cerr << "Error: Cannot create the disk file.\n";
		return false;
	}
	
	std::vector<char> emptyBlock(DISK_SIZE, 0);
	disk.write(reinterpret_cast<char*>(emptyBlock.data()), static_cast<int>(emptyBlock.size()));
	if (!disk.good()){
		std::cerr << "Error: Cannot create the disk file.\n";
		return false;
	}
	disk.flush();
	std::cout << "Disk successfully created.\n";
	disk.close();

	return true;
}

bool initialiseSuperblock(System& fs) {
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for superblock initialisation.\n";
		return false;
	}
	Superblock* superblock = new Superblock();
	{
		std::unique_lock<std::shared_mutex> lock(fs.superblockMutex);
		superblock->blockSize = BLOCK_SIZE;
		superblock->totalBlocks = TOTAL_BLOCKS;
		superblock->freeBlocks = TOTAL_BLOCKS - (SUPER_BLOCKS + BITMAP_BLOCKS + ROOT_DIR_BLOCKS + BPLUS_TREE_BLOCKS);
		superblock->fatStart = BITMAP_START * BLOCK_SIZE;
		superblock->dataStart = DATA_START * BLOCK_SIZE;
	}
	disk.seekp(SUPER_BLOCK_START * BLOCK_SIZE, std::ios::beg);
	disk.write(reinterpret_cast<char*>(superblock), sizeof(Superblock));
	fs.loadSuperblock(disk);
	if (disk.fail()){
		std::cout << "Error: Cannot load superblock into memory.\n";
		disk.clear();
		return false;
	}
	std::cout << "Superblock initialised and stored on disk.\n";
	std::cout << "Successfully loaded superblock in memory.\n";
	
	delete superblock;
	disk.close();
	return true;
}

bool initialiseFAT(System& fs){
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for FAT table initialisation.\n";
		return false;
	}

	std::vector<char> buffer(TOTAL_BLOCKS / 8, 0);

	for (int i = 0; i < DATA_START; i++){
		buffer[i / 8] |= (1 << (7 - (i % 8)));
	}
	disk.seekp(BITMAP_START * BLOCK_SIZE, std::ios::beg);
	disk.write(buffer.data(), buffer.size());
	if (disk.bad()){
		std::cerr << "Error: Cannot write bitmap to disk.\n";
		return false;
	}
	disk.flush();
	std::cout << "Bitmap initialised and stored in the disk.\n";
	fs.loadBitMap(disk);
	if (disk.fail()){
		std::cout << "Error: Cannot load bitmap into memory.\n";
		disk.clear();
		return false;
	}
	std::cout << "Successfully loaded bitmap in memory.\n";
	disk.close();

	return true;
}

bool initialiseFileEntries(System& fs){
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for file entry initialisation.\n";
	}
	
	std::vector<SerializableFileEntry> rootDirectory(ORDER - 1);
	for (int i = 0; i < ORDER - 1; i++){
		rootDirectory[i] = SerializableFileEntry();
	}
	
	for (int i = 0; i < ROOT_DIR_BLOCKS; i++) {
		disk.seekg((ROOT_DIR_START + i) * BLOCK_SIZE, std::ios::beg);
		disk.write(reinterpret_cast<char*>(rootDirectory.data()), sizeof(rootDirectory));
		if (disk.bad()){
			std::cerr << "Error: Cannot write file entries to disk.\n";
		}
	}
	disk.flush();
	
	std::cout << "File entries initialised and stored in the disk.\n";
	{
		std::unique_lock<std::shared_mutex> lock(fs.metaMutex);
		fs.metaDataTable.reserve(MAX_FILES);
	}	
	fs.loadDirectoryTable(disk);
	if (disk.fail()){
		std::cerr << "Error: Cannot load directory entries into memory.\n";
		disk.clear();
		return false;
	}
	std::cout << "Successfully loaded directory entries into memory.\n";
	disk.close();

	return true;
}

bool initialiseUsers(System& fs) {
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for file entry initialisation.\n";
		return false;
	}

	fs.userTable.clear();
	fs.userDatabase.clear();
	fs.groupTable.clear();

	++fs.totalUsers;
	fs.userTable[0] = "root";
	fs.groupTable[0] = "root";
	
	User* userScoped = new User();
	userScoped->user_id = 0;
	userScoped->group_id = 0;
	strncpy(userScoped->userName, "root", USER_NAME_LENGTH);
	userScoped->userName[USER_NAME_LENGTH - 1] = '\0';
	userScoped->password = 0;
	fs.userDatabase.push_back(userScoped);

	// fs.user = *userScoped;
	return true;
}

bool System::formatFileSystem() {
	std::cout << "Starting File System...\n";

	bool check = true;
	const std::string diskPath = DISK_PATH;
	check = initialiseDisk(diskPath);
	if (!check)	return false;
	check = initialiseSuperblock(*this);
	if (!check)	return false;
	check = initialiseFAT(*this);
	if (!check)	return false;
	check = initialiseFileEntries(*this);
	if (!check)	return false;
	check = initialiseUsers(*this);
	if (!check)	return false;
	journalManager->loadJournal();
	
	std::cout << "File system formatting completed successfully.\n";
    std::cout << "Disk layout:\n";
    std::cout << "  Superblock Start  : Block " << SUPER_BLOCK_START << '\n';
    std::cout << "  Bitmap Start      : Block " << BITMAP_START << '\n';
    std::cout << "  B+ Tree Start     : Block " << BPLUS_TREE_START << '\n';
    std::cout << "  Root Directory    : Block " << ROOT_DIR_START << '\n';
    std::cout << "  Data Blocks Start : Block " << DATA_START << '\n';
	{
		std::shared_lock<std::shared_mutex> lock(superblockMutex);
    	std::cout << "  Free Blocks       : " << superblock.freeBlocks << " / " << superblock.totalBlocks << '\n';
	}
	return true;
}

bool System::loadFromDisk() {
	std::cout << "Starting File System...\n";
	bool check = true;
	
	std::fstream disk(DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	check = loadSuperblock(disk);
	if (!check)	return false;
	check = loadBitMap(disk);
	if (!check)	return false;
	if (!Entries->loadBPlusTree(disk)) {
		std::cerr << "Error: Cannot load B+ Tree.\n";
		exit(EXIT_FAILURE);
	}
	check = loadDirectoryTable(disk);
	if (!check)	return false;
	check = loadUsers(disk);
	if (!check)	return false;
	journalManager->loadJournal();
	
	std::cout << "File system loading completed successfully.\n";
    std::cout << "Disk layout:\n";
    std::cout << "  Superblock Start  : Block " << SUPER_BLOCK_START << '\n';
    std::cout << "  Bitmap Start      : Block " << BITMAP_START << '\n';
    std::cout << "  B+ Tree Start     : Block " << BPLUS_TREE_START << '\n';
    std::cout << "  Meta Data Table   : Block " << ROOT_DIR_START << '\n';
    std::cout << "  Data Blocks Start : Block " << DATA_START << '\n';
    std::cout << "  Free Blocks       : " << superblock.freeBlocks << " / " << superblock.totalBlocks << '\n';

	return true;
}

void System::saveInDisk() {
	std::fstream disk(DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	std::cout << "Saving File system state.\n";
    std::cout << "Disk layout:\n";
    std::cout << "  Superblock Start  : Block " << SUPER_BLOCK_START << '\n';
    std::cout << "  Bitmap Start      : Block " << BITMAP_START << '\n';
    std::cout << "  B+ Tree Start     : Block " << BPLUS_TREE_START << '\n';
    std::cout << "  Meta Data Table   : Block " << ROOT_DIR_START << '\n';
    std::cout << "  Data Blocks Start : Block " << DATA_START << '\n';
    std::cout << "  Free Blocks       : " << superblock.freeBlocks << " / " << superblock.totalBlocks << '\n';
	saveBitMap(disk);
	saveSuperblock(disk);
	Entries->saveBPlusTree(disk);
	saveDirectoryTableEntire(disk);	
	saveUsers(disk);
}
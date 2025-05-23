#include "disk.h"

void initialiseDisk(const std::string& diskName){
	std::ofstream disk(diskName, std::ios::binary);
	disk.clear();
	if (!disk){
		std::cerr << "Error: Cannot create the disk file.\n";
		return;
	}
	
	std::vector<char> emptyBlock(DISK_SIZE, 0);
	disk.write(reinterpret_cast<char*>(emptyBlock.data()), emptyBlock.size());
	if (!disk.good()){
		std::cerr << "Error: Cannot create the disk file.\n";
		return;
	}
	disk.flush();
	std::cout << "Disk successfully created.\n";
	disk.close();
}

void initialiseSuperblock(System& fs) {
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for superblock initialisation.\n";
		return;
	}

	Superblock* superblock = new Superblock();
	superblock->blockSize = BLOCK_SIZE;
	superblock->totalBlocks = TOTAL_BLOCKS;
	superblock->freeBlocks = TOTAL_BLOCKS - (SUPER_BLOCKS + BITMAP_BLOCKS + ROOT_DIR_BLOCKS + BPLUS_TREE_BLOCKS);
	superblock->fatStart = BITMAP_START * BLOCK_SIZE;
	superblock->dataStart = DATA_START * BLOCK_SIZE;
	
	disk.seekp(SUPER_BLOCK_START * BLOCK_SIZE, std::ios::beg);
	disk.write(reinterpret_cast<char*>(superblock), sizeof(Superblock));
	fs.loadSuperblock(disk);
	if (disk.fail()){
		std::cout << "Error: Cannot load superblock into memory.\n";
		disk.clear();
		return;
	}
	std::cout << "Superblock initialised and stored on disk.\n";
	std::cout << "Successfully loaded superblock in memory.\n";
	
	delete superblock;
	disk.close();
}

void initialiseFAT(System& fs){
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for FAT table initialisation.\n";
		return;
	}

	std::vector<char> buffer(TOTAL_BLOCKS / 8, 0);

	for (int i = 0; i < DATA_START; i++){
		buffer[i / 8] |= (1 << (7 - (i % 8)));
	}
	disk.seekp(BITMAP_START * BLOCK_SIZE, std::ios::beg);
	disk.write(buffer.data(), buffer.size());
	if (disk.bad()){
		std::cerr << "Error: Cannot write bitmap to disk.\n";
		return;
	}
	disk.flush();
	std::cout << "Bitmap initialised and stored in the disk.\n";
	fs.loadBitMap(disk);
	if (disk.fail()){
		std::cout << "Error: Cannot load bitmap into memory.\n";
		disk.clear();
		return;
	}
	std::cout << "Successfully loaded bitmap in memory.\n";
	disk.close();
}

void initialiseFileEntries(System& fs){
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
	fs.metaDataTable.reserve(MAX_FILES);
	fs.loadDirectoryTable(disk);
	if (disk.fail()){
		std::cerr << "Error: Cannot load directory entries into memory.\n";
		disk.clear();
		return;
	}
	std::cout << "Successfully loaded directory entries into memory.\n";
	disk.close();
}

void initialiseUsers(System& fs) {
	std::fstream disk(fs.DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	if (!disk){
		std::cerr << "Error: Cannot open disk for file entry initialisation.\n";
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

	fs.user = *userScoped;
}

void System::formatFileSystem() {
	std::cout << "Starting File System...\n";

	const std::string diskPath = DISK_PATH;
	initialiseDisk(diskPath);
	initialiseSuperblock(*this);
	initialiseFAT(*this);
	initialiseFileEntries(*this);
	initialiseUsers(*this);
	journalManager->loadJournal();
	
	std::cout << "File system formatting completed successfully.\n";
    std::cout << "Disk layout:\n";
    std::cout << "  Superblock Start  : Block " << SUPER_BLOCK_START << '\n';
    std::cout << "  Bitmap Start      : Block " << BITMAP_START << '\n';
    std::cout << "  B+ Tree Start     : Block " << BPLUS_TREE_START << '\n';
    std::cout << "  Root Directory    : Block " << ROOT_DIR_START << '\n';
    std::cout << "  Data Blocks Start : Block " << DATA_START << '\n';
    std::cout << "  Free Blocks       : " << superblock.freeBlocks << " / " << superblock.totalBlocks << '\n';
}

void System::loadFromDisk() {
	std::cout << "Starting File System...\n";
	
	std::fstream disk(DISK_PATH, std::ios::in | std::ios::out | std::ios::binary);
	loadSuperblock(disk);
	loadBitMap(disk);
	if (!Entries->loadBPlusTree(disk)) {
		std::cerr << "Error: Cannot load B+ Tree.\n";
		exit(0);
	}
	loadDirectoryTable(disk);
	loadUsers(disk);
	journalManager->loadJournal();
	journalManager->recoverUncommitedOperations();
	
	std::cout << "File system loading completed successfully.\n";
    std::cout << "Disk layout:\n";
    std::cout << "  Superblock Start  : Block " << SUPER_BLOCK_START << '\n';
    std::cout << "  Bitmap Start      : Block " << BITMAP_START << '\n';
    std::cout << "  B+ Tree Start     : Block " << BPLUS_TREE_START << '\n';
    std::cout << "  Meta Data Table   : Block " << ROOT_DIR_START << '\n';
    std::cout << "  Data Blocks Start : Block " << DATA_START << '\n';
    std::cout << "  Free Blocks       : " << superblock.freeBlocks << " / " << superblock.totalBlocks << '\n';
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
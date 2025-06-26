#include "rollback.h"

void System::rollbackMetadataIndex(std::fstream &disk, Superblock &originalSuperblock, int orgIndex, std::vector<int> &newlyAllocatedBlocks){
	disk.clear();
	// std::cout << "\tBefore: \n";
	std::cout << superblock.freeBlocks << '\n';
	// if (orgIndex != -1)	std::cout << metaDataTable[orgIndex]->fileName << '\n';
	{
		std::unique_lock<std::shared_mutex> lock(metaMutex);
		if (orgIndex != -1)	metaDataTable[orgIndex] = new FileEntry();
	}
	superblock = originalSuperblock;
	std::cout << superblock.freeBlocks << '\n';
	// if (orgIndex != -1)	std::cout << metaDataTable[orgIndex]->fileName << '\n';
	freeBitMapBlocks(disk, newlyAllocatedBlocks);
	// std::cout << "\t\tRollback completed successfully. File system state restored.\n";
}

void System::rollbackMetadataOrg(std::fstream &disk, Superblock &originalSuperblock, FileEntry* orgFileEntry, int orgIndex, std::vector<int> &newlyAllocatedBlocks){
	disk.clear();
	// std::cout << "\tBefore: \n";
	std::cout << superblock.freeBlocks << '\n';
	// if (orgIndex != -1)	std::cout << metaDataTable[orgIndex]->fileName << '\n';
	{
		std::unique_lock<std::shared_mutex> lock(metaMutex);
		if (orgIndex != -1)	metaDataTable[orgIndex] = new FileEntry();
		FileEntry* entryToDelete = metaDataTable[orgIndex];
		metaDataTable[orgIndex] = orgFileEntry;
		if (entryToDelete)	delete entryToDelete;
	}
	
	superblock = originalSuperblock;
	
	std::cout << superblock.freeBlocks << '\n';
	// if (orgIndex != -1)	std::cout << metaDataTable[orgIndex]->fileName << '\n';
	freeBitMapBlocks(disk, newlyAllocatedBlocks);
	// std::cout << "\t\tRollback completed successfully. File system state restored.\n";
}

#include "filesystem.h"

// namespace fileSystemOperations {
void createFile(System& fs, ClientSession* session, std::fstream &disk, const std::string &fileName, const int &fileSize, FileEntry* newFile, const int& index, uint16_t permissions) {
	// std::cout << "Creating file: '" << fileName << "':\n";
	
	if (!helpers::isValidFileName(fileName)){
		session->oss << "Error: File name not valid (Length must be less than 32, and no special characters except '.' and '_')\n";
		// std::cerr << "\tError: File name not valid (Length must be less than 32, and no special characters except '.' and '_')\n";
		return;
	}
	std::string savedName = std::to_string(session->user.user_id) + std::to_string(index) + "F_" + fileName;
	int requiredBlocks = (fileSize + BLOCK_SIZE - 1)/BLOCK_SIZE;
	std::vector<int> allocatedBlocks = fs.allocateBitMapBlocks(disk, requiredBlocks, session);
	if (allocatedBlocks.empty()){
		session->oss << "Error: Try creating file again.\n";
		// std::cerr << "\tError: Try creating file again.\n";
		return;
	}
	int size = static_cast<int>(allocatedBlocks.size());
	Superblock originalSuperblock = fs.superblock;
	int extentIndex = 0;
	for (int i = 0; i < size; i++){
		if (extentIndex >= MAX_EXTENTS){
			session->oss << "Error: Exceeded max extents while creating file.\n";
			// std::cerr << "\tError: Exceeded max extents while creating file.\n";
			// std::cerr << "\tAttempting rollback\n";
			fs.rollbackMetadataIndex(disk, fs.superblock, fs.metaIndex - 1, allocatedBlocks);
			return;
		}
		int startBlock = allocatedBlocks[i];
		int length = 1;
		while ((i + 1) < size || allocatedBlocks[i + 1] == allocatedBlocks[i] + 1){
			length++;
			i++;
		}
		newFile->extents[extentIndex].length = length;
		newFile->extents[extentIndex].startBlock = startBlock;
		newFile->numExtents++;
		extentIndex++;
	}
	newFile->fileSize = fileSize;
	newFile->isDirectory = false;
	newFile->parentIndex = index;
	newFile->dirID = index;
	uint64_t current_time = static_cast<int>(std::time(nullptr));
	newFile->created_at = current_time;
	newFile->modified_at = current_time;
	newFile->accessed_at = current_time;
	newFile->owner_id = session->user.user_id;
	newFile->group_id = session->user.group_id;
	newFile->permissions = permissions;
	newFile->attributes = 0;
	
	fs.superblock.freeBlocks -= requiredBlocks;
	FileEntry* parentDir = nullptr;
	{
		std::shared_lock<std::shared_mutex> lock(fs.metaMutex);
		if (newFile->parentIndex != 0){
			for (auto& entry : fs.metaDataTable) {
				if (entry->dirID == newFile->parentIndex && entry->isDirectory){
					parentDir = entry;
					break;
				}
			}
			if (!parentDir) {
				session->oss << "Error: No parent directory found.\n";
				// std::cerr << "Error: No parent directory found.\n";
				return;
			}
			parentDir->fileSize += newFile->fileSize;
		}
	}
	session->user.totalSize += newFile->fileSize;
	
	fs.Entries->insertFileEntry(savedName, fs.metaIndex);
	// int key = fs.Entries->insertFileEntry(savedName, fs.metaIndex);
	fs.metaDataTable.push_back(newFile);
	fs.metaIndex++;
	
	int save = fs.saveDirectoryTable(disk, fs.metaIndex-1, session);
	if (save == 0){
		session->oss << "Error: Corrupted file entry(Cannot update file entry) with index: " << fs.metaIndex - 1 << ".\n";
		// std::cout << "\tError: Corrupted file entry(Cannot update file entry) with index: " << fs.metaIndex - 1 << ".\n";
		// std::cout << "\tAttempting rollback\n";
		fs.rollbackMetadataIndex(disk, originalSuperblock, fs.metaIndex - 1, allocatedBlocks);
		return;
	}
	save = fs.saveSuperblock(disk);
	if (save == 0){
		session->oss << "Error: Cannot update Superblock after creating file.\n";
		// std::cerr << "\tError: Cannot update Superblock after creating file.\n";
		// std::cout << "\tAttempting rollback\n";
		fs.rollbackMetadataIndex(disk, originalSuperblock, fs.metaIndex - 1, allocatedBlocks);
		fs.Entries->removeFileEntry(savedName);
		return;
	}
	disk.flush();
	// std::cout << "\tFile '" << fileName << "' created successfully, at block: " << newFile->extents->startBlock << " and key: " << key << '\n';
}
void writeFileData(System& fs, ClientSession* session, std::fstream &disk, FileEntry* file, const int fileIndex, const std::string &fileContent, bool append){
	// std::cout << "Writing content to file '" << file->fileName << "'.\n";
	std::vector<int> newlyAllocatedBlocks;
	Superblock originalSuperBlock = fs.superblock;
	FileEntry* orgFileEntry;
	{
		std::shared_lock<std::shared_mutex> lock(fs.metaMutex);
		orgFileEntry = fs.metaDataTable[fileIndex];
	}
	int reqBlocksUpdate = 0;
	if (append){
		int bytesWritten = file->fileSize % BLOCK_SIZE;
		int remaining = (bytesWritten == 0) ? 0 : BLOCK_SIZE - bytesWritten;
		int actualBytesWritten = 0;
		int requiredBlocks = (static_cast<int>(fileContent.size()) - remaining + BLOCK_SIZE - 1)/BLOCK_SIZE;
		if (fs.superblock.freeBlocks < requiredBlocks){
			session->oss << "Error: Not enough storage to append data.\n";
			// std::cout << "\tError: Not enough storage to append data.\n";
			return;
		}
		reqBlocksUpdate = requiredBlocks;
		if (remaining < BLOCK_SIZE) {
			int block = file->extents[file->numExtents - 1].startBlock + file->extents[file->numExtents - 1].length - 1;
			int writeSize = std::min(remaining, static_cast<int>(fileContent.size()));
			disk.seekp(block * BLOCK_SIZE + bytesWritten, std::ios::beg);
			disk.write(fileContent.data(), writeSize);
			if (disk.fail()){
				session->oss << "Error: Cannot append data(first block).\n";
				// std::cout << "\tError: Cannot append data(first block).\n";
				disk.clear();
				std::vector<char> emptyBlock(0, writeSize);
				disk.seekp(block * BLOCK_SIZE + bytesWritten, std::ios::beg);
				disk.write(emptyBlock.data(), writeSize);
				return;
			}
			actualBytesWritten += writeSize;
		}
		if (actualBytesWritten != static_cast<int>(fileContent.size())) {
			std::vector<int> newlyAllocatedBlocks = fs.allocateBitMapBlocks(disk, requiredBlocks, session);
			int size = static_cast<int>(newlyAllocatedBlocks.size()), extentIndex = file->numExtents;
			for (int i = 0; i < size; i++){
				if (extentIndex > MAX_EXTENTS){
					session->oss << "Error: Exceeded max extents while creating file.\n";
					// std::cerr << "\tError: Exceeded max extents while creating file.\n";
					// std::cerr << "\tAttempting rollback\n";
					fs.rollbackMetadataIndex(disk, fs.superblock, -1, newlyAllocatedBlocks); // No FileEntry modification until
					// RollingBack already written data
					int block = orgFileEntry->extents[orgFileEntry->numExtents - 1].startBlock + orgFileEntry->extents[orgFileEntry->numExtents - 1].length - 1;
					int writeSize = std::min(remaining, static_cast<int>(fileContent.size()));
					std::vector<char> emptyBlock(0, writeSize);
					disk.seekp(block * BLOCK_SIZE + bytesWritten, std::ios::beg);
					disk.write(emptyBlock.data(), writeSize);
					return;
				}
			
				int startBlock = newlyAllocatedBlocks[i];
				int length = 1;
				while ((i + 1) != size || newlyAllocatedBlocks[i + 1] == newlyAllocatedBlocks[i] + 1){
					length++;
					i++;
				}
				file->extents[extentIndex].length = length;
				file->extents[extentIndex].startBlock = startBlock;
				file->numExtents++;
				extentIndex++;
			}
			for (int block : newlyAllocatedBlocks){
				int toWrite = std::min(BLOCK_SIZE, static_cast<int>(fileContent.size() - actualBytesWritten));
				disk.seekp(block * BLOCK_SIZE, std::ios::beg);
				disk.write(fileContent.data() + actualBytesWritten, toWrite);
				if (disk.bad()){
					session->oss << "Error: Failed to write to newly allocated blocks in append mode.\n";
					// std::cerr << "\tError: Failed to write to newly allocated blocks in append mode.\n";
					// std::cerr << "\tAttempting rollback:\n";
					fs.rollbackMetadataOrg(disk, originalSuperBlock, orgFileEntry, fileIndex, newlyAllocatedBlocks);
					return;
				}
				disk.flush();
				actualBytesWritten += toWrite;
			}
		}
	}
	else{
		int requiredBlocks = (fileContent.size() + BLOCK_SIZE - 1)/BLOCK_SIZE;
		int allocatedBlocks = (file->fileSize + BLOCK_SIZE - 1)/BLOCK_SIZE;
		if (requiredBlocks > allocatedBlocks){
			int difference = requiredBlocks - allocatedBlocks;
			if (fs.superblock.freeBlocks < difference){
				session->oss << "Error: Not enough storage for additional data.\n";
				// std::cerr << "\tError: Not enough storage for additional data.\n";
				return;
			}
			reqBlocksUpdate = difference;
			int block = file->extents[file->numExtents - 1].startBlock + file->extents[file->numExtents - 1].length - 1;
			std::vector<int> newlyAllocatedBlocks = fs.allocateBitMapBlocks(disk, difference, session);
			int i = 0;
			while (newlyAllocatedBlocks[i] == block + 1){
				file->extents[file->numExtents - 1].length += 1;
				i++;
			}
			if (i <= static_cast<int>(newlyAllocatedBlocks.size()) - 1) {
				int size = newlyAllocatedBlocks.size(), extentIndex = file->numExtents;
				for (int i = 0; i < size; i++){
					if (extentIndex > MAX_EXTENTS){
						session->oss << "Error: Exceeded max extents while creating file.\n";
						// std::cerr << "\tError: Exceeded max extents while creating file.\n";
						// std::cerr << "\tAttempting rollback\n";
						fs.rollbackMetadataOrg(disk, fs.superblock, orgFileEntry, fileIndex, newlyAllocatedBlocks);
						return;
					}
					int startBlock = newlyAllocatedBlocks[i];
					int length = 1;
					while ((i + 1) != size || newlyAllocatedBlocks[i + 1] == newlyAllocatedBlocks[i] + 1){
						length++;
						i++;
					}
					file->extents[extentIndex].length = length;
					file->extents[extentIndex].startBlock = startBlock;
					file->numExtents++;
					extentIndex++;
				}
			}
		}
		else if (requiredBlocks < allocatedBlocks){
			int countedBlocks = 0, extentIndex = 0, finalExtentIndex = 0;
			while (countedBlocks < requiredBlocks){
				countedBlocks += file->extents[extentIndex].length; // Count total blocks in each extent
				if (countedBlocks > requiredBlocks){
					int remBlocks = countedBlocks - requiredBlocks; // If the total blocks exceeds the required blocks
					file->extents[extentIndex].length -= remBlocks; // Due to the additional in last added extent blocks
					finalExtentIndex = extentIndex;
					for (int i = remBlocks; i > 0; i--)	newlyAllocatedBlocks.push_back(file->extents[extentIndex].startBlock + file->extents[extentIndex].length - remBlocks + i);
					extentIndex++;
					while (extentIndex < file->numExtents){
						int ln = file->extents[extentIndex].length;
						for (int i = 0; i < ln; i++)	newlyAllocatedBlocks.push_back(file->extents[extentIndex].startBlock + i);
						extentIndex++;
					}
				}
				file->numExtents = finalExtentIndex + 1;
			}
			fs.freeBitMapBlocks(disk, newlyAllocatedBlocks);
			for (int i = file->numExtents; i < MAX_EXTENTS; i++){
				if (file->extents[i].startBlock == -1)	break;
				else{
					file->extents[i].startBlock = -1;
					file->extents[i].length = 0;
				}
			}
		}
		int bytesWritten = 0, extentIndex = 0;
		while (extentIndex < file->numExtents){
			int ln = file->extents[extentIndex].length;
			for (int i = 0; i < ln; i++){
				size_t dataBytes = std::min(BLOCK_SIZE, static_cast<int>(fileContent.size() - bytesWritten));
				disk.seekp((file->extents[extentIndex].startBlock + i) * BLOCK_SIZE, std::ios::beg);
				disk.write(fileContent.data() + bytesWritten, dataBytes);
				if (disk.bad()){
					session->oss << "Error: Failed to write data to disk for file '" << file->fileName << "' at extent: " << extentIndex << ".\n";
					// std::cerr << "\tError: Failed to write data to disk for file '" << file->fileName << "' at extent: " << extentIndex << ".\n";
					return;
				}		
				bytesWritten += dataBytes;
			}
			extentIndex++;
		}
		disk.flush();
	}
	FileEntry* parentDir = nullptr;
	{
		std::shared_lock<std::shared_mutex> lock(fs.metaMutex);
		if (file->parentIndex != 0){
			for (auto& entry : fs.metaDataTable) {
				if (entry->dirID == file->parentIndex && entry->isDirectory && entry->owner_id == session->user.user_id){
					parentDir = entry;
					break;
				}
			}
			if (!parentDir) {
				session->oss << "Error: No parent directory found.\n";
				// std::cerr << "Error: No parent directory found.\n";
				return;
			}
			if (parentDir->fileSize >= file->fileSize)
				parentDir->fileSize -= file->fileSize;
		}
	}
	session->user.totalSize -= file->fileSize;
	file->fileSize = append ? fileContent.size() + file->fileSize : fileContent.size();
	if (file->parentIndex != 0)
		parentDir->fileSize += file->fileSize;
	uint64_t current_time = std::time(nullptr);
	file->modified_at = current_time;
	file->accessed_at = current_time;
	fs.superblock.freeBlocks -= reqBlocksUpdate;
	int save = fs.saveDirectoryTable(disk, fileIndex, session);
	if (save == 0) {
		// std::cerr << "\tAttempting rollback\n";
		fs.rollbackMetadataOrg(disk, originalSuperBlock, orgFileEntry, fileIndex, newlyAllocatedBlocks);
		return;
	}
	save = fs.saveSuperblock(disk);
	if (save == 0){
		// std::cerr << "\tAttempting rollback\n";
		fs.rollbackMetadataOrg(disk, originalSuperBlock, orgFileEntry, fileIndex, newlyAllocatedBlocks);
		return;
	}
	save = fs.saveBitMap(disk);
	if (save == 0){
		// std::cerr << "\tError: Cannot update bitmap. Attempting rollback\n";
		fs.rollbackMetadataOrg(disk, originalSuperBlock, orgFileEntry, fileIndex, newlyAllocatedBlocks);
		return;
	}
	disk.flush();
	// std::cout << "\tData written sucessfully for the file: '" << file->fileName << "'.\n";
	// for (auto& filex : file->extents) {
	// 	std::cout << "\tExtent: " << filex.startBlock << " and length: " << filex.length << '\n';
	// }
}
std::string System::readFileData(std::fstream &disk, FileEntry* file, ClientSession* session){	
	// std::cout << "Reading data from file '" << file->fileName << "'.\n";
	std::string fileContent;
	int extent = 0;
	// std::cout << "File Name: " << file->fileName << '\n';
	// std::cout << "File Size: " << file->fileSize << '\n';
	while (extent < file->numExtents){
		int ln = file->extents[extent].length, bytesToRead = 0, bytesRead = 0;
		// Counting total bytes to read from all extents
		for (int i = 0; i < ln; i++){
			bytesToRead = std::min(BLOCK_SIZE, std::max(0, file->fileSize - bytesRead));
			bytesRead += bytesToRead;
		}
		std::streamsize size = file->extents[extent].length * BLOCK_SIZE;
		std::string buffer(size, '\0');
		// std::vector<char> buffer(bytesToRead, 0);
		disk.seekg(file->extents[extent].startBlock * BLOCK_SIZE, std::ios::beg);
		// disk.read(buffer.data(), bytesToRead);
		disk.read(&buffer[0], bytesRead);
		if (disk.fail()){
			session->oss << "\nError: Cannot read file contents at block: " << file->extents[extent].startBlock << ".\n";
			disk.clear();
			return "";
		}
		// std::cout << "\nReading from block: " << file->extents[extent].startBlock << " with length: " << file->extents[extent].length << ":\n";
		// std::cout << buffer;
		// for (char ch : buffer){
		// 	std::cout << ch;
		// }
		fileContent.append(buffer.begin(), buffer.end());
		if (bytesRead >= file->fileSize)	break;
		extent++;
	}
	// std::cout << "\n\tFile contents read successfully.\n";
	fileContent.insert(fileContent.end(), '\n');
	return fileContent;
}
void deleteFile(System& fs, ClientSession* session, std::fstream &disk, FileEntry* file, const int fileInd){
	// std::cout << "Deleting file '" << file->fileName << '\n';
	if (!disk){
		session->oss << "Error: Disk not accessible while deleting a file.\n";
		// std::cerr << "\tError: Disk not accessible while deleting a file.\n";
		return;
	}
	std::vector<int> newlyAllocatedBlocks;
	int freed = 0;
	std::vector<char> emptyBlock(BLOCK_SIZE, 0);
	int extent = 0;
	while (extent < MAX_EXTENTS){
		int ln = file->extents[extent].length;
		for (int i = 0; i < ln; i++)	newlyAllocatedBlocks.push_back(file->extents[extent].startBlock + i);
		extent++;
		freed += ln;
	}
	for(int block : newlyAllocatedBlocks){
		disk.seekp(block * BLOCK_SIZE, std::ios::beg);
		disk.write(emptyBlock.data(), emptyBlock.size());
		if (disk.fail()){
			session->oss << "Error: Cannot erase data.\n";
			// std::cerr << "\tError: Cannot erase data.\n";
			disk.clear();
			return;
		}	
	}
	// std::cout << "\tClearing up space\n";
	fs.freeBitMapBlocks(disk, newlyAllocatedBlocks);
	fs.saveBitMap(disk);
	{
		std::unique_lock<std::shared_mutex> lock(fs.metaMutex);
		fs.metaDataTable[fileInd] = new FileEntry();
	}
	fs.superblock.freeBlocks += freed;
	int save = fs.saveDirectoryTable(disk, fileInd, session);
	if (save == 0){
		session->oss << "Error: File entry update error during file deletion\n";
		// std::cerr << "\tError: File entry update error during file deletion\n";
		return;
	}
	save = fs.saveSuperblock(disk);
	if (save == 0){
		session->oss << "Error: Superblock update error during file deletion\n";
		// std::cerr << "\tError: Superblock update error during file deletion\n";
		return;
	}
	fs.Entries->removeFileEntry(file->fileName);
	
	disk.flush();
	// std::cout << "\tFile deleted successfully.\n";
}

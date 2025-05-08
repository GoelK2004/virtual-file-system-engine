#pragma once

#include "define.h"

struct Superblock{
	int totalBlocks;
	int freeBlocks;
	int blockSize;
	int fatStart;
	int dataStart;
};
struct Extent{
	int startBlock;
	int length;

	Extent(){
		for (int i = 0; i < MAX_EXTENTS; i++){
			startBlock = -1;
			length = 0;
		}
	}
};

struct SerializableFileEntry;
struct FileEntry{
	char fileName[FILE_NAME_LENGTH];
	int fileSize;
	int numExtents;
	Extent extents[MAX_EXTENTS];
	bool isDirectory;
	int parentIndex;
	int dirID;

	uint64_t created_at;
    uint64_t modified_at;
    uint64_t accessed_at;

	uint32_t owner_id;
	uint32_t group_id;
    uint16_t permissions;
	uint8_t attributes;

	std::mutex lockMutex;
	int readerCount = 0;
	bool isWriteLocked = false;
	int writersWaiting = 0;
	int openCount = 0;
	std::condition_variable readerCV;
    std::condition_variable writerCV;

	FileEntry() : fileSize(0), numExtents(0), extents(), isDirectory(false), parentIndex(-1), created_at(0), modified_at(0), accessed_at(0),
		owner_id(-1), group_id(-1), permissions(0640), attributes(0) {
			fileName[0] = '\0';
		}
	FileEntry(const std::string &name) : fileSize(0), numExtents(0), isDirectory(false), parentIndex(-1), created_at(0), modified_at(0), accessed_at(0),
		owner_id(-1), group_id(-1), permissions(0640), attributes(0) {
			strncpy(fileName, name.c_str(), sizeof(fileName));
			fileName[sizeof(fileName) - 1] = '\0';
			for (int i = 0; i < MAX_EXTENTS; ++i)	extents[i] = Extent();
		}
	FileEntry(const SerializableFileEntry& file);
};
struct SerializableFileEntry {
	char fileName[FILE_NAME_LENGTH];
	int size;
	int created_at;
	int modified_at;
	int accessed_at;
	uint32_t owner_id;
	uint32_t group_id;
    uint16_t permissions;
	uint8_t attributes;
	int extentCount;
	bool isDirectory;
	int parentIndex;
	int dirID;
	Extent extents[MAX_EXTENTS];

	SerializableFileEntry() : size(0), created_at(0), modified_at(0), accessed_at(0), owner_id(-1), group_id(-1), permissions(0640), attributes(0), extentCount(0), isDirectory(false), parentIndex(-1), extents() {
		fileName[0] = '\0';
	};

	SerializableFileEntry(const FileEntry& file);
};

struct User {
    char userName[USER_NAME_LENGTH];
    int password;
	uint32_t totalSize;
    uint32_t user_id;
    uint32_t group_id;
	
	User() : password(0), totalSize(0), user_id(0), group_id(0) {
		strncpy(userName, "root", USER_NAME_LENGTH);
		userName[USER_NAME_LENGTH - 1] = '\0';
	}
};

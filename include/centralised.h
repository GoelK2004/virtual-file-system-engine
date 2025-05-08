#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

#include "system.h"

// #include "./define.h"
// #include "./rollback.h"
// #include "./metaDataManager.h"

// Centralised BITMAP
// extern std::vector<bool> FATTABLE;

// void loadBitMap(std::fstream &disk);
// int saveBitMap(std::fstream &disk);
// std::vector<int> allocateBitMapBlocks(std::fstream &disk, int numBlocks);
// void freeBitMapBlocks(std::fstream &disk, const std::vector<int> &blocks);

// Centralised Directory Table
// extern std::vector<FileEntry*> metaDataTable;
// extern MetadataManager Entries;

// void loadDirectoryTable(std::fstream &disk);
// int saveDirectoryTable(std::fstream &disk, int index);
// int saveDirectoryTableEntire(std::fstream &disk);

// Centralised Superblock
// extern Superblock superblock;

// void loadSuperblock(std::fstream &disk);
// int saveSuperblock(std::fstream &disk);

// Centralised User
// int saveUsers(std::fstream& disk);
// void loadUsers(std::fstream& disk);

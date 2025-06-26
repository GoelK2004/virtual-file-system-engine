#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdint>

#include "define.h"
#include "rollback.h"
#include "helpers.h"
#include "system.h"

// class System;

// namespace fileSystemOperations {
	// 	void createFile(System& fs, std::fstream &disk, const std::string &fileName, const int &fileSize,  FileEntry* newFile, uint16_t permissions = 0640);
	// 	void writeFileData(System& fs, std::fstream &disk, FileEntry* file, const int fileIndex, const std::string &fileContent, bool append);
	// std::string readFileData(System& fs, std::fstream &disk, FileEntry* file);
	// 	void deleteFile(System& fs, std::fstream &disk, FileEntry* file, const int fileInd);
// }
// std::string readFileData(std::fstream &disk, FileEntry* file);

#endif

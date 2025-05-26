#pragma once

#include <string>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>

#include "system.h"
#include "filesystem.h"
#include "fileFeatures.h"
#include "journaling.h"

// std::string readData(const std::string& fileName);
// bool writeData(const std::string &fileName, const std::string &fileContent, bool append, const bool check = false, uint64_t timestamp = 0);
// bool deleteDataFile(const std::string &fileName, const bool check = false, uint64_t timestamp = 0);
// bool deleteDataDir(const std::string &fileName, const bool check = false, uint64_t timestamp = 0);
// bool createFiles(const std::string& fileName, const int& fileSize = BLOCK_SIZE, uint16_t permissions = 0644, const bool check = false, uint64_t timestamp = 0);
// bool renameDir(const std::string &fileName, const std::string &newName, const bool check = false, uint64_t timestamp = 0);
// void list();
// void fileMetadata(const std::string& fileName);

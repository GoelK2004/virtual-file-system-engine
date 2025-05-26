#pragma once

#include <iostream>
#include "system.h"
#include "helpers.h"

bool hasPermission(const FileEntry& file, uint32_t user_id, uint32_t group_id, int permission_type);
// int setAttributes(std::fstream& disk, const std::string& fileName, int attribute);
// int clearAttributes(std::fstream& disk, const std::string& fileName, int attribute);
std::string getAttributeString(const FileEntry* file);
// void renameFile(FileEntry* file, const std::string& newName);
std::string permissionToString(FileEntry* entry);

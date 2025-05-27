#include "helpers.h"

namespace helpers{
	bool isValidFileName(const std::string& fileName){

		if (fileName.empty() || fileName.size() > 31)
			return false;
		if (fileName == "." || fileName == "..")
			return false;
		return std::all_of(fileName.begin(), fileName.end(), [](const char ch){
			return std::isalnum(ch) || ch == '.' || ch == '_';
		});
	}
}

int System::extractPath(const std::string& path, int& currentIndex) {
	FileEntry* dir = nullptr;
	std::stringstream ss(path);
	std::string token;
	while (getline(ss, token, '/')) {
		if (token.empty())	continue;
		if (token == ".")	continue;
		if (token == "..") {
			if (currentIndex == 0)	currentDir = 0;
			else if (!dir) {
				for (const auto& entry : metaDataTable)	{
					if (entry->dirID == currentIndex) {
						currentIndex = entry->parentIndex;
						break;
					}
				}
				for (const auto& entry : metaDataTable) {
					if (entry->dirID == currentIndex) {
						dir = entry;
						break;
					}
				}
			} 
			else {
				currentIndex = dir->parentIndex;
				for (const auto& entry : metaDataTable) {
					if (entry->dirID == currentIndex && entry->owner_id == user.user_id && entry->isDirectory) {
						dir = entry;
						break;
					}
				}
			} 
		} else {
			std::string searchDir = std::to_string(user.user_id) + std::to_string(currentIndex) + "D_" + token;
			const int dirIndex = Entries->getDir(searchDir);
			if (dirIndex == -1) {
				std::cerr << "Error: Path could not be resolved(dir not found).\n";
				return -1;
			}
			dir = metaDataTable[dirIndex];
			if (dir == nullptr || dir->fileName[0] == '\0'){
				std::cerr << "Error: Path could not be resolved(dir misplace).\n";
				return -1;
			}
			currentIndex = dir->dirID;
		}
	}
	return currentIndex;
}
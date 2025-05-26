#include "fileFeatures.h"

bool hasPermission(const FileEntry& file, uint32_t user_id, uint32_t group_id, int permission_type) {
	if (user_id == file.owner_id)
		return ((file.permissions >> 6) & permission_type);
	else if (group_id == file.group_id)
		return ((file.permissions >> 3) & permission_type);
	return ((file.permissions >> 0) & permission_type);
}
std::string permissionToString(FileEntry* entry) {
	std::string permission;
	const std::string symbol[3] = {"r", "w", "x"};
	for (int i = 6; i >= 0; i -= 3) {
		for (int j = 0; j < 3; j++) {
			permission += ((entry->permissions >> (i + (2 - j)) & 1) ? symbol[j] : "-");
		}
	}
	return permission;
}
int setAttributes(System& fs, std::fstream& disk, const std::string& fileName, int attribute) {
	std::string file = std::to_string(fs.currentDir) + "F_" + fileName;
	int searchFileIndex = fs.Entries->getFile(file);
	if (searchFileIndex == -1) {
		std::cerr << "\tError: File not found.\n";
		return 0;
	}
	FileEntry* searchFile = fs.metaDataTable[searchFileIndex];
	uint8_t tempAttributes = searchFile->attributes;
	searchFile->attributes |= attribute;
	int save = fs.saveDirectoryTable(disk, searchFileIndex);
	if (save == 0) {
		fs.metaDataTable[searchFileIndex]->attributes = tempAttributes;
		std::cerr << "\tError: Cannot update attributes.\n";
		return 0;
	}
	std::cout << "Successfully updated attributes.\n";
	return 1;
}
int clearAttributes(System& fs, std::fstream& disk, const std::string& fileName, int attribute) {
	std::string file = std::to_string(fs.currentDir) + "F_" + fileName;
	int searchFileIndex = fs.Entries->getFile(file);
	if (searchFileIndex == -1) {
		std::cerr << "\tError: File not found.\n";
		return 0;
	}
	FileEntry* searchFile = fs.metaDataTable[searchFileIndex];
	uint8_t tempAttributes = searchFile->attributes;
	searchFile->attributes &= ~attribute;
	int save = fs.saveDirectoryTable(disk, searchFileIndex);
	if (save == 0) {
		fs.metaDataTable[searchFileIndex]->attributes = tempAttributes;
		std::cerr << "\tError: Cannot update attributes.\n";
		return 0;
	}
	std::cout << "Successfully cleared attribute.\n";
	return 1;
}
std::string getAttributeString(const FileEntry* file) {
	std::string attributes;
	if (file->attributes & ATTRIBUTES_HIDDEN) {
		attributes += "Hidden, ";
	}
	if (file->attributes & ATTRIBUTES_SYSTEM) {
		attributes += "System, ";
	}
	if (file->attributes & ATTRIBUTES_ARCHIVE) {
		attributes += "Archived, ";
	}
	if (!(file->permissions & PERMISSION_WRITE)) {
		attributes += "ReadOnly, ";
	}
	if (!attributes.empty()) {
		attributes.pop_back();
		attributes.pop_back();
	} else {
		attributes = "None";
	}
	return attributes;
}
void renameFile(System& fs, FileEntry* file, const std::string& newName) {
	std::string newFileName = std::to_string(fs.user.user_id) + std::to_string(fs.currentDir) + "F_" + newName;
	if (helpers::isValidFileName(newFileName)){
		strncpy(file->fileName, newFileName.c_str(), FILE_NAME_LENGTH - 1);
		file->fileName[FILE_NAME_LENGTH - 1] = '\0';
		file->modified_at = std::time(nullptr);
		std::cout << "Successfully renamed file.\n";
	} else {
		std::cerr << "Error: Invalid file name\n";
	}
}
#include "structs.h"

SerializableFileEntry::SerializableFileEntry(const FileEntry& file) {
	std::memcpy(fileName, file.fileName, FILE_NAME_LENGTH);
	size = file.fileSize;
	created_at = file.created_at;
	modified_at = file.modified_at;
	accessed_at = file.accessed_at;
	owner_id = file.owner_id;
	group_id = file.group_id;
	permissions = file.permissions;
	attributes = file.attributes;
	extentCount = file.numExtents;
	isDirectory = file.isDirectory;
	parentIndex = file.parentIndex;
	dirID = file.dirID;	
	std::memcpy(extents, file.extents, sizeof(extents));
}

FileEntry::FileEntry(const SerializableFileEntry& file) : fileSize(file.size) {
	strncpy(fileName, file.fileName, FILE_NAME_LENGTH);
	numExtents = file.extentCount;
	isDirectory = file.isDirectory;
	parentIndex = file.parentIndex;
	dirID = file.dirID;
	created_at = file.created_at;
	modified_at = file.modified_at;
	accessed_at = file.accessed_at;
	owner_id = file.owner_id;
	group_id = file.group_id;
	permissions = file.permissions;
	attributes = file.attributes;
	std::memcpy(extents, file.extents, sizeof(extents));
}

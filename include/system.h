#ifndef SYSTEM_H
#define SYSTEM_H

#include <iostream>
#include <fstream>

#include "fileSystemInterface.h"
#include "mountManager.h"

#include "structs.h"
#include "define.h"
#include "journaling.h"
#include "metaDataManager.h"

class JournalManager;
class MetadataManager;

class System final : public FileSystemInterface {
private:
	std::string DISK_PATH;
	std::vector<bool> FATTABLE;
	std::vector<FileEntry*> metaDataTable;
	Superblock superblock;
	std::vector<User*> userDatabase;
	std::unordered_map<uint32_t, std::string> userTable;
	std::unordered_map<uint32_t, std::string> groupTable;
	User user;
	friend class JournalManager;
	friend class MetadataManager;
	// friend class MetadataManager;
	// friend class JournalManager;
	
	int totalUsers = 0;
	int totalGroups = 0;
	int currentDir = 0;
	int availableDirEntry = 1;
	int metaIndex = 0;

	friend void createFile(System& fs, std::fstream &disk, const std::string &fileName, const int &fileSize,  FileEntry* newFile, uint16_t permissions);
	friend void writeFileData(System& fs, std::fstream &disk, FileEntry* file, const int fileIndex, const std::string &fileContent, bool append);
	// friend std::string readFileData(System& fs, std::fstream &disk, FileEntry* file);
	friend void deleteFile(System& fs, std::fstream &disk, FileEntry* file, const int fileInd);
	
	void loadBitMap(std::fstream &disk);
	int saveBitMap(std::fstream &disk);
	std::vector<int> allocateBitMapBlocks(std::fstream &disk, int numBlocks);
	void freeBitMapBlocks(std::fstream &disk, const std::vector<int> &blocks);
	void loadDirectoryTable(std::fstream &disk);
	int saveDirectoryTable(std::fstream &disk, int index);
	int saveDirectoryTableEntire(std::fstream &disk);
	void loadSuperblock(std::fstream &disk);
	int saveSuperblock(std::fstream &disk);
	int saveUsers(std::fstream& disk);
	void loadUsers(std::fstream& disk);
	
	friend void initialiseSuperblock(System& fs);
	friend void initialiseFAT(System& fs);
	friend void initialiseFileEntries(System& fs);
	friend void initialiseUsers(System& fs);

	void formatFileSystem();
	void loadFromDisk();
	void saveInDisk();

	bool createDirectory(const std::string &directoryName);
	FileEntry* resolvePath(std::fstream &disk, const std::string &path);
	bool changeDirectory(const std::string &dirName);
	std::string createPathM();


	static void printHelpM();
	bool chmodFileM(const std::string& fileName, int newPermissions);
	bool chownM(const std::string& fileName, const std::string& changeOwnership);
	bool chgrpCommand(const std::string& fileName, uint32_t new_group_id);
	void whoamiM() const;
	void loginM(const std::string& username, const std::string& password);
	void userAddM(const std::string& userName, const std::string& password, uint32_t group_id = -1);
	void showUsersM();
	void showGroupsM();
	void treeM(const std::string& path = "/", int depth = 0, const std::string& prefix = "");
	std::vector<FileEntry*> getDirectoryEntries(FileEntry* dir);
	
	// friend bool hasPermission(System& fs, const FileEntry& file, uint32_t user_id, uint32_t group_id, int permission_type);
	friend int setAttributes(System& fs, std::fstream& disk, const std::string& fileName, int attribute);
	friend int clearAttributes(System& fs, std::fstream& disk, const std::string& fileName, int attribute);
	// friend std::string getAttributeString(System& fs, const FileEntry* file);
	friend void renameFile(System& fs, FileEntry* file, const std::string& newName);
	// friend std::string permissionToString(System& fs, FileEntry* entry);
	
	std::string readData(const std::string& fileName);
	bool writeData(const std::string &fileName, const std::string &fileContent, bool append, const bool check = false, uint64_t timestamp = 0);
	bool deleteDataFile(const std::string &fileName, const bool check = false, uint64_t timestamp = 0);
	bool deleteDataDir(const std::string &fileName, const bool check = false, uint64_t timestamp = 0);
	bool createFiles(const std::string& fileName, const int& fileSize = BLOCK_SIZE, uint16_t permissions = 0644, const bool check = false, uint64_t timestamp = 0);
	bool renameFiles(const std::string &fileName, const std::string &newName, const bool check = false, uint64_t timestamp = 0);
	void list();
	void fileMetadata(const std::string& fileName);

	void rollbackMetadataIndex(std::fstream &disk, Superblock &originalSuperblock, int orgIndex, std::vector<int> &newlyAllocatedBlocks);
	void rollbackMetadataOrg(std::fstream &disk, Superblock &originalSuperblock, FileEntry* orgFileEntry, int orgIndex, std::vector<int> &newlyAllocatedBlocks);

public:
	JournalManager* journalManager;
	MetadataManager* Entries;

    explicit System(const std::string& diskPath);
    ~System();
	
	std::string createPath() override;
	void printHelp() override;
    // void format() override;
	void load() override;
	bool create(const std::string& path) override;
	bool create(const std::string& path, const int& fileSize) override;
	std::string read(const std::string& path) override;
	bool write(const std::string& path, const std::string& data) override;
	bool append(const std::string& path, const std::string& data) override;
	bool remove(const std::string& path) override;
	bool rename(const std::string& oldName, const std::string& newName) override;
	bool mkdir(const std::string& path) override;
	bool rmdir(const std::string& path) override;
	bool cd(const std::string& path) override;
	void ls() override;
	void stat(const std::string& path) override;
	bool chmod(const std::string& path, int mode) override;
	bool chown(const std::string& path, const std::string& uid) override;
	bool chgrp(const std::string& path, uint32_t gid) override;
	void whoami() const override;
	void login(const std::string& user_id, const std::string& password) override;
	void userAdd(const std::string& userName, const std::string& password, uint32_t group_id = -1) override;
	void showUsers() override;
	void showGroups() override;
	void tree(const std::string& path = "/", int depth = 0, const std::string& prefix = "") override;

	// LOGS
	void show() override;
	void bTree() override;
};

#endif

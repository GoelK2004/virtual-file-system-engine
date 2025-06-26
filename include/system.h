#pragma once

#include <iostream>
#include <fstream>
#include <shared_mutex>

#include "fileSystemInterface.h"
#include "mountManager.h"

#include "structs.h"
#include "define.h"
#include "journaling.h"
#include "metaDataManager.h"

class JournalManager;
class MetadataManager;
struct FileJournaling;

class System final : public FileSystemInterface {
private:
	std::shared_mutex FATMutex;
	std::shared_mutex metaMutex;
	std::shared_mutex superblockMutex;
	std::shared_mutex userDataMutex;
	std::shared_mutex userTableMutex;
	std::shared_mutex groupTableMutex;
	std::shared_mutex userCountMutex;
	std::shared_mutex groupCountMutex;
	std::shared_mutex dirEntryMutex;
	std::shared_mutex metaIndexMutex;

	std::string DISK_PATH;
	std::vector<bool> FATTABLE; // Shared
	std::vector<FileEntry*> metaDataTable; // Shared
	Superblock superblock; // Shared
	std::vector<User*> userDatabase; // Shared
	std::unordered_map<uint32_t, std::string> userTable; // Shared
	std::unordered_map<uint32_t, std::string> groupTable; // Shared
	// User user;	// Unique
	// static std::string cliMSG; // Unique
	// static std::ostringstream oss; // Unique
	friend class JournalManager;
	friend class MetadataManager;
	
	int totalUsers = 0; // Shared
	int totalGroups = 0; // Shared
	// int currentDir = 0;	// Unique
	int availableDirEntry = 1; // Shared
	int metaIndex = 0; // Shared

	friend void createFile(System& fs, ClientSession* session, std::fstream &disk, const std::string &fileName, const int &fileSize,  FileEntry* newFile, const int& index, uint16_t permissions);
	friend void writeFileData(System& fs, ClientSession* session, std::fstream &disk, FileEntry* file, const int fileIndex, const std::string &fileContent, bool append);
	std::string readFileData(std::fstream &disk, FileEntry* file, ClientSession* session);
	friend void deleteFile(System& fs, ClientSession* session, std::fstream &disk, FileEntry* file, const int fileInd);
	
	bool loadBitMap(std::fstream &disk);
	int saveBitMap(std::fstream &disk);
	std::vector<int> allocateBitMapBlocks(std::fstream &disk, int numBlocks, ClientSession* session);
	void freeBitMapBlocks(std::fstream &disk, const std::vector<int> &blocks);
	bool loadDirectoryTable(std::fstream &disk);
	int saveDirectoryTable(std::fstream &disk, int index, ClientSession* session);
	int saveDirectoryTableEntire(std::fstream &disk);
	bool loadSuperblock(std::fstream &disk);
	int saveSuperblock(std::fstream &disk);
	int saveUsers(std::fstream& disk);
	bool loadUsers(std::fstream& disk);
	
	friend bool initialiseSuperblock(System& fs);
	friend bool initialiseFAT(System& fs);
	friend bool initialiseFileEntries(System& fs);
	friend bool initialiseUsers(System& fs);

	bool formatFileSystem();
	bool loadFromDisk();
	void saveInDisk();

	bool createDirectory(const std::string &directoryName, ClientSession* session);
	FileEntry* resolvePath(std::fstream &disk, const std::string &path, ClientSession* session);
	bool changeDirectory(const std::string &dirName, ClientSession* session);
	std::string createPathM(ClientSession* session);


	static void printHelpM(ClientSession* session);
	bool chmodFileM(const std::string& fileName, int newPermissions, ClientSession* session);
	bool chownM(const std::string& fileName, const std::string& changeOwnership, ClientSession* session);
	bool chgrpCommand(const std::string& fileName, uint32_t new_group_id, ClientSession* session);
	void whoamiM(ClientSession* session);
	void loginM(const std::string& username, const std::string& password, ClientSession* session);
	void userAddM(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id = -1);
	void showUsersM(ClientSession* session);
	void showGroupsM(ClientSession* session);
	void treeM(ClientSession* session, const std::string& path = "/", int depth = 0, const std::string& prefix = "");
	std::vector<FileEntry*> getDirectoryEntries(FileEntry* dir, ClientSession* session);
	
	// friend bool hasPermission(System& fs, const FileEntry& file, uint32_t user_id, uint32_t group_id, int permission_type);
	friend int setAttributes(System& fs, std::fstream& disk, const std::string& fileName, int attribute, ClientSession* session);
	friend int clearAttributes(System& fs, std::fstream& disk, const std::string& fileName, int attribute, ClientSession* session);
	// friend std::string getAttributeString(System& fs, const FileEntry* file);
	// friend std::string permissionToString(System& fs, FileEntry* entry);
	
	std::string readData(const std::string& fileName, ClientSession* session);
	bool writeData(const std::string &fileName, const std::string &fileContent, bool append, ClientSession* session, const bool check = false, uint64_t timestamp = 0, FileJournaling* entry = nullptr);
	bool deleteDataFile(const std::string &fileName, ClientSession* session, const bool check = false, uint64_t timestamp = 0, FileJournaling* entry = nullptr);
	bool deleteDataDir(const std::string &fileName, ClientSession* session, const bool check = false, uint64_t timestamp = 0, FileJournaling* entry = nullptr);
	bool createFiles(const std::string& fileName, ClientSession* session, const int& fileSize = BLOCK_SIZE, uint16_t permissions = 0644, const bool check = false, uint64_t timestamp = 0, FileJournaling* entry = nullptr);
	bool renameFiles(const std::string &fileName, const std::string &newName, ClientSession* session, const bool check = false, uint64_t timestamp = 0, FileJournaling* entry = nullptr);
	bool recursiveDelete(const std::string& filename, ClientSession* session);
	void list(ClientSession* session);
	void fileMetadata(const std::string& fileName, ClientSession* session);

	void rollbackMetadataIndex(std::fstream &disk, Superblock &originalSuperblock, int orgIndex, std::vector<int> &newlyAllocatedBlocks);
	void rollbackMetadataOrg(std::fstream &disk, Superblock &originalSuperblock, FileEntry* orgFileEntry, int orgIndex, std::vector<int> &newlyAllocatedBlocks);

	int extractPath(const std::string& path, int& currentIndex, ClientSession* session);

public:
	JournalManager* journalManager;
	MetadataManager* Entries;

    explicit System(const std::string& diskPath);
    ~System();
	
	std::string createPath(ClientSession* session) override;
	void printHelp(ClientSession* session) override;
    // void format() override;
	bool load() override;
	bool create(const std::string& path, ClientSession* session) override;
	bool create(const std::string& path, const int& fileSize, ClientSession* session) override;
	std::string read(const std::string& path, ClientSession* session) override;
	bool write(const std::string& path, const std::string& data, ClientSession* session) override;
	bool append(const std::string& path, const std::string& data, ClientSession* session) override;
	bool remove(const std::string& path, ClientSession* session) override;
	bool rename(const std::string& oldName, const std::string& newName, ClientSession* session) override;
	bool mkdir(const std::string& path, ClientSession* session) override;
	bool rmdir(const std::string& path, ClientSession* session) override;
	bool rmrdir(const std::string& path, ClientSession* session) override;
	bool cd(const std::string& path, ClientSession* session) override;
	void ls(ClientSession* session) override;
	void stat(const std::string& path, ClientSession* session) override;
	bool chmod(const std::string& path, int mode, ClientSession* session) override;
	bool chown(const std::string& path, const std::string& uid, ClientSession* session) override;
	bool chgrp(const std::string& path, uint32_t gid, ClientSession* session) override;
	void whoami(ClientSession* session) override;
	void login(const std::string& user_id, const std::string& password, ClientSession* session) override;
	void userAdd(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id = -1) override;
	void showUsers(ClientSession* session) override;
	void showGroups(ClientSession* session) override;
	void tree(ClientSession* session, const std::string& path = "/", int depth = 0, const std::string& prefix = "") override;

	// LOGS
	void show() override;
	void bTree() override;
};

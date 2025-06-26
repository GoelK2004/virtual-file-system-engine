#include "journaling.h"

void JournalManager::loadJournal() {
	std::unique_lock<std::mutex> lock(journalMutex);
	journals.clear();
	std::ifstream journalFile(journalFilePath);
	if (!journalFile.is_open()) {
		std::cerr << "[Journal] No existing journal found at: " << journalFilePath << "\n";
		return;
	}
	
	std::string line;
	journalFile.seekg(0, std::ios::beg);
	while (journalFile) {
		std::streampos pos = journalFile.tellg();
		std::getline(journalFile, line);
		if (line.empty())	break;
		journals.push_back(FileJournaling::deSerialise(line));
		entryOffsets.push_back(pos);
	}
	// while (getline(journalFile, line)) {
	// 	try {
	// 		FileJournaling entry = FileJournaling::deSerialise(line);
	// 		journals.push_back(entry);
	// 	} catch (const std::exception& e) {
	// 		std::cerr << "[Journal] Failed to parse journal line: " << line << "[Error: " << e.what() << "]\n";
	// 	}
	// }
	journalFile.close();
	std::cout << "[Journal] Loaded " << journals.size() << " entries from journal.\n";
}
void JournalManager::appendToFile(const FileJournaling& journal) {
	// std::unique_lock<std::mutex> lock(journalMutex);
	std::fstream journalFile(journalFilePath, std::ios::app);
	journalFile.seekg(0, std::ios::end);
	std::streampos pos = journalFile.tellg();
	if (!journalFile.is_open()) {
		std::cerr << "[Journal] Error: Unable to open journal file for writing.\n";
		return;
	}
	journalFile << journal.serialise() << "\n";
	entryOffsets.push_back(pos);
	journalFile.close();
}
uint64_t JournalManager::logOperation(std::string user, const std::string& op, const std::string& fileName, const std::string& newFileName, const std::string& data, uint32_t fileSize, const int currentDir) {
	uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	FileJournaling entry {
		user,
		timestamp,
		op,
		fileName,
		newFileName,
		currentDir,
		fileSize,
		data,
		false,
		false
	};
	{
		std::unique_lock<std::mutex> lock(journalMutex);
		journals.push_back(entry);
		appendToFile(entry);
	}
	return timestamp;
}
void JournalManager::markCommitted(uint64_t timestamp) {
	std::unique_lock<std::mutex> lock(journalMutex);
	int low = 0, high = static_cast<int>(journals.size()) - 1, mid;
	while (low <= high) {
		mid = (low + high) / 2;
		if (journals[mid].timestamp == timestamp) {
			journals[mid].committed = true;
			std::fstream journalFile(journalFilePath, std::ios::binary | std::ios::in | std::ios::out);
			if (!journalFile.is_open())	throw std::runtime_error("Failed to open the journal file for updation.\n");
			
			journalFile.seekp(static_cast<std::streamoff>(entryOffsets[mid]), std::ios::beg);
			std::string line;
			std::getline(journalFile, line);
			size_t lastDelimiter = line.rfind('|');
			if (lastDelimiter == std::string::npos || lastDelimiter + 1 >= line.size()) {
				throw std::runtime_error("Malformed journal entry — missing committed flag.");
			}
			std::streampos committedPos = entryOffsets[mid] + static_cast<std::streamoff>(lastDelimiter + 1);
			journalFile.seekp(static_cast<std::streamoff>(committedPos), std::ios::beg);
			journalFile.put('1');
			
			size_t secondLast = line.rfind('|', lastDelimiter - 1);
			if (secondLast == std::string::npos)
				throw std::runtime_error("Malformed line: only one delimiter found.");
			committedPos = entryOffsets[mid] + static_cast<std::streamoff>(secondLast + 1);
			journalFile.seekp(static_cast<std::streamoff>(committedPos), std::ios::beg);
			journalFile.put('1');
			
			journalFile.close();
			break;
		} else if (journals[mid].timestamp < timestamp) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}
}
void JournalManager::rewriteJournal() {
	std::unique_lock<std::mutex> lock(journalMutex);
	std::ofstream outFile(journalFilePath, std::ios::trunc);
	if (!outFile.is_open())
		throw std::runtime_error("Unable to open journal file for rewriting");
	for (const auto& entry : journals) {
		outFile << entry.serialise() << "\n";
	}
	outFile.close();
}
void JournalManager::recoverUncommitedOperations(std::string user, ClientSession* session) {
	std::vector<FileJournaling> uncommitted;
	{
		std::unique_lock<std::mutex> lock(journalMutex);
		for (const auto& entry : journals) {
			if (!entry.committed && entry.user == user) {
				uncommitted.push_back(entry);
			}
		}
	}
	updateUncommitedOperations(uncommitted, session);
}
void JournalManager::updateUncommitedOperations(std::vector<FileJournaling>& uncommitted, ClientSession* session) {
	for (auto& entry : uncommitted) {
		entry.check = true;
		if (entry.operation == OP_CREATE) {
			int searchFileIndex = system->Entries->getFile(entry.fileName);
			if (searchFileIndex != -1) {
				FileEntry* searchFile;
				{
					std::shared_lock<std::shared_mutex> lock(system->metaMutex);
					searchFile = system->metaDataTable[searchFileIndex];
				}
				if (searchFile && searchFile->parentIndex == entry.directory) {
					continue;
				}
			} else {
				entry.fileName.erase(0, 2 + static_cast<int>(std::to_string(entry.directory).length() + std::to_string(session->user.user_id).length()));
				system->createFiles(entry.fileName, session, entry.fileSize, 0644, entry.check, entry.timestamp, &entry);
			}
		} else if (entry.operation == OP_DELETE_DIR) {
			entry.fileName.erase(0, 2 + static_cast<int>(std::to_string(entry.directory).length() + std::to_string(session->user.user_id).length()));
			system->deleteDataDir(entry.fileName, session, entry.check, entry.timestamp, &entry);
		} else if (entry.operation == OP_DELETE_FILE) {
			entry.fileName.erase(0, 2 + static_cast<int>(std::to_string(entry.directory).length() + std::to_string(session->user.user_id).length()));
			system->deleteDataFile(entry.fileName, session, entry.check, entry.timestamp, &entry);
		} else if (entry.operation == OP_WRITE) {
			entry.fileName.erase(0, 2 + static_cast<int>(std::to_string(entry.directory).length() + std::to_string(session->user.user_id).length()));
			system->writeData(entry.fileName, entry.data, false, session, entry.check, entry.timestamp, &entry);
		} else if (entry.operation == OP_WRITE_APPEND) {
			entry.fileName.erase(0, 2 + static_cast<int>(std::to_string(entry.directory).length() + std::to_string(session->user.user_id).length()));
			system->writeData(entry.fileName, entry.data, true, session, entry.check, entry.timestamp, &entry);
		} else if (entry.operation == OP_RENAME) {
			entry.fileName.erase(0, 2 + static_cast<int>(std::to_string(entry.directory).length() + std::to_string(session->user.user_id).length()));
			system->renameFiles(entry.fileName, entry.newFileName, session, entry.check, entry.timestamp, &entry);
		} else	continue;
	}
}
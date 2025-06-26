#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <stdexcept>
#include <fstream>
#include <iostream>

#include "multithreading.h"
#include "system.h"

#define OP_WRITE "WRITE"
#define OP_WRITE_APPEND "WRITEA"
#define OP_CREATE "CREATE"
#define OP_DELETE_DIR "DELETE DIR"
#define OP_DELETE_FILE "DELETE FILE"
#define OP_RENAME "RENAME"

class System;

struct FileJournaling {
	std::string user;
	uint64_t timestamp;
	std::string operation;
	std::string fileName;
	std::string newFileName;
	int directory;
	uint32_t fileSize;
	std::string data;
	bool check;
	bool committed;

	std::string serialise() const {
		return user + "|" + std::to_string(timestamp) + "|" +
		operation + "|" + fileName + "|" + newFileName + "|" +
		std::to_string(directory) + "|" +
		data + "|" + std::to_string(fileSize) + "|" +
		(check ? "1" : "0") + "|" +
		(committed ? "1" : "0");
	}

	static FileJournaling deSerialise(const std::string& line) {
		FileJournaling journal;
		size_t pos = 0, start = 0;
		std::vector<std::string> tokens;
		while ((pos = line.find('|', start)) != std::string::npos) {
			tokens.push_back(line.substr(start, pos - start));
			start = pos + 1;
		}
		tokens.push_back(line.substr(start));
		if (tokens.size() != 10)	throw std::runtime_error("Malformed journal entry");

		journal.user = tokens[0];
		journal.timestamp = std::stoull(tokens[1]);
		journal.operation = tokens[2];
		journal.fileName = tokens[3];
		journal.newFileName = tokens[4];
		journal.directory = std::stoi(tokens[5]);
		journal.data = tokens[6];
		journal.fileSize = std::stoul(tokens[7]);
		journal.check = (tokens[8] == "1");
		journal.committed = (tokens[9] == "1");

		return journal;
	}
};


class JournalManager {
	private:
		System* system;
	
		std::vector<FileJournaling> journals;
		std::vector<std::streampos> entryOffsets;
		std::string journalFilePath;
		std::mutex journalMutex;
	
		void appendToFile(const FileJournaling& journal);
		void rewriteJournal();
		void updateUncommitedOperations(std::vector<FileJournaling>& uncommitted, ClientSession* session);
	
		public:
		JournalManager(System* system, const std::string& path) : system(system) {
			journalFilePath = path;
		};
		void loadJournal();
		uint64_t logOperation(std::string user, const std::string& op, const std::string& fileName, const std::string& newFileName, const std::string& data, uint32_t fileSize, const int currentDir);
		void markCommitted(uint64_t timestamp);
		void recoverUncommitedOperations(std::string user, ClientSession* session);
};

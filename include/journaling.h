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
	uint64_t timestamp;
	std::string operation;
	std::string fileName;
	uint32_t fileSize;
	std::string data;
	bool check;
	bool committed;

	std::string serialise() const {
		return std::to_string(timestamp) + "|" +
		operation + "|" + fileName + "|" +
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
		if (tokens.size() != 7)	throw std::runtime_error("Malformed journal entry");

		journal.timestamp = std::stoull(tokens[0]);
		journal.operation = tokens[1];
		journal.fileName = tokens[2];
		journal.data = tokens[3];
		journal.fileSize = std::stoul(tokens[4]);
		journal.check = (tokens[5] == "1");
		journal.committed = (tokens[6] == "1");

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
		void updateUncommitedOperations(std::vector<FileJournaling>& uncommitted);
	
		public:
		JournalManager(System* system, const std::string& path) : system(system) {
			journalFilePath = path;
		};
		void loadJournal();
		uint64_t logOperation(const std::string& op, const std::string& fileName, const std::string& data, uint32_t fileSize);
		void markCommitted(uint64_t timestamp);
		void recoverUncommitedOperations();
};

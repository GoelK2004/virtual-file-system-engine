#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "structs.h"

class FileSystemInterface {
	public:
		virtual ~FileSystemInterface() = default;

		virtual std::string createPath(ClientSession* session) = 0;
		virtual void printHelp(ClientSession* session) = 0;

		// Initial load
		// virtual void format() = 0;
		virtual bool load() = 0;
		
		// File & Directory Management
		virtual bool create(const std::string& path, ClientSession* session) = 0;
		virtual bool create(const std::string& path, const int& fileSize, ClientSession* session) = 0;
		virtual std::string read(const std::string& path, ClientSession* session) = 0;
		virtual bool write(const std::string& path, const std::string& data, ClientSession* session) = 0;
		virtual bool append(const std::string& path, const std::string& data, ClientSession* session) = 0;
		virtual bool remove(const std::string& path, ClientSession* session) = 0;
		virtual bool rename(const std::string& oldName, const std::string& newName, ClientSession* session) = 0;
	
		virtual bool mkdir(const std::string& path, ClientSession* session) = 0;
		virtual bool rmdir(const std::string& path, ClientSession* session) = 0;
		virtual bool rmrdir(const std::string& path, ClientSession* session) = 0;
	
		// Navigation
		virtual bool cd(const std::string& path, ClientSession* session) = 0;
		virtual void ls(ClientSession* session) = 0;
		// virtual std::string pwd() const = 0;
	
		// File info
		virtual void stat(const std::string& path, ClientSession* session) = 0;
	
		// Permissions
		virtual bool chmod(const std::string& path, int mode, ClientSession* session) = 0;
		virtual bool chown(const std::string& path, const std::string& uid, ClientSession* session) = 0;
		virtual bool chgrp(const std::string& path, uint32_t gid, ClientSession* session) = 0;
	
		// User Utilities
		virtual void whoami(ClientSession* session) = 0;
		virtual void login(const std::string& user_id, const std::string& password, ClientSession* session) = 0;
		virtual void userAdd(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id) = 0;
		virtual void showUsers(ClientSession* session) = 0;
		virtual void showGroups(ClientSession* session) = 0;
	
		// File Tree
		virtual void tree(ClientSession* session, const std::string& path, int depth, const std::string& prefix) = 0;

		// LOGS
		virtual void show() = 0;
		virtual void bTree() = 0;
};

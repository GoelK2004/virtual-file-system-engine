#pragma once

#include <string>
#include <vector>
#include <cstdint>

class FileSystemInterface {
	public:
		virtual ~FileSystemInterface() = default;

		virtual std::string createPath() = 0;
		virtual void printHelp() = 0;

		// Initial load
		// virtual void format() = 0;
		virtual void load() = 0;
		
		// File & Directory Management
		virtual bool create(const std::string& path) = 0;
		virtual bool create(const std::string& path, const int& fileSize) = 0;
		virtual std::string read(const std::string& path) = 0;
		virtual bool write(const std::string& path, const std::string& data) = 0;
		virtual bool append(const std::string& path, const std::string& data) = 0;
		virtual bool remove(const std::string& path) = 0;
		virtual bool rename(const std::string& oldName, const std::string& newName) = 0;
	
		virtual bool mkdir(const std::string& path) = 0;
		virtual bool rmdir(const std::string& path) = 0;
	
		// Navigation
		virtual bool cd(const std::string& path) = 0;
		virtual void ls() = 0;
		// virtual std::string pwd() const = 0;
	
		// File info
		virtual void stat(const std::string& path) = 0;
	
		// Permissions
		virtual bool chmod(const std::string& path, int mode) = 0;
		virtual bool chown(const std::string& path, const std::string& uid) = 0;
		virtual bool chgrp(const std::string& path, uint32_t gid) = 0;
	
		// User Utilities
		virtual void whoami() const = 0;
		virtual void login(const std::string& user_id, const std::string& password) = 0;
		virtual void userAdd(const std::string& userName, const std::string& password, uint32_t group_id) = 0;
		virtual void showUsers() = 0;
		virtual void showGroups() = 0;
	
		// File Tree
		virtual void tree(const std::string& path, int depth, const std::string& prefix) = 0;
		
		// LOGS
		virtual void show() = 0;
		virtual void bTree() = 0;
};

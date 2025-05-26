#include <iostream>
#include "VFS.h"

void VFSManager::mount(FileSystemInterface* fileSystem) {
    fs = std::move(fileSystem);
    std::cout << "File system mounted successfully.\n";
}

void VFSManager::unmount() {
    delete fs;
    std::cout << "File system unmounted.\n";
}

bool VFSManager::isMounted() const {
    return fs != nullptr;
}

std::string VFSManager::createPath() {
	if (isMounted())    return fs->createPath();
    return "";
}

void VFSManager::printHelp() {
	if (isMounted())    fs->printHelp();
}

// void VFSManager::format() {
// 	if (isMounted())    fs->format();
// }

// void VFSManager::load() {
// 	if (isMounted())    fs->load();
// }

bool VFSManager::create(const std::string& path) {
    return isMounted() ? fs->create(path) : false;
}

bool VFSManager::create(const std::string& path, const int& fileSize) {
    return isMounted() ? fs->create(path, fileSize) : false;
}

std::string VFSManager::read(const std::string& path) {
    return isMounted() ? fs->read(path) : "";
}

bool VFSManager::write(const std::string& path, const std::string& data) {
    return isMounted() ? fs->write(path, data) : false;
}

bool VFSManager::append(const std::string& path, const std::string& data) {
    return isMounted() ? fs->append(path, data) : false;
}

bool VFSManager::remove(const std::string& path) {
    return isMounted() ? fs->remove(path) : false;
}

bool VFSManager::rename(const std::string& oldName, const std::string& newName) {
    return isMounted() ? fs->rename(oldName, newName) : false;
}

bool VFSManager::mkdir(const std::string& path) {
    return isMounted() ? fs->mkdir(path) : false;
}

bool VFSManager::rmdir(const std::string& path) {
    return isMounted() ? fs->rmdir(path) : false;
}

bool VFSManager::cd(const std::string& path) {
    return isMounted() ? fs->cd(path) : false;
}

void VFSManager::ls() {
    if (isMounted())    fs->ls();
}

// std::string VFSManager::pwd() const {
//     return isMounted() ? fs->pwd() : "/";
// }

void VFSManager::stat(const std::string& path) {
    if (isMounted()) fs->stat(path);
}

bool VFSManager::chmod(const std::string& path, int mode) {
    return isMounted() ? fs->chmod(path, mode) : false;
}

bool VFSManager::chown(const std::string& path, const std::string& uid) {
    return isMounted() ? fs->chown(path, uid) : false;
}

bool VFSManager::chgrp(const std::string& path, uint32_t gid) {
    return isMounted() ? fs->chgrp(path, gid) : false;
}

void VFSManager::login(const std::string& user_id, const std::string& password) {
    if (isMounted()) fs->login(user_id, password);
}

void VFSManager::whoami() const {
    if (isMounted()) fs->whoami();
}

void VFSManager::userAdd(const std::string& userName, const std::string& password, uint32_t group_id) {
	if (isMounted()) fs->userAdd(userName, password, group_id);
}

void VFSManager::showUsers() {
	if (isMounted()) fs->showUsers();
}

void VFSManager::showGroups() {
	if (isMounted()) fs->showGroups();
}

void VFSManager::tree(const std::string& path, int depth, const std::string& prefix) {
    if (isMounted()) fs->tree(path, depth, prefix);
}

// LOGS
void VFSManager::bTree() {
    if (isMounted()) fs->bTree();
}
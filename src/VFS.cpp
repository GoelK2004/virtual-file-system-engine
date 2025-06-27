#include "VFS.h"

void VFSManager::mount(FileSystemInterface* fileSystem) {
    fs = std::move(fileSystem);
    std::cout << "File system mounted successfully.\n";
}

void VFSManager::unmount() {
    if (fs) delete fs;
    fs = nullptr;
    std::cout << "File system unmounted.\n";
}

bool VFSManager::isMounted() const {
    return fs != nullptr;
}

std::string VFSManager::createPath(ClientSession* session) {
	if (isMounted())    return fs->createPath(session);
    return "";
}

void VFSManager::printHelp(ClientSession* session) {
	if (isMounted())    fs->printHelp(session);
}

// void VFSManager::format() {
// 	if (isMounted())    fs->format();
// }

// void VFSManager::load() {
// 	if (isMounted())    fs->load();
// }

bool VFSManager::create(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->create(path, session) : false;
}

bool VFSManager::create(const std::string& path, const int& fileSize, ClientSession* session) {
    return isMounted() ? fs->create(path, fileSize, session) : false;
}

std::string VFSManager::read(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->read(path, session) : "";
}

bool VFSManager::write(const std::string& path, const std::string& data, ClientSession* session) {
    return isMounted() ? fs->write(path, data, session) : false;
}

bool VFSManager::append(const std::string& path, const std::string& data, ClientSession* session) {
    return isMounted() ? fs->append(path, data, session) : false;
}

bool VFSManager::remove(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->remove(path, session) : false;
}

bool VFSManager::rename(const std::string& oldName, const std::string& newName, ClientSession* session) {
    return isMounted() ? fs->rename(oldName, newName, session) : false;
}

bool VFSManager::mkdir(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->mkdir(path, session) : false;
}

bool VFSManager::rmdir(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->rmdir(path, session) : false;
}

bool VFSManager::rmrdir(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->rmrdir(path, session) : false;
}

bool VFSManager::cd(const std::string& path, ClientSession* session) {
    return isMounted() ? fs->cd(path, session) : false;
}

void VFSManager::ls(ClientSession* session) {
    if (isMounted())    fs->ls(session);
}

// std::string VFSManager::pwd() const {
//     return isMounted() ? fs->pwd() : "/";
// }

void VFSManager::stat(const std::string& path, ClientSession* session) {
    if (isMounted()) fs->stat(path, session);
}

bool VFSManager::chmod(const std::string& path, int mode, ClientSession* session) {
    return isMounted() ? fs->chmod(path, mode, session) : false;
}

bool VFSManager::chown(const std::string& path, const std::string& uid, ClientSession* session) {
    return isMounted() ? fs->chown(path, uid, session) : false;
}

bool VFSManager::chgrp(const std::string& path, uint32_t gid, ClientSession* session) {
    return isMounted() ? fs->chgrp(path, gid, session) : false;
}

void VFSManager::login(const std::string& user_id, const std::string& password, ClientSession* session) {
    if (isMounted()) fs->login(user_id, password, session);
}

void VFSManager::whoami(ClientSession* session) const {
    if (isMounted()) fs->whoami(session);
}

void VFSManager::userAdd(const std::string& userName, const std::string& password, ClientSession* session, uint32_t group_id) {
	if (isMounted()) fs->userAdd(userName, password, session, group_id);
}

void VFSManager::showUsers(ClientSession* session) {
	if (isMounted()) fs->showUsers(session);
}

void VFSManager::showGroups(ClientSession* session) {
	if (isMounted()) fs->showGroups(session);
}

void VFSManager::tree(ClientSession* session, const std::string& path, int depth, const std::string& prefix) {
    if (isMounted()) fs->tree(session, path, depth, prefix);
}

// LOGS
void VFSManager::bTree() {
    if (isMounted()) fs->bTree();
}
void VFSManager::show() {
    if (isMounted()) fs->show();
}
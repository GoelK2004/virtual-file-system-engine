#pragma once

#include "metaStruct.h"
#include "hash.h"

class System;

class MetadataManager {
private:
    System* system;
    BPlusTree bptree;

public:
    MetadataManager(System* system, int treeOrder) : system(system), bptree(treeOrder) {};

    int insertFileEntry(const std::string& fileName, const int metaIndex) {
        int key = hashFileName(fileName);
        bptree.insert(key, metaIndex);
        return key;
    }

    bool updateIdx(const std::string& fileName, int idx) {
        int key = hashFileName(fileName);
        return bptree.update(key, idx);
    }

    int getFile(const std::string& fileName) {
        int key = hashFileName(fileName);
        return bptree.searchFile(key);
    }

    int getDir(const std::string& dirName) {
        int key = hashFileName(dirName);
        return bptree.searchDir(key);
    }

    void removeFileEntry(const std::string& fileName) {
        int key = hashFileName(fileName);
        bptree.remove(key);
    }

    void printMetadataTree() {
        bptree.printTree();
    }

    bool loadBPlusTree(std::fstream& disk) {
        return bptree.loadBPlusTree(disk);
    }

    void saveBPlusTree(std::fstream& disk) {
        bptree.saveBPlusTree(disk);
    }

    void deleteBPlusTree() {
        bptree.deleteTree();
    }
};

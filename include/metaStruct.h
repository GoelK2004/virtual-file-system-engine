#pragma once

#include <fstream>
#include <vector>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <cstring>
#include <algorithm>
#include "define.h"

struct BPlusTreeNode{
	int nodeID;
	bool isLeaf;
	std::vector<int> keys;
	BPlusTreeNode* parent;

	std::vector<BPlusTreeNode*> children;

	std::vector<int> values;
	BPlusTreeNode* nextLeaf;

	BPlusTreeNode() : isLeaf(false), parent(nullptr), nextLeaf(nullptr) {}
};

class BPlusTree {
	private:
		int order;
		int nodes = 0;
		BPlusTreeNode* root;
		
		BPlusTreeNode* findLeafNode(int key);

		void splitLeafNode(BPlusTreeNode* leaf);
		void insertInternal(int middleKey, BPlusTreeNode* leftChild, BPlusTreeNode* rightChild);
		void splitInternalNode(BPlusTreeNode* parent);
		
		void handleLeafUnderflow(BPlusTreeNode* leaf);
		void mergeLeafNodes(BPlusTreeNode* leaf);
		void handleInternalUnderflow(BPlusTreeNode* node);
	
	public:
		BPlusTree(int order);
		// ~BPlusTree();
	
		int saveBPlusTree(std::fstream &disk);
		int loadBPlusTree(std::fstream &disk);

		void insert(int key, const int metaIndex);
		int search(int key);
		int searchFile(int key);
		int searchDir(int key);
		void remove(int key);
		void printTree();
		
		void deleteTree();
};

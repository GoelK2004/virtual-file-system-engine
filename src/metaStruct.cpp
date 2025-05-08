#include "metaStruct.h"

/*
Each node will save:
nodeID (4 bytes)
isLeaf (1 byte)
numKeys (4 bytes)
keys[4] (4 × 4 = 16 bytes)
If leaf:
values[4] (4 × 4 = 16 bytes)
nextLeafID (4 bytes)
If internal:
children[5] (5 × 4 = 20 bytes)
*/

int BPlusTree::saveBPlusTree(std::fstream &disk) {
    if (!disk.is_open()) {
        std::cerr << "Error: Cannot access disk to save B+ Tree.\n";
        return 0;
    }

    std::vector<char> buffer(BLOCK_SIZE, 0);
    int offset = 0;
    int blockIndex = 0;

    std::queue<BPlusTreeNode*> q;
	std::unordered_set<int> visited;
    q.push(root);

    while (!q.empty()) {
        BPlusTreeNode* current = q.front();
        q.pop();

        if (!current || visited.count(current->nodeID)) continue;
		visited.insert(current->nodeID);

        std::vector<char> nodeBuffer;
        nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&current->nodeID), reinterpret_cast<char*>(&current->nodeID) + sizeof(int));
		uint8_t leafFlag = static_cast<uint8_t>(current->isLeaf);
        nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&leafFlag), reinterpret_cast<char*>(&leafFlag) + sizeof(uint8_t));
        int numKeys = current->keys.size();
        nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&numKeys), reinterpret_cast<char*>(&numKeys) + sizeof(int));
        for (int key : current->keys) {
            nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&key), reinterpret_cast<char*>(&key) + sizeof(int));
        }
        if (current->isLeaf) {
            for (int val : current->values) {
				nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&val), reinterpret_cast<char*>(&val) + sizeof(int));
            }
            int nextLeafID = (current->nextLeaf) ? current->nextLeaf->nodeID : -1;
            nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&nextLeafID), reinterpret_cast<char*>(&nextLeafID) + sizeof(int));
        } else {
			int numChildren = current->children.size();
			nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&numChildren), reinterpret_cast<char*>(&numChildren) + sizeof(int));
            for (BPlusTreeNode* child : current->children) {
                int childID = (child) ? child->nodeID : -1;
                nodeBuffer.insert(nodeBuffer.end(), reinterpret_cast<char*>(&childID), reinterpret_cast<char*>(&childID) + sizeof(int));
                if (child) q.push(child);
            }
        }

        if (offset + nodeBuffer.size() > BLOCK_SIZE) {
            disk.seekp(BPLUS_TREE_START * BLOCK_SIZE + blockIndex * BLOCK_SIZE, std::ios::beg);
            disk.write(buffer.data(), BLOCK_SIZE);

            std::fill(buffer.begin(), buffer.end(), 0);
            offset = 0;
            blockIndex++;
        }

        memcpy(buffer.data() + offset, nodeBuffer.data(), nodeBuffer.size());
        offset += nodeBuffer.size();
    }

    if (offset > 0) {
        disk.seekp(BPLUS_TREE_START * BLOCK_SIZE + blockIndex * BLOCK_SIZE, std::ios::beg);
        disk.write(buffer.data(), BLOCK_SIZE);
        disk.flush();
    }

	disk.flush();
    return 1;
}

int BPlusTree::loadBPlusTree(std::fstream &disk) {
    if (!disk.is_open()) {
        std::cerr << "Error: Cannot access disk to load B+ Tree.\n";
        return 0;
    }

    disk.seekg(BPLUS_TREE_START * BLOCK_SIZE, std::ios::beg);
	std::vector<char> buffer(BLOCK_SIZE);
	
	std::unordered_map<int, BPlusTreeNode*> nodeMap;
    std::unordered_map<int, std::vector<int>> tempChildrenMap;
    std::unordered_map<int, int> tempNextLeafMap;
	
	while (disk.read(buffer.data(), BLOCK_SIZE)){
		size_t offset = 0;
		while (offset <= BLOCK_SIZE) {
			int nodeID;
			memcpy(&nodeID, buffer.data() + offset, sizeof(int));
			if (nodeID == 0)	break;
			offset += sizeof(int);
			
			if (offset + sizeof(uint8_t) > BLOCK_SIZE)	break;
			uint8_t leafFlag;
            memcpy(&leafFlag, buffer.data() + offset, sizeof(uint8_t));
            offset += sizeof(uint8_t);
			bool isLeaf = static_cast<bool>(leafFlag);

			if (offset + sizeof(int) > BLOCK_SIZE) break;
            int numKeys;
            memcpy(&numKeys, buffer.data() + offset, sizeof(int));
            offset += sizeof(int);

			if (offset + numKeys * sizeof(int) > BLOCK_SIZE) break;
            std::vector<int> keys(numKeys);
            for (int i = 0; i < numKeys; ++i) {
                memcpy(&keys[i], buffer.data() + offset, sizeof(int));
                offset += sizeof(int);
            }

			BPlusTreeNode* newNode = new BPlusTreeNode();
			newNode->nodeID = nodeID;
			newNode->isLeaf = isLeaf;
			newNode->keys = keys;

			if (isLeaf) {
				if (offset + keys.size() * sizeof(int) > BLOCK_SIZE)	break;
				for (int i = 0; i < numKeys; i++) {
					int value;
					memcpy(&value, buffer.data() + offset, sizeof(int));
					newNode->values.push_back(value);
					offset += sizeof(int);
				}
				int nextLeafNode;
				memcpy(&nextLeafNode, buffer.data() + offset, sizeof(int));
				offset += sizeof(int);
				tempNextLeafMap[newNode->nodeID] = nextLeafNode;
			} else {
				if (offset + sizeof(int) > BLOCK_SIZE)	break;
				int numChildren;
				memcpy(&numChildren, buffer.data() + offset, sizeof(int));
				offset += sizeof(int);

				if (offset + numChildren * sizeof(int) > BLOCK_SIZE)	break;
				std::vector<int> childIDs;
				for (int i = 0; i < numChildren; i++) {
					int child;
					memcpy(&child, buffer.data() + offset, sizeof(int));
					offset += sizeof(int);
					childIDs.push_back(child);
				}
				tempChildrenMap[newNode->nodeID] = childIDs;
			}
			nodeMap[newNode->nodeID] = newNode;
		}
	}

	for (auto& [id, node] : nodeMap) {
		if (!node->isLeaf) {
			for (int childID : tempChildrenMap[id]) {
				if (nodeMap.count(childID)){
					node->children.push_back(nodeMap[childID]);
					nodeMap[childID]->parent = node;
				}
			}
		}
		else {
			int nextLeafId = tempNextLeafMap[id];
			if (nodeMap.count(nextLeafId)) {
				node->nextLeaf = nodeMap[nextLeafId];
			}
		}
	}
	
	if (nodeMap.empty()) {
        root = new BPlusTreeNode();
		root->isLeaf = true;
		root->nodeID = ++nodes;
		return 0;
    }

	root = nodeMap.begin()->second;
	return 1;
}


BPlusTree::BPlusTree(int order) {
	this->order = order;
	root = new BPlusTreeNode();
	root->isLeaf = true;
	root->nodeID = ++nodes;
}
// BPlusTree::~BPlusTree() {
// 	std::function<void(BPlusTreeNode*)> deleteNodes = [&](BPlusTreeNode* node){
// 		if (!node)	return;
// 		for (BPlusTreeNode* child : node->children)	deleteNodes(child);
// 		delete node;
// 	};
// 	deleteNodes(root);
// }
BPlusTreeNode* BPlusTree::findLeafNode(int key){
	BPlusTreeNode* node = root;
	while (!node->isLeaf) {
		int i = 0;
		while (i < static_cast<int>(node->keys.size()) && key >= node->keys[i])	i++;
		node = node->children[i];
	}
	return node;
}
void BPlusTree::splitInternalNode(BPlusTreeNode* parent) {
	int middleIndex = parent->keys.size() / 2;
	int middleElement = parent->keys[middleIndex];

	BPlusTreeNode* newNode = new BPlusTreeNode();
	newNode->nodeID = ++nodes;
	newNode->keys.assign(parent->keys.begin() + middleIndex + 1, parent->keys.end());
	newNode->children.assign(parent->children.begin() + middleIndex + 1, parent->children.end());
	
	parent->keys.resize(middleIndex);
	parent->children.resize(middleIndex + 1);

	for (BPlusTreeNode* child : newNode->children) {
		child->parent = newNode;
	}

	insertInternal(middleElement, parent, newNode);
}
void BPlusTree::insertInternal(int middleKey, BPlusTreeNode* leftChild, BPlusTreeNode* rightChild) {
	if (!leftChild->parent) {
		root = new BPlusTreeNode();
		root->nodeID = ++nodes;
		root->keys.push_back(middleKey);
		root->children.push_back(leftChild);
		root->children.push_back(rightChild);
		leftChild->parent = root;
		rightChild->parent = root;
		return;
	}
	BPlusTreeNode* parent = leftChild->parent;
	auto it = std::upper_bound(parent->keys.begin(), parent->keys.end(), middleKey);
	int index = it - parent->keys.begin();

	parent->keys.insert(it, middleKey);
	parent->children.insert(parent->children.begin() + index + 1, rightChild);
	rightChild->parent = parent;
	
	if (static_cast<int>(parent->keys.size()) >= order)	splitInternalNode(parent);
}
void BPlusTree::splitLeafNode(BPlusTreeNode* leaf){
	int midIndex = leaf->keys.size() / 2;
	
	BPlusTreeNode* newLeaf = new BPlusTreeNode();
	newLeaf->nodeID = ++nodes;
	newLeaf->isLeaf = true;
	newLeaf->keys.assign(leaf->keys.begin() + midIndex, leaf->keys.end());
	newLeaf->values.assign(leaf->values.begin() + midIndex, leaf->values.end());

	leaf->keys.resize(midIndex);
	leaf->values.resize(midIndex);
	
	newLeaf->nextLeaf = leaf->nextLeaf;
	leaf->nextLeaf = newLeaf;

	int middleKey = newLeaf->keys.front();
	insertInternal(middleKey, leaf, newLeaf);
}
void BPlusTree::insert(int key, const int metaIndex) {
	BPlusTreeNode* leaf = findLeafNode(key);

	auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
	int index = it - leaf->keys.begin();
	leaf->keys.insert(it, key);
	leaf->values.insert(leaf->values.begin() + index, metaIndex);
	

	if (static_cast<int>(leaf->keys.size()) >= order)	splitLeafNode(leaf);
	if (static_cast<int>(leaf->keys.size()) < (order + 1) / 2)	handleLeafUnderflow(leaf);
}

int BPlusTree::search(int key) {
	BPlusTreeNode* leaf = findLeafNode(key);
	int low = 0, high = leaf->keys.size() - 1, mid = -1;
	while (low <= high){
		mid = (low + high) / 2;
		if (leaf->keys[mid] == key)	return leaf->values[mid];
		if (leaf->keys[mid] > key)	high = mid - 1;
		else if (leaf->keys[mid] < key)	low = mid + 1;
	}
	return	-1;

	auto it = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
	if (it != leaf->keys.end()){
		int index = it - leaf->keys.begin();
		if (leaf->keys[index] == key)	return leaf->values[index];
	}
	return -1;
}

void BPlusTree::handleInternalUnderflow(BPlusTreeNode* node) {
	if (!node->parent){
		if (node->keys.empty() && !node->isLeaf && node->children.size() == 1) {
			root = node->children[0];
			root->parent = nullptr;
			delete node;
		}
		return;
	}

	BPlusTreeNode* parent = node->parent;
	int nodeIndex = -1;
	for (int i = 0; i < static_cast<int>(parent->children.size()); ++i) {
		if (parent->children[i] == node) {
			nodeIndex = i;
			break;
		}
	}

	if (nodeIndex > 0) {
		BPlusTreeNode* leftSibling = parent->children[nodeIndex - 1];
		if (static_cast<int>(leftSibling->keys.size()) > (order + 1) / 2 - 1) {
			node->keys.insert(node->keys.begin(), leftSibling->keys.back());
			node->children.insert(node->children.begin(), leftSibling->children.back());
			node->children.front()->parent = node;
			
			leftSibling->keys.pop_back();
			leftSibling->children.pop_back();
			
			parent->keys[nodeIndex - 1] = node->keys.front();
			return;
		}
	}
	if (nodeIndex < static_cast<int>(parent->children.size()) - 1) {
		BPlusTreeNode* rightSibling = parent->children[nodeIndex + 1];
		if (static_cast<int>(rightSibling->keys.size()) > (order + 1) / 2 - 1) {
			node->keys.push_back(rightSibling->keys.front());
			node->children.push_back(rightSibling->children.front());
			node->children.back()->parent = node;
	
			rightSibling->keys.erase(rightSibling->keys.begin());
			rightSibling->children.erase(rightSibling->children.begin());
	
			parent->keys[nodeIndex] = rightSibling->keys.front();
			return;
		}
	}

	if (nodeIndex > 0) {
		BPlusTreeNode* leftSibling = parent->children[nodeIndex - 1];

		leftSibling->keys.insert(leftSibling->keys.end(), node->keys.begin(), node->keys.end());
		leftSibling->children.insert(leftSibling->children.end(), node->children.begin(), node->children.end());
		for (auto child : leftSibling->children)	child->parent = leftSibling;
		
		parent->keys.erase(parent->keys.begin() + nodeIndex - 1);
		parent->children.erase(parent->children.begin() + nodeIndex);
		
		delete node;
		
		if (static_cast<int>(parent->keys.size()) < (order + 1) / 2 - 1)	handleInternalUnderflow(parent);
		
		return;
	}
	if (nodeIndex < static_cast<int>(parent->children.size()) - 1) {
		BPlusTreeNode* rightSibling = parent->children[nodeIndex + 1];
		
		node->keys.insert(node->keys.end(), rightSibling->keys.begin(), rightSibling->keys.end());
		node->children.insert(node->children.end(), rightSibling->children.begin(), rightSibling->children.end());
		for (auto child : rightSibling->children)	child->parent = node;

		parent->keys.erase(parent->keys.begin() + nodeIndex);
		parent->children.erase(parent->children.begin() + nodeIndex + 1);

		delete node;

		if (static_cast<int>(parent->keys.size()) < (order + 1) / 2 - 1)	handleInternalUnderflow(parent);

		return;
	}
}
void BPlusTree::handleLeafUnderflow(BPlusTreeNode* leaf) {
	if (!leaf->parent)	return;
	
	BPlusTreeNode* parent = leaf->parent;
	int leafIndex = -1;
	for (int i = 0; i < static_cast<int>(parent->children.size()); i++) {
		if (parent->children[i] == leaf) {
			leafIndex = i;
			break;
		}
	}

	if (leafIndex > 0) {
		BPlusTreeNode* leftSibling = parent->children[leafIndex - 1];
		if (static_cast<int>(leftSibling->keys.size()) > (order + 1) / 2 - 1) {
			leaf->keys.insert(leaf->keys.begin(), leftSibling->keys.back());
			leaf->values.insert(leaf->values.begin(), leftSibling->values.back());

			leftSibling->keys.pop_back();
			leftSibling->values.pop_back();

			parent->keys[leafIndex - 1] = leaf->keys.front();
			return;
		}
	}
	if (leafIndex < static_cast<int>(parent->children.size()) - 1) {
		BPlusTreeNode* rightSibling = parent->children[leafIndex + 1];
		if (static_cast<int>(rightSibling->keys.size()) > (order + 1) / 2 - 1) {
			leaf->keys.push_back(rightSibling->keys.front());
			leaf->values.push_back(rightSibling->values.front());
	
			rightSibling->keys.erase(rightSibling->keys.begin());
			rightSibling->values.erase(rightSibling->values.begin());
	
			parent->keys[leafIndex] = rightSibling->keys.front();
			return;
		}
	}

	mergeLeafNodes(leaf);
}
void BPlusTree::mergeLeafNodes(BPlusTreeNode* leaf) {
	if (!leaf->parent)	return;

	BPlusTreeNode* parent = leaf->parent;
	int leafIndex = -1;
	for (int i = 0; i < static_cast<int>(parent->children.size()); i++) {
		if (parent->children[i] == leaf) {
			leafIndex = i;
			break;
		}
	}

	if (leafIndex > 0) {
		BPlusTreeNode* leftSibling = parent->children[leafIndex - 1];

		leftSibling->keys.insert(leftSibling->keys.end(), leaf->keys.begin(), leaf->keys.end());
		leftSibling->values.insert(leftSibling->values.end(), leaf->values.begin(), leaf->values.end());
		leftSibling->nextLeaf = leaf->nextLeaf;
		
		parent->keys.erase(parent->keys.begin() + leafIndex - 1);
		parent->children.erase(parent->children.begin() + leafIndex);
		
		delete leaf;
		
		if (static_cast<int>(parent->keys.size()) < (order + 1) / 2 - 1)	handleInternalUnderflow(parent);
		
		return;
	}
	if (leafIndex < static_cast<int>(parent->children.size()) - 1) {
		BPlusTreeNode* rightSibling = parent->children[leafIndex + 1];
		
		leaf->keys.insert(leaf->keys.end(), rightSibling->keys.begin(), rightSibling->keys.end());
		leaf->values.insert(leaf->values.end(), rightSibling->values.begin(), rightSibling->values.end());
		leaf->nextLeaf = rightSibling->nextLeaf;

		parent->keys.erase(parent->keys.begin() + leafIndex);
		parent->children.erase(parent->children.begin() + leafIndex + 1);

		delete leaf;

		if (static_cast<int>(parent->keys.size()) < (order + 1) / 2 - 1)	handleInternalUnderflow(parent);

		return;
	}
}
void BPlusTree::remove(int key) {
	BPlusTreeNode* leaf = findLeafNode(key);
	if (!leaf)	return;
	int low = 0, high = leaf->keys.size() - 1, mid = -1, index = -1;
	while (low <= high){
		mid = (low + high) / 2;
		if (leaf->keys[mid] == key)	{
			index = mid;
			break;
		}
		if (leaf->keys[mid] > key)	high = mid - 1;
		else if (leaf->keys[mid] < key)	low = mid + 1;
	}

	if (index == -1)	return;
	leaf->keys.erase(leaf->keys.begin() + index);
	leaf->values.erase(leaf->values.begin() + index);

	if (index == 0 && leaf->parent != nullptr) {
		BPlusTreeNode* parent = leaf->parent;
		for (int i = 0; i < static_cast<int>(parent->keys.size()); i++){
			if (parent->children[i] == leaf && i > 0 && !leaf->keys.empty()) {
				parent->keys[i-1] = leaf->keys[0];
				break;
			}
		}
	}

	if (leaf != root && static_cast<int>(leaf->keys.size()) < (order + 1) / 2 - 1) {
		handleLeafUnderflow(leaf);
	}

	if (root->keys.empty() && !root->isLeaf) {
		root = root->children[0];
		root->parent = nullptr;
	}
}

void BPlusTree::printTree() {
	if (!root) {
		std::cout << "Tree is empty.\n";
		return;
	}

	std::queue<BPlusTreeNode*> q;
	q.push(root);

	while (!q.empty()) {
		int levelSize = q.size();
		for (int i = 0; i < levelSize; i++) {
			BPlusTreeNode* node = q.front();
			q.pop();
			if (node->isLeaf) {
				std::cout << "[Leaf: ";
				for (int i = 0; i < static_cast<int>(node->keys.size()); i++) std::cout << node->keys[i] << ':' << node->values[i] << ' ';
				std::cout << "]  ";
			}
			else {
				std::cout << "[Internal: ";
				for (int key : node->keys) std::cout << key << " ";
				std::cout << "]  ";

				for (BPlusTreeNode* child : node->children) {
					q.push(child);
				}
			}
		}
		std::cout << "\n";
	}
}


int BPlusTree::searchFile(int key) {
	BPlusTreeNode* leaf = findLeafNode(key);
	int low = 0, high = leaf->keys.size() - 1, mid = -1;
	while (low <= high){
		mid = (low + high) / 2;
		if (leaf->keys[mid] == key)	return leaf->values[mid];
		if (leaf->keys[mid] > key)	high = mid - 1;
		else if (leaf->keys[mid] < key)	low = mid + 1;
	}
	return	-1;
}
int BPlusTree::searchDir(int key) {
	BPlusTreeNode* leaf = findLeafNode(key);
	int low = 0, high = leaf->keys.size() - 1, mid = -1;
	while (low <= high){
		mid = (low + high) / 2;
		if (leaf->keys[mid] == key)	return leaf->values[mid];
		if (leaf->keys[mid] > key)	high = mid - 1;
		else if (leaf->keys[mid] < key)	low = mid + 1;
	}
	return	-1;
}

void BPlusTree::deleteTree() {
	std::queue<BPlusTreeNode*> q;
	q.push(root);

	while (!q.empty()) {
		BPlusTreeNode* node = q.front();
		q.pop();

		if (!node->isLeaf) {
			for (const auto& other : node->children) {
				q.push(other);
			}
		}

		delete node;
	}
}
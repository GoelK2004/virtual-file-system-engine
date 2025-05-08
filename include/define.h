#ifndef DEFINE_H
#define DEFINE_H

#include <cstdint>
#include <cstring>
#include <string>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>

#define DISK_SIZE 104857600	// 100MB
#define BLOCK_SIZE 4096	// 4KB (MINIMUM: 400)
#define FILE_NAME_LENGTH 32
#define MAX_EXTENTS 5
#define ORDER 5 // 30 MAX due to block size
#define MAX_FILES 3000

#define TOTAL_BLOCKS (DISK_SIZE / BLOCK_SIZE)
#define SUPER_BLOCKS (1)
#define BITMAP_BLOCKS ((((TOTAL_BLOCKS + 7)/ 8) + BLOCK_SIZE - 1) / BLOCK_SIZE)
#define BPLUS_LEAF_NODES (MAX_FILES + ORDER - 2) / (ORDER - 1)
#define BPLUS_TREE_BLOCKS (BPLUS_LEAF_NODES + BPLUS_INTERNAL_NODES)
#define ROOT_DIR_BLOCKS (MAX_FILES + ORDER - 1) / (ORDER - 1)

#define SUPER_BLOCK_START (0)
#define BITMAP_START (SUPER_BLOCK_START + SUPER_BLOCKS)
#define BPLUS_TREE_START (BITMAP_START + BITMAP_BLOCKS)
#define ROOT_DIR_START (BPLUS_TREE_START + BPLUS_TREE_BLOCKS)
#define DATA_START (ROOT_DIR_START + ROOT_DIR_BLOCKS)

#define PERMISSION_READ 0b100
#define PERMISSION_WRITE 0b010
#define PERMISSION_EXECUTE 0b001

#define PERMISSION_OWNER_SHIFT 6
#define PERMISSION_GROUP_SHIFT 3
#define PERMISSION_OTHER_SHIFT 0

#define ATTRIBUTES_HIDDEN 0b100
#define ATTRIBUTES_SYSTEM 0b010
#define ATTRIBUTES_ARCHIVE 0b001

#define USER_NAME_LENGTH 15

namespace pr{
    constexpr int computeInternalNodes(int leafNodes, int order) {
        int total = 0, current = leafNodes;
        while (current > 1) {
            int parent = (current + order - 1) / order;
            total += parent;
            current = parent;
        }
        return total;
    }
}
constexpr int BPLUS_INTERNAL_NODES = pr::computeInternalNodes(BPLUS_LEAF_NODES, ORDER);

#endif
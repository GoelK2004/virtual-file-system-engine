# Virtual File System (VFS)

A high-performance, modular, and feature-rich **Virtual File System** built from scratch in **C++**, designed to emulate real-world file systems like ext4 or NTFS. This system includes advanced components such as journaling, permission management, metadata caching, and multi-filesystem mounting, making it a complete simulation of how modern file systems work.

---

## Features

### Core File System
- Disk-based file system built using a raw `myDisk.img` file
- Custom block size support for flexibility
- File and directory creation with user-defined sizes

### Metadata Management
- `FileEntry` system supporting:
  - File name, size, extents
  - Timestamps (created, modified, accessed)
  - Permissions (owner, group, others)
  - Attributes (hidden, read-only, system, archive)
- Centralized metadata table with B+ Tree indexing for fast access

### 🗃️ Efficient File Allocation
- **Bitmap + Extents** used for block allocation
- Reduced fragmentation and fast read/write performance

### Journaling System
- Write-Ahead Logging for crash recovery
- Journaling of create, delete, write, rename, and permission change operations
- Recovery on boot using uncommitted journal entries

### User and Permission Management
- User/group management (`userAdd`, `login`, `chown`, `chmod`, `chgrp`)
- `whoami` command to show current session
- Secure password handling
- UID/GID mapping and enforcement of file access rights

### Metadata Caching
- In-memory caching for Superblock, Bitmap, and Metadata
- Reduced disk I/O on frequent access

### Fault Tolerance & Rollback
- Journaling recovery
- Safe rollback of metadata and data block allocations during crashes

### Directory Management
- Nested directory support
- `cd`, `mkdir`, `rmdir`, and `tree` navigation
- Path resolution and absolute/relative path support

### Mounting System
- Mount and unmount multiple file systems at different virtual mount paths
- Independent management of mounted disks
- CLI supports switching between mounted filesystems

---

## Commands Implemented

| Command       | Description |
|---------------|-------------|
| `help`        | View all the available command |
| `create`      | Create a file with optional size |
| `write`       | Write or overwrite data to a file |
| `append`      | Append data to a file |
| `read`        | Read file content |
| `remove`      | Delete a file |
| `rename`      | Rename a file or directory |
| `mkdir`       | Create a new directory |
| `rmdir`       | Remove a directory |
| `cd`          | Change working directory |
| `ls`          | List directory contents |
| `stat`        | Show file metadata |
| `chmod`       | Change file permissions |
| `chown`       | Change file ownership |
| `chgrp`       | Change file group |
| `whoami`      | Show current logged-in user |
| `login`       | Authenticate user |
| `userAdd`     | Add new user with group |
| `showUsers`   | Display user table |
| `showGroups`  | Display group table |
| `tree`        | Display directory hierarchy |
| `format`      | Format and initialize the file system |
| `exit`        | Exit file system |

---

## Project Structure

```bash
├── include/           # Header files
├── journal/           # Journal files
├── src/               # All C++ source files (including main.cpp)
├── Makefile           # Updated build script
├── README.md          # Project documentation
└── LICENSE            # License file
 - Entry point: main.cpp
```

## License

**All Rights Reserved.**

This project is shared for educational and reference purposes only.
Reproduction, reuse, distribution, or modification is not permitted without prior written consent from the author.

For inquiries, reach out at goelkartikey2250@gmail.com

## 📬 Contact
If you're a recruiter or engineer interested in the design and implementation of this file system, feel free to reach out:
📧 goelkartikey2250@gmail.com

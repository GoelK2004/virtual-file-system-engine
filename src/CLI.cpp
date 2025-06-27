#include "CLI.h"

std::vector<std::string> parseInput(const std::string& input) {
	std::stringstream actualString(input);
	std::string token;
	std::vector<std::string> tokens;
	bool inQuotes = false;

	for (size_t i = 0; i < input.length(); i++) {
		char c = input[i];
		if (c == '"') {
			inQuotes = !inQuotes;
			if (!inQuotes && !token.empty()) {
				tokens.push_back(token);
				token.clear();
			} 
		} else if (std::isspace(c) && !inQuotes) {
			if (!token.empty()) {
				tokens.push_back(token);
				token.clear();
			}
		} else {
			token += c;
		}
	}
	if (!token.empty()) {
		tokens.push_back(token);
		token.clear();
	}

	return tokens;
}

std::string CommandLineInterface::createPath(ClientSession* session) {
	std::string path = vfs->createPath(session);
	int del = path.find(')');
	std::string newPath = path.substr(0, del) + '@' + FSName + path.substr(del);
	return newPath;
}


CommandLineInterface::CommandLineInterface(VFSManager* filesys, const std::string& fsName) : vfs(filesys) {
	FSName = fsName;
}

std::string CommandLineInterface::runCLI(std::string buffer, ClientSession* session) {
	std::string input = buffer;
	// std::string path = vfs->createPath();
	// while (true) {
	// path = vfs->createPath();
	// int del = path.find(')');
	// std::string newPath = path.substr(0, del) + '@' + FSName + path.substr(del);
	// std::cout << newPath << " > ";
	auto args = parseInput(input);
	if (args.empty())	return "";

	const std::string& cmd = args[0];
	if (cmd == "help") {
		vfs->printHelp(session);
		// return vfs->get_msg();
	}
	// else if (cmd == "format")	continue;
	// else if (cmd == "load")	continue;
	else if (cmd == "ls" && args.size() == 1) {
		vfs->ls(session);
		// return vfs->get_msg();
	}
	else if (cmd == "cd" && args.size() == 2)	vfs->cd(args[1], session);
	else if (cmd == "mkdir" && args.size() == 2) {
		vfs->mkdir(args[1], session);
		// return vfs->get_msg();
	}
	else if (cmd == "rmdir" && args.size() == 2) {
		vfs->rmdir(args[1], session);
		// return vfs->get_msg();
	}
	else if (cmd == "rm" && args.size() == 3 && args[1] == "-r") {
		vfs->rmrdir(args[2], session);
		// return vfs->get_msg();
	}
	else if (cmd == "create" && args.size() == 2) {
		vfs->create(args[1], session);
		// return vfs->get_msg();
	}
	else if (cmd == "create" && args.size() == 3) {
		vfs->create(args[1], std::stoi(args[2]), session);
		// return vfs->get_msg();
	}
    else if (cmd == "write" && args.size() == 3) {
		vfs->write(args[1], args[2], session);
		// return vfs->get_msg();
	}
    else if (cmd == "append" && args.size() == 3) {
		vfs->append(args[1], args[2], session);
		// return vfs->get_msg();
	}
    else if (cmd == "read" && args.size() == 2) {
		return vfs->read(args[1], session);
	}
	else if (cmd == "rm" && args.size() == 2) {
		vfs->remove(args[1], session);
		// vfs->get_msg();
	}
	else if (cmd == "rename" && args.size() == 3) {
		vfs->rename(args[1], args[2], session);
		// return vfs->get_msg();
	}
	else if (cmd == "stat" && args.size() == 2) {
		vfs->stat(args[1], session);
		// return vfs->get_msg();
	}
	else if (cmd == "chmod" && args.size() == 3)	vfs->chmod(args[1], std::stoi(args[2], nullptr, 8), session);
	else if (cmd == "chown" && args.size() == 3)	vfs->chown(args[1], args[2], session);
	else if (cmd == "chgrp" && args.size() == 3)	vfs->chgrp(args[1], std::stoul(args[2]), session);
	else if (cmd == "whoami" && args.size() == 1) {
		vfs->whoami(session);
		// return vfs->get_msg();
	}
	else if (cmd == "login" && args.size() == 3)	vfs->login(args[1], args[2], session);
	else if (cmd == "useradd" && args.size() == 3)	vfs->userAdd(args[1], args[2], session);
	else if (cmd == "useradd" && args.size() == 4)	vfs->userAdd(args[1], args[2], session, std::stoul(args[3]));
	else if (cmd == "showusr" && args.size() == 1)	vfs->showUsers(session);
	else if (cmd == "showgrp" && args.size() == 1)	vfs->showGroups(session);
	else if (cmd == "tree" && args.size() == 1)	vfs->tree(session);
	else if (cmd == "btree" && args.size() == 1)	vfs->bTree();
	else if (cmd == "show" && args.size() == 1) {
		vfs->show();
	}
	else if (cmd == "exit") {
		std::cout << "Exiting file system, user: " << std::string(session->user.userName) << '\n';
        // break;
    } else	return "Unknown command. Type 'help' for available commands.\n";
	// }
	return "";
}

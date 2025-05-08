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

CommandLineInterface::CommandLineInterface(VFSManager* fileSystem) : vfs(fileSystem) {}

void CommandLineInterface::runCLI() {
	std::string input;
	std::string path = vfs->createPath();
	while (true) {
		path = vfs->createPath();
		std::cout << path << '>';
		std::getline(std::cin, input);
		auto args = parseInput(input);
		if (args.empty())	continue;

		const std::string& cmd = args[0];
		if (cmd == "help")	vfs->printHelp();
		else if (cmd == "format")	continue;
		else if (cmd == "load")	continue;
		else if (cmd == "ls" && args.size() == 1)	vfs->ls();
		else if (cmd == "cd" && args.size() == 2)	vfs->cd(args[1]);
		else if (cmd == "mkdir" && args.size() == 2)	vfs->mkdir(args[1]);
		else if (cmd == "rmdir" && args.size() == 2)	vfs->rmdir(args[1]);
		else if (cmd == "create" && args.size() == 2)	vfs->create(args[1]);
		else if (cmd == "create" && args.size() == 3)	vfs->create(args[1], std::stoi(args[2]));
        else if (cmd == "write" && args.size() == 3)	vfs->write(args[1], args[2]);
        else if (cmd == "append" && args.size() == 3)	vfs->append(args[1], args[2]);
        else if (cmd == "read" && args.size() == 2)	vfs->read(args[1]);
        else if (cmd == "rm" && args.size() == 2)	vfs->remove(args[1]);
        else if (cmd == "rename" && args.size() == 3)	vfs->rename(args[1], args[2]);
        else if (cmd == "stat" && args.size() == 2)	vfs->stat(args[1]);
		else if (cmd == "chmod" && args.size() == 3)	vfs->chmod(args[1], std::stoi(args[2], nullptr, 8));
		else if (cmd == "chown" && args.size() == 3)	vfs->chown(args[1], args[2]);
		else if (cmd == "chgrp" && args.size() == 3)	vfs->chgrp(args[1], std::stoul(args[2]));
		else if (cmd == "whoami" && args.size() == 1)	vfs->whoami();
		else if (cmd == "login" && args.size() == 3)	vfs->login(args[1], args[2]);
		else if (cmd == "useradd" && args.size() == 3)	vfs->userAdd(args[1], args[2]);
		else if (cmd == "useradd" && args.size() == 4)	vfs->userAdd(args[1], args[2], std::stoul(args[3]));
		else if (cmd == "showusr" && args.size() == 1)	vfs->showUsers();
		else if (cmd == "showgrp" && args.size() == 1)	vfs->showGroups();
		else if (cmd == "tree" && args.size() == 1)	vfs->tree();
        else if (cmd == "exit") {
            std::cout << "Exiting file system.\n";
            break;
        } else	std::cout << "Unknown command. Type 'help' for available commands.\n";
	}
}

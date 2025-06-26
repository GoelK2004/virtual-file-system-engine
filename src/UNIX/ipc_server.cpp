#include "UNIX/ipc_server.h"

// std::vector<int> fd_socket(MAX_CLIENT_SUPPORT, -1);
std::map<int, ClientSession*> sessions;
int connected = 0;

ClientSession* get_fd_socket(int data_socket) {
	if (sessions.find(data_socket) != sessions.end())	return sessions.find(data_socket)->second;
	return nullptr;
}

static void add_fd_socket(int sock_fd) {
	if (sock_fd >= FD_SETSIZE) {
        std::cerr << "[ERROR] File descriptor " << sock_fd << " exceeds FD_SETSIZE limit (" << FD_SETSIZE << ")\n";
        close(sock_fd);
        return;
    }
	
	if (connected >= MAX_CLIENT_SUPPORT) {
		close(sock_fd);
		std::cerr << "Maximum client limit reached. Socket closed.\n";
		return;
	}
	
	if (sessions.find(sock_fd) == sessions.end()) {
		ClientSession* clientSession = new ClientSession();
	    sessions.insert({sock_fd, clientSession});
	    connected++;
	    return;
	}
}

static void remove_fd_socket(int sock_fd) {
	if (sessions.find(sock_fd) != sessions.end()) {
		delete sessions.find(sock_fd)->second;
		sessions.find(sock_fd)->second = nullptr;
		sessions.erase(sock_fd);
		connected--;
	}
}

static void refresh_fd_socket(fd_set* fd_set_ptr) {
	FD_ZERO(fd_set_ptr);
	for (auto it = sessions.begin(); it != sessions.end(); it++) {
		if (it->first >= FD_SETSIZE)	continue;
		FD_SET(it->first, fd_set_ptr);
	}
}

static int get_max_fd() {
	int max = -1;
	for (auto it = sessions.begin(); it != sessions.end(); it++) {
		if (max < it->first) {
			max = it->first;
		}
	}
	return max;
}

void cleanup() {
	for (auto it = sessions.begin(); it != sessions.end();) {
		delete it->second;
		it->second = nullptr;
		close(it->first);
		it = sessions.erase(it);
	}
	unlink(SOCKET_NAME);

	if (fs)	delete fs;
	fs = nullptr;
}

bool is_server_running() {
    return access(SOCKET_NAME, F_OK) == 0;
}

MountManager mountManager;
FileSystemInterface* fs = nullptr;

int ret;
fd_set readfds;
char buffer[BUFFER_SIZE];
std::string command;

void handle_client(int data_socket, ClientSession* session, CommandLineInterface cli) {
	while (session->active){
		memset(buffer, 0, BUFFER_SIZE);
		ret = read(data_socket, buffer, BUFFER_SIZE);
		command = std::string(buffer);
		if (ret == -1) {
			perror("read");
			break;
		} else if (ret == 0) {
			remove_fd_socket(data_socket);
			break;
		}
		if (command.empty())	continue;
		else if (command == "requestPath") {
			std::string response = cli.createPath(session);
			int total = response.size();
			ret = write(data_socket, &total, sizeof(total));
			if (ret == -1) {
				perror("write");
				break;
			}

			int done = 0;
			while (done < total) {
				int chunk = std::min(BUFFER_SIZE, static_cast<int>(total - done));
				ret = write(data_socket, response.data() + done, chunk);
				if (ret == -1) {
					perror("write");
					exit(EXIT_FAILURE);
				}
				done += chunk;
			}
		} 
		else {
			std::string content = cli.runCLI(command, session);
			std::string response = session->msg;
			if (content != "")	response = content;
			int total = response.size();
			ret = write(data_socket, &total, sizeof(total));
			if (ret == -1) {
				perror("write");
				break;
			}

			int done = 0;
			while (done < total) {
				int chunk = std::min(BUFFER_SIZE, static_cast<int>(total - done));
				ret = write(data_socket, response.data() + done, chunk);
				if (ret == -1) {
					perror("write");
					break;
				}
				done += chunk;
			}
		}
	}
}

void run_server() {

	sockaddr_un name;
	
	int connection_socket;

	unlink(SOCKET_NAME);
	
	connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connection_socket == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	memset(&name, 0, sizeof(sockaddr_un));
	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);
	name.sun_path[sizeof(name.sun_path) - 1] = '\0';
	
	ret = bind(connection_socket, (const sockaddr*)&name, sizeof(sockaddr_un));
	if (ret == -1) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	
	ret = listen(connection_socket, 10);
	if (ret == -1) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
		
	try{
		add_fd_socket(connection_socket);
		
		fs = new System("./disks/myDisk.img");
		VFSManager* vfsManager = new VFSManager();
		vfsManager->mount(fs); 
		mountManager.mount("/dir1", "/disks/myDisk.img", "rootFS", vfsManager);
		std::cout << "-----------------------------------------------------\n";
		CommandLineInterface cli(mountManager.getCurrentVFS(), mountManager.getCurrentFSName());
		
		while (true) {
			refresh_fd_socket(&readfds);
			int data_socket = accept(connection_socket, NULL, NULL);
			if (data_socket == -1) {
				perror("accept");
				exit(EXIT_FAILURE);
			}

			add_fd_socket(data_socket);
			std::thread(handle_client, data_socket, get_fd_socket(data_socket), std::ref(cli)).detach();
		}
		remove_fd_socket(connection_socket);
		close(connection_socket);
		unlink(SOCKET_NAME);
	}
	catch (const std::exception& e){
		std::cout << e.what() << '\n';
	}
	cleanup();
}
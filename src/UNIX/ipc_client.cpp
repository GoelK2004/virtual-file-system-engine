#include "UNIX/ipc_client.h"
#include "UNIX/ipc_server.h"
#include <limits>

void run_client() {
	struct sockaddr_un addr;
	
	int data_socket;
	int ret;
	std::string buffer;
	
	data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	if (data_socket == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_NAME, sizeof(addr.sun_path) - 1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	ret = connect(data_socket, (const sockaddr*)&addr, sizeof(sockaddr_un));
	if (ret == -1) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	int last = 0;
	while(true) {
		buffer = "requestPath";
		ret = write(data_socket, buffer.data(), BUFFER_SIZE);
		if (ret == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		int total;
		ret = read(data_socket, &total, sizeof(total));
		if (ret == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
		int track = 0;
		while (track < total) {
			int chunk = std::min(BUFFER_SIZE, static_cast<int>(total - track));
			buffer.resize(chunk);
			int bytes = read(data_socket, buffer.data(), chunk);
			if (bytes == -1) {
				perror("read data");
				exit(EXIT_FAILURE);
			}
			std::cout.write(buffer.data(), bytes);
			std::cout.flush();
			track += chunk;
		}
		std::cout << " >> ";
		buffer.clear();
		std::cin.clear();
		std::getline(std::cin, buffer);
		if (buffer == "exit") {
			shutdown(data_socket, SHUT_RD);
			last = 1;
		}
		else {
			ret = write(data_socket, buffer.data(), buffer.length());
			if (ret == -1) {
				perror("write");
				exit(EXIT_FAILURE);
			}
		}
		ret = read(data_socket, &total, sizeof(total));
		if (ret == -1) {
			perror("read size");
			exit(EXIT_FAILURE);
		}
		int received = 0;
		while (received < total) {
			int chunk = std::min(BUFFER_SIZE, static_cast<int>(total - received));
			buffer.resize(chunk);
			int bytes = read(data_socket, buffer.data(), chunk);
			if (bytes == -1) {
				perror("read data");
				exit(EXIT_FAILURE);
			}
			std::cout.write(buffer.data(), bytes);
			std::cout.flush();
			received += chunk;
		}
		if (last)	break;
	}

	close(data_socket);
}
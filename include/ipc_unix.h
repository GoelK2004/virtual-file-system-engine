#pragma once

#include <string>

// Starts a UNIX socket server bound to SOCKET_PATH.
// Returns the listening socket file descriptor or -1 on error.
int setupUnixSocketServer(const char* socket_path);

// Accepts a client connection, returns client socket fd or -1 on error.
int acceptClientConnection(int server_fd);

// Reads a message from the socket fd into a std::string.
// Returns true on success, false on failure or disconnect.
bool readMessage(int socket_fd, std::string& out_message);

// Sends a message over socket_fd.
// Returns true on success, false on failure.
bool sendMessage(int socket_fd, const std::string& message);

// Sends a command to the socket at socket_path, receives the response.
// Used by client to send commands and get replies.
std::string sendCommand(const std::string& socket_path, const std::string& command);
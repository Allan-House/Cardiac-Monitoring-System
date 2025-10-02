#include "tcp_file_server.h"
#include "logger.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

TCPFileServer::TCPFileServer(uint16_t port, const std::string& data_directory)
  : port_ {port},
    data_directory_ {data_directory},
    server_socket_ {-1},
    running_ {false},
    connections_handled_ {0},
    bytes_transferred_ {0}
{
  // Empty constructor
}


TCPFileServer::~TCPFileServer() {
  Stop();
}


bool TCPFileServer::Init() {
  LOG_INFO("Initializing TCP File Server on port %d", port_);
 
  // Verify data directory exists
  if (!std::filesystem::exists(data_directory_)) {
    LOG_ERROR("Data directory does not exist: %s", data_directory_.c_str());
    return false;
  }
 
  // Create socket
  server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_ < 0) {
    LOG_ERROR("Failed to create socket: %s", strerror(errno));
    return false;
  }
 
  // Set socket options to reuse address
  int opt = 1;
  if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    LOG_WARN("Failed to set SO_REUSEADDR: %s", strerror(errno));
  }
 
  // Bind socket to port
  struct sockaddr_in address;
  std::memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port_);
 
  if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
    LOG_ERROR("Failed to bind socket to port %d: %s", port_, strerror(errno));
    close(server_socket_);
    server_socket_ = -1;
    return false;
  }
 
  // Listen for connections
  if (listen(server_socket_, 5) < 0) {
    LOG_ERROR("Failed to listen on socket: %s", strerror(errno));
    close(server_socket_);
    server_socket_ = -1;
    return false;
  }
 
  LOG_SUCCESS("TCP Server initialized on port %d", port_);
  LOG_INFO("Serving files from: %s", data_directory_.c_str());
 
  return true;
}


void TCPFileServer::Start() {
  if (running_.load()) {
    LOG_WARN("Server already running");
    return;
  }
 
  running_ = true;
  server_thread_ = std::thread(&TCPFileServer::ServerLoop, this);
  LOG_INFO("TCP Server thread started");
}


void TCPFileServer::Stop() {
  if (!running_.load()) {
    return;
  }
 
  LOG_INFO("Stopping TCP Server...");
  running_ = false;
 
  // Close server socket to unblock accept()
  if (server_socket_ >= 0) {
    shutdown(server_socket_, SHUT_RDWR);
    close(server_socket_);
    server_socket_ = -1;
  }
 
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
 
  LOG_INFO("TCP Server stopped. Connections handled: %zu, Bytes transferred: %zu",
           connections_handled_.load(), bytes_transferred_.load());
}


void TCPFileServer::WaitForCompletion() {
  if (server_thread_.joinable()) {
    server_thread_.join();
  }
}


void TCPFileServer::ServerLoop() {
  LOG_INFO("Server listening for connections...");
 
  while (running_.load()) {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
   
    int client_socket = accept(server_socket_,
                               (struct sockaddr*)&client_address,
                               &client_len);
   
    if (client_socket < 0) {
      if (running_.load()) {
        LOG_ERROR("Failed to accept connection: %s", strerror(errno));
      }
      break;
    }
   
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    LOG_INFO("Client connected from %s:%d", client_ip, ntohs(client_address.sin_port));
   
    HandleClient(client_socket);
   
    close(client_socket);
    connections_handled_++;
  }
 
  LOG_INFO("Server loop exited");
}


void TCPFileServer::HandleClient(int client_socket) {
  char buffer[256];
  std::memset(buffer, 0, sizeof(buffer));
 
  // Read command from client
  ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
  if (bytes_read <= 0) {
    LOG_WARN("Failed to read from client or client disconnected");
    return;
  }
 
  buffer[bytes_read] = '\0';
  std::string command(buffer);
 
  // Remove trailing newline/whitespace
  command.erase(command.find_last_not_of(" \n\r\t") + 1);
 
  LOG_INFO("Received command: %s", command.c_str());
 
  if (command == "LIST") {
    HandleListCommand(client_socket);
  }
  else if (command == "GET_LATEST") {
    HandleGetLatestCommand(client_socket);
  }
  else if (command.substr(0, 4) == "GET ") {
    std::string filename = command.substr(4);
    HandleGetCommand(client_socket, filename);
  }
  else {
    SendError(client_socket, "ERROR: Unknown command. Use LIST, GET <filename>, or GET_LATEST");
  }
}


void TCPFileServer::HandleListCommand(int client_socket) {
  auto files = GetAvailableFiles();
 
  if (files.empty()) {
    SendError(client_socket, "ERROR: No files available");
    return;
  }
 
  std::ostringstream response;
  response << "OK " << files.size() << "\n";
  for (const auto& file : files) {
    response << file << "\n";
  }
 
  std::string response_str = response.str();
  send(client_socket, response_str.c_str(), response_str.length(), 0);
 
  LOG_INFO("Sent file list: %zu files", files.size());
}


void TCPFileServer::HandleGetCommand(int client_socket, const std::string& filename) {
  // Sanitize filename (prevent directory traversal)
  if (filename.find("..") != std::string::npos || filename.find("/") != std::string::npos) {
    SendError(client_socket, "ERROR: Invalid filename");
    return;
  }
 
  std::string filepath = data_directory_ + "/" + filename;
 
  if (!std::filesystem::exists(filepath)) {
    SendError(client_socket, "ERROR: File not found");
    LOG_WARN("Client requested non-existent file: %s", filename.c_str());
    return;
  }
 
  if (!SendFile(client_socket, filepath)) {
    SendError(client_socket, "ERROR: Failed to send file");
  }
}


void TCPFileServer::HandleGetLatestCommand(int client_socket) {
  std::string latest_file = FindLatestFile();
 
  if (latest_file.empty()) {
    SendError(client_socket, "ERROR: No files available");
    return;
  }
 
  std::string filepath = data_directory_ + "/" + latest_file;
 
  if (!SendFile(client_socket, filepath)) {
    SendError(client_socket, "ERROR: Failed to send file");
  }
}


std::vector<std::string> TCPFileServer::GetAvailableFiles() const {
  std::vector<std::string> files;
 
  try {
    for (const auto& entry : std::filesystem::directory_iterator(data_directory_)) {
      if (entry.is_regular_file()) {
        files.push_back(entry.path().filename().string());
      }
    }
  } catch (const std::exception& e) {
    LOG_ERROR("Error reading directory: %s", e.what());
  }
 
  std::sort(files.begin(), files.end());
  return files;
}


std::string TCPFileServer::FindLatestFile() const {
  auto files = GetAvailableFiles();
 
  if (files.empty()) {
    return "";
  }
 
  // Files are named with timestamp: cardiac_data_YYYYMMDD_HHMMSS.ext
  // Sorting alphabetically gives us chronological order
  // Return last one (most recent)
  return files.back();
}

bool TCPFileServer::SendFile(int client_socket, const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
 
  if (!file.is_open()) {
    LOG_ERROR("Failed to open file: %s", filepath.c_str());
    return false;
  }
 
  // Get file size
  std::streamsize file_size = file.tellg();
  file.seekg(0, std::ios::beg);
 
  // Send OK response with file size
  std::ostringstream header;
  header << "OK " << file_size << "\n";
  std::string header_str = header.str();
  send(client_socket, header_str.c_str(), header_str.length(), 0);
 
  LOG_INFO("Sending file: %s (%ld bytes)",
           std::filesystem::path(filepath).filename().c_str(), file_size);
 
  // Send file data in chunks
  const size_t chunk_size = 8192;
  char buffer[chunk_size];
  size_t total_sent = 0;
 
  while (file.read(buffer, chunk_size) || file.gcount() > 0) {
    std::streamsize bytes_read = file.gcount();
    ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
   
    if (bytes_sent < 0) {
      LOG_ERROR("Failed to send data: %s", strerror(errno));
      return false;
    }
   
    total_sent += bytes_sent;
  }
 
  bytes_transferred_ += total_sent;
 
  LOG_SUCCESS("File sent successfully: %zu bytes", total_sent);
  return true;
}


void TCPFileServer::SendError(int client_socket, const std::string& error_message) {
  std::string response = error_message + "\n";
  send(client_socket, response.c_str(), response.length(), 0);
  LOG_WARN("Sent error to client: %s", error_message.c_str());
}

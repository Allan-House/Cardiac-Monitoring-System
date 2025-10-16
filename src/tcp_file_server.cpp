#include "tcp_file_server.h"

#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "logger.h"

TCPFileServer::TCPFileServer(uint16_t port, const std::string& data_directory)
  : port_ {port},
    data_directory_ {data_directory},
    server_socket_ {-1},
    client_socket_ {-1},
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


void TCPFileServer::Run() {
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
 
  // Close client socket if connected
  {
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_socket_ >= 0) {
      shutdown(client_socket_, SHUT_RDWR);
      close(client_socket_);
      client_socket_ = -1;
    }
  }
 
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


void TCPFileServer::ServerLoop() {
  LOG_INFO("Server listening for connections...");
  
  // Set accept timeout to allow periodic check of running_ flag
  struct timeval timeout;
  timeout.tv_sec = 1;   // 1 second timeout
  timeout.tv_usec = 0;
  
  if (setsockopt(server_socket_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
    LOG_WARN("Failed to set socket timeout: %s", strerror(errno));
  }
 
  while (running_.load()) {
    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);
   
    int new_client_socket = accept(server_socket_,
                                    (struct sockaddr*)&client_address,
                                    &client_len);
   
    if (new_client_socket < 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        // Timeout - normal, continue loop
        continue;
      }
      if (running_.load()) {
        LOG_ERROR("Failed to accept connection: %s", strerror(errno));
      }
      break;
    }
   
    // Client connected
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    LOG_INFO("Client connected from %s:%d", client_ip, ntohs(client_address.sin_port));
    
    // Store client socket
    {
      std::lock_guard<std::mutex> lock(client_mutex_);
      
      // Close previous client if any
      if (client_socket_ >= 0) {
        LOG_WARN("New client connected, closing previous connection");
        close(client_socket_);
      }
      
      client_socket_ = new_client_socket;
    }
    
    connections_handled_++;
    
    // If files are already ready, send immediately
    if (files_ready_.load()) {
      SendAvailableFiles();
    }
  }
 
  LOG_INFO("Server loop exited");
}


bool TCPFileServer::HasConnectedClient() const {
  std::lock_guard<std::mutex> lock(client_mutex_);
  return client_socket_ >= 0;
}


void TCPFileServer::SendAvailableFiles() {
  std::lock_guard<std::mutex> lock(client_mutex_);
  
  files_ready_ = true;
  
  if (client_socket_ < 0) {
    LOG_INFO("No client connected - files will not be sent");
    return;
  }
  
  LOG_INFO("Sending files to connected client...");
  
  auto files = GetAvailableFiles();
  
  if (files.empty()) {
    SendError(client_socket_, "ERROR: No files available");
    LOG_WARN("No files available to send");
    return;
  }
  
  // Send file count
  std::ostringstream header;
  header << "FILES " << files.size() << "\n";
  std::string header_str = header.str();
  
  if (send(client_socket_, header_str.c_str(), header_str.length(), 0) < 0) {
    LOG_ERROR("Failed to send file count: %s", strerror(errno));
    close(client_socket_);
    client_socket_ = -1;
    return;
  }
  
  // Send each file
  for (const auto& filename : files) {
    std::string filepath = data_directory_ + "/" + filename;
    
    if (!SendFile(client_socket_, filepath)) {
      LOG_ERROR("Failed to send file: %s", filename.c_str());
      close(client_socket_);
      client_socket_ = -1;
      return;
    }
  }
  
  LOG_SUCCESS("All files sent successfully");
  
  // Close connection after sending
  close(client_socket_);
  client_socket_ = -1;
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


bool TCPFileServer::SendFile(int client_socket, const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
 
  if (!file.is_open()) {
    LOG_ERROR("Failed to open file: %s", filepath.c_str());
    return false;
  }
 
  // Get file size
  std::streamsize file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  // Get filename without path
  std::string filename = std::filesystem::path(filepath).filename().string();
 
  // Send file metadata: "FILE <filename> <size>\n"
  std::ostringstream metadata;
  metadata << "FILE " << filename << " " << file_size << "\n";
  std::string metadata_str = metadata.str();
  
  if (send(client_socket, metadata_str.c_str(), metadata_str.length(), 0) < 0) {
    LOG_ERROR("Failed to send file metadata: %s", strerror(errno));
    return false;
  }
 
  LOG_INFO("Sending file: %s (%ld bytes)", filename.c_str(), file_size);
 
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
 
  LOG_SUCCESS("File sent successfully: %s (%zu bytes)", filename.c_str(), total_sent);
  return true;
}


void TCPFileServer::SendError(int client_socket, const std::string& error_message) {
  std::string response = error_message + "\n";
  send(client_socket, response.c_str(), response.length(), 0);
  LOG_WARN("Sent error to client: %s", error_message.c_str());
}
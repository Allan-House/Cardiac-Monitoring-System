#ifndef TCP_FILE_SERVER_H_
#define TCP_FILE_SERVER_H_


#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
#include <thread>


/**
* @brief Simple TCP server for file transfer
*
* Listens on a specified port and handles client requests to:
* - LIST: Get list of available files
* - GET <filename>: Download specific file
* - GET_LATEST: Download most recent file (by timestamp)
*
* Thread-safe and supports sequential client connections.
*/
class TCPFileServer {
  private:
  // Configuration
  uint16_t port_;
  std::string data_directory_;
 
  // Server state
  int server_socket_;
  std::atomic<bool> running_;
  std::thread server_thread_;
 
  // Statistics
  std::atomic<size_t> connections_handled_;
  std::atomic<size_t> bytes_transferred_;
 
  public:
  /**
   * @brief Constructs TCP file server
   * @param port Port to listen on (default: 8080)
   * @param data_directory Directory containing files to serve (default: "data/processed")
   */
  explicit TCPFileServer(uint16_t port = 8080,
                         const std::string& data_directory = "data/processed");
 
  ~TCPFileServer();
 
  TCPFileServer(const TCPFileServer&) = delete;
  TCPFileServer& operator=(const TCPFileServer&) = delete;
 
  /**
   * @brief Initializes server socket and binds to port
   * @return true if successful, false otherwise
   */
  bool Init();
 
  /**
   * @brief Starts server thread to accept connections
   */
  void Start();
 
  /**
   * @brief Stops server and closes all connections
   */
  void Stop();
 
  /**
   * @brief Blocks until server thread finishes
   */
  void WaitForCompletion();
 
  // Statistics
  bool is_running() const {return running_.load();}
  size_t get_connections_handled() const {return connections_handled_.load();}
  size_t get_bytes_transferred() const {return bytes_transferred_.load();}
 
  private:
  /**
   * @brief Main server loop - accepts and handles client connections
   */
  void ServerLoop();
 
  /**
   * @brief Handles a single client connection
   * @param client_socket Socket descriptor for connected client
   */
  void HandleClient(int client_socket);
 
  /**
   * @brief Processes LIST command
   * @param client_socket Socket to send response
   */
  void HandleListCommand(int client_socket);
 
  /**
   * @brief Processes GET command
   * @param client_socket Socket to send file data
   * @param filename Name of file to send
   */
  void HandleGetCommand(int client_socket, const std::string& filename);
 
  /**
   * @brief Processes GET_LATEST command
   * @param client_socket Socket to send file data
   */
  void HandleGetLatestCommand(int client_socket);
 
  /**
   * @brief Gets list of files in data directory
   * @return Vector of filenames
   */
  std::vector<std::string> GetAvailableFiles() const;
 
  /**
   * @brief Finds most recent file by timestamp in filename
   * @return Filename of most recent file, or empty string if none found
   */
  std::string FindLatestFile() const;
 
  /**
   * @brief Sends file contents over socket
   * @param client_socket Socket to send data
   * @param filepath Full path to file
   * @return true if successful, false otherwise
   */
  bool SendFile(int client_socket, const std::string& filepath);
 
  /**
   * @brief Sends error message to client
   * @param client_socket Socket to send error
   * @param error_message Error description
   */
  void SendError(int client_socket, const std::string& error_message);
};


#endif

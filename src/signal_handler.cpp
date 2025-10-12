#include "signal_handler.h"
#include "logger.h"
#include <cstring>

// Static member initialization
std::atomic<bool> SignalHandler::shutdown_requested_{false};
std::function<void()> SignalHandler::shutdown_callback_;


void SignalHandler::HandleSignal(int signum) {
  // Set atomic flag first (async-signal-safe)
  shutdown_requested_.store(true);
  
  // Log signal reception (note: fprintf is async-signal-safe, printf is not)
  const char* signal_name = (signum == SIGINT) ? "SIGINT" : "SIGTERM";
  fprintf(stderr, "\n[SIGNAL] Received %s - initiating graceful shutdown...\n", 
          signal_name);
  fflush(stderr);
  
  // Execute callback if registered (not async-signal-safe, but acceptable here)
  if (shutdown_callback_) {
    shutdown_callback_();
  }
}


bool SignalHandler::Init(std::function<void()> callback) {
  LOG_INFO("Initializing signal handlers for SIGINT and SIGTERM");
  
  // Store callback
  shutdown_callback_ = callback;
  
  // Setup signal handlers using sigaction (preferred over signal())
  struct sigaction sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sa_handler = HandleSignal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  
  // Install SIGINT handler (Ctrl+C)
  if (sigaction(SIGINT, &sa, nullptr) != 0) {
    LOG_ERROR("Failed to install SIGINT handler: %s", strerror(errno));
    return false;
  }
  
  // Install SIGTERM handler (kill command)
  if (sigaction(SIGTERM, &sa, nullptr) != 0) {
    LOG_ERROR("Failed to install SIGTERM handler: %s", strerror(errno));
    return false;
  }
  
  LOG_SUCCESS("Signal handlers installed successfully");
  return true;
}


bool SignalHandler::ShutdownRequested() {
  return shutdown_requested_.load();
}


void SignalHandler::RequestShutdown() {
  shutdown_requested_.store(true);
  LOG_INFO("Shutdown manually requested");
}


void SignalHandler::Reset() {
  shutdown_requested_.store(false);
}

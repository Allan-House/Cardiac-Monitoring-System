#ifndef SIGNAL_HANDLER_H_
#define SIGNAL_HANDLER_H_

#include <atomic>
#include <csignal>
#include <functional>

/**
 * @brief Thread-safe signal handler for graceful shutdown
 * 
 * Provides a centralized mechanism to handle SIGINT and SIGTERM signals,
 * allowing components to register callbacks for coordinated cleanup.
 * Designed for embedded systems with emphasis on reliability and safety.
 */
class SignalHandler {
 private:
  static std::atomic<bool> shutdown_requested_;
  static std::function<void()> shutdown_callback_;
  
  /**
   * @brief Internal signal handler function
   * @param signum Signal number received
   */
  static void HandleSignal(int signum);
  
 public:
  SignalHandler() = delete;
  SignalHandler(const SignalHandler&) = delete;
  SignalHandler& operator=(const SignalHandler&) = delete;
  
  /**
   * @brief Initializes signal handlers for SIGINT and SIGTERM
   * @param callback Function to call when shutdown signal received
   * @return true if handlers installed successfully, false otherwise
   */
  static bool Init(std::function<void()> callback = nullptr);
  
  /**
   * @brief Checks if shutdown has been requested
   * @return true if SIGINT or SIGTERM received, false otherwise
   */
  static bool ShutdownRequested();
  
  /**
   * @brief Manually triggers shutdown flag
   * 
   * Useful for testing or programmatic shutdown without actual signal
   */
  static void RequestShutdown();
  
  /**
   * @brief Resets shutdown flag
   * 
   * WARNING: Use with caution. Primarily for testing scenarios.
   */
  static void Reset();
};

#endif

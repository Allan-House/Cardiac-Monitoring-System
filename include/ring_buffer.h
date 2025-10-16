#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <vector>

/**
 * @brief Thread-safe circular buffer for producer-consumer pattern.
 * 
 * Lock-free for single producer, blocking for consumer with condition
 * variable notification. Supports graceful shutdown signaling and
 * overflow handling with automatic overwrite of oldest data.
 * 
 * @tparam T Data type to store (must be copyable)
 * 
 * @note Designed for real-time data acquisition where dropping old
 *       samples is preferable to blocking the producer.
 */
template <class T>
class RingBuffer {
  private:
  std::vector<T> buffer_;
  mutable std::mutex mutex_;
  std::condition_variable data_added_;

  size_t head_ {0};
  size_t tail_ {0};
  
  const size_t max_size_;
  bool full_ {false};
  std::atomic<bool> shutdown_ {false};
  
  public:

  /**
   * @brief Constructs ring buffer with fixed capacity.
   * 
   * @param size Maximum number of elements to store
   * 
   * @note Size cannot be changed after construction
   */
  explicit RingBuffer(size_t size) :
    buffer_(size),
    max_size_ {size}
  {
    // Empty constructor.
  }
  
  /**
   * @brief Adds data to buffer (non-blocking).
   * 
   * If buffer is full, overwrites oldest unread data.
   * Thread-safe for multiple producers.
   * 
   * @param data Element to add
   * 
   * @note Returns immediately, never blocks producer
   */
  void AddData(T data) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

      if (shutdown_.load()) {
        return; // Don't add data after shutdown
      }

      buffer_.at(head_) = data;

      if (full_) {
        tail_ = (tail_ + 1) % max_size_;
      }

      head_ = (head_ + 1)  % max_size_;
      full_ = head_ == tail_;
    }
    data_added_.notify_one();
  }

  /**
   * @brief Consumes oldest element (blocking).
   * 
   * Waits for data if buffer is empty. Returns immediately
   * if shutdown is signaled.
   * 
   * @return Oldest element if available, std::nullopt on shutdown
   * 
   * @note Blocks calling thread until data available or shutdown
   */
  std::optional<T> Consume() {
    std::unique_lock<std::mutex> lock(mutex_);
    data_added_.wait(lock, [this]{return !Empty() || shutdown_.load();});
    
    if (shutdown_.load() && Empty()) {
      return std::nullopt;
    }

    auto value = buffer_.at(tail_);
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;
    return value;
  }

  /**
   * @brief Attempts to consume oldest element (non-blocking).
   * 
   * @return Oldest element if available, std::nullopt if empty or shutdown
   * 
   * @note Returns immediately, never blocks
   */
  std::optional<T> TryConsume() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (Empty() || shutdown_.load()) {
      return std::nullopt;
    }

    auto value = buffer_.at(tail_);
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;
    return value;
  }

  /**
   * @brief Signals shutdown to all waiting consumers.
   * 
   * Wakes all threads blocked on Consume() and prevents further operations.
   * Used for graceful shutdown of consumer threads.
   */
  void Shutdown() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      shutdown_.store(true);
    }
    data_added_.notify_all();
  }

  /**
   * @brief Resets buffer to empty state.
   * 
   * Clears all data and resets shutdown flag.
   * 
   * @warning Not safe to call while consumers are active
   */
  void Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;
    shutdown_.store(false);
  }

  /**
   * @brief Checks if buffer is empty.
   * 
   * @return true if no data available, false otherwise
   */
  bool Empty() const {
    return !full_ && (head_ == tail_);
  }

  /**
   * @brief Checks if buffer is at capacity.
   * 
   * @return true if buffer full, false otherwise
   */
  bool Full() const {
    return full_;
  }

  /**
   * @brief Checks if shutdown has been signaled.
   * 
   * @return true if Shutdown() was called, false otherwise
   */
  bool IsShutdown() const {
    return shutdown_.load();
  }

  /**
   * @brief Gets maximum buffer capacity.
   * 
   * @return Buffer size specified in constructor
   */
  size_t Capacity() const {
    return max_size_;
  }

  /**
   * @brief Gets current number of unread elements.
   * 
   * @return Number of elements available for consumption
   * 
   * @note Thread-safe but value may change immediately after return
   */
  size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t size = max_size_;

    if (!full_) {
      if (head_ >= tail_) {
        size = head_ - tail_;
      }
      else {
        size = max_size_ + head_ - tail_;
      }
    }

    return size;
  }
};

#endif

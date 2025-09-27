#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <vector>
#include <atomic>

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

  explicit RingBuffer(size_t size) :
    buffer_(size),
    max_size_ {size}
  {
    // Empty constructor.
  }
  
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

  std::optional<T> Front() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (Empty() || shutdown_.load()) {
      return std::nullopt;
    }
    return buffer_.at(tail_);
  }

  std::optional<T> Consume() {
    std::unique_lock<std::mutex> lock(mutex_);
    data_added_.wait(lock, [this]{return !Empty() || shutdown_.load();});
    
    if (shutdown_.load() && Empty()) {
      return std::nullopt;  // Ao invés de exception
    }

    auto value = buffer_.at(tail_);
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;
    return value;
  }

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

  void Shutdown() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      shutdown_.store(true);
    }
    data_added_.notify_all();
  }

  void Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;
    shutdown_.store(false);
  }
  
  bool Empty() const {
    return !full_ && (head_ == tail_);
  }
  
  bool Full() const {
    return full_;
  }

  bool IsShutdown() const {
    return shutdown_.load();
  }

  size_t Capacity() const {
    return max_size_;
  }

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

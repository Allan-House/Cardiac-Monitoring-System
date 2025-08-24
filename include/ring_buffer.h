#include <cstdlib>
#include <mutex>
#include <optional>
#include <vector>

template <class T>
class RingBuffer {
  private:
  std::vector<T> buffer_; // TODO (allan): array?  
  std::mutex mutex_;

  size_t head_ {0};
  size_t tail_ {0};
  
  const size_t max_size_;
  bool full_ {false};
  
  public:
  explicit RingBuffer(size_t size) :
    buffer_(size),
    max_size_ {size}
  {
    // Empty constructor.
  }
  
  void add_data(T data) {
    std::lock_guard<std::mutex> lock(mutex_);

    buffer_.at(head_) = data;

    if (full_) {
      tail_ = (tail_ + 1) % max_size_;
    }

    head_ = (head_ + 1)  % max_size_;

    full_ = head_ == tail_;
  }

  std::optional<T> front() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (empty()) {
      return std::nullopt;
    }
    
    return buffer_.at(tail_);
  }

  std::optional<T> consume() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (empty()) {
      return std::nullopt;
    }

    auto value = buffer_.at(tail_);
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;

    return value;
  }

  void reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;
  }
  
  bool empty() const {return !full_ && (head_ == tail_);} 
  
  bool full() const {return full_;}

  size_t capacity() const {return max_size_;}

  size_t size() const {
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

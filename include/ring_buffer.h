#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <vector>

// TODO (allan): atualizar documentação.

/**
 * @brief Thread-safe circular buffer implementation using a ring buffer data structure.
 * 
 * This class implements a fixed-size circular buffer that automatically overwrites
 * the oldest data when the buffer is full. All operations are thread-safe through
 * the use of mutex locks.
 * 
 * @tparam T The type of elements stored in the buffer
 */
template <class T>
class RingBuffer {
  private:
  std::vector<T> buffer_; // TODO (allan): array?  
  std::mutex mutex_;
  std::condition_variable data_added_;

  size_t head_ {0};
  size_t tail_ {0};
  
  const size_t max_size_;
  bool full_ {false};
  
  public:
  /**
   * @brief Constructs a RingBuffer with the specified capacity.
   * 
   * @param size The maximum number of elements the buffer can hold
   * @throw std::bad_alloc if memory allocation fails for the internal vector
   */
  explicit RingBuffer(size_t size) :
    buffer_(size),
    max_size_ {size}
  {
    // Empty constructor.
  }
  
  /**
   * @brief Adds a new element to the buffer.
   * 
   * If the buffer is full, the oldest element will be overwritten.
   * This operation is thread-safe.
   * 
   * @param data The element to add to the buffer
   */
  void AddData(T data) {
    {
      std::lock_guard<std::mutex> lock(mutex_);

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
   * @brief Returns the oldest element without removing it from the buffer.
   * 
   * @return std::optional<T> The oldest element if buffer is not empty, 
   *         std::nullopt otherwise
   */
  T Front() const {
    std::unique_lock<std::mutex> lock(mutex_);
    data_added_.wait(lock, [this]{return !Empty();});   
    return buffer_.at(tail_);
  }

  /**
   * @brief Removes and returns the oldest element from the buffer.
   * 
   * @return std::optional<T> The oldest element if buffer is not empty,
   *         std::nullopt otherwise
   */
  T Consume() {
    std::unique_lock<std::mutex> lock(mutex_);
    data_added_.wait(lock, [this]{return !Empty();});

    auto value = buffer_.at(tail_);
    full_ = false;
    tail_ = (tail_ + 1) % max_size_;
    return value;
  }

  /**
   * @brief Clears all elements from the buffer.
   * 
   * Resets the buffer to an empty state by setting head equal to tail.
   */
  void Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
    full_ = false;
  }
  
  /**
   * @brief Checks if the buffer is empty.
   * 
   * @return true if the buffer contains no elements, false otherwise
   */
  bool Empty() const {return !full_ && (head_ == tail_);}
  
  /**
   * @brief Checks if the buffer is full.
   * 
   * @return true if the buffer has reached its maximum capacity, false otherwise
   */
  bool Full() const {return full_;}

  /**
   * @brief Returns the maximum capacity of the buffer.
   * 
   * @return The maximum number of elements the buffer can hold
   */
  size_t Capacity() const {return max_size_;}

  /**
   * @brief Returns the current number of elements in the buffer.
   * 
   * Calculates the current size based on the head and tail positions.
   * 
   * @return The number of elements currently stored in the buffer
   */
  size_t Size() const {
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
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "ads1115.h"
#include "ecg_analyzer.h"
#include "ring_buffer.h"
#include <memory>

class Application {
  private:
  std::unique_ptr<ADS1115> ads1115_;
  std::unique_ptr<RingBuffer<Sample>> buffer_;

  public:
  Application(std::unique_ptr<ADS1115> ads1115, 
              std::unique_ptr<RingBuffer<Sample>> buffer);
  
  ~Application();

};

#endif

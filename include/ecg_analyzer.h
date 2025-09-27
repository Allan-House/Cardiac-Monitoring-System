#ifndef ECG_ANALYZER_H_
#define ECG_ANALYZER_H_

#include "ring_buffer.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <array>
#include <vector>

enum class WaveType {
  kNormal,    // Amostra normal (sem classificação específica)
  kP,         // Onda P
  kQ,         // Ponto Q
  kR,         // Pico R
  kS,         // Ponto S
  kT          // Onda T
};

struct Sample {
  float voltage;
  std::chrono::steady_clock::time_point timestamp;
  WaveType classification {WaveType::kNormal};
  // TODO (allan): adicionar index?
  
  Sample() : voltage(0.0f), timestamp(std::chrono::steady_clock::now()) {}

  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t)
    : voltage(v), timestamp(t) {}

  Sample(float v, std::chrono::time_point<std::chrono::steady_clock> t, WaveType type)
    : voltage(v), timestamp(t), classification(type) {}
};

enum class States {
  kSearchingR,
  kWaitingBuffer,
  kDetectingQ,
  kDetectingS,
  kDetectingP,
  kDetectingT,
  kFinalizing
};

class ECGAnalyzer {
  private:
  // TODO (allan): criar um threshold adaptativo.
  static constexpr float kThreshold {2.0f};
  
  // Buffers e dados
  std::vector<Sample> processed_samples_;
  std::shared_ptr<RingBuffer<Sample>> buffer_raw_;
  std::shared_ptr<RingBuffer<Sample>> buffer_classified_;
  
  // Thread control
  std::atomic<bool> processing_ {false};
  std::thread processing_thread_;
  
  // Estado da máquina de estados
  States state_ {States::kSearchingR};
  
  // Pontos de onda detectados: P, Q, R, S, T
  std::array<Sample, 5> wave_points_; // 0=P, 1=Q, 2=R, 3=S, 4=T
  
  // Configurações de amostragem
  // TODO (allan): padronizar ao longo de todo o código.
  static constexpr uint16_t kSampleRate{250};  // SPS
  static constexpr double kPeriodSeconds{1.0 / kSampleRate};  // 0.004 seconds
  
  // Constantes de tempo para detecção (em amostras)
  static constexpr size_t kSamples80ms{20};   // 80ms for Q and S
  static constexpr size_t kSamples200ms{50};  // 200ms for P
  static constexpr size_t kSamples400ms{100}; // 400ms for T
  
  // Índices dos pontos detectados
  size_t p_index_ {0};
  size_t q_index_ {0};
  size_t r_index_ {0};
  size_t s_index_ {0};
  size_t t_index_ {0};

  public:
  ECGAnalyzer(std::shared_ptr<RingBuffer<Sample>> buffer_raw,
              std::shared_ptr<RingBuffer<Sample>> buffer_classified);

  void Run();
  void Stop();

  private:
  void ProcessingLoop();
  void FSMExecute();
  
  // Funções de detecção
  void SearchRPeak();
  void CheckBufferSize();
  bool DetectRPeak();
  void DetectQPoint();
  void DetectSPoint();
  void DetectPWave();
  void DetectTWave();
  
  // Finalização
  void FinalizeDetection();
  void TransferToClassifiedBuffer();
};

#endif
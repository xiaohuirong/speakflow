#pragma once
#include <memory>
#include <string>
#include <vector>

class AsyncAudio {
public:
  enum class InputType {
    DefaultMicrophone,
    DefaultOutput,
    SpecificApplication
  };

  explicit AsyncAudio(int len_ms = 2000) : max_buffer_len_ms_(len_ms) {}
  virtual ~AsyncAudio() = default;

  virtual auto init(int sample_rate, const std::string &input = "") -> bool = 0;
  virtual auto resume() -> bool = 0;
  virtual auto pause() -> bool = 0;
  virtual auto clear() -> bool = 0;
  virtual void get(int ms, std::vector<float> &audio) = 0;

  static auto create(const std::string &type, int len_ms = 2000)
      -> std::unique_ptr<AsyncAudio>;

protected:
  int sample_rate_ = 16000;
  InputType input_type_ = InputType::DefaultMicrophone;
  bool is_initialized_ = false;
  bool is_paused_ = false;
  const int max_buffer_len_ms_;
};

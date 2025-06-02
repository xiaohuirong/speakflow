#pragma once
#include <memory>
#include <string>
#include <vector>

class Audio {
public:
  enum class InputType {
    DefaultMicrophone,
    DefaultOutput,
    SpecificApplication
  };

  explicit Audio(int len_ms = 2000) : max_buffer_len_ms_(len_ms) {}
  virtual ~Audio() = default;

  virtual auto init(int sample_rate, const std::string &input = "") -> bool = 0;
  virtual auto resume() -> bool = 0;
  virtual auto pause() -> bool = 0;
  virtual auto clear() -> bool = 0;
  virtual void get(int ms, std::vector<float> &audio) = 0;

  static auto create(const std::string &type, int len_ms = 2000)
      -> std::unique_ptr<Audio>;

protected:
  int sample_rate_ = 16000;
  InputType input_type_ = InputType::DefaultMicrophone;
  bool is_initialized_ = false;
  bool is_paused_ = false;
  const int max_buffer_len_ms_;
};

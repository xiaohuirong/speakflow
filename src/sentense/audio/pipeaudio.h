#pragma once
#include "audio.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <pipewire/pipewire.h>
#include <string>
#include <thread>
#include <vector>

class PipeWireAudio : public Audio {
public:
  explicit PipeWireAudio(int len_ms = 2000);
  ~PipeWireAudio() override;

  auto init(int sample_rate, const std::string &input = "") -> bool override;
  auto resume() -> bool override;
  auto pause() -> bool override;
  auto clear() -> bool override;
  void get(int ms, std::vector<float> &audio) override;

private:
  struct PipeWireDeleter {
    void operator()(pw_thread_loop *loop) const;
    void operator()(pw_context *ctx) const;
    void operator()(pw_core *core) const;
    void operator()(pw_stream *stream) const;
  };

  using UniquePwThreadLoop = std::unique_ptr<pw_thread_loop, PipeWireDeleter>;
  using UniquePwContext = std::unique_ptr<pw_context, PipeWireDeleter>;
  using UniquePwCore = std::unique_ptr<pw_core, PipeWireDeleter>;
  using UniquePwStream = std::unique_ptr<pw_stream, PipeWireDeleter>;

  void stop();
  void cleanup();
  void setupPipeWire();
  void setupStream();
  void startCaptureThread();
  static void onProcess(void *userdata);

  UniquePwThreadLoop mainloop_;
  UniquePwContext context_;
  UniquePwCore core_;
  UniquePwStream stream_;
  struct pw_stream_events stream_events_ = {};
  struct spa_hook stream_listener_ = {};

  std::deque<float> buffer_;
  std::mutex buffer_mutex_;
  std::condition_variable buffer_cv_;
  std::thread capture_thread_;
  std::atomic<bool> quit_{false};
  std::string input_;
  const spa_pod *params_[1];
};

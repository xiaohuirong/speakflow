#include "audio.h" // Assuming this is the base class header
#include <condition_variable>
#include <mutex>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <thread>
#include <vector>

class PipeWireAudio : public AsyncAudio {
public:
  explicit PipeWireAudio(int len_ms = 2000);
  ~PipeWireAudio() override;

  auto init(int sample_rate, const std::string &input = "") -> bool override;
  auto resume() -> bool override;
  auto pause() -> bool override;
  auto clear() -> bool override;
  void get(int ms, std::vector<float> &audio) override;

private:
  void cleanup();

  static void on_process(void *userdata);
  static void on_stream_param_changed(void *userdata, uint32_t id,
                                      const struct spa_pod *param);

  struct pw_main_loop *loop_ = nullptr;
  struct pw_stream *stream_ = nullptr;
  std::thread thread_;
  std::vector<float> buffer_;
  std::mutex buffer_mutex_;
  std::condition_variable buffer_cv_;
  std::string target_;

  static const struct pw_stream_events stream_events_;
};

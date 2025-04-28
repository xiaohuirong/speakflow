#include <QAudioDevice>
#include <QAudioSource>
#include <QDebug>
#include <QMediaDevices>
#include <QMutex>
#include <QThread>
#include <vector>

class AudioAsync : public QObject {
  Q_OBJECT
public:
  explicit AudioAsync(int len_ms, QObject *parent = nullptr);
  ~AudioAsync();

  bool init(int capture_id, int sample_rate, bool is_microphone);
  bool resume();
  bool pause();
  bool clear();
  void get(int ms, std::vector<float> &result);

private slots:
  void handleStateChanged(QAudio::State newState);
  void readAudioData();

private:
  QAudioDevice m_device;
  QAudioSource *m_audioSource = nullptr;
  QIODevice *m_audioIO = nullptr;

  int m_len_ms;
  int m_sample_rate = 0;
  bool m_running = false;

  std::vector<float> m_audio;
  size_t m_audio_pos = 0;
  size_t m_audio_len = 0;

  QMutex m_mutex;
};

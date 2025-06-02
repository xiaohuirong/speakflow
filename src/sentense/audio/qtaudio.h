#include "audio.h"
#include <QAudioDevice>
#include <QAudioSource>
#include <QDebug>
#include <QMediaDevices>
#include <QMutex>
#include <QThread>
#include <vector>

class QTAudio : public QObject, public AsyncAudio {
  Q_OBJECT
public:
  explicit QTAudio(int len_ms, QObject *parent = nullptr);
  ~QTAudio() override;

  auto init(int sample_rate, const std::string &input) -> bool override;
  auto resume() -> bool override;
  auto pause() -> bool override;
  auto clear() -> bool override;
  void get(int ms, std::vector<float> &result) override;

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

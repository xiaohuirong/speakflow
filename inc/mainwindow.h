#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chat.h"
#include "common-sdl.h"
#include "common.h"
#include "parse.h"

#include <QMainWindow>
#include <whisper.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr,
             const whisper_params &params = whisper_params());
  ~MainWindow();

private:
  Ui::MainWindow *ui;
  int running();
  whisper_params params;
  audio_async *audio;
  whisper_context_params cparams = whisper_context_default_params();
  whisper_context *ctx;
  Chat *mychat;

  std::vector<float> pcmf32;
  std::vector<float> pcmf32_old;
  std::vector<float> pcmf32_new;

  std::vector<whisper_token> prompt_tokens;

  wav_writer *wavWriter;

  bool is_running;

  std::chrono::time_point<std::chrono::high_resolution_clock> t_last;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_start;
  std::chrono::time_point<std::chrono::high_resolution_clock> t_change = t_last;
  bool last_status = 0;

private slots:
  void handleClick();
};
#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chat.h"
#include "common-sdl.h"
#include "common.h"
#include "inference.h"
#include "parse.h"

#include <QMainWindow>
#include <QTimer>
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
  whisper_params params;
  audio_async *audio;
  Chat *mychat;
  S2T *model;

  std::vector<float> pcmf32;
  std::vector<float> pcmf32_new;

  wav_writer *wavWriter;

  bool is_running;

  std::chrono::time_point<std::chrono::high_resolution_clock> t_change;
  bool last_status = 0;

  QTimer *timer;

private slots:
  void handleClick();
  int running();
};
#endif // MAINWINDOW_H

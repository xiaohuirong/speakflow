#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chat.h"
#include "common-sdl.h"
#include "common.h"
#include "document.h"
#include "inference.h"
#include "parse.h"
#include "previewpage.h"

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <whisper.h>

using namespace std;

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
  ~MainWindow() override;

private:
  Ui::MainWindow *ui;
  whisper_params params;
  audio_async *audio;
  Chat *mychat;
  S2T *model;

  vector<float> pcmf32;
  vector<float> pcmf32_new;

  bool is_running = false;

  chrono::time_point<chrono::high_resolution_clock> t_change;
  bool last_status = false;

  QTimer *timer;

  Document m_content;

  PreviewPage *page;

  function<void(const string &, bool)> chatCallback;
  function<void(const string &)> whisperCallback;

private slots:
  void handleClick();
  auto running() -> int;
};
#endif // MAINWINDOW_H

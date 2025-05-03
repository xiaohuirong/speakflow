#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cardman.h"
#include "chat.h"
#include "document.h"
#include "eventbus.h"
#include "monitorwindow.h"
#include "parse.h"
#include "previewpage.h"
#include "sentense.h"
#include "stt.h"

#include <QMainWindow>
#include <QString>
#include <QTimer>
#include <memory>
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
  using ChatCallback = std::function<void(const std::string &, bool)>;
  using QueueCallback = std::function<void(const vector<size_t> &)>;

  whisper_params params;
  whisper_full_params wparams;
  whisper_context_params cparams;

  std::shared_ptr<EventBus> eventBus;

  Sentense sentense;
  bool is_running = false;
  Document m_content;

  unique_ptr<Ui::MainWindow> ui;
  unique_ptr<PreviewPage> page;
  unique_ptr<Chat> chat;
  unique_ptr<STT> stt;
  unique_ptr<MonitorWindow> monitorwindow;

  unique_ptr<CardMan> audio_Man;

  ChatCallback chatCallback;

  void set_params();

private slots:
  void handleClick();
};
#endif // MAINWINDOW_H

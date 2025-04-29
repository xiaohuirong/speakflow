#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chat.h"

#include "sentense.h"

#include "document.h"
#include "inference.h"
#include "monitorwindow.h"
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

  Sentense *sentense;

  Chat *mychat;
  S2T *model;

  bool is_running = false;

  Document m_content;

  PreviewPage *page;

  function<void(const string &, bool)> chatCallback;
  function<void(const string &)> whisperCallback;

  MonitorWindow *monitorwindow;

private slots:
  void handleClick();
};
#endif // MAINWINDOW_H

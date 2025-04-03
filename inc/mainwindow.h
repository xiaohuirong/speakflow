#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "parse.h"
#include <QMainWindow>

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

private slots:
  void handleClick();
};
#endif // MAINWINDOW_H

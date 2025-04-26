#ifndef MONITORWINDOW_H
#define MONITORWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MonitorWindow;
}
QT_END_NAMESPACE

class MonitorWindow : public QMainWindow {
  Q_OBJECT

public:
  MonitorWindow(QWidget *parent = nullptr);
  ~MonitorWindow() override;

private:
  Ui::MonitorWindow *ui;

private slots:
};
#endif // MONITORWINDOW_H

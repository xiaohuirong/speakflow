#ifndef MONITORWINDOW_H
#define MONITORWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtWidgets/QMainWindow>
#include <qscatterseries.h>

using namespace std::chrono;

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
  void add_point(qreal time, qreal value);
  void add_point(high_resolution_clock::time_point t_now, qreal value);

private:
  Ui::MonitorWindow *ui;
  QChart *chart;
  QChartView *chartView;
  QScatterSeries *series;
  QValueAxis *axisX;
  QValueAxis *axisY;
  high_resolution_clock::time_point start_time;

private slots:
};
#endif // MONITORWINDOW_H

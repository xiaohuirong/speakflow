#include "monitorwindow.h"
#include "ui_monitorwindow.h"
#include <QChartView>
#include <QScatterSeries> // 加这个！

MonitorWindow::MonitorWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MonitorWindow) {
  ui->setupUi(this);

  series = new QScatterSeries(); // 改这里！

  chart = new QChart();
  chart->addSeries(series);
  chart->legend()->hide();
  // chart->createDefaultAxes();

  axisX = new QValueAxis;
  axisY = new QValueAxis;
  chart->addAxis(axisX, Qt::AlignBottom);
  chart->addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisX);
  series->attachAxis(axisY);

  axisX->setTitleText("Time");
  axisY->setTitleText("Value");

  chartView = new QChartView(chart);
  setCentralWidget(chartView);

  axisX->setRange(0, 30);
  axisY->setRange(-1, 3);

  start_time = high_resolution_clock::now();
}

void MonitorWindow::add_point(high_resolution_clock::time_point t_now,
                              qreal value) {

  auto duration =
      duration_cast<std::chrono::duration<double>>(t_now - start_time);

  double time_in_seconds = duration.count();

  series->append(time_in_seconds, value);

  const double window_width = 30.0; // 显示最近30秒

  axisX->setRange(time_in_seconds - window_width, time_in_seconds);
}

MonitorWindow::~MonitorWindow() { delete ui; }

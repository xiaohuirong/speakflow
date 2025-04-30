#include "queman.h"
#include <QApplication>

auto main(int argc, char *argv[]) -> int {
  QApplication app(argc, argv);

  QueueManagerWidget<QString> queueManager;

  // 设置合并函数
  queueManager.setMergeFunction(
      [](const QString &a, const QString &b) { return a + " + " + b; });

  // 设置显示函数
  queueManager.setDisplayFunction(
      [](const QString &item) { return "Item: " + item.toUpper(); });

  // 添加初始数据
  QList<QString> items = {"Apple", "Banana", "Cherry", "Date"};
  queueManager.setItems(items);

  queueManager.resize(400, 300);
  queueManager.show();

  return app.exec();
}

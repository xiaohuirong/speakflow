// frontend.h - Qt前端界面
#ifndef FRONTEND_H
#define FRONTEND_H

#include "common_interface.h"
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class Frontend : public QWidget {
  Q_OBJECT
public:
  explicit Frontend(QWidget *parent = nullptr);
  ~Frontend();

  // 获取前端回调接口
  FrontendCallbacks getCallbacks();

  // 设置后端操作接口
  void setBackendOperations(const BackendOperations &ops);

private slots:
  void onAddClicked();
  void onRemoveClicked();
  void onClearClicked();

private:
  QListWidget *listWidget;
  QLineEdit *inputLine;
  QPushButton *addButton;
  QPushButton *removeButton;
  QPushButton *clearButton;

  BackendOperations backendOps;

  void updateList();
};

#endif // FRONTEND_H

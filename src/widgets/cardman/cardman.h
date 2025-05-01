// frontend.h - Qt前端界面
#ifndef CARDMAN_H
#define CARDMAN_H

#include "common_stt.h"
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class CardMan : public QWidget {
  Q_OBJECT
public:
  explicit CardMan(QWidget *parent = nullptr);
  ~CardMan() override;

  // 获取前端回调接口
  auto getCallbacks() -> CardWidgetCallbacks;

  // 设置后端操作接口
  void setBackendOperations(const STTOperations &ops);

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

  STTOperations backendOps;

  // void updateList();
};

#endif // CARDMAN_H

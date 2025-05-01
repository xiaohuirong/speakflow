// frontend.h - Qt前端界面
#ifndef CARDMAN_H
#define CARDMAN_H

#include "common_stt.h"
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
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
  QScrollArea *scrollArea;
  QWidget *cardsContainer;
  QLineEdit *inputLine;
  QPushButton *addButton;
  QPushButton *removeButton;
  QPushButton *clearButton;

  STTOperations backendOps;
  std::vector<QFrame *> cardFrames;

  void createCard(const QString &text);
  void clearAllCards();
};

#endif // CARDMAN_H

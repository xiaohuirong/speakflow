#ifndef CARDMAN_H
#define CARDMAN_H

#include "common_stt.h"
#include "flowlayout.h"
#include <QFrame>
#include <QScrollArea>
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

public slots:
  void addCard(const QString &text);
  void removeCard(int index);
  void clearCards();

private:
  QScrollArea *scrollArea;
  QWidget *cardsContainer;
  STTOperations backendOps;
  std::vector<QFrame *> cardFrames;

  void createCard(const QString &text);
  void clearAllCards();

protected:
  FlowLayout *cardsLayout;
};
#endif // CARDMAN_H

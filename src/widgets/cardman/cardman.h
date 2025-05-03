#ifndef CARDMAN_H
#define CARDMAN_H

#include "eventbus.h"
#include "flowlayout.h"
#include <QCheckBox>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>
#include <qpushbutton.h>

class CardMan : public QWidget {
  Q_OBJECT
public:
  explicit CardMan(std::shared_ptr<EventBus> bus, QWidget *parent = nullptr);
  ~CardMan() override;

private slots:
  void addCard(const QString &text);
  void removeCard(int index);
  void clearCards();

private:
  std::shared_ptr<EventBus> eventBus;

  QScrollArea *scrollArea;
  QWidget *cardsContainer;
  QPushButton recordButton; // Changed from triggerButton to recordButton
  QPushButton sendButton;
  QPushButton clearButton;
  QCheckBox *autoTriggerCheckBox;
  std::vector<QFrame *> cardFrames;

  void createCard(const QString &text);
  void clearAllCards();
  void setupControlButtons();

  bool isRecording = false;

protected:
  FlowLayout *cardsLayout;
};
#endif // CARDMAN_H

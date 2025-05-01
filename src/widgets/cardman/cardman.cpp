#include "cardman.h"
#include "common_stt.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <algorithm>

CardMan::CardMan(QWidget *parent) : QWidget(parent) {
  scrollArea = new QScrollArea(this);
  cardsContainer = new QWidget();
  cardsLayout = new FlowLayout(cardsContainer, 10, 10, 10);

  scrollArea->setWidgetResizable(true);
  scrollArea->setWidget(cardsContainer);
  scrollArea->setFrameShape(QFrame::NoFrame);

  auto *layout = new QVBoxLayout(this);
  setupControlButtons();
  layout->addWidget(scrollArea);
  layout->setContentsMargins(0, 0, 0, 0);
}

void CardMan::setupControlButtons() {
  auto *controlPanel = new QWidget(this);
  auto *controlLayout = new QHBoxLayout(controlPanel);
  controlPanel->setLayout(controlLayout);
  controlLayout->setContentsMargins(10, 10, 10, 10);

  // Add record button with microphone icon
  recordButton = new QPushButton(controlPanel);
  recordButton->setIcon(QIcon::fromTheme("microphone-sensitivity-high"));
  recordButton->setIconSize(QSize(24, 24));
  recordButton->setFixedSize(50, 50);
  recordButton->setCheckable(false);
  recordButton->setStyleSheet("QPushButton {"
                              "   background-color: #f44336;"
                              "   border-radius: 25px;"
                              "   border: 2px solid #d32f2f;"
                              "   outline: none;"
                              "}"
                              "QPushButton:hover {"
                              "   background-color: #e53935;"
                              "}"
                              "QPushButton:pressed {"
                              "   background-color: #c62828;"
                              "}"
                              "QPushButton:focus {"
                              "   border: 2px solid #d32f2f;"
                              "}");

  // Add auto trigger checkbox with better styling
  autoTriggerCheckBox = new QCheckBox("Auto Mode", controlPanel);
  autoTriggerCheckBox->setStyleSheet("QCheckBox {"
                                     "   spacing: 5px;"
                                     "   font-size: 14px;"
                                     "}"
                                     "QCheckBox::indicator {"
                                     "   width: 18px;"
                                     "   height: 18px;"
                                     "}");

  // Add spacer to push items to the left
  controlLayout->addWidget(recordButton);
  controlLayout->addSpacing(15);
  controlLayout->addWidget(autoTriggerCheckBox);
  controlLayout->addStretch();

  // 修改信号连接，改为点击切换状态
  connect(recordButton, &QPushButton::clicked, [this]() {
    if (autoTriggerCheckBox->isChecked())
      return;

    isRecording = !isRecording;

    if (isRecording) {
      recordButton->setStyleSheet("QPushButton {"
                                  "   background-color: green;"
                                  "   border-radius: 25px;"
                                  "   border: 2px solid darkgreen;"
                                  "   outline: none;"
                                  "}"
                                  "QPushButton:hover {"
                                  "   background-color: darkgreen;"
                                  "}");
      if (backendOps.setTriggerMethod) {
        backendOps.setTriggerMethod(TriggerMethod::NO_TRIGGER);
      }
    } else {
      recordButton->setStyleSheet("QPushButton {"
                                  "   background-color: #f44336;"
                                  "   border-radius: 25px;"
                                  "   border: 2px solid #d32f2f;"
                                  "   outline: none;"
                                  "}"
                                  "QPushButton:hover {"
                                  "   background-color: #e53935;"
                                  "}");
      if (backendOps.setTriggerMethod) {
        backendOps.setTriggerMethod(TriggerMethod::ONCE_TRIGGER);
      }
    }
  });

  connect(autoTriggerCheckBox, &QCheckBox::checkStateChanged,
          [this](Qt::CheckState state) {
            if (backendOps.setTriggerMethod) {
              if (state == Qt::Checked) {
                isRecording =
                    false; // Reset recording state when auto mode is enabled
                recordButton->setEnabled(false);
                recordButton->setStyleSheet("QPushButton {"
                                            "   background-color: #9e9e9e;"
                                            "   border-radius: 25px;"
                                            "   border: 2px solid #757575;"
                                            "}"
                                            "QPushButton:hover {"
                                            "   background-color: #9e9e9e;"
                                            "}");
                backendOps.setTriggerMethod(TriggerMethod::AUTO_TRIGGER);
              } else {
                recordButton->setEnabled(true);
                recordButton->setStyleSheet("QPushButton {"
                                            "   background-color: #f44336;"
                                            "   border-radius: 25px;"
                                            "   border: 2px solid #d32f2f;"
                                            "}"
                                            "QPushButton:hover {"
                                            "   background-color: #e53935;"
                                            "}");
                backendOps.setTriggerMethod(TriggerMethod::NO_TRIGGER);
              }
            }
          });

  // Add control panel to main layout
  this->layout()->addWidget(controlPanel);
}

CardMan::~CardMan() = default;

void CardMan::createCard(const QString &text) {
  auto *card = new QFrame(cardsContainer);
  card->setFixedWidth(120);
  card->setMinimumHeight(20);
  card->setFrameShape(QFrame::StyledPanel);
  card->setLineWidth(1);
  card->setStyleSheet("QFrame {"
                      "   background-color: palette(highlight);"
                      "   color: palette(highlightedText);"
                      "   border-radius: 5px;"
                      "}");

  auto *cardLayout = new QHBoxLayout(card);

  auto *audioIcon = new QLabel(card);
  audioIcon->setPixmap(QIcon::fromTheme("audio-volume-high").pixmap(20, 20));
  cardLayout->addWidget(audioIcon);

  auto *label = new QLabel(text, card);
  label->setWordWrap(true);
  cardLayout->addWidget(label);

  cardLayout->addStretch();

  auto *closeButton = new QPushButton("X", card);
  closeButton->setStyleSheet(
      "QPushButton { border: none; font-size: 16px; color: #999; }"
      "QPushButton:hover { color: #f00; }");
  closeButton->setFixedSize(20, 20);
  connect(closeButton, &QPushButton::clicked, [this, card]() {
    auto it = std::ranges::find(cardFrames, card);
    if (it != cardFrames.end() && backendOps.removeVoice) {
      int index = std::distance(cardFrames.begin(), it);
      backendOps.removeVoice(index);
    }
  });
  cardLayout->addWidget(closeButton);

  cardsLayout->addWidget(card);
  cardFrames.push_back(card);
}

void CardMan::clearAllCards() {
  for (auto card : cardFrames) {
    cardsLayout->removeWidget(card);
    delete card;
  }
  cardFrames.clear();
}

auto CardMan::getCallbacks() -> CardWidgetCallbacks {
  return {
      .onVoiceAdded =
          [this](const std::string &length) {
            addCard(QString::fromStdString(length) + "S");
          },
      .onVoiceRemoved = [this](int index) { removeCard(index); },
      .onVoiceCleared = [this]() { clearCards(); },
  };
}

void CardMan::setBackendOperations(const STTOperations &ops) {
  backendOps = ops;
}

void CardMan::addCard(const QString &text) {
  QMetaObject::invokeMethod(
      this, [this, text]() { createCard(text); }, Qt::QueuedConnection);
}

void CardMan::removeCard(int index) {
  QMetaObject::invokeMethod(
      this,
      [this, index]() {
        if (index >= 0 && index < static_cast<int>(cardFrames.size())) {
          QFrame *card = cardFrames[index];
          cardsLayout->removeWidget(card);
          delete card;
          cardFrames.erase(cardFrames.begin() + index);
        }
      },
      Qt::QueuedConnection);
}

void CardMan::clearCards() {
  QMetaObject::invokeMethod(
      this, [this]() { clearAllCards(); }, Qt::QueuedConnection);
}

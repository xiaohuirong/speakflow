#include "cardman.h"
#include "common_stt.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
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

  // Add trigger button
  triggerButton = new QPushButton("Trigger", controlPanel);

  triggerButton->setCheckable(false); // 禁用 toggle 特性

  // Add auto trigger checkbox
  autoTriggerCheckBox = new QCheckBox("Auto Trigger", controlPanel);

  controlLayout->addWidget(triggerButton);
  controlLayout->addWidget(autoTriggerCheckBox);

  // Connect signals
  connect(triggerButton, &QPushButton::pressed, [this]() {
    if (!autoTriggerCheckBox->isChecked() && backendOps.setTriggerMethod) {
      backendOps.setTriggerMethod(TriggerMethod::NO_TRIGGER);
    }
  });

  connect(triggerButton, &QPushButton::released, [this]() {
    if (!autoTriggerCheckBox->isChecked() && backendOps.setTriggerMethod) {
      backendOps.setTriggerMethod(TriggerMethod::ONCE_TRIGGER);
    }
  });

  connect(autoTriggerCheckBox, &QCheckBox::checkStateChanged,
          [this](Qt::CheckState state) {
            if (backendOps.setTriggerMethod) {
              if (state == Qt::Checked) {
                triggerButton->setEnabled(false);
                backendOps.setTriggerMethod(TriggerMethod::AUTO_TRIGGER);
              } else {
                triggerButton->setEnabled(true);
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

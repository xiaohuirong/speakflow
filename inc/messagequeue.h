#include <QFrame>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class MessageCard : public QFrame {
  Q_OBJECT
public:
  explicit MessageCard(const QString &message, QWidget *parent = nullptr)
      : QFrame(parent) {
    setFrameShape(QFrame::Box);
    setStyleSheet("border-radius: 10px;");

    label = new QLabel(message, this);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    setLayout(layout);
  }

  // [[nodiscard]] auto sizeHint() const -> QSize override {
  //   return label->sizeHint() + QSize(20, 20); // 适当增加内边距
  // }

private:
  QLabel *label;
};

class QueueWidget : public QWidget {
  Q_OBJECT
public:
  explicit QueueWidget(QWidget *parent = nullptr) : QWidget(parent) {
    auto *mainLayout = new QVBoxLayout(this);
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    containerWidget = new QWidget();
    layout = new QVBoxLayout(containerWidget);
    layout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(containerWidget);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);
  }

  void enqueueMessage(const QString &message) {
    auto *card = new MessageCard(message, containerWidget);
    layout->addWidget(card);
    adjustHeight();
  }

public slots:
  void dequeueMessage() {
    if (layout->count() > 0) {
      QLayoutItem *item = layout->takeAt(0);
      if (item) {
        QWidget *widget = item->widget();
        if (widget) {
          auto *animation = new QPropertyAnimation(widget, "geometry");
          animation->setDuration(300);
          animation->setStartValue(widget->geometry());
          animation->setEndValue(
              widget->geometry().translated(-widget->width(), 0));
          connect(animation, &QPropertyAnimation::finished, widget,
                  &QWidget::deleteLater);
          animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        delete item;
      }
      adjustHeight();
    }
  }

private:
  void adjustHeight() {
    containerWidget->adjustSize();
    scrollArea->updateGeometry();
  }

  QVBoxLayout *layout;
  QWidget *containerWidget;
  QScrollArea *scrollArea;
};

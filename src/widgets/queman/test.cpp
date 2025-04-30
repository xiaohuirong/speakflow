#include "queman.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  MainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
    queueManager = new QueueManagerWidget<QString>(this);

    // Set up the queue manager
    queueManager->setMergeFunction(
        [](const QString &a, const QString &b) { return a + " + " + b; });
    queueManager->setDisplayFunction(
        [](const QString &item) { return "Item: " + item.toUpper(); });

    // Add initial data
    QList<QString> items = {"Apple", "Banana", "Cherry", "Date"};
    queueManager->setItems(items);

    // Create buttons
    auto *deleteSelectedBtn = new QPushButton(tr("Delete Selected"), this);
    auto *deleteAtPosBtn = new QPushButton(tr("Delete At Position"), this);
    auto *mergeBtn = new QPushButton(tr("Merge Items"), this);
    auto *clearBtn = new QPushButton(tr("Clear Queue"), this);
    auto *moveBtn = new QPushButton(tr("Move Item"), this);
    auto *moveToFrontBtn = new QPushButton(tr("Move to Front"), this);
    auto *insertBtn = new QPushButton(tr("Insert Item"), this);
    auto *appendBtn = new QPushButton(tr("Append Item"), this);

    // Button layout
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(deleteSelectedBtn);
    buttonLayout->addWidget(deleteAtPosBtn);
    buttonLayout->addWidget(mergeBtn);
    buttonLayout->addWidget(clearBtn);
    buttonLayout->addWidget(moveBtn);
    buttonLayout->addWidget(moveToFrontBtn);
    buttonLayout->addWidget(insertBtn);
    buttonLayout->addWidget(appendBtn);

    // Main layout
    auto *mainWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->addWidget(queueManager->getListWidget());
    mainLayout->addLayout(buttonLayout);
    setCentralWidget(mainWidget);

    // Connect buttons
    connect(deleteSelectedBtn, &QPushButton::clicked, queueManager,
            &QueueManagerWidget<QString>::deleteSelected);
    connect(deleteAtPosBtn, &QPushButton::clicked, [this]() {
      bool ok;
      int index = QInputDialog::getInt(this, tr("Delete Item"),
                                       tr("Enter position to delete:"), 0, 0,
                                       queueManager->count() - 1, 1, &ok);
      if (ok)
        queueManager->deleteAtPosition(index);
    });
    connect(mergeBtn, &QPushButton::clicked, [this]() {
      bool ok;
      int start = QInputDialog::getInt(this, tr("Merge Items"),
                                       tr("Start from position:"), 0, 0,
                                       queueManager->count() - 1, 1, &ok);
      if (!ok)
        return;

      int count = QInputDialog::getInt(this, tr("Merge Items"),
                                       tr("Number of items to merge:"), 2, 2,
                                       queueManager->count() - start, 1, &ok);
      if (ok)
        queueManager->mergeItems(start, count);
    });
    connect(clearBtn, &QPushButton::clicked, queueManager,
            &QueueManagerWidget<QString>::clearQueue);
    connect(moveBtn, &QPushButton::clicked, [this]() {
      bool ok;
      int distance = QInputDialog::getInt(
          this, tr("Move Item"), tr("Move distance (negative for backward):"),
          0, -queueManager->count() + 1, queueManager->count() - 1, 1, &ok);
      if (ok)
        queueManager->moveItem(queueManager->getListWidget()->currentRow(),
                               distance);
    });
    connect(moveToFrontBtn, &QPushButton::clicked, [this]() {
      queueManager->moveToFront(queueManager->getListWidget()->currentRow());
    });
    connect(insertBtn, &QPushButton::clicked, [this]() {
      bool ok;
      int index = QInputDialog::getInt(this, tr("Insert Item"),
                                       tr("Enter position to insert:"), 0, 0,
                                       queueManager->count(), 1, &ok);
      if (!ok)
        return;

      QString value = QInputDialog::getText(this, tr("Insert Item"),
                                            tr("Enter value to insert:"),
                                            QLineEdit::Normal, "", &ok);
      if (ok)
        queueManager->insertAtPosition(index, value);
    });
    connect(appendBtn, &QPushButton::clicked, [this]() {
      bool ok;
      QString value = QInputDialog::getText(this, tr("Append Item"),
                                            tr("Enter value to append:"),
                                            QLineEdit::Normal, "", &ok);
      if (ok)
        queueManager->append(value);
    });
  }

private:
  QueueManagerWidget<QString> *queueManager;
};

auto main(int argc, char *argv[]) -> int {
  QApplication app(argc, argv);

  MainWindow mainWindow;
  mainWindow.resize(600, 400);
  mainWindow.show();

  return app.exec();
}

#include "test.moc"

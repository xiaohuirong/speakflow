#ifndef PREVIEWPAGE_H
#define PREVIEWPAGE_H

#include <QWebEnginePage>

class PreviewPage : public QWebEnginePage {
  Q_OBJECT
public:
  explicit PreviewPage(QObject *parent = nullptr) : QWebEnginePage(parent) {}
  void scrollToBottom();

protected:
  auto acceptNavigationRequest(const QUrl &url, NavigationType type,
                               bool isMainFrame) -> bool override;

private:
};

#endif // PREVIEWPAGE_H

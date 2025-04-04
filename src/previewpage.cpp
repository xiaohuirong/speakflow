#include "previewpage.h"

#include <QDesktopServices>

void PreviewPage::scrollToBottom() {
  runJavaScript("window.scrollTo(0, document.body.scrollHeight);");
}

auto PreviewPage::acceptNavigationRequest(
    const QUrl &url, QWebEnginePage::NavigationType /*type*/,
    bool /*isMainFrame*/) -> bool {
  // Only allow qrc:/index.html.
  if (url.scheme() == QString("qrc"))
    return true;
  QDesktopServices::openUrl(url);
  return false;
}

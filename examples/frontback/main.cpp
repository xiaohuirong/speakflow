#include "backend.h"
#include "frontend.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // 创建后端
  Backend backend;

  // 创建前端
  Frontend frontend;

  // 设置前后端通信
  frontend.setBackendOperations(backend.getOperations());
  backend.setFrontendCallbacks(frontend.getCallbacks());

  frontend.show();
  return app.exec();
}

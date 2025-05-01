#include "cardman.h"
#include "common_stt.h"
#include <QApplication>

auto main(int argc, char *argv[]) -> int {
  QApplication app(argc, argv);

  // 创建前端
  CardMan frontend;

  const CardWidgetCallbacks frontend_callbacks = frontend.getCallbacks();

  const STTOperations backend_operations = {
      .addVoice =
          [frontend_callbacks](const std::vector<float> &audio) {
            frontend_callbacks.onVoiceAdded(std::to_string(audio[0]));
          },
      .removeVoice =
          [frontend_callbacks](const size_t index) {
            frontend_callbacks.onVoiceRemoved(index);
            return true;
          },
      .clearVoice =
          [frontend_callbacks]() { frontend_callbacks.onVoiceCleared(); }};

  // 设置前后端通信
  frontend.setBackendOperations(backend_operations);

  frontend.show();
  return app.exec();
};

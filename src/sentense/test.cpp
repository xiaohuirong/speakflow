#include "sentense.h"
#include <iostream>
#include <thread>

int main() {
  Sentense sen("../../models/silero_vad.onnx");

  if (!sen.initialize()) {
    std::cerr << "Failed to initialize sen processor" << std::endl;
    return 1;
  }

  sen.setSentenceCallback([](const std::vector<float> &sentence) {
    std::cout << "Detected sentence with " << sentence.size() << " samples"
              << std::endl;
    // 在这里处理检测到的句子
  });

  sen.start();

  // 主循环
  while (true) {
    // 处理其他任务或等待退出条件
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  cout << "end" << endl;

  sen.stop();
  return 0;
}

# a real-time speech-to-text project that uses Whisper and an LLM API for intelligent responses.

## features
1. 实现了实时语音输入，语音检测，断句，语音识别，AI对话功能。
2. 语音识别采取异步设计，单独一个线程维护一个语音缓存。采取的设计模式为策略模式，在`AsyncAudio`接口类的基础上实现了三种策略，通过条件编译开启，避免引入不必要的依赖。
    - Qt后端
    - SDL后端
    - Pipewire后端
3. 语音检测使用基于机器学习模型的方案，实现`VadIterator`类，该类提供一个关键的`process`方法，该方法可以返回返回音频的句子片段，格式为`[start_time, end_time]`。
4. 使用外观模式的设计思想，将语音输入和语音检测封装为更高级别的接口`Sentense`，但检测到新句子后自动将句子发送给前端处理模块。具体的细节为句子维护一个环形的缓冲区，每间隔2s调用语音输入接口获取这2s的语音到缓冲区。然后将缓冲区的音频输入语音检测模块获取断句信息，对于前$n-1$个句子，如果句子太短，或者句子与前句间隔过短，合并当前句子到前一个句子，否则认为读取到独立的句子，发送给前端同时移动缓冲区开始位置实现删除操作。对于最后一个句子，判读与缓冲区尾部的时间间隔是否足够大，足够大认为是合法句子，否则不进行处理，留在缓冲区。
5. 使用whisper模型对断句进行语音识别，识别结果会作为下一次识别的上下文。
6. 语音识别的文本通过liboai库发送给大语言模型获取回复。
7. 语音识别和AI对话模块均设计有队列，每个模块单独开一个线程对队列进行监控，不断对队列进行处理，但队列为空时进入等待状态，接受到后端模块发送的新队列成员后会通知处理队列进行处理，保证语音识别和AI对话的有序性。
8. 每个模块的通信通过一个事件总线来实现，以实现各个前端模块和后端模块的高度解耦，也方便前后端模块的灵活扩充。
9. 使用`spdlog`实现日志的管理与输出，`cli11`实现配置文件配置参数的高效设置。

## build

1. install depends
```bash
sudo pacman -S make cmake gcc curl pkgconf nlohmann-json whisper.cpp-git cli11 qt6-base stb
```

2. compile
```bash
cmake -B build -S .
make -C build
```

![speakflow](https://github.com/xiaohuirong/images/raw/main/speakflow/ui.png?raw=true)

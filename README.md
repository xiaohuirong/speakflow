# a real-time speech-to-text project that uses Whisper and an LLM API for intelligent responses.

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

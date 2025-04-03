# a real-time speech-to-text project that uses Whisper and an LLM API for intelligent responses.

## build

1. install depends
```bash
sudo pacman -S make cmake gcc curl pkgconf nlohmann-json whisper.cpp-git cli11
```

2. compile
```bash
cmake -B build -S .
make -C build
```

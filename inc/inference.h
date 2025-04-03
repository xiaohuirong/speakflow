#pragma once
#include "parse.h"
#include "whisper.h"
#include <vector>

int reference(whisper_params &params, std::vector<whisper_token> &prompt_tokens,
              whisper_context *ctx, std::vector<float> &pcmf32,
              int64_t duration, std::vector<float> &pcmf32_old);

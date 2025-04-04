#pragma once
#include "parse.h"
#include <vector>
#include <whisper.h>

class S2T {
public:
  S2T(whisper_params &params);
  ~S2T();
  auto inference(whisper_params &params, std::vector<float> pcmf32)
      -> std::string;

private:
  whisper_full_params wparams;
  int n_iter = 0;
  whisper_context_params cparams = whisper_context_default_params();
  whisper_context *ctx;

  std::vector<whisper_token> prompt_tokens;
};

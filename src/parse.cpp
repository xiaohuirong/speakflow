#include "parse.h"
#include <CLI/CLI.hpp>
#include <cstdio>
#include <iostream>
#include <whisper.h>

void print_whisper_params(const whisper_params &p) {
  std::cout << std::left << std::setw(20) << "name" << std::setw(10) << "value"
            << std::endl;
  std::cout << "-------------------------------------------" << std::endl;

#define PRINT_MEMBER(member)                                                   \
  std::cout << std::setw(20) << #member << std::setw(10) << p.member           \
            << std::endl;

  PRINT_MEMBER(n_threads);
  PRINT_MEMBER(step_ms);
  PRINT_MEMBER(length_ms);
  PRINT_MEMBER(keep_ms);
  PRINT_MEMBER(capture_id);
  PRINT_MEMBER(max_tokens);
  PRINT_MEMBER(audio_ctx);
  PRINT_MEMBER(beam_size);

  PRINT_MEMBER(n_samples_keep);
  PRINT_MEMBER(n_samples_step);
  PRINT_MEMBER(n_samples_len);
  PRINT_MEMBER(n_samples_30s);
  PRINT_MEMBER(n_new_line);

  PRINT_MEMBER(is_microphone);
  PRINT_MEMBER(token);
  PRINT_MEMBER(url);

  PRINT_MEMBER(vad_thold);
  PRINT_MEMBER(freq_thold);

  PRINT_MEMBER(translate);
  PRINT_MEMBER(no_fallback);
  PRINT_MEMBER(print_special);
  PRINT_MEMBER(no_context);
  PRINT_MEMBER(no_timestamps);
  PRINT_MEMBER(tinydiarize);
  PRINT_MEMBER(save_audio);
  PRINT_MEMBER(use_gpu);
  PRINT_MEMBER(flash_attn);
  PRINT_MEMBER(use_vad);

  PRINT_MEMBER(language);
  PRINT_MEMBER(model);
  PRINT_MEMBER(fname_out);

  PRINT_MEMBER(timeout);

  PRINT_MEMBER(llm);

  PRINT_MEMBER(prompt);
  PRINT_MEMBER(init_prompt);
}

auto whisper_params_parse(int argc, char **argv, whisper_params &params)
    -> bool {
  CLI::App app{"Speakflow"};
  app.set_config("--config")->expected(1, 1);

  app.add_flag("-p,--print", params.is_print, "print value then exit.")
      ->default_val(false)
      ->transform(CLI::IsMember({true, false}));
  app.add_option("-t,--threads", params.n_threads,
                 "number of threads to use during ");
  app.add_option("--step", params.step_ms, "audio step size in milliseconds");
  app.add_option("--length", params.length_ms, "audio length in milliseconds");
  app.add_option("--keep", params.keep_ms, "audio to keep from previous step");
  app.add_option("-c,--capture", params.capture_id, "capture device ID");
  app.add_option("--mt,--max-tokens", params.max_tokens,
                 "maximum number of tokens per");
  app.add_option("--ac,--audio-ctx", params.audio_ctx,
                 "audio context size (0 - all)");
  app.add_option("--bs,--beam-size", params.beam_size,
                 "beam size for beam search");
  app.add_option("--vth,--vad-thold", params.vad_thold,
                 "voice activity detection");
  app.add_option("--fth,--freq-thold", params.freq_thold,
                 "high-pass frequency cutoff");
  app.add_option("--to,--token", params.token, "Authentication token");
  app.add_option("-u,--url", params.url, "API URL");
  app.add_flag("--tr,--translate", params.translate,
               "translate from source language");
  app.add_flag("--nf,--no-fallback", params.no_fallback,
               "do not use temperature fallback");
  app.add_flag("--ps,--print-special", params.print_special,
               "print special tokens");
  app.add_flag("--no-context", params.no_context,
               "keep context between audio chunks")
      ->default_val(true)
      ->transform(CLI::IsMember({true, false}));
  app.add_option("-l,--language", params.language, "spoken language");
  app.add_option("-m,--model", params.model, "model path");
  app.add_option("-f,--file", params.fname_out, "text output file name");
  app.add_flag("--tdrz,--tinydiarize", params.tinydiarize,
               "enable tinydiarize (requires a tdrz model)");
  app.add_flag("--sa,--save-audio", params.save_audio,
               "save the recorded audio to a file");
  app.add_flag("--ng,--no-gpu", params.use_gpu, "disable GPU inference")
      ->default_val(true)
      ->transform(CLI::IsMember({true, false}));
  app.add_flag("--fa,--flash-attn", params.flash_attn,
               "flash attention during inference");
  app.add_flag("--im,--is-microphone", params.is_microphone,
               "select microphone as input");
  app.add_option("--timeout", params.timeout, "API request timeout(ms)");
  app.add_option("--llm", params.llm, "LLM model name.");
  app.add_option("--prompt", params.prompt, "LLM additional prompt");
  app.add_option("--init-prompt", params.init_prompt, "LLM initial prompt");

  CLI11_PARSE(app, argc, argv);

  params.use_vad = params.step_ms <= 0;
  params.keep_ms = std::min(params.keep_ms, params.step_ms);
  params.length_ms = std::max(params.length_ms, params.step_ms);

  params.no_timestamps = !params.use_vad;
  params.no_context |= !params.use_vad;
  params.max_tokens = 0;

  params.n_samples_step = (1e-3 * params.step_ms) * WHISPER_SAMPLE_RATE;
  params.n_samples_len = (1e-3 * params.length_ms) * WHISPER_SAMPLE_RATE;
  params.n_samples_keep = (1e-3 * params.keep_ms) * WHISPER_SAMPLE_RATE;
  params.n_samples_30s = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;

  params.n_new_line = !params.use_vad
                          ? std::max(1, params.length_ms / params.step_ms - 1)
                          : 1; // number of steps to print new line

  if (params.is_print) {
    print_whisper_params(params);
    exit(0);
  }

  return true;
}

#include "parse.h"
#include "whisper.h"
#include <cstdio>

void whisper_print_usage(int /*argc*/, char **argv,
                         const whisper_params &params) {
  fprintf(stderr, "\n");
  fprintf(stderr, "usage: %s [options]\n", argv[0]);
  fprintf(stderr, "\n");
  fprintf(stderr, "options:\n");
  fprintf(stderr, "  -h,       --help          [default] show this help "
                  "message and exit\n");
  fprintf(stderr,
          "  -t N,     --threads N     [%-7d] number of threads to use during "
          "computation\n",
          params.n_threads);
  fprintf(
      stderr,
      "            --step N        [%-7d] audio step size in milliseconds\n",
      params.step_ms);
  fprintf(stderr,
          "            --length N      [%-7d] audio length in milliseconds\n",
          params.length_ms);
  fprintf(stderr,
          "            --keep N        [%-7d] audio to keep from previous step "
          "in ms\n",
          params.keep_ms);
  fprintf(stderr, "  -c ID,    --capture ID    [%-7d] capture device ID\n",
          params.capture_id);
  fprintf(stderr,
          "  -mt N,    --max-tokens N  [%-7d] maximum number of tokens per "
          "audio chunk\n",
          params.max_tokens);
  fprintf(stderr,
          "  -ac N,    --audio-ctx N   [%-7d] audio context size (0 - all)\n",
          params.audio_ctx);
  fprintf(stderr,
          "  -bs N,    --beam-size N   [%-7d] beam size for beam search\n",
          params.beam_size);
  fprintf(stderr,
          "  -vth N,   --vad-thold N   [%-7.2f] voice activity detection "
          "threshold\n",
          params.vad_thold);
  fprintf(stderr,
          "  -fth N,   --freq-thold N  [%-7.2f] high-pass frequency cutoff\n",
          params.freq_thold);
  fprintf(stderr,
          "  -tr,      --translate     [%-7s] translate from source language "
          "to english\n",
          params.translate ? "true" : "false");
  fprintf(stderr,
          "  -nf,      --no-fallback   [%-7s] do not use temperature fallback "
          "while decoding\n",
          params.no_fallback ? "true" : "false");
  fprintf(stderr, "  -ps,      --print-special [%-7s] print special tokens\n",
          params.print_special ? "true" : "false");
  fprintf(
      stderr,
      "  -kc,      --keep-context  [%-7s] keep context between audio chunks\n",
      params.no_context ? "false" : "true");
  fprintf(stderr, "  -l LANG,  --language LANG [%-7s] spoken language\n",
          params.language.c_str());
  fprintf(stderr, "  -m FNAME, --model FNAME   [%-7s] model path\n",
          params.model.c_str());
  fprintf(stderr, "  -f FNAME, --file FNAME    [%-7s] text output file name\n",
          params.fname_out.c_str());
  fprintf(stderr,
          "  -tdrz,    --tinydiarize   [%-7s] enable tinydiarize (requires a "
          "tdrz model)\n",
          params.tinydiarize ? "true" : "false");
  fprintf(
      stderr,
      "  -sa,      --save-audio    [%-7s] save the recorded audio to a file\n",
      params.save_audio ? "true" : "false");
  fprintf(stderr, "  -ng,      --no-gpu        [%-7s] disable GPU inference\n",
          params.use_gpu ? "false" : "true");
  fprintf(
      stderr,
      "  -fa,      --flash-attn    [%-7s] flash attention during inference\n",
      params.flash_attn ? "true" : "false");
  fprintf(stderr, "\n");
}

bool whisper_params_parse(int argc, char **argv, whisper_params &params) {
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      whisper_print_usage(argc, argv, params);
      exit(0);
    } else if (arg == "-t" || arg == "--threads") {
      params.n_threads = std::stoi(argv[++i]);
    } else if (arg == "--step") {
      params.step_ms = std::stoi(argv[++i]);
    } else if (arg == "--length") {
      params.length_ms = std::stoi(argv[++i]);
    } else if (arg == "--keep") {
      params.keep_ms = std::stoi(argv[++i]);
    } else if (arg == "-c" || arg == "--capture") {
      params.capture_id = std::stoi(argv[++i]);
    } else if (arg == "-mt" || arg == "--max-tokens") {
      params.max_tokens = std::stoi(argv[++i]);
    } else if (arg == "-ac" || arg == "--audio-ctx") {
      params.audio_ctx = std::stoi(argv[++i]);
    } else if (arg == "-bs" || arg == "--beam-size") {
      params.beam_size = std::stoi(argv[++i]);
    } else if (arg == "-vth" || arg == "--vad-thold") {
      params.vad_thold = std::stof(argv[++i]);
    } else if (arg == "-fth" || arg == "--freq-thold") {
      params.freq_thold = std::stof(argv[++i]);
    } else if (arg == "-tr" || arg == "--translate") {
      params.translate = true;
    } else if (arg == "-nf" || arg == "--no-fallback") {
      params.no_fallback = true;
    } else if (arg == "-ps" || arg == "--print-special") {
      params.print_special = true;
    } else if (arg == "-kc" || arg == "--keep-context") {
      params.no_context = false;
    } else if (arg == "-l" || arg == "--language") {
      params.language = argv[++i];
    } else if (arg == "-m" || arg == "--model") {
      params.model = argv[++i];
    } else if (arg == "-f" || arg == "--file") {
      params.fname_out = argv[++i];
    } else if (arg == "-tdrz" || arg == "--tinydiarize") {
      params.tinydiarize = true;
    } else if (arg == "-sa" || arg == "--save-audio") {
      params.save_audio = true;
    } else if (arg == "-ng" || arg == "--no-gpu") {
      params.use_gpu = false;
    } else if (arg == "-fa" || arg == "--flash-attn") {
      params.flash_attn = true;
    }

    else {
      fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
      whisper_print_usage(argc, argv, params);
      exit(0);
    }
  }

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

  return true;
}

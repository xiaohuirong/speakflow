#include "inference.h"
#include "common-whisper.h"
#include "mylog.h"
#include "parse.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>

S2T::S2T(whisper_params &params) {

  cparams.use_gpu = params.use_gpu;
  cparams.flash_attn = params.flash_attn;
  ctx = whisper_init_from_file_with_params(params.model.c_str(), cparams);

  print_process_info(params, ctx);

  wparams = whisper_full_default_params(params.beam_size > 1
                                            ? WHISPER_SAMPLING_BEAM_SEARCH
                                            : WHISPER_SAMPLING_GREEDY);

  wparams.print_progress = false;
  wparams.print_special = params.print_special;
  wparams.print_realtime = false;
  wparams.print_timestamps = !params.no_timestamps;
  wparams.translate = params.translate;
  wparams.single_segment = !params.use_vad;
  wparams.max_tokens = params.max_tokens;
  wparams.language = params.language.c_str();
  wparams.n_threads = params.n_threads;
  wparams.beam_search.beam_size = params.beam_size;

  wparams.audio_ctx = params.audio_ctx;

  wparams.tdrz_enable = params.tinydiarize; // [TDRZ]

  // disable temperature fallback
  // wparams.temperature_inc  = -1.0f;
  wparams.temperature_inc = params.no_fallback ? 0.0f : wparams.temperature_inc;
}

S2T::~S2T() {
  whisper_print_timings(ctx);
  whisper_free(ctx);
}

static std::ofstream fout;

// duration = (t_last - t_start).count()
auto S2T::inference(whisper_params &params, std::vector<float> pcmf32)
    -> std::string
// run the inference
{
  std::string result;

  wparams.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
  wparams.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

  if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    // fprintf(stderr, "%s: failed to process audio\n", argv[0]);
    return {};
  }

  // print result;
  {
    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
      const char *text = whisper_full_get_segment_text(ctx, i);

      result += text;
      if (i != n_segments - 1) {
        result += ",";
      }

      if (params.no_timestamps) {
        printf("%s", text);
        fflush(stdout);

        if (params.fname_out.length() > 0) {
          fout << text;
        }
      } else {
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        std::string output = "[" + to_timestamp(t0, false) + " --> " +
                             to_timestamp(t1, false) + "]  " + text;

        if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
          output += " [SPEAKER_TURN]";
        }

        output += "\n";

        std::print("{}", output);
        fflush(stdout);

        if (params.fname_out.length() > 0) {
          fout << output;
        }
      }
    }

    if (params.fname_out.length() > 0) {
      fout << std::endl;
    }

    if (params.use_vad) {
      std::println("");
      std::println("### Transcription {} END", n_iter);
    }
  }

  ++n_iter;

  // Add tokens of the last full length segment as the prompt
  if (!params.no_context) {
    prompt_tokens.clear();

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
      const int token_count = whisper_full_n_tokens(ctx, i);
      for (int j = 0; j < token_count; ++j) {
        prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
      }
    }
  }

  fflush(stdout);
  return result;
}

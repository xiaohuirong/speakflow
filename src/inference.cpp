#include "inference.h"
#include "common-whisper.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>

static int n_iter = 0;
std::ofstream fout;

// duration = (t_last - t_start).count()
std::string inference(whisper_params &params,
                      std::vector<whisper_token> &prompt_tokens,
                      whisper_context *ctx, std::vector<float> &pcmf32,
                      int64_t duration, std::vector<float> &pcmf32_old)
// run the inference
{
  std::string result;
  whisper_full_params wparams = whisper_full_default_params(
      params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH
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

  wparams.prompt_tokens = params.no_context ? nullptr : prompt_tokens.data();
  wparams.prompt_n_tokens = params.no_context ? 0 : prompt_tokens.size();

  if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    // fprintf(stderr, "%s: failed to process audio\n", argv[0]);
    return nullptr;
  }

  // print result;
  {
    if (!params.use_vad) {
      printf("\33[2K\r");

      // print long empty line to clear the previous line
      printf("%s", std::string(100, ' ').c_str());

      printf("\33[2K\r");
    } else {
      const int64_t t1 = duration / 1000000;
      const int64_t t0 =
          std::max(0.0, t1 - pcmf32.size() * 1000.0 / WHISPER_SAMPLE_RATE);

      printf("\n");
      printf("### Transcription %d START | t0 = %d ms | t1 = %d ms\n", n_iter,
             (int)t0, (int)t1);
      printf("\n");
    }

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
      const char *text = whisper_full_get_segment_text(ctx, i);

      result += text;
      result += ",";

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

        printf("%s", output.c_str());
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
      printf("\n");
      printf("### Transcription %d END\n", n_iter);
    }
  }

  ++n_iter;

  if (!params.use_vad && (n_iter % params.n_new_line) == 0) {
    printf("\n");

    // keep part of the audio for next iteration to try to mitigate word
    // boundary issues
    pcmf32_old =
        std::vector<float>(pcmf32.end() - params.n_samples_keep, pcmf32.end());

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
  }
  fflush(stdout);
  return result;
}

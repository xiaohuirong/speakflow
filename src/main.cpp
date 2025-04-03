// Real-time speech recognition of input from a microphone
//
// A very quick-n-dirty implementation serving mainly as a proof of concept.
//
#include "chat.h"
#include "common-sdl.h"
#include "common.h"
#include "inference.h"
#include "mylog.h"
#include "parse.h"

#include <SDL_stdinc.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <whisper.h>

int main(int argc, char **argv) {
  whisper_params params;

  if (whisper_params_parse(argc, argv, params) == false) {
    return 1;
  }

  // init audio
  audio_async audio(params.length_ms);
  if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE,
                  (SDL_bool)params.is_microphone)) {
    fprintf(stderr, "%s: audio.init() failed!\n", __func__);
    return 1;
  }

  audio.resume();

  // whisper init
  if (params.language != "auto" &&
      whisper_lang_id(params.language.c_str()) == -1) {
    fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
    whisper_print_usage(argc, argv, params);
    exit(0);
  }

  struct whisper_context_params cparams = whisper_context_default_params();

  cparams.use_gpu = params.use_gpu;
  cparams.flash_attn = params.flash_attn;

  struct whisper_context *ctx =
      whisper_init_from_file_with_params(params.model.c_str(), cparams);

  std::vector<float> pcmf32(params.n_samples_30s, 0.0f);
  std::vector<float> pcmf32_old;
  std::vector<float> pcmf32_new(params.n_samples_30s, 0.0f);

  std::vector<whisper_token> prompt_tokens;

  print_process_info(params, ctx);
  bool is_running = true;

  std::ofstream fout;
  if (params.fname_out.length() > 0) {
    fout.open(params.fname_out);
    if (!fout.is_open()) {
      fprintf(stderr, "%s: failed to open output file '%s'!\n", __func__,
              params.fname_out.c_str());
      return 1;
    }
  }

  wav_writer wavWriter;
  // save wav file
  if (params.save_audio) {
    // Get current date/time for filename
    time_t now = time(0);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", localtime(&now));
    std::string filename = std::string(buffer) + ".wav";

    wavWriter.open(filename, WHISPER_SAMPLE_RATE, 16, 1);
  }
  printf("[Start speaking]\n");
  fflush(stdout);

  auto t_last = std::chrono::high_resolution_clock::now();
  const auto t_start = t_last;

  auto t_change = t_last;
  bool last_status = 0;

  Chat mychat = Chat(params);

  // main audio loop
  while (is_running) {
    if (params.save_audio) {
      wavWriter.write(pcmf32_new.data(), pcmf32_new.size());
    }
    // handle Ctrl + C
    is_running = sdl_poll_events();

    if (!is_running) {
      break;
    }

    // process new audio

    if (!params.use_vad) {
      while (true) {
        // handle Ctrl + C
        is_running = sdl_poll_events();
        if (!is_running) {
          break;
        }
        audio.get(params.step_ms, pcmf32_new);

        if ((int)pcmf32_new.size() > 2 * params.n_samples_step) {
          fprintf(stderr,
                  "\n\n%s: WARNING: cannot process audio fast enough, dropping "
                  "audio ...\n\n",
                  __func__);
          audio.clear();
          continue;
        }

        if ((int)pcmf32_new.size() >= params.n_samples_step) {
          audio.clear();
          break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      const int n_samples_new = pcmf32_new.size();

      // take up to params.length_ms audio from previous iteration
      const int n_samples_take =
          std::min((int)pcmf32_old.size(),
                   std::max(0, params.n_samples_keep + params.n_samples_len -
                                   n_samples_new));

      // printf("processing: take = %d, new = %d, old = %d\n", n_samples_take,
      // n_samples_new, (int) pcmf32_old.size());

      pcmf32.resize(n_samples_new + n_samples_take);

      for (int i = 0; i < n_samples_take; i++) {
        pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
      }

      memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(),
             n_samples_new * sizeof(float));

      pcmf32_old = pcmf32;
    } else {
      const auto t_now = std::chrono::high_resolution_clock::now();
      const auto t_diff =
          std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last)
              .count();

      if (t_diff < 2000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        continue;
      } else {
        t_last = t_now;
      }

      audio.get(2000, pcmf32_new);

      bool cur_status =
          !::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000, params.vad_thold,
                        params.freq_thold, false);
      if (cur_status ^ last_status) {
        last_status = cur_status;
        if (cur_status == 0) {
          const auto diff_len =
              std::chrono::duration_cast<std::chrono::milliseconds>(t_now -
                                                                    t_change);
          audio.get(diff_len.count() + 2000, pcmf32);
        } else {
          t_change = t_now;
          continue;
        }
      } else {
        continue;
      }
    }

    std::string result = inference(params, prompt_tokens, ctx, pcmf32,
                                   (t_last - t_start).count(), pcmf32_old);
    std::cout << result << std::endl;

    auto callback = [](const std::string &response) {
      std::cout << "Response received: " << response << std::endl;
    };

    mychat.send_async(result, callback);
  }

  audio.pause();

  whisper_print_timings(ctx);
  whisper_free(ctx);

  return 0;
}

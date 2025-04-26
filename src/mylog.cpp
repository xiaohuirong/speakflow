#include "mylog.h"

#include <print>

void print_process_info(whisper_params &params, whisper_context *ctx)
// print some info about the processing
{
  println(stderr, "");
  if (!whisper_is_multilingual(ctx)) {
    if (params.language != "en" || params.translate) {
      params.language = "en";
      params.translate = false;
      println(stderr,
              "{}: WARNING: model is not multilingual, ignoring language "
              "and translation options",
              __func__);
    }
  }
  println(
      stderr,
      "{}: processing {} samples (step = {:.1f} sec / len = {:.1f} sec / keep "
      "= {:.1f} sec), {} threads, lang = {}, task = {}, timestamps = {} ...",
      __func__, params.n_samples_step,
      float(params.n_samples_step) / WHISPER_SAMPLE_RATE,
      float(params.n_samples_len) / WHISPER_SAMPLE_RATE,
      float(params.n_samples_keep) / WHISPER_SAMPLE_RATE, params.n_threads,
      params.language, params.translate ? "translate" : "transcribe",
      params.no_timestamps ? 0 : 1);

  if (!params.use_vad) {
    println(stderr, "{}: n_new_line = {}, no_context = {:d}", __func__,
            params.n_new_line, params.no_context);
  } else {
    println(stderr, "{}: using VAD, will transcribe on speech activity",
            __func__);
  }

  println(stderr, "");
}

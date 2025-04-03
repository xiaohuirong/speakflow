#include "mylog.h"

void print_process_info(whisper_params &params, whisper_context *ctx)
// print some info about the processing
{
  fprintf(stderr, "\n");
  if (!whisper_is_multilingual(ctx)) {
    if (params.language != "en" || params.translate) {
      params.language = "en";
      params.translate = false;
      fprintf(stderr,
              "%s: WARNING: model is not multilingual, ignoring language and "
              "translation options\n",
              __func__);
    }
  }
  fprintf(
      stderr,
      "%s: processing %d samples (step = %.1f sec / len = %.1f sec / keep = "
      "%.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
      __func__, params.n_samples_step,
      float(params.n_samples_step) / WHISPER_SAMPLE_RATE,
      float(params.n_samples_len) / WHISPER_SAMPLE_RATE,
      float(params.n_samples_keep) / WHISPER_SAMPLE_RATE, params.n_threads,
      params.language.c_str(), params.translate ? "translate" : "transcribe",
      params.no_timestamps ? 0 : 1);

  if (!params.use_vad) {
    fprintf(stderr, "%s: n_new_line = %d, no_context = %d\n", __func__,
            params.n_new_line, params.no_context);
  } else {
    fprintf(stderr, "%s: using VAD, will transcribe on speech activity\n",
            __func__);
  }

  fprintf(stderr, "\n");
}

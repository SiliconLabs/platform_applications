#include "sml_output.h"
#include "kb.h"

#define SERIAL_OUT_CHARS_MAX 512

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

static char serial_out_buf[SERIAL_OUT_CHARS_MAX];
static uint8_t recent_fv[MAX_VECTOR_SIZE];
static uint16_t recent_fv_len;

static char *p_serial_out = serial_out_buf;

static void sml_output_serial(uint16_t model, uint16_t classification)
{
  int written = 0;

  written += snprintf(serial_out_buf,
                      sizeof(serial_out_buf) - 1,
                      "{\"ModelNumber\":%d,\"Classification\":%d,\"FeatureLength\":%d,\"FeatureVector\":[",
                      model,
                      classification,
                      recent_fv_len);
  for (int j = 0; j < recent_fv_len; j++) {
    written += snprintf(&serial_out_buf[written],
                        sizeof(serial_out_buf) - written,
                        "%d",
                        recent_fv[j]);
    if (j < recent_fv_len - 1) {
      serial_out_buf[written++] = ',';
    }
  }
  serial_out_buf[written++] = ']';
  serial_out_buf[written++] = '}';

  printf("%s\r\n", p_serial_out);
}

uint32_t sml_output_results(uint16_t model, uint16_t classification)
{
  uint8_t fv_len;

  sml_get_feature_vector(model, recent_fv, &fv_len);
  recent_fv_len = (uint16_t)fv_len;
  sml_output_serial(model, classification);
  // FILL_SML_OUTPUT_FUNCTIONS
  return 0;
}
// SensiML Includes
#include "kb.h"
#include "sml_output.h" // Contains functions for outputting results.
#include "sml_recognition_run.h"

// FILL_USE_TEST_DATA

#ifdef SML_USE_TEST_DATA
#include "testdata.h"
int td_index = 0;
#endif // SML_USE_TEST_DATA

int sml_recognition_run(signed short *data_batch,
                        int batch_sz,
                        uint8_t num_sensors,
                        uint32_t sensor_id)
{
  (void) sensor_id;
  int ret = 0;

  int batch_index = 0;
  signed short *data;
  for (batch_index = 0; batch_index < batch_sz; batch_index++) {
#ifdef SML_USE_TEST_DATA
    ret = kb_run_model((SENSOR_DATA_T *)&testdata[td_index++], TD_NUMCOLS, 0);
    if (td_index >= TD_NUMROWS) {
      td_index = 0;
    }
    if (ret >= 0) {
      kb_print_model_result(0, ret);
      sml_output_results(0, ret);
      kb_reset_model(0);
    }
#else
    data = &data_batch[batch_index * num_sensors];

    ret = kb_run_model((SENSOR_DATA_T *)data,
                       num_sensors,
                       KB_MODEL_slide_simple_rank_0_INDEX);
    if (ret >= 0) {
      sml_output_results(KB_MODEL_slide_simple_rank_0_INDEX, ret);
      kb_reset_model(0);
    }
#endif // SML_USE_TEST_DATA
  }
  return ret;
}

#ifndef __SML_OUTPUT_H_
#define __SML_OUTPUT_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


uint32_t sml_output_results(uint16_t model, uint16_t classification);

uint32_t sml_output_init(void * p_module);

#endif //__SML_OUTPUT_H_
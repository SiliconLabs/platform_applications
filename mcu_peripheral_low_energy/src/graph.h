/*
 * graph.h
 *
 *  Created on: Jun 3, 2019
 *      Author: siwoo
 */

#ifndef SRC_GRAPH_H_
#define SRC_GRAPH_H_

#include <stdint.h>
#include "glib.h"

#define GRAPH_WIDTH	128
#define GRAPH_HEIGHT	128

void Graph_Init(void);

void Graph_Plot(uint32_t x, uint32_t y, uint32_t color);

#endif /* SRC_GRAPH_H_ */

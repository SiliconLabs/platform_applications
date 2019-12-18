/*
 * graph.c
 *
 *  Created on: Jun 3, 2019
 *      Author: siwoo
 */

#include "graph.h"

#include "em_cmu.h"
#include "display.h"
#include "retargettextdisplay.h"
#include "textdisplay.h"
#include "dmd.h"

GLIB_Rectangle_t graphArea = { .xMin = 0, .xMax = GRAPH_WIDTH, .yMin = 0, .yMax = GRAPH_HEIGHT };
GLIB_Context_t context;

/// Enumerated 3 bit color options
/// Colors are ordered for 3 bit 0bBGR
typedef enum _Colors{
  BLACK,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
  CYAN,
  WHITE
} Color;

void FillPoint(uint32_t x, uint32_t y);

void Graph_Init(void)
{
  CMU_HFRCOBandSet(cmuHFRCOFreq_72M0Hz);

  EMSTATUS status = DMD_init(0);
  if (DMD_OK != status) {
	  while (1) ;
  }
  status = GLIB_contextInit(&context);
  if(status != GLIB_OK){
	  while(1);
  }

  context.foregroundColor = White;
  GLIB_drawRectFilled(&context, &graphArea);
  context.backgroundColor = White;
  context.foregroundColor = Red;
}

void Graph_Plot(uint32_t x, uint32_t y, uint32_t color)
{
  // Map Adc data to y axis scaling
  y = y * GRAPH_HEIGHT / 4096;

  // Clear previous point
  context.foregroundColor = White;
  GLIB_drawLineV(&context, x, 0, GRAPH_HEIGHT);
  GLIB_drawLineV(&context, x + 1, 0, GRAPH_HEIGHT);

  context.foregroundColor = color;
  FillPoint(x, y);

  DMD_updateDisplay();
}

void FillPoint(uint32_t x, uint32_t y)
{
  GLIB_drawPixel(&context, x, y);
  GLIB_drawPixel(&context, x + 1, y);
  GLIB_drawPixel(&context, x, y + 1);
  GLIB_drawPixel(&context, x + 1, y + 1);
}


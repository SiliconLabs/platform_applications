/***************************************************************************//**
 * @file graph.c
 * @version 1.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

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


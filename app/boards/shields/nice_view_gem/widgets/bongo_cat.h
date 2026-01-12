/*
 * Copyright (c) 2024 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>
#include "util.h"

// Bongocat dimensions (above the layer name)
#define BONGO_CAT_WIDTH 64
#define BONGO_CAT_HEIGHT 32
#define BONGO_CAT_X ((SCREEN_WIDTH - BONGO_CAT_WIDTH) / 2)
#define BONGO_CAT_Y 35  // Above layer name (layer at y=70)

// Animation speed thresholds (WPM)
#define BONGO_IDLE_WPM 5
#define BONGO_SLOW_WPM 30
#define BONGO_MID_WPM 60

// Animation timing (ms)
#define BONGO_IDLE_INTERVAL 800
#define BONGO_SLOW_INTERVAL 400
#define BONGO_MID_INTERVAL 200
#define BONGO_FAST_INTERVAL 100

void draw_bongo_cat(lv_obj_t *canvas, const struct status_state *state);

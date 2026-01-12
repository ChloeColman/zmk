#include <zephyr/kernel.h>
#include <math.h>
#include "wpm.h"
#include "../assets/custom_fonts.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// WPM chart dimensions
#define WPM_CHART_X 38
#define WPM_CHART_Y 100
#define WPM_CHART_WIDTH 67
#define WPM_CHART_HEIGHT 33

// Gauge dimensions
#define GAUGE_CENTER_X 72
#define GAUGE_CENTER_Y 65
#define GAUGE_RADIUS 28
#define NEEDLE_LENGTH 24

// Config defaults (can be overridden in Kconfig)
#ifndef CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE_MAX
#define CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE_MAX 100
#endif

LV_IMG_DECLARE(grid);

static void draw_gauge(lv_obj_t *canvas, uint8_t wpm) {
    lv_draw_arc_dsc_t arc_dsc;
    lv_draw_arc_dsc_init(&arc_dsc);
    arc_dsc.color = LVGL_FOREGROUND;
    arc_dsc.width = 2;

    // Draw gauge arc (from 225 to 315 degrees)
    lv_canvas_draw_arc(canvas, GAUGE_CENTER_X, GAUGE_CENTER_Y, GAUGE_RADIUS, 225, 315, &arc_dsc);

    // Draw tick marks
    lv_draw_line_dsc_t tick_dsc;
    init_line_dsc(&tick_dsc, LVGL_FOREGROUND, 1);

    for (int i = 0; i <= 4; i++) {
        float angle = (225 + i * 22.5) * M_PI / 180.0;
        int x1 = GAUGE_CENTER_X + (int)((GAUGE_RADIUS - 3) * cos(angle));
        int y1 = GAUGE_CENTER_Y - (int)((GAUGE_RADIUS - 3) * sin(angle));
        int x2 = GAUGE_CENTER_X + (int)((GAUGE_RADIUS + 2) * cos(angle));
        int y2 = GAUGE_CENTER_Y - (int)((GAUGE_RADIUS + 2) * sin(angle));

        lv_point_t points[2] = {{x1, y1}, {x2, y2}};
        lv_canvas_draw_line(canvas, points, 2, &tick_dsc);
    }

    // Calculate needle angle based on WPM (225 deg at 0, 315 deg at max)
    uint8_t max_wpm = CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE_MAX;
    uint8_t clamped_wpm = (wpm > max_wpm) ? max_wpm : wpm;
    float ratio = (float)clamped_wpm / max_wpm;
    float needle_angle = (225 + ratio * 90) * M_PI / 180.0;

    // Draw needle
    lv_draw_line_dsc_t needle_dsc;
    init_line_dsc(&needle_dsc, LVGL_FOREGROUND, 2);

    int needle_x = GAUGE_CENTER_X + (int)(NEEDLE_LENGTH * cos(needle_angle));
    int needle_y = GAUGE_CENTER_Y - (int)(NEEDLE_LENGTH * sin(needle_angle));

    lv_point_t needle_points[2] = {
        {GAUGE_CENTER_X, GAUGE_CENTER_Y},
        {needle_x, needle_y}
    };
    lv_canvas_draw_line(canvas, needle_points, 2, &needle_dsc);

    // Draw center dot
    lv_draw_rect_dsc_t dot_dsc;
    init_rect_dsc(&dot_dsc, LVGL_FOREGROUND);
    dot_dsc.radius = LV_RADIUS_CIRCLE;
    lv_canvas_draw_rect(canvas, GAUGE_CENTER_X - 2, GAUGE_CENTER_Y - 2, 5, 5, &dot_dsc);
}

static void draw_chart(lv_obj_t *canvas, const uint8_t *wpm_history) {
    // Draw grid background
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, WPM_CHART_X, WPM_CHART_Y, &grid, &img_dsc);

    // Find min/max for scaling
    uint8_t min_wpm = 255;
    uint8_t max_wpm = 0;

#if IS_ENABLED(CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE)
    min_wpm = 0;
    max_wpm = CONFIG_NICE_VIEW_GEM_WPM_FIXED_RANGE_MAX;
#else
    for (int i = 0; i < WPM_HISTORY_SIZE; i++) {
        if (wpm_history[i] < min_wpm) min_wpm = wpm_history[i];
        if (wpm_history[i] > max_wpm) max_wpm = wpm_history[i];
    }
    if (max_wpm == min_wpm) max_wpm = min_wpm + 1;
#endif

    // Draw line chart
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);

    float x_step = (float)(WPM_CHART_WIDTH - 4) / (WPM_HISTORY_SIZE - 1);

    for (int i = 0; i < WPM_HISTORY_SIZE - 1; i++) {
        float y1_ratio = (float)(wpm_history[i] - min_wpm) / (max_wpm - min_wpm);
        float y2_ratio = (float)(wpm_history[i + 1] - min_wpm) / (max_wpm - min_wpm);

        // Clamp ratios
        if (y1_ratio > 1.0f) y1_ratio = 1.0f;
        if (y2_ratio > 1.0f) y2_ratio = 1.0f;

        int x1 = WPM_CHART_X + 2 + (int)(i * x_step);
        int y1 = WPM_CHART_Y + WPM_CHART_HEIGHT - 2 - (int)(y1_ratio * (WPM_CHART_HEIGHT - 4));
        int x2 = WPM_CHART_X + 2 + (int)((i + 1) * x_step);
        int y2 = WPM_CHART_Y + WPM_CHART_HEIGHT - 2 - (int)(y2_ratio * (WPM_CHART_HEIGHT - 4));

        lv_point_t points[2] = {{x1, y1}, {x2, y2}};
        lv_canvas_draw_line(canvas, points, 2, &line_dsc);
    }
}

static void draw_wpm_text(lv_obj_t *canvas, uint8_t current_wpm) {
    // Draw "WPM" label
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &quinquefive_8, LV_TEXT_ALIGN_CENTER);
    lv_canvas_draw_text(canvas, 0, 38, SCREEN_WIDTH, &label_dsc, "WPM");

    // Draw current WPM value
    char wpm_str[8];
    snprintf(wpm_str, sizeof(wpm_str), "%d", current_wpm);

    lv_draw_label_dsc_t value_dsc;
    init_label_dsc(&value_dsc, LVGL_FOREGROUND, &quinquefive_24, LV_TEXT_ALIGN_CENTER);
    lv_canvas_draw_text(canvas, 0, 48, SCREEN_WIDTH, &value_dsc, wpm_str);
}

void draw_wpm_status(lv_obj_t *canvas, const struct status_state *state) {
    draw_gauge(canvas, state->wpm[WPM_HISTORY_SIZE - 1]);
    draw_chart(canvas, state->wpm);
    draw_wpm_text(canvas, state->wpm[WPM_HISTORY_SIZE - 1]);
}

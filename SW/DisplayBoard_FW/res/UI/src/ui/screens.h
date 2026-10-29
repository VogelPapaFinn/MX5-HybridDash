#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_RPM = 1,
    SCREEN_ID_SPEED = 2,
    SCREEN_ID_TEMPERATURE = 3,
    _SCREEN_ID_LAST = 3
};

typedef struct _objects_t {
    lv_obj_t *rpm;
    lv_obj_t *speed;
    lv_obj_t *temperature;
    lv_obj_t *rpm_label;
    lv_obj_t *obj0;
    lv_obj_t *left_indicator;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *speed_label;
    lv_obj_t *obj3;
    lv_obj_t *right_indicator;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *temperature_label;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *arc0;
    lv_obj_t *arc1;
    lv_obj_t *arc2;
    lv_obj_t *arc3;
    lv_obj_t *arc4;
    lv_obj_t *arc5;
    lv_obj_t *arc6;
    lv_obj_t *arc7;
    lv_obj_t *arc8;
    lv_obj_t *arc9;
    lv_obj_t *percent_label;
    lv_obj_t *liter_label;
    lv_obj_t *oil_can;
} objects_t;

extern objects_t objects;

void create_screen_rpm();
void tick_screen_rpm();

void create_screen_speed();
void tick_screen_speed();

void create_screen_temperature();
void tick_screen_temperature();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
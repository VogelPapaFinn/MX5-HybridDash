#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_rpm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.rpm = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // RpmLabel
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.rpm_label = obj;
            lv_obj_set_pos(obj, 0, -11);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_e1234, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "0000");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 159, 94);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_vcr_osd_mono, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "RPM");
        }
        {
            // LeftIndicator
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.left_indicator = obj;
            lv_obj_set_pos(obj, 161, 246);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_left_indicator);
            lv_obj_set_style_width(obj, 38, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_image_opa(obj, 51, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 51, 42);
            lv_obj_set_size(obj, 258, 252);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 0, 169);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 0, 0 },
                { 360, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 180, 0 },
                { 180, 360 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x19ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_rpm();
}

void tick_screen_rpm() {
}

void create_screen_speed() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.speed = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // SpeedLabel
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.speed_label = obj;
            lv_obj_set_pos(obj, 0, -11);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_e1234, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "0000");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 159, 94);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_vcr_osd_mono, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "kmh");
        }
        {
            // RightIndicator
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.right_indicator = obj;
            lv_obj_set_pos(obj, 161, 246);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_left_indicator);
            lv_image_set_rotation(obj, 1800);
            lv_obj_set_style_width(obj, 38, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 48, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_image_opa(obj, 51, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 51, 42);
            lv_obj_set_size(obj, 258, 252);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 0, 169);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 0, 0 },
                { 360, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 180, 0 },
                { 180, 360 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x19ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 360);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    tick_screen_speed();
}

void tick_screen_speed() {
}

void create_screen_temperature() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.temperature = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 360);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // TemperatureLabel
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.temperature_label = obj;
            lv_obj_set_pos(obj, 124, 148);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_e1234, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "90");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 267, 191);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_vcr_osd_mono, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "°C");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            lv_obj_set_pos(obj, 51, 42);
            lv_obj_set_size(obj, 258, 252);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 0, 169);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 0, 0 },
                { 360, 0 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_line_color(obj, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_line_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            static lv_point_precise_t line_points[] = {
                { 180, 0 },
                { 180, 360 }
            };
            lv_line_set_points(obj, line_points, 2);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_line_color(obj, lv_color_hex(0x19ff00), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc0
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc0 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 120 );
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x992600), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc1
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc1 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 133);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xc69800), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc2
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc2 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 146);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xc69800), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc3
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc3 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 159);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc4
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc4 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 172);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc5
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc5 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 185);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc6
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc6 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 198);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc7
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc7 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 211);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc8
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc8 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 224);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Arc9
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.arc9 = obj;
            lv_obj_set_pos(obj, 24, 29);
            lv_obj_set_size(obj, 313, 302);
            lv_arc_set_range(obj, 100, 100);
            lv_arc_set_value(obj, 0);
            lv_arc_set_bg_start_angle(obj, 0);
            lv_arc_set_bg_end_angle(obj, 12);
            lv_arc_set_rotation(obj, 237);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_opa(obj, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 40, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_rounded(obj, false, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // PercentLabel
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.percent_label = obj;
            lv_obj_set_pos(obj, 145, 52);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_vcr_osd_mono, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "100%");
        }
        {
            // LiterLabel
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.liter_label = obj;
            lv_obj_set_pos(obj, 124, 284);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x008f3c), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_vcr_osd_mono, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "50L");
        }
        {
            // OilCan
            lv_obj_t *obj = lv_image_create(parent_obj);
            objects.oil_can = obj;
            lv_obj_set_pos(obj, 204, 240);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_oil_can);
        }
    }
    
    tick_screen_temperature();
}

void tick_screen_temperature() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_rpm,
    tick_screen_speed,
    tick_screen_temperature,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 3) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "E1234", &ui_font_e1234 },
    { "VCR OSD MONO", &ui_font_vcr_osd_mono },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_display_t *dispp = lv_display_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_display_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_rpm();
    create_screen_speed();
    create_screen_temperature();
}
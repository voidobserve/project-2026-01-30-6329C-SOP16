#ifndef __LIGHTING_ANIMATION_H
#define __LIGHTING_ANIMATION_H

#include "includes.h"
#include "Adafruit_NeoPixel.H" // 包含灯光像素排列的定义

// #define LIGHTING_ANIMATION_LED_NUMS (12)                       // 灯光数量
#define LIGHTING_ANIMATION_RGB_NEOPIXEL_PERMUTATIONS (NEO_RGB) // RGB像素排列  (NEO_RGB RGB 顺序 R->G->B)

extern const u32 color_buff[7];         // 除了样机的mode13，其他模式下使用到的数组
extern const u32 color_buff_mode13[14]; // 对应样机的mode13，使用到的数组

u16 WS2812FX_sample_8(void);
u16 WS2812FX_sample_9(void);
u16 WS2812FX_sample_12(void);
u16 WS2812FX_sample_14(void);
u16 WS2812FX_sample_16(void);
u16 WS2812FX_sample_18(void);
u16 WS2812FX_sample_19(void);
u16 WS2812FX_sample_20(void);

u16 WS2812FX_sample_10(void);
u16 WS2812FX_sample_11(void);
u16 WS2812FX_sample_13(void);
u16 WS2812FX_sample_15(void);
u16 WS2812FX_sample_17(void);

u16 WS2812FX_sample_single_color_meteor_light(void);

void lighting_animation_mode_setting(u8 mode_index);
void lighting_animation_mode_change(void);

void lighting_animation_mode_add(void);
void lighting_animation_mode_sub(void);

void lighting_animation_speed_add(void);
void lighting_animation_speed_sub(void);

void lighting_animation_time_interval_add(void); // 时间间隔加
void lighting_animation_time_interval_sub(void); // 时间间隔减
void lighting_animation_time_interval_fast(void); // 时间间隔设置为最快
void lighting_animation_time_interval_mid(void); // 时间间隔设置为适中
void lighting_animation_time_interval_slow(void);// 时间间隔设置为最慢


void lighting_animation_dir_switch(void); // 切换方向
void lighting_animation_speed_max(void);  // 动画设置为最快速度
void lighting_animation_speed_min(void);  // 动画设置为最慢速度
void lighting_animation_speed_mid(void);  // 动画设置为中速



void lighting_animation_bright_add(void); // 亮度加
void lighting_animation_bright_sub(void); // 亮度减

// 初始化（恢复出厂设置）
void lighting_animation_init(void);
void lighting_animation_config_init(void);
#endif

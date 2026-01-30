#include "system/includes.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "ws2812fx_effect.h"
#include "Adafruit_NeoPixel.H"
#include "led_strip_drive.h"
#include "app_main.h"
#include "asm/mcpwm.h"

#include "../../apps/user_app/lighting_animation/lighting_animation.h"

// function_control effect
volatile fc_effect_t fc_effect; // 幻彩灯串效果数据
extern void set_fc_effect(void);

// FADE_SLOW：12颗
// FADE_MEDIUM：6颗
// FADE_FAST：5颗灯
// FADE_XFAST:3颗灯
const u8 fade_type[3] =
    {
        FADE_XFAST, FADE_FAST, FADE_MEDIUM //,FADE_SLOW
};

#define segment_num 1
#define _0_seg_start 0
#define _0_seg_stop 0

/**
 * @brief 设置段的颜色
 *
 * @param n
 * @param c
 */
void ls_set_colors(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;

#if LED_STRIP_RGBW

    for (i = 0; i < n; i++)
    {
        colors[i] = c[i].w << 24 | c[i].r << 16 | c[i].g << 8 | c[i].b;
    }

#elif LED_STRIP_RGB

    for (i = 0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }

#endif

    WS2812FX_setColors(0, colors);
}

// 设置fc_effect.dream_scene.rgb的颜色池
// n:0-MAX_NUM_COLORS
// c:WS2812FX颜色系，R<<16,G<<8,B在低8位
void ls_set_color(uint8_t n, uint32_t c)
{
    if (n < MAX_NUM_COLORS)
    {

#if LED_STRIP_RGBW
        fc_effect.dream_scene.rgb[n].w = (c >> 24) & 0xff;
        fc_effect.dream_scene.rgb[n].r = (c >> 16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c >> 8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;
#elif LED_STRIP_RGB
        fc_effect.dream_scene.rgb[n].r = (c >> 16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c >> 8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;

#endif
    }
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

/*----------------------------------静态色效果----------------------------------*/
// static void static_mode(void)
void static_mode(void)
{
    extern uint16_t WS2812FX_mode_static(void);
    // WS2812FX_stop(); // 加上这个，在app色盘调节时会造成灯光闪烁

    // WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
    // ls_set_colors(1, &fc_effect.rgb); // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

    WS2812FX_setSegment_colorOptions(    // 设置一段颜色的效果
        0,                               // 第0段
        0,                               // 起始位置
        // LIGHTING_ANIMATION_LED_NUMS - 1, // 结束位置(函数内部会给这个参数加一)
        fc_effect.led_num - 1, // 结束位置(函数内部会给这个参数加一)
        &WS2812FX_mode_static,           // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        100,                             // 速度
        0);                              // 选项，这里像素点大小：1

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
    ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

    WS2812FX_start();
}

/*----------------------------------彩虹效果----------------------------------*/
static void strand_rainbow(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_mutil_fade,   // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_SMALL);                 // 选项，这里像素点大小：1

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

static void SM_rainbow_eff(void)
{
    extern uint16_t SM_mode_rainbow(void);
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, fc_effect.led_num - 1,    // 起始位置，结束位置
        &SM_mode_rainbow,            // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        0);                          // 选项，这里像素点大小：3,反向/反向
    WS2812FX_start();
}

/*----------------------------------跳变效果----------------------------------*/
void strand_jump_change(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                // 第0段
        0, 0,                             // 起始位置，结束位置
        &WS2812FX_mode_single_block_scan, // 效果
        0,                                // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,      // 速度
        SIZE_MEDIUM);                     // 选项，这里像素点大小：3

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}
/*----------------------------------呼吸系列效果----------------------------------*/
void strand_breath(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_mutil_breath, // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_MEDIUM                  // 选项，这里像素点大小：3
    );

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

// 改了森木专用的呼吸效果
void single_c_breath(void)
{

    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &breath_rgb,                 // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速 度
        SIZE_MEDIUM                  // 选项，这里像素点大小：3
    );

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

// 多色循环呼吸
void multi_c_breath_cycle(void)
{

    extern uint16_t mutil_c_breath(void);

    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, fc_effect.led_num - 1,    // 起始位置，结束位置
        &mutil_c_breath,             // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        0);                          // 选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

/*----------------------------------闪烁效果----------------------------------*/
void strand_twihkle(void)
{

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mode_mutil_twihkle,    // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed * 4, // 速度
        SIZE_SMALL);                     // 选项，这里像素点大小：1
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}
// 多颜色频闪
void ls_strobe(void)
{

    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mutil_strobe,          // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed * 5, // 速度
        0);                              // 选项，这里像素点大小：3

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}
/*----------------------------------流水效果----------------------------------*/
void strand_flow_water(void)
{
    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else
    {
        option = SIZE_MEDIUM | REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mode_multi_block_scan, // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,     // 速度
        option);                         // 选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------追光效果----------------------------------*/
void strand_chas_light(void)
{

    WS2812FX_stop();
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        WS2812FX_setSegment_colorOptions(
            0,                                 // 第0段
            0, 0,                              // 起始位置，结束位置
            &WS2812FX_mode_multi_forward_same, // 效果
            0,                                 // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,       // 速度
            0);                                // 选项
    }
    else
    {
        WS2812FX_setSegment_colorOptions(
            0,                              // 第0段
            0, 0,                           // 起始位置，结束位置
            &WS2812FX_mode_multi_back_same, // 效果
            0,                              // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,    // 速度
            0);
    }
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------炫彩效果----------------------------------*/
void strand_colorful(void)
{
    uint8_t option;
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mode_multi_block_scan, // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,     // 速度
        SIZE_SMALL);                     // 选项，这里像素点大小：1
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}

/*----------------------------------渐变系列效果----------------------------------*/
void strand_grandual(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_mutil_fade,   // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_MEDIUM);                // 选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_start();
}
// 整条灯带渐变，支持多种颜色之间切换
// 颜色池：fc_effect.dream_scene.rgb[]
// 颜色数量fc_effect.dream_scene.c_n
void mutil_c_grandual(void)
{
    extern uint16_t WS2812FX_mutil_c_gradual(void);
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mutil_c_gradual,   // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_MEDIUM);                // 选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

// 纯白色渐变
void w_grandual(void)
{

    extern uint16_t breath_w(void);

    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &breath_w,                   // 效果
        WHITE,                       // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        0);                          // 选项，这里像素点大小：3,反向/反向

    WS2812FX_start();
}

/*----------------------------------跳变效果----------------------------------*/
void standard_jump(void)
{
    extern uint16_t WS2812FX_mutil_c_jump(void);
    // WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                  // 第0段
        0, 0,                               // 起始位置，结束位置
        &WS2812FX_mutil_c_jump,             // 效果
        0,                                  // 颜色，WS2812FX_setColors设置
        (fc_effect.dream_scene.speed * 40), // 速度
        0);                                 // 选项，这里像素点大小：3

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    WS2812FX_start();
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

void strand_meteor(u8 index)
{

    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                               // 第0段
        1, fc_effect.led_num,            // 起始位置，结束位置
        &WS2812FX_mode_comet_1,          // 效果
        WHITE,                           // 颜色，WS2812FX_setColors设置
        fc_effect.star_speed,            // 速度
        fade_type[index - 19] | option); // 选项，这里像素点大小：3,反向/反向
    WS2812FX_start();
}

void double_meteor(void)
{

    extern uint16_t fc_double_meteor(void);
    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                    // 第0段
        1, fc_effect.led_num, // 起始位置，结束位置
        &fc_double_meteor,    // 效果
        WHITE,                // 颜色，WS2812FX_setColors设置
        fc_effect.star_speed, // 速度
        option);              // 选项，这里像素点大小：3,反向/反向

    WS2812FX_start();
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

/**
 * @brief APP模式中，基本的七彩动态效果集合
 *
 * @param tp_num
 */
void base_Dynamic_Effect(u8 tp_num)
{
    switch (tp_num)
    {
    case 0x07: // 3色跳变
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        fc_effect.dream_scene.change_type = MODE_JUMP;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x08: // 7色跳变
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, MAGENTA);
        ls_set_color(6, WHITE);
        fc_effect.dream_scene.change_type = MODE_JUMP;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x09: // 3色渐变
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x0A: // 七彩渐变
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x0B:
        ls_set_color(0, RED);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x0c:
        ls_set_color(0, BLUE);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;
    case 0x0D:
        ls_set_color(0, GREEN);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x0E:
        ls_set_color(0, CYAN);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x0F:
        ls_set_color(0, YELLOW);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x10:
        ls_set_color(0, MAGENTA);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x11:

#if LED_STRIP_RGBW

        ls_set_color(0, ULTRAWHITE);

#elif LED_STRIP_RGB

        ls_set_color(0, WHITE);
#endif

        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

        // case 0x12:
        //     ls_set_color(0, WHITE);
        //     ls_set_color(1, BLACK);
        //     fc_effect.dream_scene.change_type = MODE_BREATH_W;
        //     fc_effect.dream_scene.c_n = 2;
        //     fc_effect.Now_state = IS_light_scene;
        //     break;

    case 0x12:
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x13:
        ls_set_color(0, BLUE);
        ls_set_color(1, RED);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x14:
        ls_set_color(0, GREEN);
        ls_set_color(1, BLUE);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x15: // 七色频闪
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);

#if LED_STRIP_RGBW

        ls_set_color(3, ULTRAWHITE);

#elif LED_STRIP_RGB

        ls_set_color(3, WHITE);
#endif

        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, MAGENTA);

        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;

        break;

    case 0x16:
        ls_set_color(0, RED);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;

        break;

    case 0x17:
        ls_set_color(0, BLUE);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;

        break;

    case 0x18:
        ls_set_color(0, GREEN);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;

        break;
    case 0x19:

        ls_set_color(0, CYAN);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;

        break;

    case 0x1a:

        ls_set_color(0, YELLOW);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;

        break;
    case 0x1b:

        ls_set_color(0, PURPLE);
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x1c:

#if LED_STRIP_RGBW

        ls_set_color(0, ULTRAWHITE);

#elif LED_STRIP_RGB

        ls_set_color(0, WHITE);
#endif
        fc_effect.dream_scene.change_type = MODE_STROBE;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
        break;

    case 0x21: // 七彩呼吸

        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, MAGENTA);
        ls_set_color(6, ULTRAWHITE);

        fc_effect.dream_scene.c_n = 7;
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_BREATH;
        fc_effect.Now_state = IS_light_scene;

        break;

    case 0x22: // 蓝白呼吸
        ls_set_color(0, 0xFF0000FF);
        ls_set_color(1, BLACK);
        fc_effect.dream_scene.change_type = MODE_SINGLE_C_BREATH;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;
    case 0x23: // 蓝白渐变
        ls_set_color(0, BLUE);
        ls_set_color(1, ULTRAWHITE);
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
        break;
    case 0x24: // 蓝色呼吸，白色呼吸，蓝白呼吸

        ls_set_color(0, BLUE);
        ls_set_color(1, ULTRAWHITE);
        ls_set_color(2, 0xFF0000FF);

        fc_effect.dream_scene.c_n = 3;
        fc_effect.dream_scene.change_type = MODE_MUTIL_C_BREATH;
        fc_effect.Now_state = IS_light_scene;
        break;
    }
    set_fc_effect();
}

/**
 * @brief 情景效果集合
 *
 */
static void ls_scene_effect(void)
{
    app_set_bright(100);
    switch (fc_effect.dream_scene.change_type)
    {

    case MODE_MUTIL_RAINBOW: // 彩虹
        strand_rainbow();
        break;

    case MODE_MUTIL_JUMP: // 跳变模式
        strand_jump_change();
        break;

    case MODE_MUTIL_BRAETH: // 呼吸模式
        strand_breath();
        break;

    case MODE_MUTIL_TWIHKLE: // 闪烁模式
        strand_twihkle();
        break;

    case MODE_MUTIL_FLOW_WATER: // 流水模式
        strand_flow_water();
        break;

    case MODE_CHAS_LIGHT: // 追光模式
        strand_chas_light();
        break;

    case MODE_MUTIL_COLORFUL: // 炫彩模式
        strand_colorful();
        break;

    case MODE_MUTIL_SEG_GRADUAL: // 渐变模式
        strand_grandual();
        break;

    case MODE_JUMP: // 标准跳变
        standard_jump();
        break;

    case MODE_MUTIL_C_GRADUAL: // 多段同时渐变
        mutil_c_grandual();
        break;

    case MODE_BREATH_W: // 白色渐变
        w_grandual();
        break;

    case MODE_STROBE: // 标准频闪
        ls_strobe();
        break;

    case MODE_SINGLE_C_BREATH:
        single_c_breath(); // 单个颜色，单色呼吸
        break;

    case MODE_MUTIL_C_BREATH:

        multi_c_breath_cycle(); // 多个颜色，循环呼吸
        break;

    case MODE_GRADUAL:

        SM_rainbow_eff();

        break;

    default:
        break;
    }
}

/**
 * @brief 涂鸦的配对效果
 *
 */
static void ls_ty_pair_effect(void)
{
}

const u8 size_type[4] =
    {
        SIZE_SMALL, SIZE_MEDIUM, SIZE_LARGE, SIZE_XLARGE};

/**
 * @brief 森木光纤灯的流星集合
 *
 */
void ls_SM_star_effect(void)
{

    extern uint16_t SM_mode_comet_1(void);
    extern uint16_t SM_mode_comet_2(void);
    extern uint16_t WS2812FX_mode_comet_1(void);

    fc_effect.period_cnt = 0;

    if (fc_effect.star_index == 1 ||
        fc_effect.star_index == 2 ||
        fc_effect.star_index == 3 ||
        fc_effect.star_index == 4) // 流星效果
    {
        WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(
            0,                                        // 第0段
            0, fc_effect.led_num - 1,                 // 起始位置，结束位置
            &SM_mode_comet_1,                         // 效果
            0,                                        // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,                     // 速度
            size_type[fc_effect.star_index - 1] | 0); // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }
    else if (fc_effect.star_index == 5 ||
             fc_effect.star_index == 6 ||
             fc_effect.star_index == 7 ||
             fc_effect.star_index == 8) // 流星效果
    {
        WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(
            0,                                              // 第0段
            0, fc_effect.led_num - 1,                       // 起始位置，结束位置
            &SM_mode_comet_1,                               // 效果
            0,                                              // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,                           // 速度
            size_type[fc_effect.star_index - 5] | REVERSE); // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }
    else if (fc_effect.star_index == 9)
    {
        WS2812FX_setSegment_colorOptions(
            0,                        // 第0段
            0, fc_effect.led_num - 1, // 起始位置，结束位置
            &SM_mode_comet_2,         // 效果
            0,                        // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,     // 速度
            size_type[0] | 0);        // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }
    else if (fc_effect.star_index == 10)
    {
        WS2812FX_setSegment_colorOptions(
            0,                        // 第0段
            0, fc_effect.led_num - 1, // 起始位置，结束位置
            &SM_mode_comet_2,         // 效果
            0,                        // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,     // 速度
            size_type[0] | REVERSE);  // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }

    else if (fc_effect.star_index == 11)
    {
        extern uint16_t music_mode1(void);
        extern uint16_t meteor(void);
        extern uint16_t meteor1(void);

        WS2812FX_setBrightness(255);
        WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(
            0,                        // 第0段
            0, fc_effect.led_num - 1, // 起始位置，结束位置
            &meteor1,                 // 效果
            0,                        // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,     // 速度
            0);                       // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }
    else if (fc_effect.star_index == 12)
    {
        extern uint16_t music_meteor3(void);
        WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(
            0,                        // 第0段
            0, fc_effect.led_num - 1, // 起始位置，结束位置
            &music_meteor3,           // 效果
            0,                        // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,     // 速度
            0);                       // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }
    else if (fc_effect.star_index == 13)
    {

        extern uint16_t music_1(void);
        WS2812FX_stop();

        WS2812FX_setSegment_colorOptions(
            0,                        // 第0段
            0, fc_effect.led_num - 1, // 起始位置，结束位置
            &music_1,                 // 效果
            0,                        // 颜色，WS2812FX_setColors设置
            fc_effect.star_speed,     // 速度
            0);                       // 选项，这里像素点大小：3 REVERSE决定方向

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
    }
}

/**
 * @brief 天奕光纤灯的流星集合
 *
 */
void ls_meteor_stat_effect(void)
{

    fc_effect.period_cnt = 0;
    if (fc_effect.star_on_off == DEVICE_ON)
    {
        // 流星效果                                    单流星
        if (fc_effect.star_index == 1)
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_1,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                fade_type[0] | 0);        // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 2) // 单流星
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_1,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                fade_type[0] | REVERSE);  // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 3) // 双流星
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &fc_double_meteor,        // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                fade_type[0] | 0);        // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 4) // 双流星
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &fc_double_meteor,        // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                fade_type[0] | REVERSE);  // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 5) // 频闪效果
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_3,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 6) // 频闪效果
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_3,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                REVERSE);                 // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 7)
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &meteor_effect_G,         // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 8)
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                    // 第0段
                1, fc_effect.led_num, // 起始位置，结束位置
                &meteor_effect_H,     // 效果
                WHITE,                // 颜色
                fc_effect.star_speed, // 速度
                0);                   // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 9) // 堆积
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_4,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 10) // 堆积
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_4,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                REVERSE);                 // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 11) // 逐点流水
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_5,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 12) // 逐点流水
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_5,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                REVERSE);                 // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 13) // 中心靠拢
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_2,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                fade_type[0] | 0);        // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 14) // 中心发撒
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_2,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                fade_type[0] | REVERSE);  // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 15) // 追逐流水
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_6,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 16) // 追逐流水
        {

            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &WS2812FX_mode_comet_6,   // 效果
                WHITE,                    // 颜色
                fc_effect.star_speed,     // 速度
                REVERSE);                 // 选项，这里像素点大小：3 REVERSE决定方向
            WS2812FX_start();
        }
        else if (fc_effect.star_index == 17) // 音乐律动1
        {
            extern uint16_t meteor(void);
            WS2812FX_stop();

            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &meteor,                  // 效果
                WHITE,                    // 颜色，WS2812FX_setColors设置
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向

            WS2812FX_start();
        }
        else if (fc_effect.star_index == 18) // 音乐律动2
        {
            extern uint16_t music_meteor3(void);
            WS2812FX_stop();

            WS2812FX_setSegment_colorOptions(
                1,                        // 第0段
                1, fc_effect.led_num - 1, // 起始位置，结束位置
                &music_meteor3,           // 效果
                WHITE,                    // 颜色，WS2812FX_setColors设置
                fc_effect.star_speed,     // 速度
                0);                       // 选项，这里像素点大小：3 REVERSE决定方向

            WS2812FX_start();
        }

        else if (fc_effect.star_index == 19 || fc_effect.star_index == 20 || fc_effect.star_index == 21) //
        {
            strand_meteor(fc_effect.star_index);
        }
        else if (fc_effect.star_index == 22)
        {
            double_meteor();
        }
    }
    // save_user_data_area3(); // 保存参数配置到flash
    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

/**
 * @brief 自定义效果集合
 *
 */
static void ls_custom_effect(void)
{

    ls_SM_star_effect();
}

/**
 * @brief 音乐效果集合
 *
 */
void ls_music_effect(void)
{
    extern uint16_t fc_music_gradual(void);
    extern uint16_t fc_music_breath(void);
    extern uint16_t fc_music_static(void);
    extern uint16_t fc_music_twinkle(void);

    void *music_effect_addr = &fc_music_gradual; // 避免出现地址空，导致不断复位
    // app_set_bright(100);                         // 设置为最大亮度
    // #if (LED_STRIP_TYPE == TYPE_Fiber_optic_lights)
    switch (fc_effect.music.m)
    {
    case 0:
        music_effect_addr = &fc_music_gradual;
        break;
    case 1:
        music_effect_addr = &fc_music_breath;
        break;
    case 2:
        music_effect_addr = &fc_music_static;
        break;
    case 3:
        music_effect_addr = &fc_music_twinkle;
        break;
    default:
        break;
    }

    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0,                               // 起始位置
        // LIGHTING_ANIMATION_LED_NUMS - 1, // 结束位置
        fc_effect.led_num - 1, // 结束位置
        music_effect_addr,               // 效果
        WHITE,                           // 颜色，WS2812FX_setColors设置（在声控模式下，这里设置的颜色无效）
        100,                             // 速度
        SIZE_MEDIUM | FADE_XSLOW         // 选项，这里像素点大小：3,反向/反向
    );

    // #elif (LED_STRIP_TYPE == TYPE_Magic_lights)

    // #endif

    WS2812FX_start();
}

/**
 * @brief 涂抹效果集合
 *
 */
static void ls_smear_adjust_effect(void)
{
}

/**
 * @brief 静态效果集合
 *
 */
static void ls_static_effect(void)
{
    static_mode();
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

/**
 * @brief 灯光模式总调度
 *
 */
void set_fc_effect(void)
{
    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        return;
    }

    switch (fc_effect.Now_state)
    {
    // 幻彩场景
    case IS_light_scene:
        ls_scene_effect();
        break;
    // ==============================================
    // 配对模式
    case ACT_TY_PAIR:
        // ls_ty_pair_effect();
        break;
        // ==============================================
        // 自定义效果模式
    case ACT_CUSTOM:
        ls_custom_effect();
        break;
        // ==============================================
        // 音乐模式
    case IS_light_music:
        ls_music_effect();
        break;
        // ==============================================
        // 涂抹模式
    case IS_smear_adjust:
        // ls_smear_adjust_effect();
        break;
        // ==============================================
        // 静态模式
    case IS_STATIC:
        extern void static_mode(void);
        static_mode();
        break;
        // ==============================================
    case METEORITE_LAMP_MODE: // 流星灯模式
        lighting_animation_mode_change();
        break;
    default:
        break;
    }
}

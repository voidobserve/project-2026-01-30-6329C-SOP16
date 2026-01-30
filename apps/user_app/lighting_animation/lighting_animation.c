#include "lighting_animation.h"
#include "Adafruit_NeoPixel.H" // 包含RGB排列顺序
#include "led_strand_effect.h"
#include "../../apps/user_app/ws2812-fx-lib/WS2812FX_C/WS2812FX.h"
#include "../../../apps/user_app/led_strip/led_strand_effect.h" // fc_effect 变量定义
#include "../../../apps/user_app/save_flash/save_flash.h"       // 包含 save_info_t 结构体类型定义，save_info 结构体变量定义

extern Segment *_seg;            // currently active segment (20 bytes)
extern Segment_runtime *_seg_rt; // currently active segment runtime (16 bytes)
extern uint16_t _seg_len;        // num LEDs in the currently active segment

const u32 color_buff[] = {RED, GREEN, BLUE, YELLOW, PINK, CYAN, WHITE};
// size 14 :
const u32 color_buff_mode13[] = {RED, RED, GREEN, GREEN, BLUE, BLUE, YELLOW, YELLOW, PINK, PINK, CYAN, CYAN, WHITE, WHITE};

/**
 * @brief
 *
 * @param offset 灯光位置
 * @param targetColor 颜色
 * @param rate 亮度，范围：0~7，值越小，流星灯尾焰的层次越明显（实际测试好像没有效果）
 */
void WS2812FX_fade_out_targetColor_in_offset(uint8_t offset, uint32_t targetColor, uint8_t rate)
{
    static const uint8_t rateMapH[] = {0, 1, 1, 1, 2, 3, 4, 6};
    static const uint8_t rateMapL[] = {0, 2, 3, 8, 8, 8, 8, 8};

    // uint8_t rate = 7; // 值越小，层次越明显
    uint8_t rateH = rateMapH[rate];
    uint8_t rateL = rateMapL[rate];

    uint32_t color = targetColor;
    int w2 = (color >> 24) & 0xff;
    int r2 = (color >> 16) & 0xff;
    int g2 = (color >> 8) & 0xff;
    int b2 = color & 0xff;

    // color = Adafruit_NeoPixel_getPixelColor(offset); // current color
    color = targetColor;
    if (rate == 0)
    { // old fade-to-black algorithm
        WS2812FX_setPixelColor(offset, (color >> 1) & 0x7F7F7F7F);
    }
    else
    { // new fade-to-color algorithm
        int w1 = (color >> 24) & 0xff;
        int r1 = (color >> 16) & 0xff;
        int g1 = (color >> 8) & 0xff;
        int b1 = color & 0xff;

        // calculate the color differences between the current and target colors
        int wdelta = w2 - w1;
        int rdelta = r2 - r1;
        int gdelta = g2 - g1;
        int bdelta = b2 - b1;

        // if the current and target colors are almost the same, jump right to the target
        // color, otherwise calculate an intermediate color. (fixes rounding issues)
        wdelta = abs(wdelta) < 3 ? wdelta : (wdelta >> rateH) + (wdelta >> rateL);
        rdelta = abs(rdelta) < 3 ? rdelta : (rdelta >> rateH) + (rdelta >> rateL);
        gdelta = abs(gdelta) < 3 ? gdelta : (gdelta >> rateH) + (gdelta >> rateL);
        bdelta = abs(bdelta) < 3 ? bdelta : (bdelta >> rateH) + (bdelta >> rateL);

        WS2812FX_setPixelColor_rgbw(offset, r1 + rdelta, g1 + gdelta, b1 + bdelta, w1 + wdelta);
    }
}

/*
    单色流星灯动画，对应样机的模式1~7
    颜色根据colors[0]来决定

    注意：
    速度值 == 单个动画时间
    如果速度值与函数内预设的不一致，动画会卡住
    函数内部增加了动画之间的时间间隔，与速度值有关
*/
u16 WS2812FX_sample_single_color_meteor_light(void)
{
    u16 animation_len = _seg_len * 2; // 动画长度，等于_seg_len + 流星灯的长度

    WS2812FX_fade_out();

    if (IS_REVERSE)
    {
        if ((_seg->stop - _seg->start) >= _seg_rt->counter_mode_step)
        {
            WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[0]);
        }
    }
    else
    {
        if (_seg_rt->counter_mode_step < _seg->stop + 1)
        {
            WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[0]);
        }
    }

    /*
        执行一次 0->_seg_len的动画所需时间由 _seg->speed 决定
        interval 控制每轮动画的时间，
        每轮动画的时间 == 执行一次 0->_seg_len的动画所需时间 + 动画之间的时间间隔
    */
    u16 interval = 0;
    // if (_seg->speed == 1000) // 速度值对应 1s，从0到_seg_len的动画时间为1s
    // {
    //     interval = animation_len * (4 + 1); //
    // }
    // else if (_seg->speed == 2000) // 速度值对应 2s，从0到_seg_len的动画时间为2s
    // {
    //     interval = animation_len * ((8 + 2) / 2);
    // }
    // else if (_seg->speed == 3000)
    // {
    //     // interval = _seg_len * ((10 + 3) *10 / 3) / 10; // 这一句会丢失部分时间精度
    //     interval = animation_len * (((double)10 + 3) / 3);
    // }
    // else if (_seg->speed == 4000)
    // {
    //     interval = animation_len * (((double)15 + 4) / 4);
    // }
    // else // 如果是 1000 ~ 4000之间的速度值，不包括 1000 和 4000
    {
#if 1
        interval = animation_len *
                   (((double)_seg->speed + save_info.cur_lighting_animation_time_interval) / _seg->speed);
#endif
    }

    // interval = animation_len; // 测试去掉动画时间间隔，动画本身所需的时间
    // interval = _seg_len; // 测试去掉动画时间间隔，动画本身所需的时间

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (interval);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        // printf("interval %u\n", interval);
    }

    // return (_seg->speed / (animation_len));
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式8
    依次进行流星动画，每个颜色的动画都有时间间隔
    红 绿 蓝

    使用 color_buff 中的颜色

    注意：
    速度值 == 单个动画时间，不包括动画时间间隔
    如果速度值与函数内预设的不一致，动画会卡住
*/
u16 WS2812FX_sample_8(void)
{
    u16 animation_len = _seg_len * 2; // 动画长度，等于_seg_len + 流星灯的长度

    WS2812FX_fade_out();

    // printf("_seg_rt->aux_param: %u\n", (u16)_seg_rt->aux_param);

    if (IS_REVERSE)
    {
        if ((_seg->stop - _seg->start) >= _seg_rt->counter_mode_step)
            WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
    }
    else
    {
        if (_seg_rt->counter_mode_step < _seg->stop + 1)
            WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
    }

    /*
        执行一次 0->_seg_len的动画所需时间由 _seg->speed 决定
        interval 控制每轮动画的时间，
        每轮动画的时间 == 执行一次 0->_seg_len的动画所需时间 + 动画之间的时间间隔
    */
    u16 interval = 0;
#if 0
    if (_seg->speed == 1000) // 速度值对应 1s，从0到_seg_len的动画时间为1s
    {
        interval = animation_len * (4 + 1); //
    }
    else if (_seg->speed == 2000) // 速度值对应 2s，从0到_seg_len的动画时间为2s
    {
        interval = animation_len * ((8 + 2) / 2);
    }
    else if (_seg->speed == 3000)
    {
        // interval = _seg_len * ((10 + 3) *10 / 3) / 10; // 这一句会丢失部分时间精度
        interval = animation_len * (((double)10 + 3) / 3);
    }
    else if (_seg->speed == 4000)
    {
        interval = animation_len * (((double)15 + 4) / 4);
    }
#endif

#if 1
    interval = animation_len *
               (((double)_seg->speed + save_info.cur_lighting_animation_time_interval) / _seg->speed);
#endif

    // interval = animation_len; // 测试去掉动画时间间隔，动画本身所需的时间
    // interval = _seg_len; // 测试去掉动画时间间隔，动画本身所需的时间

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (interval);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        // 每轮动画结束，切换颜色
        // 颜色索引对应 color_buff[]
        if (_seg_rt->aux_param == 0)
        {
            _seg_rt->aux_param = 1;
        }
        else if (_seg_rt->aux_param == 1)
        {
            _seg_rt->aux_param = 2;
        }
        else if (_seg_rt->aux_param == 2)
        {
            _seg_rt->aux_param = 0;
        }
    }

    // return (_seg->speed / (animation_len));
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式9
    依次进行流星动画，每个颜色的动画都有时间间隔
    红 绿 蓝 白 黄 粉 cyan

    注意：
    速度值 == 单个动画时间
    如果速度值与函数内预设的不一致，动画会卡住
*/
u16 WS2812FX_sample_9(void)
{
    u16 animation_len = _seg_len * 2;
    WS2812FX_fade_out();

    if (IS_REVERSE)
    {
        if ((_seg->stop - _seg->start) >= _seg_rt->counter_mode_step)
            WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
    }
    else
    {
        if (_seg_rt->counter_mode_step < _seg->stop + 1)
            WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);
    }

    /*
        执行一次 0->_seg_len的动画所需时间由 _seg->speed 决定
        interval 控制每轮动画的时间，
        每轮动画的时间 == 执行一次 0->_seg_len的动画所需时间 + 动画之间的时间间隔
    */
    u16 interval = 0;
#if 0
    if (_seg->speed == 1000) // 速度值对应 1s，从0到_seg_len的动画时间为1s
    {
        interval = animation_len * (4 + 1); //
    }
    else if (_seg->speed == 2000) // 速度值对应 2s，从0到_seg_len的动画时间为2s
    {
        interval = animation_len * ((8 + 2) / 2);
    }
    else if (_seg->speed == 3000)
    {
        // interval = _seg_len * ((10 + 3) *10 / 3) / 10; // 这一句会丢失部分时间精度
        interval = animation_len * (((double)10 + 3) / 3);
    }
    else if (_seg->speed == 4000)
    {
        interval = animation_len * (((double)15 + 4) / 4);
    }
#endif

#if 1
    interval = animation_len *
               (((double)_seg->speed + save_info.cur_lighting_animation_time_interval) / _seg->speed);
#endif

    // interval = animation_len; // 测试去掉动画时间间隔，动画本身所需时间

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (interval);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        // 每轮动画结束，切换颜色
        // 颜色索引对应 color_buff[]
        if (_seg_rt->aux_param == 0)
        {
            _seg_rt->aux_param = 1; // GREEN 绿
        }
        else if (_seg_rt->aux_param == 1)
        {
            _seg_rt->aux_param = 2; // BLUE 蓝
        }
        else if (_seg_rt->aux_param == 2)
        {
            _seg_rt->aux_param = 6; // WHITE 白
        }
        else if (_seg_rt->aux_param == 6)
        {
            _seg_rt->aux_param = 3; // YELLOW 黄
        }
        else if (_seg_rt->aux_param == 3)
        {
            _seg_rt->aux_param = 4; // PINK 粉
        }
        else if (_seg_rt->aux_param == 4)
        {
            _seg_rt->aux_param = 5; // CYAN
        }
        else if (_seg_rt->aux_param == 5)
        {
            _seg_rt->aux_param = 0; // RED 红
        }
    }

    // return (_seg->speed / (animation_len));
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式12
    红 绿 蓝 黄 粉 cyan 白 依次进行流水动画，亮度均匀，几乎没有动画时间间隔
    例如:
    红色流水一次，灯光全部熄灭，再到绿色流水一次，灯光全部熄灭...

    注意，传入的速度值，在有关灯珠数量的系数上，应该在灯光数量的基础上加一，因为每一轮动画结束后有个灯光全部熄灭的动画
    例如：速度值 == 单个动画时间
*/
u16 WS2812FX_sample_12(void)
{
    if (_seg_rt->counter_mode_step < _seg_len)
    {
        uint32_t led_offset = _seg_rt->counter_mode_step;
        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
        }
    }
    else
    {
        // 一轮动画结束后，让所有灯光显示黑色（熄灭）
        for (u16 i = 0; i < _seg_len; i++)
        {
            WS2812FX_setPixelColor(i, BLACK);
        }
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len + 1); // 加一，是因为动画最后是所有灯光熄灭

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        _seg_rt->aux_param++;
        if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
        {
            _seg_rt->aux_param = 0;
        }
    }

    // return (_seg->speed / (_seg_len + 1)); // 加一，是因为动画最后是所有灯光熄灭
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式14
    11个灯为一组，进行流水动画，顺序：红 绿 蓝 黄 粉 cyan 白（一开始是红色全亮），没有动画时间间隔
    注意：
    1. 一开始是红灯全亮，之后点亮第一个灯，从第二个灯开始色流水，依次进行流水动画
    2. 由于是11个灯为一组，总数是12个灯，传入的速度值，在有关灯珠数量的系数上，应该在灯光数量的基础上减一
        例如： 速度值 == 单个动画时间


    动画拆分：
    1. 首次进入，所有灯显示红色
    2. 第一个灯显示绿色
    3. 从第二个灯开始流水
    4. 流水到最后一个灯的同时，第一个灯要换成下一种颜色
    5. 第一个灯换到下一种颜色显示后，继续流水，从第二个灯开始流水
    除了首次进入，依次执行步骤 345
*/
u16 WS2812FX_sample_14(void)
{
    if (_seg_rt->counter_mode_call == 0)
    {
        // 动画刚开始时，所有灯光填充第1个颜色(灯光全部显示第一个颜色)，从第2个颜色开始流水
        for (u16 i = 0; i < _seg_len; i++)
        {
            WS2812FX_setPixelColor(i, _seg->colors[_seg_rt->aux_param]);
        }

        // _seg_rt->counter_mode_step = 0; // 刚进入该模式，从第一个灯开始上色（刚开始就是0，可以不写）

        _seg_rt->aux_param++; // 切换到下一种颜色

        // return (_seg->speed / (_seg_len - 1));
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }

    if (0 == _seg_rt->counter_mode_step)
    {
        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop, _seg->colors[_seg_rt->aux_param]);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start, _seg->colors[_seg_rt->aux_param]);
        }
        _seg_rt->counter_mode_step = 1;

        if (1 == _seg_rt->counter_mode_call) // 刚进入该模式，从第一个灯开始上色
            // return (_seg->speed / (_seg_len - 1));
            return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }

    if (_seg_rt->counter_mode_step < _seg_len)
    {
        uint32_t led_offset = _seg_rt->counter_mode_step;

        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
        }

        if (_seg_rt->counter_mode_step >= _seg_len - 1)
        {
            u8 index = 0;
            index = _seg_rt->aux_param + 1;
            if (index >= 7) // 移植时需要后改为对应数组长度
            {
                index = 0;
            }

            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop, _seg->colors[index]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start, _seg->colors[index]);
            }
        }
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        _seg_rt->aux_param++;
        if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
        {
            _seg_rt->aux_param = 0;
        }
    }

    // return (_seg->speed / (_seg_len - 1));
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式16
    单个灯为一组，进行流水动画，背景为黑色
    顺序：红 绿 蓝 黄 粉 cyan 白
    每轮动画结束后，灯光全灭，灭灯的时间很短（动画时间间隔很短）

    注意：
    速度值 == 单个动画时间
*/
u16 WS2812FX_sample_16(void)
{
    if (_seg_rt->counter_mode_step < _seg_len)
    {
        uint32_t led_offset = _seg_rt->counter_mode_step;
        u32 black_offset = 0;
        if (led_offset == 0)
        {
            black_offset = _seg->stop;
        }
        else
        {
            black_offset = led_offset - 1;
        }

        // Adafruit_NeoPixel_clear(); // 节省时间，不使用该语句

        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - black_offset, BLACK);
            WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + black_offset, BLACK);
            WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
        }
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        _seg_rt->aux_param++;
        if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
        {
            _seg_rt->aux_param = 0;
        }
    }

    // return (_seg->speed / _seg_len);
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式18
    mode12 + mode16的动画组合
    先跑完mode12，再跑mode16

    注意：
    传入的速度值 == 单个动画运行时间
*/
u16 WS2812FX_sample_18(void)
{
    if (0 == _seg_rt->counter_mode_step)
    {
        // 清除上一轮的颜色残留
        Adafruit_NeoPixel_clear();
    }

    if (0 == _seg_rt->aux_param3)
    {
        if (_seg_rt->counter_mode_step < _seg_len)
        {
            uint32_t led_offset = _seg_rt->counter_mode_step;
            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
            }
        }
        else
        {
            // 一轮动画结束后，让所有灯光显示黑色（熄灭）
            for (u16 i = 0; i < _seg_len; i++)
            {
                WS2812FX_setPixelColor(i, BLACK);
            }
        }

        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len + 1);
    }
    else
    {
        if (_seg_rt->counter_mode_step < _seg_len)
        {
            uint32_t led_offset = _seg_rt->counter_mode_step;
            u32 black_offset = 0;
            if (led_offset == 0)
            {
                black_offset = _seg->stop;
            }
            else
            {
                black_offset = led_offset - 1;
            }

            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop - black_offset, BLACK);
                WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start + black_offset, BLACK);
                WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
            }
        }

        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);
    }

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        _seg_rt->aux_param++;
        if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
        {
            _seg_rt->aux_param = 0;

            if (_seg_rt->aux_param3 == 0)
            {
                _seg_rt->aux_param3 = 1;
            }
            else
            {
                _seg_rt->aux_param3 = 0;
            }
        }
    }

    if (_seg_rt->aux_param3 == 0)
    {
        // return (_seg->speed / (_seg_len + 1));
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }
    else
    {
        // return (_seg->speed / _seg_len);
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }
}

/*
    动画效果，对应样机模式19
    mode14 + mode16的动画组合
    先跑完mode14，再跑mode16

    注意：
    传入的速度值 == 模式运行时间

    _seg_rt->aux_param3 bit0 用于控制当前循环是要跑mode14还是mode16
                        ==0，当前要跑mode14
                        ==1，当前要跑mode16
    _seg_rt->aux_param3 bit1
                        ==0，表示动画还在mode14；
                        ==1，表示动画在mode16结束，需要切换为mode14

*/
u16 WS2812FX_sample_19(void)
{
    if (1 == ((_seg_rt->aux_param3 >> 1) & 0x01))
    {
        _seg_rt->aux_param = 0;
        _seg_rt->aux_param2 = 0;
        _seg_rt->aux_param3 = 0;
        _seg_rt->counter_mode_call = 0;
        _seg_rt->counter_mode_step = 0;
    }

    if (0 == (_seg_rt->aux_param3 & 0x01))
    {
        if (_seg_rt->counter_mode_call == 0)
        {
            // 动画刚开始时，所有灯光填充第1个颜色(灯光全部显示第一个颜色)，从第2个颜色开始流水
            for (u16 i = 0; i < _seg_len; i++)
            {
                WS2812FX_setPixelColor(i, _seg->colors[_seg_rt->aux_param]);
            }

            // _seg_rt->counter_mode_step = 0; // 刚进入该模式，从第一个灯开始上色（刚开始就是0，可以不写）

            _seg_rt->aux_param++; // 切换到下一种颜色

            // return (_seg->speed / (_seg_len - 1)); // 提前返回
            return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
        }

        if (0 == _seg_rt->counter_mode_step)
        {
            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start, _seg->colors[_seg_rt->aux_param]);
            }
            _seg_rt->counter_mode_step = 1;

            if (1 == _seg_rt->counter_mode_call)       // 刚进入该模式，从第一个灯开始上色
                // return (_seg->speed / (_seg_len - 1)); // 提前返回
                return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
        }

        if (_seg_rt->counter_mode_step < _seg_len)
        {
            uint32_t led_offset = _seg_rt->counter_mode_step;

            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
            }

            if (_seg_rt->counter_mode_step >= _seg_len - 1)
            {
                u8 index = 0;
                index = _seg_rt->aux_param + 1;
                if (index >= 7) // 移植时需要后改为对应数组长度
                {
                    index = 0;
                }

                if (IS_REVERSE)
                {
                    WS2812FX_setPixelColor(_seg->stop, _seg->colors[index]);
                }
                else
                {
                    WS2812FX_setPixelColor(_seg->start, _seg->colors[index]);
                }
            }
        }

        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

        if (_seg_rt->counter_mode_step == 0)
        {
            SET_CYCLE;

            if (0 == _seg_rt->aux_param)
            {
                // 如果红色流水灯刚跑完
                _seg_rt->aux_param3 |= 0x01 << 0;
                // 清除上一轮的颜色残留
                Adafruit_NeoPixel_clear();
                // return (_seg->speed / (_seg_len - 1));
                return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
            }

            _seg_rt->aux_param++;
            if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
            {
                _seg_rt->aux_param = 0;
            }
        }

        // return (_seg->speed / (_seg_len - 1));
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }
    else
    {
        if (_seg_rt->counter_mode_step < _seg_len)
        {
            uint32_t led_offset = _seg_rt->counter_mode_step;
            u32 black_offset = 0;
            if (led_offset == 0)
            {
                black_offset = _seg->stop;
            }
            else
            {
                black_offset = led_offset - 1;
            }

            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop - black_offset, BLACK);
                WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start + black_offset, BLACK);
                WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
            }
        }

        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

        if (_seg_rt->counter_mode_step == 0)
        {
            SET_CYCLE;

            _seg_rt->aux_param++;
            if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
            {
                _seg_rt->aux_param = 0;
                _seg_rt->aux_param3 &= ~(0x01 << 0);
                _seg_rt->aux_param3 |= (0x01 << 1);

                // 清除上一轮的颜色残留
                Adafruit_NeoPixel_clear();
            }
        }

        // return (_seg->speed / _seg_len);
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }
}

/*
    动画效果，对应样机模式20
    mode12 + mode14 的动画组合
    先跑完mode12，再跑mode14

    注意：
    传入的速度值 == 模式运行时间

    _seg_rt->aux_param3 bit0 用于控制当前循环是要跑 mode12 还是 mode14
                        0--当前要跑mode12
                        1--当前要跑mode14
    _seg_rt->aux_param3 bit1
                        0--当前动画还在mode12，无需对该标志位做处理
                        1--当前动画刚从mode12跑完，需要清除一下该标志位，以及相关变量，让mode14从头开始跑
*/
u16 WS2812FX_sample_20(void)
{
    if (1 == ((_seg_rt->aux_param3 >> 1) & 0x01))
    {
        _seg_rt->counter_mode_call = 0;
        _seg_rt->counter_mode_step = 0;
        _seg_rt->aux_param = 0;
        _seg_rt->aux_param3 &= ~(0x01 << 1);
    }

    if (0 == (_seg_rt->aux_param3 & 0x01))
    {
        if (_seg_rt->counter_mode_step < _seg_len)
        {
            uint32_t led_offset = _seg_rt->counter_mode_step;
            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
            }
        }
        else
        {
            // 一轮动画结束后，让所有灯光显示黑色（熄灭）
            for (u16 i = 0; i < _seg_len; i++)
            {
                WS2812FX_setPixelColor(i, BLACK);
            }
        }

        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len + 1);

        if (_seg_rt->counter_mode_step == 0)
        {
            SET_CYCLE;

            _seg_rt->aux_param++;
            if (_seg_rt->aux_param >= 7) // 移植时需要后改为对应数组长度
            {
                _seg_rt->aux_param = 0;
                _seg_rt->aux_param3 |= (0x01 << 0);
                _seg_rt->aux_param3 |= (0x01 << 1);
            }
        }

        // return (_seg->speed / (_seg_len + 1));
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }
    else
    {
        if (_seg_rt->counter_mode_call == 0)
        {
            // 动画刚开始时，所有灯光填充第1个颜色(灯光全部显示第一个颜色)，从第2个颜色开始流水
            for (u16 i = 0; i < _seg_len; i++)
            {
                WS2812FX_setPixelColor(i, _seg->colors[_seg_rt->aux_param]);
            }

            // _seg_rt->counter_mode_step = 0; // 刚进入该模式，从第一个灯开始上色（刚开始就是0，可以不写）

            _seg_rt->aux_param++; // 切换到下一种颜色

            // return (_seg->speed / (_seg_len - 1)); // 提前返回
            return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
        }

        if (0 == _seg_rt->counter_mode_step && 7 != _seg_rt->aux_param)
        {
            if (IS_REVERSE)
            {
                WS2812FX_setPixelColor(_seg->stop, _seg->colors[_seg_rt->aux_param]);
            }
            else
            {
                WS2812FX_setPixelColor(_seg->start, _seg->colors[_seg_rt->aux_param]);
            }
            _seg_rt->counter_mode_step = 1;

            if (1 == _seg_rt->counter_mode_call)       // 刚进入该模式，从第一个灯开始上色
                // return (_seg->speed / (_seg_len - 1)); // 提前返回
                return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
        }

        if (_seg_rt->counter_mode_step < _seg_len)
        {
            uint32_t led_offset = _seg_rt->counter_mode_step;

            /*
                如果不是当前动画对应的颜色数组color_buff的第0个下标对应的颜色进行流水，
                每次流水从第2个灯开始，流水结束的同时，将第1个灯的颜色设置为color_buff下一个下标对应的颜色
            */
            if (7 != _seg_rt->aux_param)
            {
                if (IS_REVERSE)
                {
                    WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
                }
                else
                {
                    WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
                }

                if (_seg_rt->counter_mode_step >= _seg_len - 1)
                {
                    u8 index = 0;
                    index = _seg_rt->aux_param + 1;
                    if (index >= 7) // 移植时需要后改为对应数组长度
                    {
                        index = 0;
                    }

                    if (IS_REVERSE)
                    {
                        WS2812FX_setPixelColor(_seg->stop, _seg->colors[index]);
                    }
                    else
                    {
                        WS2812FX_setPixelColor(_seg->start, _seg->colors[index]);
                    }
                }
            }
            else
            {
                /*
                    如果是当前动画对应的颜色数组color_buff的第0个下标对应的颜色进行流水，
                    正常进行流水动画，不会点亮最后一个灯。
                    例如，有第1到第12个灯依次流水，流水不会点亮第12个灯，点亮完第11个灯，
                        当前流水便结束
                */
                if (IS_REVERSE)
                {
                    WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[0]);
                }
                else
                {
                    WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[0]);
                }
            }
        }

        _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

        if (_seg_rt->counter_mode_step == 0)
        {
            SET_CYCLE;

            _seg_rt->aux_param++;
            if (_seg_rt->aux_param >= (7 + 1)) // 移植时需要后改为对应数组长度 + 1，最后一个是红色流水，流水完毕，直接切换到下一个模式
            {
                _seg_rt->aux_param = 0;
                _seg_rt->aux_param3 &= ~(0x01 << 0);

                // 清除上一轮的颜色残留
                Adafruit_NeoPixel_clear();
            }
        }

        // return (_seg->speed / (_seg_len - 1));
        return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
    }
}

/*
    动画效果，对应样机的模式10
    红+绿+蓝+红+绿+蓝，总共六个灯为一组，组成流星灯动画，
    动画时间和动画时间间隔与模式1~7的一致，颜色的亮度没有明显的衰减

    注意：
    速度值 == 单个动画时间
    如果速度值与函数内预设的不一致，动画会卡住
*/
u16 WS2812FX_sample_10(void)
{
    u16 animation_len = _seg_len + 5;

    // 清除上一轮的颜色残留
    Adafruit_NeoPixel_clear();

    if (_seg_rt->counter_mode_step < animation_len)
    {
        uint32_t led_offset = _seg_rt->counter_mode_step;

        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - led_offset, RED);

            if (_seg_rt->counter_mode_step >= 1)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - led_offset + 1, GREEN, 7);
            }

            if (_seg_rt->counter_mode_step >= 2)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - led_offset + 2, BLUE, 7);
            }

            if (_seg_rt->counter_mode_step >= 3)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - led_offset + 3, RED, 7);
            }

            if (_seg_rt->counter_mode_step >= 4)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - led_offset + 4, GREEN, 7);
            }

            if (_seg_rt->counter_mode_step >= 5)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - led_offset + 5, BLUE, 7);
            }
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + led_offset, RED);

            if (_seg_rt->counter_mode_step >= 1)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + led_offset - 1, GREEN, 7);
            }

            if (_seg_rt->counter_mode_step >= 2)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + led_offset - 2, BLUE, 7);
            }

            if (_seg_rt->counter_mode_step >= 3)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + led_offset - 3, RED, 7);
            }

            if (_seg_rt->counter_mode_step >= 4)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + led_offset - 4, GREEN, 7);
            }

            if (_seg_rt->counter_mode_step >= 5)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + led_offset - 5, BLUE, 7);
            }
        }
    }

    /*
    执行一次 0->_seg_len的动画所需时间由 _seg->speed 决定
    interval 控制每轮动画的时间，
    每轮动画的时间 == 执行一次 0->_seg_len的动画所需时间 + 动画之间的时间间隔
*/
    u16 interval = 0;
#if 0
    if (_seg->speed == 1000) // 速度值对应 1s，从0到_seg_len的动画时间为1s
    {
        interval = (animation_len) * (4 + 1); //
    }
    else if (_seg->speed == 2000) // 速度值对应 2s，从0到_seg_len的动画时间为2s
    {
        interval = (animation_len) * ((8 + 2) / 2);
    }
    else if (_seg->speed == 3000)
    {
        interval = (animation_len) * (((double)10 + 3) / 3);
    }
    else if (_seg->speed == 4000)
    {
        interval = (animation_len) * (((double)15 + 4) / 4);
    }
#endif

#if 1
    interval = animation_len *
               (((double)_seg->speed + save_info.cur_lighting_animation_time_interval) / _seg->speed);
#endif

    // interval = (_seg_len + 5); // 测试去掉动画时间间隔，动画本身所需的时间
    // interval = (animation_len); // 测试去掉动画时间间隔，动画本身所需的时间

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (interval);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;
    }

    // return (_seg->speed / (_seg_len + 5)); // 相当于从第1个灯流水到 第 WS2812_LED_NUM_MAX + 5个灯
    // return (_seg->speed / (animation_len)); //
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机的模式11
    红+绿+蓝+黄+粉+cyan，六个灯为一组，组成第一组流星灯动画，
    cyan+粉+黄+蓝+绿+红，六个灯为一组，组成第二组流星灯动画，

    两组动画合起来，动画时间分为：1s、2s、3s、4s
    动画时间和动画时间间隔与模式1~7的一致，颜色的亮度没有明显的衰减

    注意：
    速度值 == 单个动画时间
    如果速度值与函数内预设的不一致，动画会卡住

*/
u16 WS2812FX_sample_11(void)
{
    /*
        动画长度
        1个灯 + 5个灯作为流星灯尾焰 + 一帧熄灯，动画从0开始，这里要加6
    */
    u16 animation_time = _seg_len + 6;

    // 清除上一轮的颜色残留
    Adafruit_NeoPixel_clear();

    u32 offset = _seg_rt->counter_mode_step;
    if (_seg_rt->counter_mode_step < animation_time) //
    {
        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, RED);

            if (_seg_rt->counter_mode_step >= 1)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - _seg_rt->counter_mode_step + 1, GREEN, 7);
            }

            if (_seg_rt->counter_mode_step >= 2)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - _seg_rt->counter_mode_step + 2, BLUE, 7);
            }

            if (_seg_rt->counter_mode_step >= 3)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - _seg_rt->counter_mode_step + 3, YELLOW, 7);
            }

            if (_seg_rt->counter_mode_step >= 4)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - _seg_rt->counter_mode_step + 4, PINK, 7);
            }

            if (_seg_rt->counter_mode_step >= 5)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - _seg_rt->counter_mode_step + 5, CYAN, 7);
            }
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, RED);

            if (_seg_rt->counter_mode_step >= 1)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + _seg_rt->counter_mode_step - 1, GREEN, 7);
            }

            if (_seg_rt->counter_mode_step >= 2)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + _seg_rt->counter_mode_step - 2, BLUE, 7);
            }

            if (_seg_rt->counter_mode_step >= 3)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + _seg_rt->counter_mode_step - 3, YELLOW, 7);
            }

            if (_seg_rt->counter_mode_step >= 4)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + _seg_rt->counter_mode_step - 4, PINK, 7);
            }

            if (_seg_rt->counter_mode_step >= 5)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + _seg_rt->counter_mode_step - 5, CYAN, 7);
            }
        }
    }
    else
    {
        // _seg_rt->counter_mode_step已经超出_seg_len，需要拿一个变量存放修正后的位置：
        offset = _seg_rt->counter_mode_step - animation_time;

        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - offset, CYAN);

            if (offset >= 1)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - offset + 1, PINK, 7);
            }

            if (offset >= 2)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - offset + 2, YELLOW, 7);
            }

            if (offset >= 3)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - offset + 3, BLUE, 7);
            }

            if (offset >= 4)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - offset + 4, GREEN, 7);
            }

            if (offset >= 5)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->stop - offset + 5, RED, 7);
            }
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + offset, CYAN);

            if (offset >= 1)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + offset - 1, PINK, 7);
            }

            if (offset >= 2)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + offset - 2, YELLOW, 7);
            }

            if (offset >= 3)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + offset - 3, BLUE, 7);
            }

            if (offset >= 4)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + offset - 4, GREEN, 7);
            }

            if (offset >= 5)
            {
                WS2812FX_fade_out_targetColor_in_offset(_seg->start + offset - 5, RED, 7);
            }
        }
    }

    /*
        执行一次 0->_seg_len的动画所需时间由 _seg->speed 决定
        interval 控制每轮动画的时间，
        每轮动画的时间 == 执行一次 0->_seg_len的动画所需时间 + 动画之间的时间间隔
    */
    u16 interval = 0;
#if 0
    if (_seg->speed == 1000) // 速度值对应 1s，从0到_seg_len的动画时间为1s
    {
        interval = (animation_time * 2) * (4 + 1); //
    }
    else if (_seg->speed == 2000) // 速度值对应 2s，从0到_seg_len的动画时间为2s
    {
        interval = (animation_time * 2) * ((8 + 2) / 2);
    }
    else if (_seg->speed == 3000)
    {
        interval = (animation_time * 2) * (((double)10 + 3) / 3);
    }
    else if (_seg->speed == 4000)
    {
        interval = (animation_time * 2) * (((double)15 + 4) / 4);
    }
#endif

#if 1
    interval = (animation_time * 2) *
               (((double)_seg->speed + save_info.cur_lighting_animation_time_interval) / _seg->speed);
#endif
    // interval = (animation_time * 2); // 测试去掉动画时间间隔，动画本身所需的时间

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (interval);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;
    }

    // return (_seg->speed / ((_seg_len + 6) * 2)); // 相当于从第1个灯流水到 第 WS2812_LED_NUM_MAX + 5个灯 + 1帧全部熄灭
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画模式13，对应样机的模式13

    所有灯都点亮，颜色顺序：红 绿 蓝 黄 粉 cyan 白，每个颜色用(以)两个灯为一组点亮，
    逆序流水，头尾闭合相连

    注意：
    传入的速度值 == 一轮动画的运行时间
    要使用 模式13 对应的颜色数组
    _seg->colors的空间 要大于等于 模式13 对应的颜色数组，否则需要修改对应的宏

*/
u16 WS2812FX_sample_13(void)
{
    if (_seg_rt->counter_mode_step < _seg_len)
    {
        if (IS_REVERSE)
        {
            for (u16 i = 0; i < _seg_len; i++) // 灯珠的位置
            {
                u16 color_index = _seg_rt->aux_param; // 存放颜色数组的下标
                for (u16 j = 0; j < i; j++)
                {
                    color_index++;
                    if (color_index >= 14)
                    {
                        color_index = 0;
                    }
                }

                WS2812FX_setPixelColor(_seg->start + i, _seg->colors[color_index]);
            }

            if (_seg_rt->aux_param > 0)
            {
                _seg_rt->aux_param--;
            }
            else
            {
                _seg_rt->aux_param = 13;
            }
        }
        else
        {
            for (u16 i = 0; i < _seg_len; i++) // 灯珠的位置
            {
                u16 color_index = _seg_rt->aux_param; // 存放颜色数组的下标
                for (u16 j = 0; j < i; j++)
                {
                    color_index++;
                    if (color_index >= 14)
                    {
                        color_index = 0;
                    }
                }

                WS2812FX_setPixelColor(_seg->start + i, _seg->colors[color_index]);
            }

            _seg_rt->aux_param++; // 切换颜色
            if (_seg_rt->aux_param >= 14)
            {
                _seg_rt->aux_param = 0;
            }
        }
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;
    }

    // return (_seg->speed / _seg_len);
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式15
    一开始是单个灯的红色流水，到了第8个灯开始，增加尾部，继续流水。
    一轮动画结束，所有灯光会熄灭，然后很快开始下一轮

    例如：
    从第8个灯开始，动画依次是：
    红
    绿 红
    蓝 绿 红
    黄 蓝 绿 红
    粉 黄 蓝 绿 红
    灯熄灭 粉 黄 蓝 绿
    灯熄灭 灯熄灭 粉 黄 蓝
    ...

    注意：
    传入的速度值 == 一轮动画的运行时间

*/
u16 WS2812FX_sample_15(void)
{
    // 单个灯流水
    if (_seg_rt->counter_mode_step < _seg_len - 5) // 后面5个灯是多种颜色流水，这里要减5
    {
        uint32_t led_offset = _seg_rt->counter_mode_step;
        u32 black_offset = 0;
        if (led_offset == 0)
        {
            black_offset = _seg->stop;
        }
        else
        {
            black_offset = led_offset - 1;
        }

        if (IS_REVERSE)
        {
            WS2812FX_setPixelColor(_seg->stop - black_offset, BLACK);
            WS2812FX_setPixelColor(_seg->stop - led_offset, _seg->colors[_seg_rt->aux_param]);
        }
        else
        {
            WS2812FX_setPixelColor(_seg->start + black_offset, BLACK);
            WS2812FX_setPixelColor(_seg->start + led_offset, _seg->colors[_seg_rt->aux_param]);
        }
    }
    else
    {
        // 多种颜色组合的流水灯动画
        Adafruit_NeoPixel_clear();

        if (IS_REVERSE)
        {
            if (_seg_rt->counter_mode_step <= _seg_len - 1) // 从 0 到 _seg_len - 1，对应 _seg_len 个灯珠
            {
                // 多种颜色组合的流水灯，前半段动画，有效显示的灯珠数量越来越多

                WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);

                for (u16 i = 0; i < _seg_rt->aux_param3; i++) // 尾部数量
                {
                    WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step + (1 + i), _seg->colors[_seg_rt->aux_param + i + 1]);
                }

                if (_seg_rt->aux_param3 < 5)
                    _seg_rt->aux_param3++;
            }
            else
            {
                // 多种颜色组合的流水灯，后半段动画，有效显示的灯珠数量会越来越少

                // printf("_seg_rt->aux_param3 %lu\n", _seg_rt->aux_param3);

                // 优化后的程序：
                for (u16 i = 0; i < (_seg_len + 5 - _seg_rt->counter_mode_step - 1); i++)
                {
                    // WS2812FX_setPixelColor(_seg_len - 1 - (_seg_len + 5 - _seg_rt->counter_mode_step - 1 - 1) + i, _seg->colors[4 - i]);
                    WS2812FX_setPixelColor((_seg_len + 5 - _seg_rt->counter_mode_step - 2) - i, _seg->colors[4 - i]);
                }

                // 未优化的程序：
                // if (5 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(3, _seg->colors[4]);
                //     WS2812FX_setPixelColor(2, _seg->colors[3]);
                //     WS2812FX_setPixelColor(1, _seg->colors[2]);
                //     WS2812FX_setPixelColor(0, _seg->colors[1]);
                // }
                // else if (4 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(2, _seg->colors[4]);
                //     WS2812FX_setPixelColor(1, _seg->colors[3]);
                //     WS2812FX_setPixelColor(0, _seg->colors[2]);
                // }
                // else if (3 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(1, _seg->colors[4]);
                //     WS2812FX_setPixelColor(0, _seg->colors[3]);
                // }
                // else if (2 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(0, _seg->colors[4]);
                // }
            }
        }
        else
        {
            if (_seg_rt->counter_mode_step <= _seg_len - 1) // 从 0 到 _seg_len - 1，对应 _seg_len 个灯珠
            {
                // 多种颜色组合的流水灯，前半段动画，有效显示的灯珠数量越来越多

                WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step, _seg->colors[_seg_rt->aux_param]);

                for (u16 i = 0; i < _seg_rt->aux_param3; i++) // 尾部数量
                {
                    WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step - (1 + i), _seg->colors[_seg_rt->aux_param + i + 1]);
                }

                if (_seg_rt->aux_param3 < 5)
                    _seg_rt->aux_param3++;
            }
            else
            {
                // 多种颜色组合的流水灯，后半段动画，有效显示的灯珠数量会越来越少

                // printf("_seg_rt->aux_param3 %lu\n", _seg_rt->aux_param3);

                // 优化后的程序：
                for (u16 i = 0; i < (_seg_len + 5 - _seg_rt->counter_mode_step - 1); i++)
                {
                    WS2812FX_setPixelColor(_seg_len - 1 - (_seg_len + 5 - _seg_rt->counter_mode_step - 1 - 1) + i, _seg->colors[4 - i]);
                }

                // 未优化的程序：
                // if (5 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(_seg_len - 1 - 3, _seg->colors[4]);
                //     WS2812FX_setPixelColor(_seg_len - 1 - 2, _seg->colors[3]);
                //     WS2812FX_setPixelColor(_seg_len - 1 - 1, _seg->colors[2]);
                //     WS2812FX_setPixelColor(_seg_len - 1 - 0, _seg->colors[1]);
                // }
                // else if (4 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(_seg_len - 1 - 2, _seg->colors[4]);
                //     WS2812FX_setPixelColor(_seg_len - 1 - 1, _seg->colors[3]);
                //     WS2812FX_setPixelColor(_seg_len - 1 - 0, _seg->colors[2]);
                // }
                // else if (3 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(_seg_len - 1 - 1, _seg->colors[4]);
                //     WS2812FX_setPixelColor(_seg_len - 1 - 0, _seg->colors[3]);
                // }
                // else if (2 == (_seg_len + 5 - _seg_rt->counter_mode_step))
                // {
                //     WS2812FX_setPixelColor(_seg_len - 1 - 0, _seg->colors[4]);
                // }
            }
        }
    }

    /*
        整个动画分为： 0 ~ _seg_len + 4个步骤
        0 ~ _seg_len - 5 是单个灯流水
        _seg_len - 4 ~ _seg_len + 5 是多个灯流水
            _seg_len - 4 -> _seg_len 是前半段
            _seg_len -> _seg_len + 5 是后半段
    */
    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len + 5);

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        _seg_rt->aux_param = 0;
        _seg_rt->aux_param3 = 0;
    }

    // return (_seg->speed / (_seg_len + 5)); // 动画的步骤 0 ~ _seg_len + 4
    return (_seg->speed / 24);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

/*
    动画效果，对应样机模式17
    所有灯颜色跳变
    顺序：红 绿 蓝 黄 粉 cyan 白
    执行一次顺序所需的时间与速度值有关

    注意：
    传入的速度值 == 一轮动画的运行时间
*/
u16 WS2812FX_sample_17(void)
{
    for (u16 i = 0; i < _seg_len; i++)
    {
        WS2812FX_setPixelColor(i, _seg->colors[_seg_rt->aux_param]);
    }

    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len);

    if (IS_REVERSE)
    {
        if (_seg_rt->aux_param > 0)
        {
            _seg_rt->aux_param--;
        }
        else
        {
            _seg_rt->aux_param = 6;
        }
    }
    else
    {
        _seg_rt->aux_param++;
        if (_seg_rt->aux_param >= 7)
        {
            _seg_rt->aux_param = 0;
        }
    }

    if (_seg_rt->counter_mode_step == 0)
    {
        SET_CYCLE;

        _seg_rt->aux_param = 0;
    }

    // return (_seg->speed / (_seg_len));
    return (_seg->speed / 12);// 由于灯的数量有数百个，这里改成固定值，不随着灯珠数量变化
}

#if 1

/**
 * @brief 设置为指定模式
 *
 * @param mode_index 模式索引，注意不能超出范围
 */
void lighting_animation_mode_setting(u8 mode_index)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
    {
        return;
    }

    if (mode_index < 1 || mode_index > 20)
    {
        // 模式索引超出范围，直接退出
        printf("mode index out of range\n");
        return;
    }

    fc_effect.Now_state = METEORITE_LAMP_MODE; // 流星灯模式
    save_info.cur_lighting_animation_mode = mode_index;
    lighting_animation_mode_change();
}

/*
    在 模式 1 - 7，单色流星灯动画使用到的数组，[0]为流星灯颜色，[1]为背景色（设置背景好像没有效果）
*/
static volatile u32 color_buff_in_mode_1_7[2] = {BLACK, BLACK};

/*
    根据模式切换灯光动画
    根据 save_info.cur_lighting_animation_mode ，切换到对应的动画


*/
void lighting_animation_mode_change(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    fc_effect.Now_state = METEORITE_LAMP_MODE;    // 流星灯模式

    mode_ptr lighting_mode_ptr = NULL;

    printf("cur mode %u\n", (u16)save_info.cur_lighting_animation_mode);

    // mode 1 ~ 7，单色流星灯
    if (save_info.cur_lighting_animation_mode >= 1 &&
        save_info.cur_lighting_animation_mode <= 7)
    {
        switch (save_info.cur_lighting_animation_mode)
        {
        case 1:
            // color = RED;
            color_buff_in_mode_1_7[0] = RED;
            break;
        case 2:
            color_buff_in_mode_1_7[0] = GREEN;
            break;
        case 3:
            color_buff_in_mode_1_7[0] = BLUE;
            break;
        case 4:
            color_buff_in_mode_1_7[0] = WHITE;
            break;
        case 5:
            color_buff_in_mode_1_7[0] = YELLOW;
            break;
        case 6:
            color_buff_in_mode_1_7[0] = PINK;
            break;
        case 7:
            color_buff_in_mode_1_7[0] = CYAN;
            break;
        }

        color_buff_in_mode_1_7[1] = BLACK; // 背景为黑色（设置背景好像没有效果）

        Adafruit_NeoPixel_clear(); // 清空缓存残留
        WS2812FX_show();

        WS2812FX_stop();
        WS2812FX_setSegment_colorsOptions(
            0,                                          // 第0段
            0,                                          // 起始位置
            // LIGHTING_ANIMATION_LED_NUMS - 1,            // 结束位置
            fc_effect.led_num - 1,            // 结束位置
            &WS2812FX_sample_single_color_meteor_light, // 效果
            color_buff_in_mode_1_7,                     // 颜色，WS2812FX_setColors设置
            save_info.cur_speed,                        // 速度
            save_info.cur_options);                     // 选项
        WS2812FX_start();
    }
    else
    {
        if (8 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_8;
        }
        else if (9 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_9;
        }
        else if (10 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_10;
        }
        else if (11 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_11;
        }
        else if (12 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_12;
        }
        else if (13 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_13;
        }
        else if (14 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_14;
        }
        else if (15 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_15;
        }
        else if (16 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_16;
        }
        else if (17 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_17;
        }
        else if (18 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_18;
        }
        else if (19 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_19;
        }
        else if (20 == save_info.cur_lighting_animation_mode)
        {
            lighting_mode_ptr = WS2812FX_sample_20;
        }

        Adafruit_NeoPixel_clear(); // 清空缓存残留
        WS2812FX_show();

        WS2812FX_stop();
        if (WS2812FX_sample_13 == lighting_mode_ptr)
        {
            WS2812FX_setSegment_colorsOptions(
                0,                               // 第0段
                0,                               // 起始位置
                // LIGHTING_ANIMATION_LED_NUMS - 1, // 结束位置
                fc_effect.led_num - 1, // 结束位置
                lighting_mode_ptr,               // 效果
                color_buff_mode13,               // 颜色，WS2812FX_setColors设置
                save_info.cur_speed,             // 速度
                save_info.cur_options);          // 选项
        }
        else
        {
            WS2812FX_setSegment_colorsOptions(
                0,                               // 第0段
                0,                               // 起始位置
                // LIGHTING_ANIMATION_LED_NUMS - 1, // 结束位置
                fc_effect.led_num - 1, // 结束位置
                lighting_mode_ptr,               // 效果
                color_buff,                      // 颜色，WS2812FX_setColors设置
                save_info.cur_speed,             // 速度
                save_info.cur_options);          // 选项
        }
        WS2812FX_start();
    }
}

void lighting_animation_mode_add(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    fc_effect.Now_state = METEORITE_LAMP_MODE; // 流星灯模式
    // if (save_info.cur_lighting_animation_mode < 8) // 测试用
    if (save_info.cur_lighting_animation_mode < 20) //
    {
        save_info.cur_lighting_animation_mode++;
    }

    // 保存信息：
    // xxxx

    lighting_animation_mode_change();
}

void lighting_animation_mode_sub(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    fc_effect.Now_state = METEORITE_LAMP_MODE;     // 流星灯模式
    if (save_info.cur_lighting_animation_mode > 1) // 最小为模式1，没有模式0
    {
        save_info.cur_lighting_animation_mode--;
    }

    // 保存信息：
    // xxxx

    lighting_animation_mode_change();
}

void lighting_animation_speed_add(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 数值越小，对应的动画速度越快

    // 数值越小，对应的动画速度越快
    if (save_info.cur_speed >= 4000)
    {
        save_info.cur_speed = 3000;
    }
    else if (save_info.cur_speed >= 3000)
    {
        save_info.cur_speed = 2000;
    }
    else if (save_info.cur_speed >= 2000)
    {
        save_info.cur_speed = 1000;
    }
    else
    {
        save_info.cur_speed = 1000;
    }

    printf("cur speed %u\n", save_info.cur_speed);

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

void lighting_animation_speed_sub(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 数值越大，对应的动画速度越慢

    // 样机只有四种速度挡位：

    // 数值越大，对应的动画速度越慢
    if (save_info.cur_speed <= 1000)
    {
        save_info.cur_speed = 2000;
    }
    else if (save_info.cur_speed <= 2000)
    {
        save_info.cur_speed = 3000;
    }
    // else if (save_info.cur_speed >= 2000)
    else
    {
        save_info.cur_speed = 4000;
    }

    printf("cur speed %u\n", save_info.cur_speed);

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 添加动画时间间隔(动画时间间隔变长)
void lighting_animation_time_interval_add(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 单位：ms
    // if (save_info.cur_lighting_animation_time_interval < 4000)
    // {
    //     save_info.cur_lighting_animation_time_interval = 4000;
    // }
    // else if (save_info.cur_lighting_animation_time_interval < 8000)
    if (save_info.cur_lighting_animation_time_interval < 8000)
    {
        save_info.cur_lighting_animation_time_interval = 8000;
    }
    else if (save_info.cur_lighting_animation_time_interval < 10000)
    {
        save_info.cur_lighting_animation_time_interval = 10000;
    }
    else if (save_info.cur_lighting_animation_time_interval < 15000)
    {
        save_info.cur_lighting_animation_time_interval = 15000;
    }

    printf("cur time interval %u\n", save_info.cur_lighting_animation_time_interval);
    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 减少动画时间间隔(动画时间间隔变短)
void lighting_animation_time_interval_sub(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    if (save_info.cur_lighting_animation_time_interval >= 15000)
    {
        save_info.cur_lighting_animation_time_interval = 10000;
    }
    else if (save_info.cur_lighting_animation_time_interval >= 10000)
    {
        save_info.cur_lighting_animation_time_interval = 8000;
    }
    // else if (save_info.cur_lighting_animation_time_interval >= 8000)
    else
    {
        save_info.cur_lighting_animation_time_interval = 4000;
    }

    printf("cur time interval %u\n", save_info.cur_lighting_animation_time_interval);
    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 切换灯光动画的方向(正向、反向)
void lighting_animation_dir_switch(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    if (save_info.cur_options == REVERSE)
    {
        // 如果当前方向为反向，则切换为正向
        save_info.cur_options = NO_OPTIONS;
    }
    else
    {
        // 如果当前方向为正向，则切换为反向
        save_info.cur_options = REVERSE;
    }

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 动画设置为最快速度
void lighting_animation_speed_max(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    save_info.cur_speed = 1000;

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 动画设置为最慢速度
void lighting_animation_speed_min(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    save_info.cur_speed = 4000;

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 动画设置为中速
void lighting_animation_speed_mid(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 中速在 2000 或 3000
    save_info.cur_speed = 3000;

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 时间间隔设置为最快
void lighting_animation_time_interval_fast(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 时间间隔快在 4000
    save_info.cur_lighting_animation_time_interval = 4000;

    printf("cur time interval %u\n", save_info.cur_lighting_animation_time_interval);
    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 时间间隔设置为最慢
void lighting_animation_time_interval_slow(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 时间间隔最慢在 15000
    save_info.cur_lighting_animation_time_interval = 15000;

    printf("cur time interval %u\n", save_info.cur_lighting_animation_time_interval);
    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 时间间隔设置为适中
void lighting_animation_time_interval_mid(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 时间间隔中间在 8000 或 10000
    save_info.cur_lighting_animation_time_interval = 8000;

    printf("cur time interval %u\n", save_info.cur_lighting_animation_time_interval);

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

void lighting_animation_bright_add(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    // 亮度不能超过100%
    if (fc_effect.b < 255 - (255 / 10))
    {
        fc_effect.b += 255 / 10;
    }
    else
    {
        fc_effect.b = 255;
    }

    printf("fc_effect.b == %u\n", (u16)fc_effect.b);

    WS2812FX_setBrightness(fc_effect.b);

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

void lighting_animation_bright_sub(void)
{
    if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
        return;

    if (fc_effect.b > 10 + (255 / 10))
    {
        fc_effect.b -= 255 / 10;
    }
    else
    {
        fc_effect.b = 10;
    }

    printf("fc_effect.b == %u\n", (u16)fc_effect.b);

    WS2812FX_setBrightness(fc_effect.b);

    if (fc_effect.Now_state == METEORITE_LAMP_MODE)
    {
        // 重新开始跑动画
        lighting_animation_mode_change();
    }
}

// 初始化（恢复出厂设置）
void lighting_animation_init(void)
{
    // if (save_info.flag_is_light_on == 0) // 如果灯光没有开启，直接返回
    //     return;

    fc_effect.Now_state = METEORITE_LAMP_MODE; // 流星灯模式
    save_info.cur_lighting_animation_mode = 1; // 灯光动画模式1
    save_info.cur_speed = 3000;
    // save_info.cur_lighting_animation_time_interval = 10.0; // 动画时间间隔 10s
    save_info.cur_lighting_animation_time_interval = 10000; // 动画时间间隔 10s
    // save_info.cur_brightness = 100;
    // save_info.cur_brightness = 10; // 实际观察不到亮度有变化

    save_info.flag_is_light_on = 1;
    fc_effect.on_off_flag = DEVICE_ON;

    save_info.cur_options = NO_OPTIONS;

    // save_info.sequence = NEO_RGB; // RGB 顺序 R->G->B

    lighting_animation_mode_change();
}

// 第一次上电的时候，初始化变量：
void lighting_animation_config_init(void)
{
    fc_effect.Now_state = METEORITE_LAMP_MODE;              // 默认跑流星灯模式
    save_info.cur_lighting_animation_mode = 1;              // 灯光动画模式1
    save_info.cur_speed = 3000;                             // 动画速度
    save_info.cur_lighting_animation_time_interval = 10000; // 动画时间间隔 10s
    save_info.flag_is_light_on = 1;
    fc_effect.on_off_flag = DEVICE_ON;
    save_info.cur_options = NO_OPTIONS;
}

#endif

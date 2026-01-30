#include "led_strip_voice.h"
#include "asm/adc_api.h"
#include "led_strip_drive.h"

#define MAX_SOUND 10
struct MUSIC_VOICE_T
{
    u8 sound_trg;
    u8 meteor_trg;
    u32 adc_sum;
    u32 adc_sum_n;
    int sound_buf[MAX_SOUND];
    u8 sound_cnt;
    int c_v;
    int v;
    u8 valid;
};

struct MUSIC_VOICE_T music_voic = {

    .sound_trg = 0,
    .meteor_trg = 0,
    .adc_sum = 0,
    .adc_sum_n = 0,
    .sound_cnt = 0,
    .valid = 0,
    .v = 0,
    .c_v = 0,
};

// 获取声控结果
// 触发条件：（（当前声音大小 - 平均值）* 100 ）/ 平均值 > 灵敏度（0~100）
// 0:没触发
// 1:触发
u8 get_sound_result(void)
{
    u8 p_trg;
    p_trg = music_voic.sound_trg;
    music_voic.sound_trg = 0;
    return p_trg;
}

u8 get_meteor_result(void)
{
    u8 p_metemor_trg;
    p_metemor_trg = music_voic.meteor_trg;
    music_voic.meteor_trg = 0;
    return p_metemor_trg;
}

void sound_handle(void)
{
#if 0
    u16 adc;
    u8 i;
    // 记录adc值

    if (fc_effect.on_off_flag == DEVICE_ON && (fc_effect.Now_state == IS_light_music))
    // if(fc_effect.on_off_flag == DEVICE_ON && (fc_effect.star_index ==11 || fc_effect.star_index ==12 || fc_effect.star_index ==13))
    {

        music_voic.sound_buf[music_voic.sound_cnt] = check_mic_adc();
        music_voic.c_v = music_voic.sound_buf[music_voic.sound_cnt]; // 记录当前值
        music_voic.sound_cnt++;

        if (music_voic.sound_cnt > (MAX_SOUND - 1))
        {
            music_voic.sound_cnt = 0;
            music_voic.valid = 1;
            music_voic.v = 0;
            for (i = 0; i < MAX_SOUND; i++)
            {
                music_voic.v += music_voic.sound_buf[i];
            }
            music_voic.v = music_voic.v / MAX_SOUND; // 计算平均值
        }

        if (music_voic.valid)
        {

            if (music_voic.c_v > music_voic.v)
            {
                if ((music_voic.c_v - music_voic.v) * 100 / music_voic.v > fc_effect.music.s) // 很灵敏
                {
                    music_voic.sound_trg = 1;  // 七彩声控
                    music_voic.meteor_trg = 1; // 流星声控
                    WS2812FX_trigger(); // 让主循环扫描到，立即切换动画；如果没有这一句，声控的灵敏度会差一些
                }
            }
        }
    }
    else
    {
        music_voic.valid = 0;
    }
#endif

#if 1 // 移植其他项目的声控程序

    if (fc_effect.on_off_flag != DEVICE_ON)
    {
        return;
    }

#define SAMPLE_N 20
    static volatile u32 adc_sum = 0;
    static volatile u32 adc_sum_n = 0;
    static volatile u8 adc_v_n = 0;
    static volatile u8 adc_avrg_n = 0;
    static volatile u16 adc_v[SAMPLE_N] = {0};
    static volatile u32 adc_avrg[10] = {0}; // 记录5个平均值
    static volatile u32 adc_total[15] = {0};

    u8 trg_v = 0;
    volatile u16 adc = 0;
    u32 adc_all = 0;
    u32 adc_ttl = 0;

    // 记录adc值
    adc = check_mic_adc(); // 每次进入，采集一次ad值（即使不在声控模式，也会占用一些时间）

    // printf("adc = %d", adc);

    if (adc >= 1000)
    {
        return;
    }

    // if (adc < 1000) // 当ADC值大于1000，说明硬件电路有问题
    if (adc_sum_n < 2000)
    {
        // 从0开始，一直加到2000，每10ms加一，总共要20s
        adc_sum_n++;
    }

    if (adc_sum_n == 2000)
    {
        if (adc / (adc_sum / adc_sum_n) > 3)
            return; // adc突变，大于平均值的3倍，丢弃改值

        adc_sum = adc_sum - adc_sum / adc_sum_n;
    }

    adc_sum += adc; // 累加adc值

    adc_v_n %= SAMPLE_N;
    adc_v[adc_v_n] = adc;
    adc_v_n++;
    adc_all = 0;

    // 计算ad值总和
    for (u8 i = 0; i < SAMPLE_N; i++)
    {
        adc_all += adc_v[i];
    }

    // 获取ad值平均值
    adc_avrg_n %= 10;
    adc_avrg[adc_avrg_n] = adc_all / SAMPLE_N;
    adc_avrg_n++;
    adc_ttl = 0;

    // 在平均值的基础上，再求总和
    for (u8 i = 0; i < 10; i++)
    {
        adc_ttl += adc_avrg[i];
    }

    memmove((u8 *)adc_total, (u8 *)adc_total + 4, 14 * 4); // 将 src 指向的内存区域中的前 n 个字节复制到 dest 指向的内存区域（能够安全地处理内存重叠的情况）

    adc_total[14] = adc_ttl / 10; // 总数平均值

    if (adc_sum_n != 0)
    {
        u32 adc_sum_avrg = adc_sum / adc_sum_n;
        if (adc * fc_effect.music.s / 100 > adc_sum_avrg)
        {
            if (fc_effect.Now_state == IS_light_music)
            {
                // WS2812FX_triggered_by_colorful_lights();
                music_voic.sound_trg = 1;  // 七彩声控
                music_voic.meteor_trg = 1; // 流星声控
                WS2812FX_trigger();        // 让主循环扫描到，立即切换动画；如果没有这一句，声控的灵敏度会差一些
            }
        }
    }
    // } // if (adc < 1000) // 当ADC值大于1000，说明硬件电路有问题

#endif // 移植其他项目的声控程序
}

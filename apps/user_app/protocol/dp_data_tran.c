#include "cpu.h"
#include "led_strip_sys.h"
#include "paint_tool.h"
#include "dp_data_tran.h"
#include "led_strand_effect.h"
#include "ble_multi_profile.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "btstack/le/att.h"
#include "ble_user.h"
#include "btstack/le/ble_api.h"
#include "led_strip_drive.h"
#include "one_wire.h"

#include "../../../apps/user_app/save_flash/save_flash.h" // 包含 save_info_t 结构体类型定义，save_info 结构体变量定义

dp_data_header_t dp_data_header; // 涂鸦DP数据头
dp_switch_led_t dp_switch_led;   // DPID_SWITCH_LED开关
dp_work_mode_t dp_work_mode;     // DPID_WORK_MODE工作模式
dp_countdown_t dp_countdown;     // DPID_COUNTDOWN倒计时
dp_music_data_t dp_music_data;   // DPID_MUSIC_DATA音乐灯
// dp_secene_data_t  dp_secene_data;  //DPID_RGBIC_LINERLIGHT_SCENE炫彩情景
dp_lednum_set_t dp_lednum_set; // DPID_LED_NUMBER_SET led点数设置
dp_draw_tool_t dp_draw_tool;   // DPID_DRAW_TOOL 涂抹功能

/************************************************************************************
 *@  函数：string_hex_Byte
 *@  描述：把字符串转为HEX，支持小写abcdef,输出uint8、uint16、uint32类型的hex
 *@  形参1: str         <字符输入>
 *@  形参2: out_Byte    <输出的1、2、4字节>
 *@  返回：unsigned long
 ************************************************************************************/
unsigned long string_hex_Byte(char *str, unsigned char out_Byte)
{
    int i = 0;
    unsigned char temp = 0;
    unsigned long hex = 0;
    if ((out_Byte == 1) || (out_Byte == 2) || (out_Byte == 4))
    {
        while (i < (out_Byte * 2))
        {
            hex <<= 8;
            if (str[i] >= '0' && str[i] <= '9')
            {
                temp = (str[i] & 0x0f);
            }
            else if (str[i] >= 'a' && str[i] <= 'f')
            {
                temp = ((str[i] + 0x09) & 0x0f);
            }
            ++i;
            if (out_Byte != 1)
            {
                temp <<= 4;
                if (str[i] >= '0' && str[i] <= '9')
                {
                    temp |= (str[i] & 0x0f);
                }
                else if (str[i] >= 'a' && str[i] <= 'f')
                {
                    temp |= ((str[i] + 0x09) & 0x0f);
                }
            }
            ++i;
            hex |= temp;
        }
    }
    else
        return 0xffffffff; // 错误类型
    return hex;
}

/*****************************************
 *@  函数：dp_extract_data_handle
 *@  描述：dp数据提取
 *@  形参: buff
 *@  返回：DP数据长度
 ****************************************/
unsigned short dp_extract_data_handle(unsigned char *buff)
{
    unsigned char num = 0;
    unsigned char max;
    unsigned char temp;
    /*提取涂鸦DP数据头*/
    dp_data_header.id = buff[0];
    dp_data_header.type = buff[1];
    dp_data_header.len = buff[2];
    dp_data_header.len <<= 8;
    dp_data_header.len |= buff[3];

    // m_hsv_to_rgb(&colour_R,&colour_G,&colour_B,colour_H,colour_S,colour_V);

    /*提取DP数据*/
    switch (dp_data_header.id)
    {
    case DPID_SWITCH_LED: // 开关(可下发可上报)
        printf("\r\n DPID_SWITCH_LED");

        fc_effect.on_off_flag = buff[4];
        if (fc_effect.on_off_flag == DEVICE_ON)
        {
            soft_turn_on_the_light();
        }
        else
        {
            soft_turn_off_lights();
        }
        break;

    case DPID_WORK_MODE: // 工作模式(可下发可上报)
        dp_work_mode.mode = buff[4];

        printf("dp_work_mode.mode = %d\r\n", dp_work_mode.mode);
        printf("\r\n");

        break;

    case DPID_BRIGHT_VALUE: // 白光亮度(可下发可上报)
        break;

    case DPID_COLOUR_DATA: // 彩光(可下发可上报)
        break;
#if 0
    case DPID_COUNTDOWN://倒计时(可下发可上报)
        //DP协议参数
        dp_countdown.time = buff[4];
        dp_countdown.time <<= 8;
        dp_countdown.time |= buff[5];
        dp_countdown.time <<= 8;
        dp_countdown.time |= buff[6];
        dp_countdown.time <<= 8;
        dp_countdown.time |= buff[7];

        printf("dp_countdown.time = %d\r\n",dp_countdown.time);
        printf("\r\n");

      //效果参数
        fc_effect.countdown.time = dp_countdown.time;

        printf("fc_effect.countdown.time = %d\r\n",fc_effect.countdown.time);
        printf("\r\n");

        break;
#endif
    case DPID_MUSIC_DATA: // 音乐律动(只下发)
                          // DP协议参数
        dp_music_data.change_type = string_hex_Byte(&buff[4], 1);
        dp_music_data.colour.h_val = string_hex_Byte(&buff[5], 2);
        dp_music_data.colour.s_val = string_hex_Byte(&buff[9], 2);
        dp_music_data.colour.v_val = string_hex_Byte(&buff[13], 2);
        dp_music_data.white_b = string_hex_Byte(&buff[17], 2);
        dp_music_data.ct = string_hex_Byte(&buff[21], 2);

        break;

    case DPID_RGBIC_LINERLIGHT_SCENE: // 炫彩情景(可下发可上报)
        fc_effect.Now_state = IS_light_scene;

        // 变化类型、模式
        fc_effect.dream_scene.change_type = buff[4];
        // 方向
        fc_effect.dream_scene.direction = buff[5];
        // 段大小
        fc_effect.dream_scene.seg_size = buff[6];
        if (fc_effect.dream_scene.c_n > MAX_NUM_COLORS)
        {
            fc_effect.dream_scene.c_n = MAX_NUM_COLORS;
        }
        // 颜色数量
        fc_effect.dream_scene.c_n = buff[7];
        // 清除rgb[0~n]数据
        memset(fc_effect.dream_scene.rgb, 0, sizeof(fc_effect.dream_scene.rgb));

        // 数据循环传输
        for (num = 0; num < fc_effect.dream_scene.c_n; num++)
        {
            fc_effect.dream_scene.rgb[num].r = buff[8 + num * 3];
            fc_effect.dream_scene.rgb[num].g = buff[9 + num * 3];
            fc_effect.dream_scene.rgb[num].b = buff[10 + num * 3];
        }
        printf("\n fc_effect.dream_scene.c_n=%d", fc_effect.dream_scene.c_n);
        printf_buf(fc_effect.dream_scene.rgb, fc_effect.dream_scene.c_n * sizeof(color_t));
        printf("\n fc_effect.dream_scene.direction=%d", fc_effect.dream_scene.direction);
        set_fc_effect();
        break;

    case DPID_LED_NUMBER_SET: // led点数设置(可下发可上报)
                              // DP协议参数
        dp_lednum_set.lednum = buff[4];
        dp_lednum_set.lednum <<= 8;
        dp_lednum_set.lednum |= buff[5];
        dp_lednum_set.lednum <<= 8;
        dp_lednum_set.lednum |= buff[6];
        dp_lednum_set.lednum <<= 8;
        dp_lednum_set.lednum |= buff[7];

        printf("dp_lednum_set.lednum = %d\r\n", dp_lednum_set.lednum);
        printf("\r\n");

        // 效果参数
        fc_effect.led_num = dp_lednum_set.lednum;

        if (fc_effect.led_num >= 48)
            fc_effect.led_num = 48;

        break;

    case DPID_DRAW_TOOL: // 涂抹功能(可下发可上报)
        // DP协议参数
        dp_draw_tool.tool = buff[5];

        dp_draw_tool.colour.h_val = buff[7];
        dp_draw_tool.colour.h_val <<= 8;
        dp_draw_tool.colour.h_val |= buff[8];
        dp_draw_tool.colour.s_val = buff[9] * 10;
        dp_draw_tool.colour.v_val = buff[10] * 10;

        dp_draw_tool.white_b = buff[11] * 10;

        if (buff[13] == 0x81)
        {
            dp_draw_tool.led_place = buff[14];
            dp_draw_tool.led_place <<= 8;
            dp_draw_tool.led_place |= buff[15];
        }

        // 效果参数
        if (dp_draw_tool.white_b != 0)
        {
            dp_draw_tool.colour.h_val = 0;
            dp_draw_tool.colour.s_val = 0;
            dp_draw_tool.colour.v_val = dp_draw_tool.white_b;
        }

        // effect_smear_adjust_updata((smear_tool_e )dp_draw_tool.tool, &(dp_draw_tool.colour), &dp_draw_tool.led_place);

        break;
    }
    return (dp_data_header.len + sizeof(dp_data_header_t));
}

u8 Ble_Addr[6]; // 蓝牙地址
extern hci_con_handle_t ZD_HCI_handle;

extern ALARM_CLOCK alarm_clock[3];
extern TIME_CLOCK time_clock;

/**
 * @brief 向app反馈信息
 *
 * @param p   数据内容
 * @param len  数据长度
 */
void zd_fb_2_app(u8 *p, u8 len)
{
    uint8_t fc_buffer[20]; // 发送缓存
    memcpy(fc_buffer, Ble_Addr, 6);
    memcpy(fc_buffer + 6, p, len);
    ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, fc_buffer, len + 6, ATT_OP_AUTO_READ_CCC);
}

/*********************************************************
 *
 *      APP反馈 API
 *
 *********************************************************/

// 向app发送灯的开关状态
void app_feedback_led_on_off_state(void)
{
    u8 send_buff[3];

    // 灯的开关状态：
    send_buff[0] = 0x01;
    send_buff[1] = 0x01;
    send_buff[2] = fc_effect.on_off_flag; //
    zd_fb_2_app(send_buff, 3);
    // 流星灯的开关状态：
    send_buff[0] = 0x2F;
    send_buff[1] = 0x02;
    // send_buff[2] = save_info.flag_is_light_on;
    send_buff[2] = fc_effect.on_off_flag;
    zd_fb_2_app(send_buff, 3);
}

/**
 * @brief 向app反馈灯开关状态
 *          fb_led_on_off_state   feedback_led_on_off_state
 */
void fb_led_on_off_state(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x01;
    tp_buffer[1] = 0x01;
    tp_buffer[2] = fc_effect.on_off_flag; //

    zd_fb_2_app(tp_buffer, 3);
}
/**
 * @brief 反馈音乐模式
 *
 */
void fb_led_music_mode(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x06;
    tp_buffer[1] = 0x06;
    tp_buffer[2] = fc_effect.music.m; //

    zd_fb_2_app(tp_buffer, 3);
}

/**
 * @brief 反馈流星速度
 *
 */
void fd_meteor_speed(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x01;
    tp_buffer[2] = fc_effect.app_star_speed;

    zd_fb_2_app(tp_buffer, 3);
}

/**
 * @brief 反馈流星周期
 *
 */
void fd_meteor_cycle(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x03;
    tp_buffer[2] = fc_effect.meteor_period;

    zd_fb_2_app(tp_buffer, 3);
}

/**
 * @brief 反馈流星开关
 *
 */
void fd_meteor_on_off(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x02;
    // tp_buffer[2] =  fc_effect.star_on_off;
    tp_buffer[2] = fc_effect.on_off_flag;
    zd_fb_2_app(tp_buffer, 3);
}

void fb_bright(void)
{
    uint8_t tp_buffer[3];
    tp_buffer[0] = 0x04;
    tp_buffer[1] = 0x03;
    tp_buffer[2] = fc_effect.app_b;

    zd_fb_2_app(tp_buffer, 3);
}

void fb_speed(void)
{

    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x04;
    tp_buffer[1] = 0x04;
    tp_buffer[2] = fc_effect.app_speed;

    zd_fb_2_app(tp_buffer, 3);
}

// 向app反馈声控的灵敏度
void fb_sensitive(void)
{

    uint8_t tp_buffer[6];
    // tp_buffer[0] = 0x02;
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x05;
    tp_buffer[2] = fc_effect.music.s;
    // tp_buffer[2] = 100 - fc_effect.music.s;

    zd_fb_2_app(tp_buffer, 3);
}

void fb_rgb_value(void)
{

    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x04;
    tp_buffer[1] = 0x01;
    tp_buffer[2] = 0x1e;
    tp_buffer[3] = fc_effect.rgb.r;
    tp_buffer[4] = fc_effect.rgb.g;
    tp_buffer[5] = fc_effect.rgb.b;

    zd_fb_2_app(tp_buffer, 6);
}

void fb_RGBsequence(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x04;
    tp_buffer[1] = 0x05;
    tp_buffer[2] = fc_effect.sequence;
    zd_fb_2_app(tp_buffer, 6);
}

/**
 * @brief 反馈电机速度
 *
 */
void fb_motor_speed(void)
{
    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x07;
    tp_buffer[2] = fc_effect.base_ins.period;
    zd_fb_2_app(tp_buffer, 6);
}

/**
 * @brief 反馈电机模式
 *
 */
void fb_motor_mode(void)
{

    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x06;
    tp_buffer[2] = fc_effect.base_ins.mode;
    zd_fb_2_app(tp_buffer, 6);
}

/**
 * @brief 反馈电机模式
 *
 */
void fb_motor_switch(void)
{

    uint8_t tp_buffer[6];
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x08;
    tp_buffer[2] = fc_effect.motor_on_off;
    zd_fb_2_app(tp_buffer, 6);
}

void app_feedback_meteor_lights_speed(void)
{
    u8 feedback_speed = 100 - (u32)(save_info.cur_speed - 1000) * 100 / 3000; // 将 1000~4000 转换成 0~100 的数值;
    printf("feedback_speed:%u\n", (u16)feedback_speed);

    // 向app反馈流星灯速度（app的速度值是0~100，这里需要做转换）
    u8 tp_buffer[3]; // 存放要向app发送的数据
    tp_buffer[0] = 0x2F;
    tp_buffer[1] = 0x01;
    // tp_buffer[2] = (u32)(save_info.cur_speed - 1000) * 100 / 3000; // 将 1000~4000 转换成 0~100 的数值
    tp_buffer[2] = feedback_speed;
    zd_fb_2_app(tp_buffer, 3);
}

void app_feedback_meteor_lights_brightness(void)
{
    // 向app反馈亮度（app的亮度是 0~100，这里需要做转换）
    fc_effect.app_b = (u32)fc_effect.b * 100 / 255;
    fb_bright(); // 向app返回亮度数据
}

/* 解析中道数据，主要是静态模式，和动态效果的“基本”效果 */
void parse_zd_data(unsigned char *LedCommand)
{
    printf("\n====================================\n");
    printf("recv_data:\n");
    for (u8 i = 0; i < 10; i++)
    {
        printf("0x %x \t", LedCommand[i]);
    }
    printf("\n");
    printf("\n====================================\n");

    uint8_t Send_buffer[20]; // 发送缓存
    memcpy(Send_buffer, Ble_Addr, 6);
    if (LedCommand[0] == 0x01 && LedCommand[1] == 0x03) // 与APP同步数据
    {
#if 0
//灯光
        // -----------------设备信息------------------------------
        Send_buffer[6] = 0x07;
        Send_buffer[7] = 0x01;
        Send_buffer[8] = 0x01;
        Send_buffer[9] = 0x02;  //灯具类型：RGBW
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 10, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);
        //-------------------发送开关机状态---------------------------
        Send_buffer[6] = 0x01;
        Send_buffer[7] = 0x01;
        Send_buffer[8] = get_on_off_state(); // 目前状态
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送亮度---------------------------
        Send_buffer[6] = 0x04;
        Send_buffer[7] = 0x03;
        Send_buffer[8] = fc_effect.app_b;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送速度---------------------------
        Send_buffer[6] = 0x04;
        Send_buffer[7] = 0x04;
        Send_buffer[8] = fc_effect.app_speed;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送灯带长度---------------------------
        Send_buffer[6] = 0x04;
        Send_buffer[7] = 0x08;
        Send_buffer[8] = fc_effect.led_num>>8;
        Send_buffer[9] = fc_effect.led_num&0xff;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 10, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);



        //-------------------发送静态RGB模式--------------------------
        Send_buffer[6] = 0x04;
        Send_buffer[7] = 0x01;
        Send_buffer[8] = 0x1e;
        Send_buffer[9] = fc_effect.rgb.r;
        Send_buffer[10] = fc_effect.rgb.g;
        Send_buffer[11] = fc_effect.rgb.b;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送闹钟1定时数据--------------------------
        Send_buffer[6] = 0x05;
        Send_buffer[7] = 0x00;
        Send_buffer[8] = alarm_clock[0].hour;
        Send_buffer[9] = alarm_clock[0].minute;
        Send_buffer[10] = alarm_clock[0].on_off;
        Send_buffer[11] = alarm_clock[0].mode;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送闹钟2定时数据--------------------------
        Send_buffer[6] = 0x05;
        Send_buffer[7] = 0x01;
        Send_buffer[8] = alarm_clock[1].hour;
        Send_buffer[9] = alarm_clock[1].minute;
        Send_buffer[10] = alarm_clock[1].on_off;
        Send_buffer[11] = alarm_clock[1].mode;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送闹钟3定时数据--------------------------
        Send_buffer[6] = 0x05;
        Send_buffer[7] = 0x02;
        Send_buffer[8] = alarm_clock[2].hour;
        Send_buffer[9] = alarm_clock[2].minute;
        Send_buffer[10] = alarm_clock[2].on_off;
        Send_buffer[11] = alarm_clock[2].mode;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 12, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------发送RGB接口模式--------------------------
        Send_buffer[6] = 0x04;
        Send_buffer[7] = 0x05;
        Send_buffer[8] = fc_effect.sequence;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);

        //-------------------声控模式（手机麦或外麦--------------------------
        Send_buffer[6] = 0x06;
        Send_buffer[7] = 0x07;
        Send_buffer[8] = fc_effect.music.m_type;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);
        //-------------------本地麦克风模式--------------------------
        Send_buffer[6] = 0x06;
        Send_buffer[7] = 0x06;
        Send_buffer[8] = fc_effect.music.m ;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);
#endif
        //-------------------发送开关机状态---------------------------
        Send_buffer[6] = 0x01;
        Send_buffer[7] = 0x01;
        Send_buffer[8] = get_on_off_state(); // 目前状态
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);
        // 流星
        //-------------------流星开关--------------------------
        Send_buffer[6] = 0x2F;
        Send_buffer[7] = 0x02;
        // Send_buffer[8] = fc_effect.star_on_off;
        // Send_buffer[8] = fc_effect.on_off_flag;
        Send_buffer[8] = get_on_off_state();
        printf("cur on off state: %u\n", (u16)Send_buffer[8]);
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);
        //-------------------流星速度--------------------------
        // Send_buffer[6] = 0x2F;
        // Send_buffer[7] = 0x01;
        // Send_buffer[8] = fc_effect.app_star_speed;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        //-------------------流星周期--------------------------
        // Send_buffer[6] = 0x2F;
        // Send_buffer[7] = 0x03;
        // Send_buffer[8] = fc_effect.meteor_period;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);

#if 1
        //-------------------发送灵敏度---------------------------
        Send_buffer[6] = 0x2F;
        Send_buffer[7] = 0x05;
        Send_buffer[8] = fc_effect.music.s;
        ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
        os_time_dly(1);
#endif
    }
    else
    {
        //---------------------------------接收到开关灯命令-----------------------------------
        if (LedCommand[0] == 0x01 && LedCommand[1] == 0x01)
        // if(LedCommand[0]==0x2F && LedCommand[1]==0x02)
        {
            // 板子上只有流星灯，所以流星灯和灯光的开关状态是一样的
            printf("app set on off/n");

            extern void set_on_off_led(u8 on_off);

            /*
                根据接收到的指令来控制 开灯/关灯（函数内部会向app反馈当前开关机状态）
            */
            set_on_off_led(LedCommand[2]);
        }
        //--------------------------下面所有的操作都需要在开灯状态下操作-----------------------------------

        //-------------------------------- 流星开关 -----------------------------------
        else if (LedCommand[0] == 0x2F && LedCommand[1] == 0x02)
        {
            // 板子上只有流星灯，所以流星灯和灯光的开关状态是一样的

            // 在app流星灯界面，选择对应的模式 开/关，设置完成后，需要返回对应的信息给app
            printf("app set Meteorite lamp on/off\n");

            /*
                根据接收到的指令来控制 开灯/关灯
                函数内部会向app反馈当前开关机状态
            */
            set_on_off_led(LedCommand[2]);
        }

        if (get_on_off_state()) //
        {
            //---------------------------------更新RGB-----------------------------------
            if (LedCommand[0] == 0x04 && LedCommand[1] == 0x01 && LedCommand[2] == 0x1e)
            {
                // 会设置灯光为静态模式，RGB值 R、G、B
                set_static_mode(LedCommand[3], LedCommand[4], LedCommand[5]);
                fb_rgb_value(); // feedback，返回设置好的rgb值
            }

            //---------------------------------调节亮度0-100-----------------------------------
            if (LedCommand[0] == 0x04 && LedCommand[1] == 0x03)
            {
                // 在app模式界面，调节亮度，范围0-100（注意，只有静态模式才调节亮度）
                printf("set brightness\n");
                extern void app_set_bright(u8 tp_b);
                app_set_bright(LedCommand[2]);
                fb_bright(); // 向app返回亮度数据
            }

            //---------------------------------调节速度0-100-----------------------------------
            if (LedCommand[0] == 0x04 && LedCommand[1] == 0x04)
            {
                // 在app模式界面，调节速度，范围0-100
                printf("app set speed\n");
                /*
                    样机的动画速度
                    最快 1000 ， 对应 1s
                    最慢 4000 ， 对应 4s

                    app设置100， save_info.cur_speed == 1000 (动画速度最快)
                    app设置0， save_info.cur_speed == 4000 (动画速度最慢)
                    app设置25， save_info.cur_speed == 2250
                    其他值， save_info.cur_speed > 1000 && save_info.cur_speed < 4000
                */
                save_info.cur_speed = (u16)(4000 - (u16)((u32)(4000 - 1000) * LedCommand[2] / 100));
                printf("save_info.cur_speed %u\n", save_info.cur_speed);

                if (fc_effect.Now_state == METEORITE_LAMP_MODE)
                {
                    // 如果当前是流星模式，才重新开始跑动画
                    lighting_animation_mode_change();
                }

                // 向app反馈数据
                uint8_t tp_buffer[3];
                tp_buffer[0] = 0x04;
                tp_buffer[1] = 0x04;
                tp_buffer[2] = LedCommand[2];
                zd_fb_2_app(tp_buffer, 3);

                // extern void app_set_speed(u8 tp_speed);
                // app_set_speed(LedCommand[2]);
                // fb_speed();
            }
            //---------------------------------更改RGB接口-----------------------------------
            if (LedCommand[0] == 0x04 && LedCommand[1] == 0x05)
            {
                // 更改RGB线序
                extern void app_set_RGBsequence(u8 s);
                app_set_RGBsequence(LedCommand[2]);
                fb_RGBsequence();
            }
            //---------------------------------W（灰度调节）控制----------------------------
            // if (LedCommand[0] == 0x04 && LedCommand[1] == 0x06)
            // {
            //     printf("set w\n");
            //     extern void app_set_w(u8 tp_w);
            //     app_set_w(LedCommand[2]);
            // }
            //---------------------------------灯带长度-----------------------------------
            // if (LedCommand[0] == 0x04 && LedCommand[1] == 0x08)
            // {
            // }
            //---------------------------------手机音乐律动 手机麦克风-----------------------------------
            if (LedCommand[0] == 0x06 && LedCommand[1] == 0x04)
            {
                // 接收app发送过来的数据，直接显示对应的颜色
                if (fc_effect.music.m_type == PHONE_MIC) // 确认当前是不是手机麦模式
                {
                    app_set_bright(LedCommand[5]);
                    set_static_mode(LedCommand[2], LedCommand[3], LedCommand[4]);
                }
            }

            //---------------------------------外麦声控模式-----------------------------------
            if (LedCommand[0] == 0x06 && LedCommand[1] == 0x06)
            {
                extern void app_set_music_mode(u8 tp_m);
                if (fc_effect.music.m_type == EXTERIOR_MIC) // 确认当前是不是外模模式
                {
                    app_set_music_mode(LedCommand[2]);
                    Send_buffer[6] = 0x06;
                    Send_buffer[7] = 0x06;
                    Send_buffer[8] = LedCommand[2];
                    ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
                }
            }
            //---------------------------------设备手机麦或者外麦-----------------------------------
            if (LedCommand[0] == 0x06 && LedCommand[1] == 0x07)
            {
                // 设置是用手机麦还是外部麦
                extern void set_music_type(u8 ty);
                set_music_type(LedCommand[2]);
                Send_buffer[6] = 0x06;
                Send_buffer[7] = 0x07;
                Send_buffer[8] = LedCommand[2];
                ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, 9, ATT_OP_AUTO_READ_CCC);
            }

            //---------------------------------设置麦克风，电机，流星灵敏度-----------------------------------
            if (LedCommand[0] == 0x2F && LedCommand[1] == 0x05)
            {
                // 声控灵敏度范围：0~100
                extern void app_set_sensitive(u8 tp_s); 
                app_set_sensitive(LedCommand[2]);
                fb_sensitive();
            }

            //--------------------------  流星相关 ----------------------------------------------

            // --------------------------------流星模式-----------------------------------
            if (LedCommand[0] == 0x2F && LedCommand[1] == 0x00) //  && fc_effect.star_on_off == DEVICE_ON)
            {
                // 在app模式界面/流星灯界面，选择对应的模式(流星模式，音乐律动模式)
                printf("app set Meteorite lamp mode \n");
                app_set_mereor_mode(LedCommand[2]); // 函数内部会根据传参设置好 fc_effect.Now_state
            }

            //-------------------------------- 流星速度-----------------------------------
            if (LedCommand[0] == 0x2F && LedCommand[1] == 0x01) // && fc_effect.star_on_off == DEVICE_ON)
            {
                // 设置速度值，范围：0-100
                printf("app set Meteorite lamp speed \n");

                // 将传入的0~100的速度值转换为1000~4000的数值：
                save_info.cur_speed = (u16)(4000 - (u16)((u32)(4000 - 1000) * LedCommand[2] / 100));
                printf("save_info.cur_speed %u\n", save_info.cur_speed);

                if (fc_effect.Now_state == METEORITE_LAMP_MODE)
                {
                    // 如果当前是流星模式，才重新跑动画
                    lighting_animation_mode_change();
                }

                // 向app反馈流星灯速度
                uint8_t tp_buffer[3];
                tp_buffer[0] = 0x2F;
                tp_buffer[1] = 0x01;
                tp_buffer[2] = LedCommand[2];
                zd_fb_2_app(tp_buffer, 3);
            }

            // --------------------------------流星灯时间间隔-----------------------------------
            // if (LedCommand[0] == 0x2F && LedCommand[1] == 0x03 && fc_effect.star_on_off == DEVICE_ON)
            if (LedCommand[0] == 0x2F && LedCommand[1] == 0x03)
            {
                // 设置流星灯动画间的时间间隔，范围：2-20
                u16 timer_interval = 0;
                printf("app set lamp time interval"); // app 设置动画时间间隔

                // 将app传入的2~20的数值转换为0~18的数值
                if (LedCommand[2] >= 2 && LedCommand[2] <= 20)
                {
                    timer_interval = LedCommand[2] - 2;
                }

                /*
                    将传入的2~20的时间间隔转换为4000~15000的数值：
                    时间最短 4000，对应 4s
                    时间最长 15000，对应 15s

                    app设置 2 ， 对应的数值 4000
                    app设置 20 ， 对应的数值 15000
                    app设置 15 ， 对应的数值 7944

                    对应的计算公式：
                    数值 = 4000 + (15000 - 4000) * 时间间隔 / 18
                */
                // if (save_info.cur_lighting_animation_mode >= 1 && save_info.cur_lighting_animation_mode <= 10)
                {
                    save_info.cur_lighting_animation_time_interval = (u16)(4000 + (u16)((u32)(15000 - 4000) * timer_interval / 18));
                }

                printf("save_info.cur_lighting_animation_time_interval %u\n", save_info.cur_lighting_animation_time_interval);

                if (fc_effect.Now_state == METEORITE_LAMP_MODE)
                {
                    // 如果当前是流星模式，才重新跑动画
                    lighting_animation_mode_change();
                }

                // 向app反馈时间间隔：
                uint8_t tp_buffer[3];
                tp_buffer[0] = 0x2F;
                tp_buffer[1] = 0x03;
                tp_buffer[2] = timer_interval + 2; // 由于前面减去了2，这里要加2
                zd_fb_2_app(tp_buffer, 3);
            }
        }
    }
}

/* APP数据解析入口函数 */
void parse_led_strip_data(u8 *pBuf, u8 len)
{

    /* 涂鸦DP协议解析 */
    dp_extract_data_handle(pBuf);
    /* 中道孔明灯协议解析 */
    parse_zd_data(pBuf);

    // save_info_write();      // 保存 save_info 数据
    // save_user_data_area3(); // 保存参数配置到flash
    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

void tuya_fb_sw_state(void)
{
    dp_data_header_t *p_dp;
    u8 dp_data[4 + 1];
    p_dp = (dp_data_header_t *)dp_data;
    p_dp->id = DPID_SWITCH_LED;
    p_dp->type = DP_TYPE_BOOL;
    p_dp->len = __SWP16(1);
    dp_data[4] = 1; // 默认开机
}

/* -------------------------------------DPID_CONTROL_DATA 调节模式----------------------------- */

// 调节(只下发)
// 备注:类型：字符串;
// Value: 011112222333344445555  ;
// 0：   变化方式，0表示直接输出，1表示渐变;
// 1111：H（色度：0-360，0X0000-0X0168）;
// 2222：S (饱和：0-1000, 0X0000-0X03E8);
// 3333：V (明度：0-1000，0X0000-0X03E8);
// 4444：白光亮度（0-1000）;
// 5555：色温值（0-1000）
//  typedef struct
//  {
//      uint8_t change_type;        //变化方式，0表示直接输出，1表示渐变;
//      hsv_t c;
//      uint16_t white_b;        //白光亮度
//      uint16_t ct;             //色温
//  }dp_control_t; //DPID_CONTROL_DATA

/* -------------------------------------DPID_SCENE_DATA 场景----------------------------- */

// 场景(可下发可上报)
// 备注:Value: 0011223344445555666677778888
// 00：情景号
// 11：单元切换间隔时间（0-100）
// 22：单元变化时间（0-100）
// 33：单元变化模式（0 静态 1 跳变 2 渐变）
// 4444：H（色度：0-360，0X0000-0X0168）
// 5555：S (饱和：0-1000, 0X0000-0X03E8)
// 6666：V (明度：0-1000，0X0000-0X03E8)
// 7777：白光亮度（0-1000）
// 8888：色温值（0-1000）
// 注：数字 1-8 的标号对应有多少单元就有多少组
//  #pragma pack (1)
//  typedef struct
//  {
//      uint8_t sw_time;            //单元切换间隔时间
//      uint8_t chg_time;            //单元变化时间
//      uint8_t change_type;        //单元变化模式（0 静态 1 跳变 2 渐变）
//      hsv_t c;
//      uint16_t white_b;           //白光亮度
//      uint16_t ct;                //色温
//  }dp_secene_data_t;                 //场景数据

// typedef struct
// {
//     uint8_t secene_n;           //情景号
//     dp_secene_data_t  *data;        //多组
// }dp_secene_t;
// #pragma pack ()
/*
typedef struct
{
    color_t c[MAX_CORLOR_N];               //颜色池
    uint8_t s;                              //速度
    uint8_t n;                             //当前颜色数量
    uint8_t change_type;                   //变化模式（0 静态 1 跳变 2 渐变）
}fc_effect_t;       //全彩效果

 */
// void dp_secene_data_handle(u8 *pIn)
// {
//     dp_data_header_t *header = (dp_secene_t*) pIn;
//     uint8_t len, i = 0;
//     if(header->type != DP_TYPE_STRING)
//     {
//         #ifdef MY_DEBUG
//         printf("\n dp_secene_data_handle type err");
//         #endif
//         return;
//     }

//     /* 提取涂鸦效果数据 */
//     uint8_t secene_data[ sizeof(dp_data_header_t) * MAX_CORLOR_N + 1];

//     len = string2hex(pIn + sizeof(dp_data_header_t), &secene_data, __SWP16(header->len));

//     #ifdef MY_DEBUG
//     printf("\n secene_data =");
//     printf_buf(secene_data, len);
//     #endif

//     /* 提取颜色数量 */
//     fc_effect.n = (len - 1) / sizeof(dp_secene_data_t);
//     #ifdef MY_DEBUG
//     printf("\n dp_secene_data_t =%d",sizeof(dp_secene_data_t));
//     #endif
//     /* 提取颜色 */
//     dp_secene_data_t *pdata;
//     pdata = (dp_secene_data_t *) (secene_data + 1); //第1个byte是情景号
//     m_hsv_to_rgb(   &fc_effect.c[i].r, &fc_effect.c[i].g, &fc_effect.c[i].b, \
//                     __SWP16(pdata->c.h_val), \
//                     __SWP16(pdata->c.s_val), \
//                     __SWP16(pdata->c.v_val));
//     /* 提取速度 */
//     fc_effect.s = pdata->chg_time;
//     fc_effect.change_type = pdata->change_type;

//     #ifdef MY_DEBUG
//     printf("\n rgb =");
//     printf_buf(fc_effect.c[i], 3);
//     printf("\n fc_effect.n = %d",fc_effect.n);

//     #endif
//     i++;
//     while(i < fc_effect.n)
//     {

//         pdata = (dp_secene_data_t *) (secene_data + 1 + sizeof(dp_secene_data_t) * i); //第1个byte是情景号
//         m_hsv_to_rgb(   &fc_effect.c[i].r, &fc_effect.c[i].g, &fc_effect.c[i].b, \
//                     __SWP16(pdata->c.h_val), \
//                     __SWP16(pdata->c.s_val), \
//                     __SWP16(pdata->c.v_val));

//         #ifdef MY_DEBUG
//         printf("\n rgb =");
//         printf_buf(fc_effect.c[i], 3);
//         #endif
//         i++;
//     }
// }

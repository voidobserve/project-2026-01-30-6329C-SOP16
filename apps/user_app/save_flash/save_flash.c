
#include "system/includes.h"
#include "syscfg_id.h"
#include "save_flash.h"

/*
    大致功能：
    需要写入flash时，调用：
    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);

    用户消息处理线程：
    save_user_data_enable() 使能延时写入flash的操作

    主循环：
    save_user_data_time_count_down() 倒计时
    save_user_data_handle() 延时写入flash的操作使能，并且倒计时到来，执行写入flash操作
*/
#define FLASH_CRC_DATA 0xC5 

static volatile u16 time_count_down = 0; // 存放当前的倒计时
static volatile u8 flag_is_enable_count_down = 0; // 标志位，是否使能了保存，进入倒计时
static volatile u8 flag_is_enable_to_save = 0; // 标志位，是否使能保存，

u8 my_ble_state = 1; // 默认开启BLE模块 /* 原本是433遥控器相关文件定义，现在移植到这里 */

// volatile save_flash_t save_flash_structure = {0};

volatile save_info_t save_info = {0};
 


void read_flash_device_status_init(void)
{
    int ret; 
    save_flash_t save_flash_structure = {0};

    // local_irq_disable(); // 禁用中断
    ret = syscfg_read(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_flash_structure), sizeof(save_flash_t));
    // local_irq_enable(); // 使能中断

    if (ret != sizeof(save_flash_t))
    {
        // 如果读取到的数据个数不一致
        // printf("read save info error \n");
        memset((u8 *)&save_flash_structure, 0, sizeof(save_flash_t));
    }

    os_time_dly(1);

    if (save_flash_structure.header != FLASH_CRC_DATA) // 第一次上电
    {
        fc_data_init();
        my_ble_state = 1; // 默认开启BLE模块
        // printf("is first power on\n");
        save_user_data_area3(); // 将初始化后的数据写回flash
    }
    else
    {
        // memcpy((u8 *)(&fc_effect), (u8 *)(&save_flash_structure.fc_save), sizeof(fc_effect_t));
        fc_effect = save_flash_structure.fc_save; // 结构体变量赋值
        my_ble_state = save_flash_structure.sa_ble_state;
        // RF433_CODE = save_flash3.sa_rf433_code;
        // printf("is not first power on\n");
    } 
}

// 把用户数据写到区域3
void save_user_data_area3(void)
{ 
    save_flash_t save_flash_structure = {0};
    int ret = 0;
    save_flash_structure.header = FLASH_CRC_DATA; // 表示数据有效
    save_flash_structure.sa_ble_state = my_ble_state;
    // save_data.sa_rf433_code = RF433_CODE;
    // memcpy((u8 *)(&save_flash_structure.fc_save), (u8 *)(&fc_effect), sizeof(fc_effect_t));
    // save_flash_structure.fc_save = fc_effect; // 结构体变量赋值
    memcpy((u8 *)(&save_flash_structure.fc_save), (u8 *)(&fc_effect), sizeof(fc_effect_t));

    os_time_dly(1);

    // local_irq_disable(); // 禁用中断
    ret = syscfg_write(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_flash_structure), sizeof(save_flash_t));
    // local_irq_enable(); // 使能中断
    if (ret != sizeof(save_flash_t))
    {
        // 如果实际写入的数据与配置的参数不一致
    }
 
    printf("save fc_data done \n"); 
}

// 从 flash 读出 save_info
void save_info_read(void)
{ 
    int ret = 0;

    // local_irq_disable(); // 禁用中断
    ret = syscfg_read(CFG_USER_SAVE_INFO_ID, (u8 *)(&save_info), sizeof(save_info_t));
    // local_irq_enable(); // 使能中断
    
    if (ret != sizeof(save_info_t))
    {
        // 如果读取到的数据个数不一致
        // printf("read save info error \n");
        memset((u8 *)&save_info, 0, sizeof(save_info_t));
    }

    if (save_info.is_data_valid != FLASH_CRC_DATA) // 第一次上电
    {
        printf("is first power on\n");

        save_info.is_data_valid = FLASH_CRC_DATA;
        lighting_animation_config_init(); // 初始化参数

        save_info_write(); // 将初始化后的数据写回flash
    }
    else
    {
        printf("is not first power on\n");
    }
}

// 写入 save_info 到flash
void save_info_write(void)
{
    // printf("%s %d\n", __func__, __LINE__);
    int ret = 0;
    save_info.is_data_valid = FLASH_CRC_DATA; // 表示数据有效

    os_time_dly(1);

    // local_irq_disable(); // 禁用中断
    ret = syscfg_write(CFG_USER_SAVE_INFO_ID, (u8 *)(&save_info), sizeof(save_info_t));
    // local_irq_enable(); // 使能中断
    // printf("ret %d \n", ret);
    if (ret != sizeof(save_info_t))
    {
        // 如果实际写入的数据与配置的参数不一致
    }
 
    printf("save user info done \n"); 
} 


/**
 * @brief 从 flash 读出用户数据，如果是第一次上电，会初始化数据
 * 
 */
void user_data_read(void)
{
    read_flash_device_status_init();
    save_info_read();
}

/**
 * @brief 将用户数据写入 flash
 * 
 */
void user_data_write(void)
{
    save_user_data_area3();
    save_info_write();
}
 
/**
 * @brief 写入flash倒计时
 *      10ms调用一次，不需要特别准确
 *
 *      如果 flag_is_enable_count_down == 1，表示使能倒计时
 *      如果 flag_is_enable_count_down == 0，表示未使能倒计时
 *
 *      计时结束，将 flag_is_enable_to_save 置一
 */
void save_user_data_time_count_down(void)
{
    if (0 == flag_is_enable_count_down)
    {
        return;
    }

    if (time_count_down > 0)
    {
        time_count_down--;
    }

    if (0 == time_count_down)
    {
        flag_is_enable_count_down = 0;
        flag_is_enable_to_save = 1;
        // save_user_data_area3();
    }
}
void save_user_data_enable(void)
{
    flag_is_enable_count_down = 0;
    time_count_down = DELAY_SAVE_FLASH_TIMES / 10; // DELAY_SAVE_FLASH_TIMES / 10 ms计时，实现 DELAY_SAVE_FLASH_TIMES ms延时
    flag_is_enable_count_down = 1;
}

void save_user_data_handle(void)
{
    if (flag_is_enable_to_save)
    {
        flag_is_enable_to_save = 0;
        user_data_write();
    }
}
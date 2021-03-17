/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: fake_nor.c
**
** 创   建   人: Pan yanqi (潘延麒)
**
** 文件创建日期: 2021 年 03 月 17 日
**
** 描        述: RAM模拟NorFlash驱动，相关数据定义
*********************************************************************************************************/
#include "nor_util.h"
#include "fake_nor.h"

BOOL get_sector_is_bad(INT sector_no){
    BOOL is_bad;
    API_ThreadLock();
    is_bad = sector_infos[sector_no].is_bad;
    API_ThreadUnlock();
    return is_bad;
}

VOID assign_sector_bad(INT sector_no){
    API_ThreadLock();
    sector_infos[sector_no].is_bad = TRUE;
    API_ThreadUnlock();
}

VOID generate_bad_sector(){
    UINT i;

    API_ThreadLock();
    printf("[generating bad sector...]\n");
    API_ThreadUnlock();
    
    while (TRUE)
    {
        for (i = 0; i < NOR_FLASH_NSECTOR; i++)
        { 
            if(((float)NOR_FLASH_MAX_ERASE_CNT * 0.7 - sector_infos[i].erase_cnt) <= 0      /* 擦了最大次数的70% */
                && sector_infos[i].is_bad == FALSE) {                                       /* 且该Sector不是坏块 */
                srand(time(LW_NULL));
                INT percent = rand() % 170 + 70;                                            /* 产生70 - 170的随机数 */
                INT threshold = 70 * NOR_FLASH_MAX_ERASE_CNT / sector_infos[i].erase_cnt;   /* 70 * 10 / 7 = 100 ~ 70 */
                if(percent >= threshold) {                                                  /* 70% ~ 100%的概率变为坏块 */
                    assign_sector_bad(i);
                }
            }
        }    
        sleep(1);                                                                           /* 每1s检测1次 */
    }
}

VOID nor_summary(){
    CHAR temp_buffer[44];
    INT clean_sector = 0, i = 0, bad_sector = 0;
    pretty_print("| [total sector]", "#512 |", DONT_CENTRAL);

    sprintf(temp_buffer, "%d(s) |", NOR_FLASH_MAX_ERASE_CNT);
    pretty_print("| [max erase count]", temp_buffer, DONT_CENTRAL);
    lib_memset(temp_buffer, 0, 44);

    

    for (i = 0; i < NOR_FLASH_NSECTOR; i++)
    {
        if(sector_infos[i].erase_cnt == 0){
            clean_sector++;
        }
    }
    sprintf(temp_buffer, "#%d |", clean_sector);
    pretty_print("| [clean sector]", temp_buffer, DONT_CENTRAL);
    lib_memset(temp_buffer, 0, 44);

    pretty_print("| [erase count]", "|", DONT_CENTRAL);
    for (i = 0; i < NOR_FLASH_NSECTOR; i++)
    {
        CHAR header_buffer[44];
        CHAR content_buffer[44];
        lib_memset(header_buffer, 0, sizeof(CHAR) * 44);
        lib_memset(content_buffer, 0, sizeof(CHAR) * 44);
        sprintf(header_buffer, "| [sector %d]", i);
        if(sector_infos[i].is_bad){
            bad_sector++;
            sprintf(content_buffer, "-bad  #%d erase(s) |", sector_infos[i].erase_cnt);
        }
        else
            sprintf(content_buffer, "-nice #%d erase(s) |", sector_infos[i].erase_cnt);
        pretty_print(header_buffer, content_buffer, DONT_CENTRAL);
    }

    sprintf(temp_buffer, "#%d |", bad_sector);
    pretty_print("| [bad sector(s)]", temp_buffer, DONT_CENTRAL);
    lib_memset(temp_buffer, 0, 44);

    show_divider(LW_NULL);
    return;
}

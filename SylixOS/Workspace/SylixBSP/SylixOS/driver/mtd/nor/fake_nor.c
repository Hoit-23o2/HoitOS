/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: fake_nor.c
**
** ��   ��   ��: Pan yanqi (������)
**
** �ļ���������: 2021 �� 03 �� 17 ��
**
** ��        ��: RAMģ��NorFlash������������ݶ���
*********************************************************************************************************/
#include "nor_util.h"
#include "fake_nor.h"
#include "nor.h"
/*********************************************************************************************************
** ��������: get_sector_is_bad
** ��������: �жϸ�Sector�Ƿ��ǻ��ģ�������FAKE��ʹ��
** �䡡��  : sector_no          sector���
** �䡡��  : �ǻ��飬����TRUE�����򷵻�FALSE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
BOOL get_sector_is_bad(INT sector_no){
    BOOL is_bad;
    API_ThreadLock();
    is_bad = sector_infos[sector_no].is_bad;
    API_ThreadUnlock();
    return is_bad;
}
/*********************************************************************************************************
** ��������: assign_sector_bad
** ��������: ��ֵ����
** �䡡��  : sector_no          sector���
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID assign_sector_bad(INT sector_no){
    API_ThreadLock();
    sector_infos[sector_no].is_bad = TRUE;
    API_ThreadUnlock();
}
/*********************************************************************************************************
** ��������: generate_bad_sector
** ��������: ���ɻ��飬������FAKE��ʹ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID generate_bad_sector(){
    if(!IS_FAKE_MODE())
        return;
    UINT i;

    API_ThreadLock();
    pretty_print("[generating bad sector...]", "", DO_CENTRAL);
    API_ThreadUnlock();
    
    while (TRUE)
    {
        for (i = 0; i < NOR_FLASH_NSECTOR; i++)
        { 
            if(((float)NOR_FLASH_MAX_ERASE_CNT * 0.7 - sector_infos[i].erase_cnt) <= 0      /* ������������70% */
                && sector_infos[i].is_bad == FALSE) {                                       /* �Ҹ�Sector���ǻ��� */
                srand(time(LW_NULL));
                INT percent = rand() % 170 + 70;                                            /* ����70 - 170������� */
                INT threshold = 70 * NOR_FLASH_MAX_ERASE_CNT / sector_infos[i].erase_cnt;   /* 70 * 10 / 7 = 100 ~ 70 */
                if(percent >= threshold) {                                                  /* 70% ~ 100%�ĸ��ʱ�Ϊ���� */
                    assign_sector_bad(i);
                }
            }
        }    
        sleep(1);                                                                           /* ÿ1s���1�� */
    }
}
/*********************************************************************************************************
** ��������: nor_summary
** ��������: ���ݳ�ʼ��ģʽ�����Ϣ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID nor_summary(){
    show_divider(LW_NULL);
    CHAR temp_buffer[TEMP_BUF_SZ];
    if(IS_FAKE_MODE()){
        INT clean_sector = 0, i = 0, bad_sector = 0;
        pretty_print("[total sector]", "#35", DONT_CENTRAL);

        sprintf(temp_buffer, "%d(s)", NOR_FLASH_MAX_ERASE_CNT);
        pretty_print("[max erase count]", temp_buffer, DONT_CENTRAL);
        lib_memset(temp_buffer, 0, TEMP_BUF_SZ);
        for (i = 0; i < NOR_FLASH_NSECTOR; i++)
        {
            if(sector_infos[i].erase_cnt == 0){
                clean_sector++;
            }
        }
        sprintf(temp_buffer, "#%d", clean_sector);
        pretty_print("[clean sector]", temp_buffer, DONT_CENTRAL);
        lib_memset(temp_buffer, 0, TEMP_BUF_SZ);

        pretty_print("[erase count]", "", DONT_CENTRAL);
        for (i = 0; i < NOR_FLASH_NSECTOR; i++)
        {
            CHAR header_buffer[TEMP_BUF_SZ];
            CHAR content_buffer[TEMP_BUF_SZ];
            lib_memset(header_buffer, 0, sizeof(CHAR) * TEMP_BUF_SZ);
            lib_memset(content_buffer, 0, sizeof(CHAR) * TEMP_BUF_SZ);
            sprintf(header_buffer, "[sector %d]", i);
            if(sector_infos[i].is_bad){
                bad_sector++;
                sprintf(content_buffer, "-bad  #%d erase(s)", sector_infos[i].erase_cnt);
            }
            else
                sprintf(content_buffer, "-nice #%d erase(s)", sector_infos[i].erase_cnt);
            pretty_print(header_buffer, content_buffer, DONT_CENTRAL);
        }

        sprintf(temp_buffer, "#%d", bad_sector);
        pretty_print("[bad sector(s)]", temp_buffer, DONT_CENTRAL);
        lib_memset(temp_buffer, 0, TEMP_BUF_SZ);
    }
    else {
        lib_memset(temp_buffer, 0, TEMP_BUF_SZ);
        INT i;
        for (i = 0; i < NOR_FLASH_NSECTOR; i++)
        {
            CHAR header_buffer[TEMP_BUF_SZ];
            CHAR content_buffer[TEMP_BUF_SZ];
            lib_memset(header_buffer, 0, TEMP_BUF_SZ);
            lib_memset(content_buffer, 0, TEMP_BUF_SZ);
            sprintf(header_buffer, "[sector %d]", i);
            sprintf(content_buffer, "size: %dKB", _G_am29LV160DB_sector_infos[i].sector_size);
        }
    }
    show_divider(LW_NULL);
    return;
}

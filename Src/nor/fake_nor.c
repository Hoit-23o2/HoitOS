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
** �ļ���������: 2021 �� 02 �� 24 ��
**
** ��        ��: RAMģ��NorFlash����
*********************************************************************************************************/

#include "fake_nor.h"
#include "fake_nor_cmd.h"

BOOL get_sector_is_bad(INT sectro_no){
    BOOL is_bad;
    API_ThreadLock();
    is_bad = sector_infos[sectro_no].is_bad;
    API_ThreadUnlock();
    return is_bad;
}

VOID assign_sector_bad(INT sectro_no){
    API_ThreadLock();
    sector_infos[sectro_no].is_bad = TRUE;
    API_ThreadUnlock();
}
/*********************************************************************************************************
** ��������: generate_bad_sector
** ��������: �Զ���⻵���̣߳���ĳ��sector�Ĳ�д������������д������70%����ô����70%�ĸ��ʱ�Ϊ���飬
            �ﵽ100%������100%�ļ��ʱ�Ϊ���顣
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID generate_bad_sector(){
    UINT i;

    API_ThreadLock();
    printf("[generating bad sector...]\n");
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
** ��������: nor_init
** ��������: ��ʼ��fake nor flash
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID nor_init(void){
    if((nor_flash_base_addr = (PCHAR)malloc(NOR_FLASH_SZ)) < 0){
        printf("[Low Memory] can't create nor flash\n");
        return;
    }
    lib_memset(nor_flash_base_addr, 0, NOR_FLASH_SZ);
    if((sector_infos = (sector_info_t *)malloc(NOR_FLASH_NSECTOR * sizeof(sector_info_t))) < 0){
        printf("[Low Memory] can't create sector summary\n");
        return;
    }
    lib_memset(sector_infos, 0, NOR_FLASH_NSECTOR * sizeof(sector_info_t));
    printf("base addr: %p; sector_infos: %p\n", nor_flash_base_addr, sector_infos);
    
    INT i;
    for (i = 0; i < NOR_FLASH_NSECTOR; i++)
    {
        sector_infos[i].erase_cnt = 0;
        sector_infos[i].is_bad = FALSE;
    }
    pretty_print("[nor init success]", "use 'fls -h' to get help");

    API_ThreadCreate("t_generate_bad_block",
                     (PTHREAD_START_ROUTINE)generate_bad_sector,
                     LW_NULL,
                     LW_NULL);                                          /*  Create generate bad block thread */     
    register_nor_cmd();
}
/*********************************************************************************************************
** ��������: reset_nor
** ��������: ����nor flash
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID reset_nor(){
    CHAR temp[40] = {'\0'};
    SHOW_DIVIDER();
    lib_memset(nor_flash_base_addr, 0, NOR_FLASH_SZ);
    sprintf(temp, "%dB", NOR_FLASH_SZ);
    pretty_print("[norflash reset]:", temp);
    
    lib_memset(temp, 0, 40);
    lib_memset(sector_infos, 0, NOR_FLASH_NSECTOR * sizeof(sector_info_t));
    sprintf(temp, "#%d", NOR_FLASH_NSECTOR);
    pretty_print("[reset sectors]:", temp);

    pretty_print("[reset success]", "");
    SHOW_DIVIDER();
}

/*********************************************************************************************************
** ��������: erase_nor
** ��������: ����ѡ�����nor flash��һ����
** �䡡��  : offset        ƫ�Ƶ�ַ
**           ops          ѡ�������ERASE_SECTOR|ERASE_BLOCK|ERASE_CHIP
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID erase_nor(UINT offset, ERASE_OPTIONS ops){
    PCHAR addr = GET_NOR_FLASH_ADDR(offset);
    INT i;
    if(addr < nor_flash_base_addr){
        printf("[ERASE: target address can't lower than base]\n");
        return;
    }
    if(addr > nor_flash_base_addr + NOR_FLASH_SZ){
        printf("[ERASE: target address can't out bound of flash]\n");
        return;
    }
    switch (ops)
    {
    case ERASE_CHIP:
        printf("[erasing the whole chip...]\n");
        lib_memset(nor_flash_base_addr, 0, sizeof(CHAR) * NOR_FLASH_SZ);        /* ��������ڴ� */
        for (i = 0; i < NOR_FLASH_NSECTOR; i++)                                 /* ������������ */
        {
            sector_infos[i].erase_cnt++;
        }
        printf("[erasing success]\n");
        break;

    case ERASE_BLOCK:{
        UINT block_no = GET_NOR_FLASH_BLK(offset);                              /* ��ȡoffset��Ӧ��block�� */
        UINT sector_no = GET_NOR_FLASH_SECTOR(offset);                          /* ����block��Ӧ��sector����λ�� */
        printf("[erasing block %d...]\n", block_no);    
        PCHAR block_addr = GET_NOR_FLASH_BLK_ADDR(block_no);                    /* ��ȡblock��Ӧ�ĵ�ַ */
        lib_memset(block_addr, 0, NOR_FLASH_BLKSZ);                             /* ����block�׵�ַ��Ӧ��block */
        for (i = 0; i < NOR_FLASH_SECTPBLK; i++)                                /* ������������ */
        {
            sector_infos[i + sector_no].erase_cnt++;
        }
        printf("[erasing success]\n");
        break;
    }

    case ERASE_SECTOR:{
        UINT sector_no = GET_NOR_FLASH_SECTOR(offset);
        printf("[erasing sector %d]\n", sector_no);
        PCHAR sector_addr = GET_NOR_FLASH_SECTOR_ADDR(sector_no);
        lib_memset(sector_addr, 0, NOR_FLASH_SECTORSZ);
        sector_infos[sector_no].erase_cnt++;
        printf("[erasing success]\n");
        break;
    }
    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: read_nor
** ��������: ����ѡ���offset����ȡ����
** �䡡��  : offset         �ײ�ƫ��
**           ops            ��ȡѡ�������READ_BYTE|READ_SECTOR
** �䡡��  : ��ȡ���ݵĽṹ��read_content_t
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
read_content_t read_nor(UINT offset, READ_OPTIONS ops){
    PCHAR addr = GET_NOR_FLASH_ADDR(offset);
    read_content_t result;
    if(addr < nor_flash_base_addr){
        printf("[READ: target address can't lower than base]\n");
        result.is_success = FALSE;
        return result;
    }
    if(addr > nor_flash_base_addr + NOR_FLASH_SZ){
        printf("[READ: target address can't out bound of flash]\n");
        result.is_success = FALSE;
        return result;
    }
    switch (ops)
    {
    case READ_BYTE:                                                     /* ��ȡ1���ֽ� */
        printf("READ BYTE\n");
        result.content = addr;
        result.size = 1;
        break;
    case READ_SECTOR:           
        printf("READ SECT\n");                                          /* ��ȡ1��Sector */
        UINT sector_no = GET_NOR_FLASH_SECTOR(offset);
        result.content = GET_NOR_FLASH_SECTOR_ADDR(sector_no);
        result.size = NOR_FLASH_SECTORSZ;
        break;
    default:
        break;
    }
    return result;
}
/*********************************************************************************************************
** ��������: write_nor
** ��������: ����ѡ����offset��д�볤��Ϊsize������
** �䡡��  : offset         �ײ�ƫ��
**           content        �����ײ�ָ��
             size           ���ݴ�С
** �䡡��  : NONE
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
VOID write_nor(UINT offset, PCHAR content, UINT size){
    PCHAR addr = GET_NOR_FLASH_ADDR(offset);
    INT i;
    if(addr < nor_flash_base_addr){
        printf("[WRITE: target address can't lower than base]\n");
        return;
    }
    if(addr + size > nor_flash_base_addr + NOR_FLASH_SZ){                             /* ����д���� */
        printf("[WRITE: low memory]\n");
        return;
    }
    INT start_sector_no = GET_NOR_FLASH_SECTOR(offset);
    INT end_sector_no = ((offset + size) % NOR_FLASH_SECTORSZ) == 0 ?                     /* �ж��ǲ������� */
                                    GET_NOR_FLASH_SECTOR(offset + size) - 1 : 
                                    GET_NOR_FLASH_SECTOR(offset + size)     ;
    INT nsectors = end_sector_no - start_sector_no + 1;                                   /* ��Ҫ������sector�� */
    PCHAR start_sector_addr = GET_NOR_FLASH_SECTOR_ADDR(start_sector_no);                 /* �����ʼsector��ַ */
    PCHAR buffer;
    if((buffer = (PCHAR)malloc(nsectors * NOR_FLASH_SECTORSZ)) < 0){              /* ������ */
        printf("[Buffer Init Failed]\n");
        return;
    }                      
    INT start_offset = addr - start_sector_addr;
    for (i = 0; i < nsectors; i++)                                                    /* �ȶ�ȡ */
    {
        PCHAR sector_addr = GET_NOR_FLASH_SECTOR_ADDR(start_sector_no + i);
        UINT sector_offset = sector_addr - nor_flash_base_addr;
        read_content_t content = read_nor(sector_offset, READ_SECTOR);
        lib_memcpy(buffer + i * NOR_FLASH_SECTORSZ, content.content, content.size);
    }
    lib_memcpy(buffer + start_offset, content, size);
    for (i = 0; i < nsectors; i++)                                                    /* �ٲ��� */
    {
        PCHAR sector_addr = GET_NOR_FLASH_SECTOR_ADDR(start_sector_no + i);
        erase_nor(sector_addr - nor_flash_base_addr, ERASE_SECTOR);
    }
    for (i = 0; i < nsectors; i++)                                                    /* ���д�� */
    {
        UINT sector_offset = i * NOR_FLASH_SECTORSZ;
        PCHAR p = buffer + sector_offset;
        if(get_sector_is_bad(i)){                                                     /* ����޸� */
            printf("[sector #%d is bad, there may be some error(s), remember to check]\n", start_sector_no + i);
            PCHAR pe = p + NOR_FLASH_SECTORSZ;
            for (; p < pe; p++)
            {
                INT possibily = rand() % 100 + 1;
                INT random_change = rand() % 127;                                      /* 0 ~ 127 ascii */
                if(possibily >= 50){                                                   /* 50%�ļ���д�� */
                    *p += random_change;
                }
            } 
        }
        lib_memcpy(start_sector_addr + sector_offset, p, NOR_FLASH_SECTORSZ);
    }
    free(buffer);
    printf("[write %d bytes success]\n", size);
}









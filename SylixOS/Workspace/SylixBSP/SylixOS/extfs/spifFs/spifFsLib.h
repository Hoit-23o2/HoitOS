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
** ��   ��   ��: spifFsLib.h
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ����
*********************************************************************************************************/
#ifndef SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_
#define SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_

#define SPIFFS_CONFIG_MAGIC             (0x5201314)

#define SPIFFS_CFG_LOGIC_PAGE_SZ(pfs)     ((pfs)->cfg.uiLogicPageSize)
#define SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs)    ((pfs)->cfg.uiLogicBlkSize)
#define SPIFFS_CFG_PHYS_SZ(pfs)         ((pfs)->cfg.uiPhysSize)
#define SPIFFS_CFG_PHYS_ERASE_SZ(pfs)   ((pfs)->cfg.uiPhysEraseBlkSize)
#define SPIFFS_CFG_PHYS_ADDR(pfs)       ((pfs)->cfg.uiPhysAddr)

#endif /* SYLIXOS_EXTFS_SPIFFS_SPIFFSLIB_H_ */

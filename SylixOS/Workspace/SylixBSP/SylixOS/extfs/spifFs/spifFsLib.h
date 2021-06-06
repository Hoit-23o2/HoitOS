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
** 文   件   名: spifFsLib.h
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统核心
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

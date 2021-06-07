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
** 文   件   名: spifFsGlue.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 06日
**
** 描        述: Spiffs文件系统胶水层，即上层实现
*********************************************************************************************************/
#include "spifFsType.h"
#include "spifFsLib.h"
/*********************************************************************************************************
** 函数名称: __spiffs_mount
** 功能描述: 挂载文件
** 输　入  : pfs          文件头
**          pConfig         配置文件
**          pucWorkBuffer   分配WorkBuffer
**          puiFdSpace      分配的文件描述符区域
**          uiFdSpaceSize   分配的文件描述符区域大小
**          pCache          分配的缓存空间
**          uiCacheSize     分配的Cached大小
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *puiFdSpace, UINT32 uiFdSpaceSize,
                     VOID *pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc){
    //TODO:什么意思？
    PVOID           pUserData;      
    UINT8           uiAddrLSB;                                  /* 用于指针对齐 */
    pUserData     = LW_NULL;
    pfs->hVolLock = API_SemaphoreMCreate("spiffs_volume_lock", LW_PRIO_DEF_CEILING,
                                         LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                         LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);              /* 创建文件头锁 */
    if (!pfs->hVolLock) {                                       /*  无法创建卷锁                */
        __SHEAP_FREE(pfs);
        return  (PX_ERROR);
    }

    lib_memset(pfs, 0, sizeof(SPIFFS_VOLUME));
    lib_memset(puiFdSpace, 0, uiFdSpaceSize);
    lib_memcpy(&pfs->cfg, pConfig, sizeof(SPIFFS_CONFIG));

    pfs->pUserData  = pUserData;
    pfs->uiBlkCount = SPIFFS_CFG_PHYS_SZ(pfs) / SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);
    pfs->pucWorkBuffer = pucWorkBuffer;
    pfs->pucLookupWorkBuffer = pucWorkBuffer + SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);

    /* 对齐puiFdSpace */
}


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
#include "spifFsGlue.h"
#include "spifFsCache.h"
#include "spifFsLib.h"
#include "../driver/mtd/nor/nor.h"

#define __spiffsAlign8Byte(pMem, uiMemSZ) \
do {\
    UINT8           uiAddrLSB;      /* 用于指针对齐 */\
    uiAddrLSB = ((UINT8)(intptr_t)pMem) & (sizeof(PVOID) - 1);\
    if(uiAddrLSB){\
        pMem += (sizeof(PVOID) - uiAddrLSB);\
        uiMemSZ -= (sizeof(PVOID) - uiAddrLSB);\
    }\
}while(0)
/*********************************************************************************************************
** 函数名称: __spiffs_format
** 功能描述: 格式化Norflash介质，第一次挂载时需要
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 __spiffs_format(PSPIFFS_VOLUME pfs){
    INT32 iRes;
    SPIFFS_BLOCK_IX blkIX = 0;
    if(pfs->uiMountedFlag != 0){
        pfs->uiErrorCode = SPIFFS_ERR_MOUNTED;
        return -1;
    }
    //LOCK
    while (blkIX < pfs->uiBlkCount)
    {
        pfs->uiMaxEraseCount = 0;
        iRes = spiffsEraseBlk(pfs, blkIX);
        if(iRes != SPIFFS_OK){
            iRes = SPIFFS_ERR_ERASE_FAIL;
        }
        pfs->uiErrorCode = iRes;
        blkIX++;
    }
    //UNLOCK
    return 0;
}
/*********************************************************************************************************
** 函数名称: __spiffs_probe_fs
** 功能描述: 探测Spiffs文件系统
** 输　入  : pConfig        配置文件
** 输　出  : 文件系统大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 __spiffs_probe_fs(PSPIFFS_CONFIG pConfig){
    SPIFFS_API_DBG("%s\n", __func__);
    INT32           iRes = SPIFFS_OK;
    UINT32          uiAddr;
    SPIFFS_VOLUME   dummyFs;
    SPIFFS_OBJ_ID   objIdMagic[3];
    SPIFFS_OBJ_ID   blkIXCount[3];  /* 数量递减，第一个代表All Blk，第二个代表All Blk - 1……  */
    SPIFFS_BLOCK_IX blkIX;

    lib_memcpy(&dummyFs, pConfig, sizeof(SPIFFS_CONFIG));
    dummyFs.uiBlkCount = 0;

    for (blkIX = 0; blkIX < 3; blkIX++)
    {
        uiAddr = SPIFFS_MAGIC_PADDR(&dummyFs, blkIX);
        iRes = read_nor(uiAddr, (PUCHAR)&objIdMagic[blkIX], sizeof(SPIFFS_OBJ_ID));
        blkIXCount[blkIX] = objIdMagic[blkIX] ^ SPIFFS_MAGIC(&dummyFs, 0);
        SPIFFS_CHECK_RES(iRes);
    }
    /* BlkCount - blkIX */
    if (blkIXCount[0] < 3) 
        return SPIFFS_ERR_PROBE_TOO_FEW_BLOCKS;
    // check that the order is correct, take aborted erases in calculation
    // first block aborted erase
    //TODO: objIdMagic[0] == (SPIFFS_OBJ_ID)(-1)为何能够说明aborted erase
    if (objIdMagic[0] == (SPIFFS_OBJ_ID)(-1) && blkIXCount[1] - blkIXCount[2] == 1) {
        return (blkIXCount[1]+1) * pConfig->uiLogicBlkSize;
    }
    // second block aborted erase
    if (objIdMagic[1] == (SPIFFS_OBJ_ID)(-1) && blkIXCount[0] - blkIXCount[2] == 2) {
        return blkIXCount[0] * pConfig->uiLogicBlkSize;
    }
    // third block aborted erase
    if (objIdMagic[2] == (SPIFFS_OBJ_ID)(-1) && blkIXCount[0] - blkIXCount[1] == 1) {
        return blkIXCount[0] * pConfig->uiLogicBlkSize;
    }
    // no block has aborted erase
    if (blkIXCount[0] - blkIXCount[1] == 1 && blkIXCount[1] - blkIXCount[2] == 1) {
        return blkIXCount[0] * pConfig->uiLogicBlkSize;
    }

    return SPIFFS_ERR_PROBE_NOT_A_FS;
}
/*********************************************************************************************************
** 函数名称: __spiffs_mount
** 功能描述: 挂载文件
** 输　入  : pfs          文件头
**          pConfig         配置文件
**          pucWorkBuffer   分配WorkBuffer
**          pucFdSpace      分配的文件描述符区域
**          uiFdSpaceSize   分配的文件描述符区域大小
**          pCache          分配的缓存空间
**          uiCacheSize     分配的Cached大小
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *pucFdSpace, UINT32 uiFdSpaceSize,
                     PUCHAR pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc){
    //TODO:什么意思？
    PVOID           pUserData = LW_NULL;      
    INT32           iRes = SPIFFS_OK;
    pfs->hVolLock = API_SemaphoreMCreate("spiffs_volume_lock", LW_PRIO_DEF_CEILING,
                                         LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                         LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);              /* 创建文件头锁 */
    if (!pfs->hVolLock) {                                       /*  无法创建卷锁                */
        __SHEAP_FREE(pfs);
        return  (PX_ERROR);
    }

    lib_memset(pfs, 0, sizeof(SPIFFS_VOLUME));
    lib_memset(pucFdSpace, 0, uiFdSpaceSize);
    lib_memcpy(&pfs->cfg, pConfig, sizeof(SPIFFS_CONFIG));

    pfs->pUserData              = pUserData;
    pfs->uiBlkCount             = SPIFFS_CFG_PHYS_SZ(pfs) / SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);
    pfs->pucWorkBuffer          = pucWorkBuffer;
    pfs->pucLookupWorkBuffer    = pucWorkBuffer + SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);

    /* 对齐puiFdSpace，8字节对齐 */
    __spiffsAlign8Byte(pucFdSpace, uiFdSpaceSize);
    pfs->pucFdSpace = pucFdSpace;
    pfs->uiFdCount  = uiFdSpaceSize / sizeof(SPIFFS_FD);
    
    /* 对齐Cache，8字节对齐 */
    __spiffsAlign8Byte(pCache, uiCacheSize);
    if(uiCacheSize & (sizeof(PVOID) - 1)) {
        uiCacheSize -= (uiCacheSize & (sizeof(PVOID) - 1));
    }   
    pfs->pCache         = pCache;
    pfs->uiCacheSize    = uiCacheSize;

    iRes = spiffsCacheInit(pfs);

    pfs->uiConfigMagic = SPIFFS_CONFIG_MAGIC;
    //TODO: 扫描介质
    iRes = spiffsObjLookUpScan(pfs);

    SPIFFS_DBG("page index byte len:         "_SPIPRIi"\n", (UINT32)SPIFFS_CFG_LOG_PAGE_SZ(pfs));
    SPIFFS_DBG("object lookup pages:         "_SPIPRIi"\n", (UINT32)SPIFFS_OBJ_LOOKUP_PAGES(pfs));
    SPIFFS_DBG("page pages per block:        "_SPIPRIi"\n", (UINT32)SPIFFS_PAGES_PER_BLOCK(pfs));
    SPIFFS_DBG("page header length:          "_SPIPRIi"\n", (UINT32)sizeof(SPIFFS_PAGE_HEADER));
    /* 一个Index Header的Index Page所能索引的最大页面数 */
    SPIFFS_DBG("object header index entries: "_SPIPRIi"\n", (UINT32)SPIFFS_OBJ_HDR_IX_LEN(pfs));
    /* 一个非Index Header的Index Page所能索引的最大页面数 */
    SPIFFS_DBG("object index entries:        "_SPIPRIi"\n", (UINT32)SPIFFS_OBJ_IX_LEN(pfs));
    /* 配置的最大文件描述符 */
    SPIFFS_DBG("available file descriptors:  "_SPIPRIi"\n", (UINT32)pfs->uiFdCount);
    SPIFFS_DBG("free blocks:                 "_SPIPRIi"\n", (UINT32)pfs->uiFreeBlks);

    pfs->checkCallbackFunc = checkCallbackFunc;
    pfs->uiMountedFlag = 1;
    
    return SPIFFS_OK;
}
/*********************************************************************************************************
** 函数名称: __spiffs_unmount
** 功能描述: 卸载文件系统
** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID __spiffs_unmount(PSPIFFS_VOLUME pfs){
    SPIFFS_API_DBG("%s\n", __func__);
    UINT32 i;
    PSPIFFS_FD pFds = (PSPIFFS_FD)pfs->pucFdSpace;
    PSPIFFS_FD pCurFd;
    if(pfs->uiMountedFlag == 0){
        return;
    }
    for (i = 0; i < pfs->uiFdCount; i++)
    {
        pCurFd = &pFds[i];
        if(pCurFd->fileN != 0) {
            //TODO: 还未揭开面纱
        }
    }
    pfs->uiMountedFlag = 0;
    return;
}
/*********************************************************************************************************
** 函数名称: __spiffs_create
** 功能描述: 创建一个新文件
** 输　入  : pfs          文件头
**           pcPath        文件路径
**           mode          文件模式
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 __spiffs_create(PSPIFFS_VOLUME pfs, const PCHAR pcPath, SPIFFS_MODE mode) {
    SPIFFS_API_DBG("%s '%s'\n", __func__, pcPath);
    //TODO: mode为何没有
    (VOID)          mode; 
    SPIFFS_OBJ_ID   objId;
    INT32           iRes;

    SPIFFS_API_CHECK_MOUNT(pfs);
    if (lib_strlen(pcPath) > SPIFFS_OBJ_NAME_LEN - 1) {
        SPIFFS_API_CHECK_RES(pfs, SPIFFS_ERR_NAME_TOO_LONG);
    }
    iRes = spiffsObjLookUpFindFreeObjId(pfs, &objId, pcPath);
    SPIFFS_CHECK_RES(iRes);
    iRes = spiffsObjectCreate();
    return 0;
}


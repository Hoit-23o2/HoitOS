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
** ��   ��   ��: spifFsGlue.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 06��
**
** ��        ��: Spiffs�ļ�ϵͳ��ˮ�㣬���ϲ�ʵ��
*********************************************************************************************************/
#include "spifFsGlue.h"
#include "spifFsCache.h"
#include "spifFsLib.h"
#include "../driver/mtd/nor/nor.h"

#define __spiffsAlign8Byte(pMem, uiMemSZ) \
do {\
    UINT8           uiAddrLSB;      /* ����ָ����� */\
    uiAddrLSB = ((UINT8)(intptr_t)pMem) & (sizeof(PVOID) - 1);\
    if(uiAddrLSB){\
        pMem += (sizeof(PVOID) - uiAddrLSB);\
        uiMemSZ -= (sizeof(PVOID) - uiAddrLSB);\
    }\
}while(0)
/*********************************************************************************************************
** ��������: __spiffs_format
** ��������: ��ʽ��Norflash���ʣ���һ�ι���ʱ��Ҫ
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __spiffs_probe_fs
** ��������: ̽��Spiffs�ļ�ϵͳ
** �䡡��  : pConfig        �����ļ�
** �䡡��  : �ļ�ϵͳ��С
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_probe_fs(PSPIFFS_CONFIG pConfig){
    SPIFFS_API_DBG("%s\n", __func__);
    INT32           iRes = SPIFFS_OK;
    UINT32          uiAddr;
    SPIFFS_VOLUME   dummyFs;
    SPIFFS_OBJ_ID   objIdMagic[3];
    SPIFFS_OBJ_ID   blkIXCount[3];  /* �����ݼ�����һ������All Blk���ڶ�������All Blk - 1����  */
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
    //TODO: objIdMagic[0] == (SPIFFS_OBJ_ID)(-1)Ϊ���ܹ�˵��aborted erase
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
** ��������: __spiffs_mount
** ��������: �����ļ�
** �䡡��  : pfs          �ļ�ͷ
**          pConfig         �����ļ�
**          pucWorkBuffer   ����WorkBuffer
**          pucFdSpace      ������ļ�����������
**          uiFdSpaceSize   ������ļ������������С
**          pCache          ����Ļ���ռ�
**          uiCacheSize     �����Cached��С
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *pucFdSpace, UINT32 uiFdSpaceSize,
                     PUCHAR pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc){
    //TODO:ʲô��˼��
    PVOID           pUserData = LW_NULL;      
    INT32           iRes = SPIFFS_OK;
    pfs->hVolLock = API_SemaphoreMCreate("spiffs_volume_lock", LW_PRIO_DEF_CEILING,
                                         LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
                                         LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                         LW_NULL);              /* �����ļ�ͷ�� */
    if (!pfs->hVolLock) {                                       /*  �޷���������                */
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

    /* ����puiFdSpace��8�ֽڶ��� */
    __spiffsAlign8Byte(pucFdSpace, uiFdSpaceSize);
    pfs->pucFdSpace = pucFdSpace;
    pfs->uiFdCount  = uiFdSpaceSize / sizeof(SPIFFS_FD);
    
    /* ����Cache��8�ֽڶ��� */
    __spiffsAlign8Byte(pCache, uiCacheSize);
    if(uiCacheSize & (sizeof(PVOID) - 1)) {
        uiCacheSize -= (uiCacheSize & (sizeof(PVOID) - 1));
    }   
    pfs->pCache         = pCache;
    pfs->uiCacheSize    = uiCacheSize;

    iRes = spiffsCacheInit(pfs);

    pfs->uiConfigMagic = SPIFFS_CONFIG_MAGIC;
    //TODO: ɨ�����
    iRes = spiffsObjLookUpScan(pfs);

    SPIFFS_DBG("page index byte len:         "_SPIPRIi"\n", (UINT32)SPIFFS_CFG_LOG_PAGE_SZ(pfs));
    SPIFFS_DBG("object lookup pages:         "_SPIPRIi"\n", (UINT32)SPIFFS_OBJ_LOOKUP_PAGES(pfs));
    SPIFFS_DBG("page pages per block:        "_SPIPRIi"\n", (UINT32)SPIFFS_PAGES_PER_BLOCK(pfs));
    SPIFFS_DBG("page header length:          "_SPIPRIi"\n", (UINT32)sizeof(SPIFFS_PAGE_HEADER));
    /* һ��Index Header��Index Page�������������ҳ���� */
    SPIFFS_DBG("object header index entries: "_SPIPRIi"\n", (UINT32)SPIFFS_OBJ_HDR_IX_LEN(pfs));
    /* һ����Index Header��Index Page�������������ҳ���� */
    SPIFFS_DBG("object index entries:        "_SPIPRIi"\n", (UINT32)SPIFFS_OBJ_IX_LEN(pfs));
    /* ���õ�����ļ������� */
    SPIFFS_DBG("available file descriptors:  "_SPIPRIi"\n", (UINT32)pfs->uiFdCount);
    SPIFFS_DBG("free blocks:                 "_SPIPRIi"\n", (UINT32)pfs->uiFreeBlks);

    pfs->checkCallbackFunc = checkCallbackFunc;
    pfs->uiMountedFlag = 1;
    
    return SPIFFS_OK;
}
/*********************************************************************************************************
** ��������: __spiffs_unmount
** ��������: ж���ļ�ϵͳ
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
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
            //TODO: ��δ�ҿ���ɴ
        }
    }
    pfs->uiMountedFlag = 0;
    return;
}
/*********************************************************************************************************
** ��������: __spiffs_create
** ��������: ����һ�����ļ�
** �䡡��  : pfs          �ļ�ͷ
**           pcPath        �ļ�·��
**           mode          �ļ�ģʽ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_create(PSPIFFS_VOLUME pfs, const PCHAR pcPath, SPIFFS_MODE mode) {
    SPIFFS_API_DBG("%s '%s'\n", __func__, pcPath);
    //TODO: modeΪ��û��
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


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
#include "spifFsFDLib.h"
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
    SPIFFS_API_DBG("%stat\n", __func__);
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
    SPIFFS_API_DBG("%stat\n", __func__);
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
    
    SPIFFS_API_DBG("%stat '%stat'\n", __func__, pcPath);
    //TODO: 1. modeΪ��û�� 2. Lock/Unlock
    (VOID)          mode; 
    SPIFFS_OBJ_ID   objId;
    INT32           iRes;

    SPIFFS_API_CHECK_MOUNT(pfs);
    if (lib_strlen(pcPath) > SPIFFS_OBJ_NAME_LEN - 1) {
        SPIFFS_API_CHECK_RES(pfs, SPIFFS_ERR_NAME_TOO_LONG);
    }
    
    iRes = spiffsObjLookUpFindFreeObjId(pfs, &objId, pcPath);
    SPIFFS_CHECK_RES(iRes);
    iRes = spiffsObjectCreate(pfs,objId, pcPath, SPIFFS_TYPE_FILE, LW_NULL);
    SPIFFS_CHECK_RES(iRes);

    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffs_open
** ��������: ��/����һ�����ļ�
** �䡡��  : pfs          �ļ�ͷ
**           pcPath        �ļ�·��
**           mode          �ļ�ģʽ: SPIFFS_O_APPEND, SPIFFS_O_TRUNC, SPIFFS_O_CREAT, SPIFFS_O_RDONLY,
**                                  SPIFFS_O_WRONLY, SPIFFS_O_RDWR, SPIFFS_O_DIRECT, SPIFFS_O_EXCL, Ignored
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
SPIFFS_FILE __spiffs_open(PSPIFFS_VOLUME pfs, const PCHAR pcPath, SPIFFS_FLAGS flags, SPIFFS_MODE mode){
    (VOID)mode;
    PSPIFFS_FD      pFd;
    SPIFFS_PAGE_IX  pageIX;
    INT32           iRes;
    SPIFFS_OBJ_ID   objId;

    SPIFFS_API_CHECK_MOUNT(pfs);
    SPIFFS_API_DBG("%stat '%stat' "_SPIPRIfl "\n", __func__, pcPath, flags);
    if (strlen(pcPath) > SPIFFS_OBJ_NAME_LEN - 1) {
        return SPIFFS_ERR_NAME_TOO_LONG;
    }
    iRes = spiffsFdFindNew(pfs, &pFd, pcPath);                              /* ��ȡһ��pFd */
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_CHECK_RES(iRes);

    iRes = spiffsObjectFindObjectIndexHeaderByName(pfs, pcPath, &pageIX);   /* ����Name�ҵ�IndexHeader */
    if ((flags & SPIFFS_O_CREAT) == 0) {                                    /* ����ļ��Ϸ��� */
        if (iRes < SPIFFS_OK) {
            spiffsFdReturn(pfs, pFd->fileN);
        }
        SPIFFS_CHECK_RES(iRes);
    }

    if (iRes == SPIFFS_OK &&                                                /* ֻ��SPIFFS_O_CREAT �� SPIFFS_O_EXCL�������ˣ� */
        (flags & (SPIFFS_O_CREAT | SPIFFS_O_EXCL)) == (SPIFFS_O_CREAT | SPIFFS_O_EXCL)) {
        // creat and excl and file exists - fail
        iRes = SPIFFS_ERR_FILE_EXISTS;
        spiffsFdReturn(pfs, pFd->fileN);
        SPIFFS_CHECK_RES(iRes);
    }

    if ((flags & SPIFFS_O_CREAT) && iRes == SPIFFS_ERR_NOT_FOUND) {         /* �����ļ� */
        // no need to enter conflicting name here, already looked for it above
        iRes = spiffsObjLookUpFindFreeObjId(pfs, &objId, LW_NULL);          /* �ҵ�һ���յ� Obj Id*/
        if (iRes < SPIFFS_OK) {
            spiffsFdReturn(pfs, pFd->fileN);
        }
        SPIFFS_CHECK_RES(iRes);
        iRes = spiffsObjectCreate(pfs, objId, pcPath, SPIFFS_TYPE_FILE, &pageIX);   /* ����һ��OBJECT */
        if (iRes < SPIFFS_OK) {
            spiffsFdReturn(pfs, pFd->fileN);
        }
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);
        flags &= ~SPIFFS_O_TRUNC;
    } 
    else {
        if (iRes < SPIFFS_OK) {
            spiffsFdReturn(pfs, pFd->fileN);
        }
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);
    }
    iRes = spiffsObjectOpenByPage(pfs, pageIX, pFd, flags, mode);
    if (iRes < SPIFFS_OK) {
        spiffsFdReturn(pfs, pFd->fileN);
    }
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_CHECK_RES(iRes);
    
    if (flags & SPIFFS_O_TRUNC) {
        iRes = spiffsObjectTruncate(pFd, 0, 0);
        if (iRes < SPIFFS_OK) {
            spiffsFdReturn(pfs, pFd->fileN);
        }
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);
    }

    pFd->uiFdOffset = 0;

    //SPIFFS_UNLOCK(pfs);

    return (SPIFFS_FILE)pFd->fileN; //SPIFFS_FH_OFFS(pfs, pFd->fileN);
}
/*********************************************************************************************************
** ��������: __spiffs_open_by_dirent
** ��������: ��һ����Ŀ¼
** �䡡��  : pfs          �ļ�ͷ
**           pcPath        �ļ�·��
**           mode          �ļ�ģʽ: SPIFFS_O_APPEND, SPIFFS_O_TRUNC, SPIFFS_O_CREAT, SPIFFS_O_RDONLY,
**                                  SPIFFS_O_WRONLY, SPIFFS_O_RDWR, SPIFFS_O_DIRECT, SPIFFS_O_EXCL, Ignored
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
SPIFFS_FILE __spiffs_open_by_dirent(PSPIFFS_VOLUME pfs, PSPIFFS_DIRENT pDirent, 
                                    SPIFFS_FLAGS flags, SPIFFS_MODE mode){
    PSPIFFS_FD pFd;
    INT32 iRes;
    SPIFFS_API_DBG("%stat '%stat':"_SPIPRIid " "_SPIPRIfl "\n", 
                   __func__, pDirent->ucName, pDirent->objID, flags);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    //SPIFFS_LOCK(pfs);


    iRes = spiffsFdFindNew(pfs, &pFd, 0);
    
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);

    iRes = spiffsObjectOpenByPage(pfs, pDirent->pageIX, pFd, flags, mode);
    if (iRes < SPIFFS_OK) {
        spiffsFdReturn(pfs, pFd->fileN);
    }

    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    
    if (flags & SPIFFS_O_TRUNC) {
        iRes = spiffsObjectTruncate(pFd, 0, LW_FALSE);
        if (iRes < SPIFFS_OK) {
            spiffsFdReturn(pfs, pFd->fileN);
        }
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    }
    pFd->uiFdOffset = 0;
    //SPIFFS_UNLOCK(pfs);
    return pFd->fileN;
}
/*********************************************************************************************************
** ��������: __spiffs_read
** ��������: ���ļ�
** �䡡��  : pfs          �ļ�ͷ
**           pcPath        �ļ�·��
**           mode          �ļ�ģʽ: SPIFFS_O_APPEND, SPIFFS_O_TRUNC, SPIFFS_O_CREAT, SPIFFS_O_RDONLY,
**                                  SPIFFS_O_WRONLY, SPIFFS_O_RDWR, SPIFFS_O_DIRECT, SPIFFS_O_EXCL, Ignored
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_read(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PVOID pContent, INT32 iLen){
    SPIFFS_API_DBG("%stat "_SPIPRIfd " "_SPIPRIi "\n", __func__, fileHandler, iLen);
    INT32 iRes = spiffsFileRead(pfs, fileHandler, pContent, iLen);
    if (iRes == SPIFFS_ERR_END_OF_OBJECT) {
        iRes = 0;
    }
    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffs_lseek
** ��������: �ƶ��ļ�ָ��
** �䡡��  : pfs          �ļ�ͷ
**           pcPath        �ļ�·��
**           mode          �ļ�ģʽ: SPIFFS_O_APPEND, SPIFFS_O_TRUNC, SPIFFS_O_CREAT, SPIFFS_O_RDONLY,
**                                  SPIFFS_O_WRONLY, SPIFFS_O_RDWR, SPIFFS_O_DIRECT, SPIFFS_O_EXCL, Ignored
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_lseek(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, UINT32 uiOffset, INT iSeekFlag){
    PSPIFFS_FD pFd;
    INT32      iRes;
    
    SPIFFS_API_DBG("%stat "_SPIPRIfd " "_SPIPRIi " %stat\n", 
                    __func__, fileHandler, uiOffset, 
                    (const char* []){"SET","CUR","END","???"}[MIN(iSeekFlag, 3)]);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    //SPIFFS_LOCK(pfs);

    iRes = spiffsFdGet(pfs, fileHandler, &pFd);
    
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_CHECK_RES(iRes);

    // #if SPIFFS_CACHE_WR
    // spiffs_fflush_cache(pfs, fileHandler);
    // #endif
    spiffsCacheFflush(pfs, fileHandler);            /* �����file��ص�Cache������ */

    INT32 iFileSize = pFd->uiSize == SPIFFS_UNDEFINED_LEN ? 0 : pFd->uiSize;

    switch (iSeekFlag) {
    case SPIFFS_SEEK_CUR:           /* �ӵ�ǰλ�ÿ�ʼ */
        uiOffset = pFd->uiFdOffset + uiOffset;
        break;
    case SPIFFS_SEEK_END:           /* ���ļ�ĩβ��ʼ */
        uiOffset = iFileSize + uiOffset;
        break;
    }
    if (uiOffset < 0) {
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, SPIFFS_ERR_SEEK_BOUNDS);
        iRes = SPIFFS_ERR_SEEK_BOUNDS; 
        SPIFFS_CHECK_RES(iRes);
    }
    if (uiOffset > iFileSize) {     /* Offset ���� �ļ���С�Ͳ����� */
        pFd->uiFdOffset = iFileSize;
        iRes = SPIFFS_ERR_END_OF_OBJECT;
    }
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_CHECK_RES(iRes);

    SPIFFS_SPAN_IX spanIXObjData = (uiOffset > 0 ? (uiOffset - 1) : 0) / SPIFFS_DATA_PAGE_SIZE(pfs);    /* ����Data Page��Span IX */
    SPIFFS_SPAN_IX spanIXObjIX = SPIFFS_OBJ_IX_ENTRY_SPAN_IX(pfs, spanIXObjData);                       /* �����ӦIndex Page��Span IX */
    
    if (pFd->spanIXObjIXCursor != spanIXObjIX) {    /* �����ļ��� ObjIX ��ָ��spanIX */
        SPIFFS_PAGE_IX pageIX;
        iRes = spiffsObjLookUpFindIdAndSpan(pfs, pFd->objId | SPIFFS_OBJ_ID_IX_FLAG, spanIXObjIX, 
                                            0, &pageIX);
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_CHECK_RES(iRes);    
        pFd->spanIXObjIXCursor = spanIXObjIX;
        pFd->pageIXObjIXCursor = pageIX;
    }
    /* �����ļ��ڲ�ָ�� */
    pFd->uiFdOffset = uiOffset;

    //SPIFFS_UNLOCK(pfs);

    return uiOffset;
}
/*********************************************************************************************************
** ��������: __spiffs_remove
** ��������: �Ƴ�һ���ļ����൱�ڰ���ȫ���ض�
** �䡡��  : pfs          �ļ�ͷ
**           pcPath        �ļ�·��
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_remove(PSPIFFS_VOLUME pfs, const PCHAR pcPath) {
    PSPIFFS_FD pFd;
    SPIFFS_PAGE_IX pageIX;
    INT32 iRes;

    SPIFFS_API_DBG("%stat '%stat'\n", __func__, pcPath);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    if (strlen(pcPath) > SPIFFS_OBJ_NAME_LEN - 1) {
        SPIFFS_API_CHECK_RES(pfs, SPIFFS_ERR_NAME_TOO_LONG);
    }
    //SPIFFS_LOCK(pfs);

    iRes = spiffsFdFindNew(pfs, &pFd, LW_NULL);             /* ��ȡһ���µ�pFd */
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_CHECK_RES(iRes);

    iRes = spiffsObjectFindObjectIndexHeaderByName(pfs, (PUCHAR)pcPath, &pageIX);    /* �ҵ�pcPath��Ӧ��spanIX = 0��Indexҳ���Ӧ��pageIX */
    if (iRes != SPIFFS_OK) {
        spiffsFdReturn(pfs, pFd->fileN);
    }
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsObjectOpenByPage(pfs, pageIX, pFd, 0, 0);   /* �����ҳ�棬����ȡ��pcPath��Ӧ���ļ� */
    if (iRes != SPIFFS_OK) {
        spiffsFdReturn(pfs, pFd->fileN);
    }
    
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsObjectTruncate(pFd, 0, LW_TRUE);            /* �ضϲ��Ƴ� */
    if (iRes != SPIFFS_OK) {
        spiffsFdReturn(pfs, pFd->fileN);
    }
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);
    
    //SPIFFS_UNLOCK(pfs);
    return 0;
}
/*********************************************************************************************************
** ��������: __spiffs_fremove
** ��������: �Ƴ�һ���ļ����൱�ڰ���ȫ���ض�
** �䡡��  : pfs          �ļ�ͷ
**           fileHandler   �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_fremove(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler) {
    SPIFFS_API_DBG("%stat "_SPIPRIfd "\n", __func__, fileHandler);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    //SPIFFS_LOCK(pfs);

    PSPIFFS_FD pFd;
    INT32 iRes;
    iRes = spiffsFdGet(pfs, fileHandler, &pFd);
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    if ((pFd->flags & SPIFFS_O_WRONLY) == 0) {
        iRes = SPIFFS_ERR_NOT_WRITABLE;
        //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
        SPIFFS_API_CHECK_RES(pfs, iRes);
    }
    //TODO:�����������ģ�
    spiffsCacheFdRelease(pfs, pFd->pCachePage);

    iRes = spiffsObjectTruncate(pFd, 0, LW_TRUE);
    
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    //SPIFFS_UNLOCK(pfs);

    return 0;
}
/*********************************************************************************************************
** ��������: __spiffs_stat
** ��������: ����һ���ļ���������pStat
** �䡡��  : pfs          �ļ�ͷ
**           fileHandler   �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_stat(PSPIFFS_VOLUME pfs, const PCHAR pcPath, PSPIFFS_STAT pStat) {
    SPIFFS_API_DBG("%s '%s'\n", __func__, pcPath);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    if (strlen(pcPath) > SPIFFS_OBJ_NAME_LEN - 1) {
        SPIFFS_API_CHECK_RES(pfs, SPIFFS_ERR_NAME_TOO_LONG);
    }
    //SPIFFS_LOCK(pfs);

    INT32 iRes;
    SPIFFS_PAGE_IX pageIX;

    iRes = spiffsObjectFindObjectIndexHeaderByName(pfs, (PUCHAR)pcPath, &pageIX);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsStatPageIX(pfs, pageIX, 0, pStat);

    //SPIFFS_UNLOCK(pfs);

    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffs_fstat
** ��������: ����һ���ļ���������pStat
** �䡡��  : pfs          �ļ�ͷ
**           fileHandler   �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_fstat(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler, PSPIFFS_STAT pStat) {
    SPIFFS_API_DBG("%s " _SPIPRIfd "\n", __func__, fileHandler);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    //SPIFFS_LOCK(pfs);

    INT32 iRes;
    SPIFFS_PAGE_IX pageIX;
    PSPIFFS_FD pFd;

    iRes = spiffsFdGet(pfs, fileHandler, &pFd);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsStatPageIX(pfs, pFd->pageIXObjIXHdr, fileHandler, pStat);

    //SPIFFS_UNLOCK(pfs);

    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffs_fflush
** ��������: ����һ���ļ���������pStat
** �䡡��  : pfs          �ļ�ͷ
**           fileHandler   �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_fflush(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler) {
    SPIFFS_API_DBG("%s "_SPIPRIfd "\n", __func__, fileHandler);
    (void)fileHandler;
    SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    INT32 iRes = SPIFFS_OK;
    //SPIFFS_LOCK(pfs);
    iRes = spiffsCacheFflush(pfs, fileHandler);
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs,iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);
    //SPIFFS_UNLOCK(pfs);
    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffs_close
** ��������: �ر�һ���򿪵��ļ�����ҪˢдCache���ͷ�pFd
** �䡡��  : pfs          �ļ�ͷ
**           fileHandler   �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_close(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler) {
    SPIFFS_API_DBG("%s "_SPIPRIfd "\n", __func__, fileHandler);
    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);

    INT32 iRes = SPIFFS_OK;
    //SPIFFS_LOCK(pfs);

    //fileHandler = SPIFFS_FH_UNOFFS(pfs, fileHandler);

    iRes = spiffsCacheFflush(pfs, fileHandler);
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsFdReturn(pfs, fileHandler);
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);
    //SPIFFS_UNLOCK(pfs);

    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffs_rename
** ��������: ������
** �䡡��  : pfs          �ļ�ͷ
**           fileHandler   �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_rename(PSPIFFS_VOLUME pfs, const PCHAR pcOldPath, const PCHAR pcNewPath) {
    SPIFFS_API_DBG("%s %s %s\n", __func__, pcOldPath, pcNewPath);

    //SPIFFS_API_CHECK_CFG(pfs);
    SPIFFS_API_CHECK_MOUNT(pfs);
    if (strlen(pcNewPath) > SPIFFS_OBJ_NAME_LEN - 1 ||
        strlen(pcOldPath) > SPIFFS_OBJ_NAME_LEN - 1) {
        SPIFFS_API_CHECK_RES(pfs, SPIFFS_ERR_NAME_TOO_LONG);
    }
    //SPIFFS_LOCK(pfs);

    SPIFFS_PAGE_IX pageIXOld, pageIXDummy;
    PSPIFFS_FD pFd;

    INT32 iRes = spiffsObjectFindObjectIndexHeaderByName(pfs, (PUCHAR)pcOldPath, &pageIXOld);
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);
    /* ���Ŀ��·����ͻ */
    iRes = spiffsObjectFindObjectIndexHeaderByName(pfs, (PUCHAR)pcNewPath, &pageIXDummy);
    
    if (iRes == SPIFFS_ERR_NOT_FOUND) {
        iRes = SPIFFS_OK;
    } 
    else if (iRes == SPIFFS_OK) {
        iRes = SPIFFS_ERR_CONFLICTING_NAME;
    }
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsFdFindNew(pfs, &pFd, LW_NULL);     /* ��һ���µ�fd */
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsObjectOpenByPage(pfs, pageIXOld, pFd, 0, 0);       /* ��ԭ����ҳ�浽fd�� */
    if (iRes != SPIFFS_OK) {
        spiffsFdReturn(pfs, pFd->fileN);
    }
    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);

    iRes = spiffsObjectUpdateIndexHdr(pfs, pFd, pFd->objId, pFd->pageIXObjIXHdr, 
                                      LW_NULL, (PUCHAR)pcNewPath, 0, &pageIXDummy); /* ����һ��IndexHdr */
    if (iRes == SPIFFS_OK) {
        /* �޸Ķ�Ӧfd��hashֵ */
        spiffsFdTemporalCacheRehash(pfs, pcOldPath, pcNewPath);
    }
    
    spiffsFdReturn(pfs, pFd->fileN);        /* �ر��ļ� */

    //SPIFFS_API_CHECK_RES_UNLOCK(pfs, iRes);
    SPIFFS_API_CHECK_RES(pfs, iRes);
    //SPIFFS_UNLOCK(pfs);

    return iRes;
}
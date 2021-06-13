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
** ��   ��   ��: spifFsCache.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ�����
*********************************************************************************************************/
#include "spifFsCache.h"
#include "spifFsFDLib.h"
#include "../SylixOS/driver/mtd/nor/nor.h"

#define WRITE_BACK 1
#define NO_WRITE_BACK 0

#define SPIFFS_CHECK_CACHE_EXIST(pCache)            !((pCache->uiCpageUseMap & pCache->uiCpageUseMask) \
                                                    == 0)                                             /* ���Cache�Ƿ����ҳ�� */       
#define SPIFFS_CHECK_CACHE_FREE(pCache)             ((pCache->uiCpageUseMap & pCache->uiCpageUseMask) \
                                                    != pCache->uiCpageUseMask)                        /* ���Cache�Ƿ��п���ҳ�� */
#define SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, i)    ((pCache->uiCpageUseMap & (1 << i)) == 1)
#define SPIFFS_MAP_USE_CACHE_PAGE(pCache, ix)       pCache->uiCpageUseMap |= (1 << ix);
#define SPIFFS_MAP_FREE_CACHE_PAGE(pCache, ix)      pCache->uiCpageUseMap &= ~(1 << ix);
/*********************************************************************************************************
** ��������: __spiffsCachePageFree
** ��������: �ͷ�Cache��uiIXλ�õ�Cache Page
** �䡡��  : pfs          �ļ�ͷ
**          uiIX          �����Cache�е�ҳ����
**          bIsWriteBack  �Ƿ�д��Flash
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffsCachePageFree(PSPIFFS_VOLUME pfs, UINT uiIX, BOOL bIsWriteBack){
    INT32 iRes = SPIFFS_OK;
    PSPIFFS_CACHE pCache            = SPIFFS_GET_CACHE_HDR(pfs);;
    PSPIFFS_CACHE_PAGE pCachePage   = SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, uiIX);
    PUCHAR pPageContent;
    if(SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, uiIX)){                    /* ��ҳ������Ч�� */
        //TODO: SPIFFS_CACHE_FLAG_TYPE_WR??
        if(bIsWriteBack &&                                               
           (pCachePage->flags & SPIFFS_CACHE_FLAG_TYPE_WR) == 0 &&      /*  ҳ�������ǿ�д��?? */
           (pCachePage->flags & SPIFFS_CACHE_FLAG_DIRTY)){              /* ҳ�汻д�ࣨ��д���ˣ� */
            pPageContent = SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, uiIX);
            SPIFFS_CACHE_DBG("CACHE_FREE: write cache page "_SPIPRIi" pix "_SPIPRIpg"\n", uiIX, pCachePage->pageIX);
            iRes = write_nor(SPIFFS_PAGE_TO_PADDR(pfs, uiIX), pPageContent, 
                             SPIFFS_CFG_LOGIC_PAGE_SZ(pfs), WRITE_KEEP);
        }

        if(pCachePage->flags & SPIFFS_CACHE_FLAG_TYPE_WR){
            SPIFFS_CACHE_DBG("CACHE_FREE: free cache page "_SPIPRIi" objid "_SPIPRIid"\n", uiIX, pCachePage->objId);
        }
        else {
            SPIFFS_CACHE_DBG("CACHE_FREE: free cache page "_SPIPRIi" pix "_SPIPRIpg"\n", uiIX, pCachePage->pageIX);
        }
        SPIFFS_MAP_FREE_CACHE_PAGE(pCache, uiIX);
        pCachePage->flags = 0;
    }

    return iRes;
}

/*********************************************************************************************************
** ��������: __spiffsCachePageRemoveOldest
** ��������: �Ƴ�Cache������ϵ�һ��ҳ��
** �䡡��  : pfs          �ļ�ͷ
**          uiFlagMask     ��־λ����
**          uiFlags        ��־λ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffsCachePageRemoveOldest(PSPIFFS_VOLUME pfs, UINT8 uiFlagMask, UINT8 uiFlags){
    /* Flag Mask : 1 << 7 */
    /* Flag      : 0 */
    INT32 iRes = SPIFFS_OK;
    PSPIFFS_CACHE pCache = SPIFFS_GET_CACHE_HDR(pfs);
    PSPIFFS_CACHE_PAGE pCachePage = LW_NULL;

    INT i;
    INT iCandidateIX    = -1;
    
    UINT32 uiAge;
    UINT32 uiOldestVal  = 0;

    if(SPIFFS_CHECK_CACHE_FREE(pCache)){
        return iRes;
    }
    /* Cache��ҳ��ռ���� */
    for (i = 0; i < pCache->uiCpageCount; i++)
    {
        pCache = SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i);
        uiAge = pCache->uiLastAccess - pCachePage->uiLastAccess;
        if(uiAge > uiOldestVal && 
           ((pCachePage->flags & uiFlagMask) == uiFlags)){
            uiOldestVal     = uiAge;
            iCandidateIX    = i;
        }
    }

    if(iCandidateIX >= 0){
        iRes = __spiffsCachePageFree(pfs, iCandidateIX, WRITE_BACK);
    }
    return iRes;
}
/*********************************************************************************************************
** ��������: __spiffsCachePageHit
** ��������: ���ҳ��pageIX�Ƿ���Cache�����У�������У��򷵻�ҳ��
** �䡡��  : pfs          �ļ�ͷ
**          pageIX         �����ҳ��
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PSPIFFS_CACHE_PAGE __spiffsCachePageHit(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX){
    PSPIFFS_CACHE pCache = SPIFFS_GET_CACHE_HDR(pfs);
    INT i;
    PSPIFFS_CACHE_PAGE pCachePage;
    if(!SPIFFS_CHECK_CACHE_EXIST(pCache)){     /* ���Cache���Ƿ��п���ҳ�� */
        return LW_NULL;
    }    
    for (i = 0; i < pCache->uiCpageCount; i++)
    {
        pCachePage = SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i);
        //TODO��SPIFFS_CACHE_FLAG_TYPE_WR������ʲô�� ��LOOKUP PAGE ��
        if(SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, i) && 
           (pCachePage->flags & SPIFFS_CACHE_FLAG_TYPE_WR) == 0 &&      
           pCachePage->pageIX == pageIX){
            SPIFFS_CACHE_DBG("CACHE_HIT: hit cache page " _SPIPRIi  " for " _SPIPRIpg "\n", i, pageIX);
            return pCachePage;
        }
    }
    return LW_NULL;
}
/*********************************************************************************************************
** ��������: __spiffsCachePageAllocate
** ��������: Cache����һ��ҳ��
** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PSPIFFS_CACHE_PAGE __spiffsCachePageAllocate(PSPIFFS_VOLUME pfs) {
  PSPIFFS_CACHE pCache = SPIFFS_GET_CACHE_HDR(pfs);
  PSPIFFS_CACHE_PAGE pCachePage;
  INT i;
  if (!SPIFFS_CHECK_CACHE_FREE(pCache)) {   /* û�п���ҳ���� */
    return LW_NULL;
  }
  for (i = 0; i < pCache->uiCpageCount ; i++) {
    if (!SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, i)) {
      pCachePage = SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i);
      SPIFFS_MAP_USE_CACHE_PAGE(pCache, i);
      pCachePage->uiLastAccess = pCache->uiLastAccess;
      return pCachePage;
    }
  }
  return LW_NULL;
}
/*********************************************************************************************************
** ��������: spiffsCacheRead
** ��������: ��Cache��δ���о��滻���߶���
** �䡡��  : pfs          �ļ�ͷ
**          pageIX         �����ҳ��
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsCacheRead(PSPIFFS_VOLUME pfs, UINT8 uiOps, SPIFFS_FILE fileHandler, 
                     UINT32 uiAddr, UINT32 uiLen, PUCHAR pDst){
    (VOID)fileHandler;      /* ��ʹ�� */
    PSPIFFS_CACHE       pCache = SPIFFS_GET_CACHE_HDR(pfs);
    PSPIFFS_CACHE_PAGE  pCachePage = __spiffsCachePageHit(pfs, SPIFFS_PADDR_TO_PAGE(pfs, uiAddr));
    PUCHAR              pPageContent;
    INT32               iRes = SPIFFS_OK;
    INT32               iRes2;
    if(uiLen > SPIFFS_CFG_LOGIC_PAGE_SZ(pfs)){      /* ������������߼�ҳ���С */
        return SPIFFS_ERR_CACHE_OVER_RD;
    }

    pCache->uiLastAccess++;

    if(pCachePage){         /* ���� */
        pfs->uiCacheHits++;
        pCachePage->uiLastAccess = pCache->uiLastAccess;
        pPageContent = SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, pCachePage->uiIX);
        lib_memcpy(pDst, pPageContent + SPIFFS_PADDR_TO_PAGE_OFFSET(pfs, uiAddr), uiLen);
    }
    else {                  /* δ���� */
        if((uiOps & SPIFFS_OP_TYPE_MASK) == SPIFFS_OP_T_OBJ_LU2){   /* ��Cache */
            iRes = read_nor(uiAddr, pDst, uiLen);
            return iRes;
        }
        pfs->uiCacheMisses++;
        iRes = __spiffsCachePageRemoveOldest(pfs, SPIFFS_CACHE_FLAG_TYPE_WR, 0);
        pCachePage = __spiffsCachePageAllocate(pfs);
        if(pCachePage){
            //TODO: Ϊɶ��WRTHRU
            pCachePage->flags   = SPIFFS_CACHE_FLAG_WRTHRU;
            pCachePage->pageIX  = SPIFFS_PADDR_TO_PAGE(pfs, uiAddr);
            SPIFFS_CACHE_DBG("CACHE_ALLO: allocated cache page "_SPIPRIi" for pix "_SPIPRIpg "\n", pCachePage->uiIX, pCachePage->pageIX);
            iRes2 = read_nor(uiAddr - SPIFFS_PADDR_TO_PAGE_OFFSET(pfs, uiAddr), 
                             SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, pCachePage->uiIX), 
                             SPIFFS_CFG_LOGIC_PAGE_SZ(pfs));
            if(iRes2 != SPIFFS_OK){                 /* !!����������д���� */
                iRes = iRes2;           
            }
            pPageContent = SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, pCachePage->uiIX);
            lib_memcpy(pDst, pPageContent + SPIFFS_PADDR_TO_PAGE_OFFSET(pfs, uiAddr), uiLen);
        }
        else {
            iRes2 = read_nor(uiAddr, pDst, uiLen);  /* ֱ�Ӷ� */
            if(iRes2 != SPIFFS_OK){                 /* !!����������д���� */
                iRes = iRes2;           
            }
        }
    }
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsCacheInit
** ��������: ��ʼ��pfs->pCache�νṹ���ṹ���£�

----------------------------------------------------------------------------------------------------------
                |                   |                  |                    |              |   ...
SPIFFS_Cache    |   SPIFFS_Page_HDR |                  |    SPIFFS_Page_HDR |              |   ...
                |                   |                  |                    |              |   ...
----------------------------------------------------------------------------------------------------------

** �䡡��  : pfs          �ļ�ͷ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsCacheInit(PSPIFFS_VOLUME pfs){
    UINT32 uiSize;
    UINT32 uiCacheMask;
    INT i;
    INT iCacheEntries;
    SPIFFS_CACHE spiffsCache;
    PSPIFFS_CACHE pCache;

    if(pfs->pCache == LW_NULL){
        return SPIFFS_ERR_CACHE_NO_INIT;
    }
    uiSize          = pfs->uiCacheSize;
    uiCacheMask     = 0;
    iCacheEntries   = (uiSize - sizeof(SPIFFS_CACHE)) / SPIFFS_CACHE_PAGE_SIZE(pfs);    /* ��ȥ����ͷ������ */
    
    if(iCacheEntries <= 0){
        return SPIFFS_ERR_CACHE_NO_MEM;
    }
    /* ��¼��Щλ���� */
    for (i = 0; i < iCacheEntries; i++)
    {
        uiCacheMask <<= 1;
        uiCacheMask |= 1;
    }

    lib_memset(&spiffsCache, 0, sizeof(SPIFFS_CACHE));
    spiffsCache.uiCpageCount = iCacheEntries;
    spiffsCache.Cpages = (PUCHAR)((PUCHAR)pfs->pCache + sizeof(SPIFFS_CACHE));
    spiffsCache.uiCpageUseMap   = (UINT32)-1;
    spiffsCache.uiCpageUseMask  = uiCacheMask;

    lib_memcpy(pfs->pCache, &spiffsCache, sizeof(SPIFFS_CACHE));
    
    
    pCache = SPIFFS_GET_CACHE_HDR(pfs);
    lib_memset(pCache->Cpages, 0 ,
               pCache->uiCpageCount * SPIFFS_GET_CACHE_PAGE_SIZE(pfs));
    
    pCache->uiCpageUseMap &= ~(pCache->uiCpageUseMask);         /* 111/000000 */
    for (i = 0; i < pCache->uiCpageCount; i++)
    {
        SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i)->uiIX = i;
    }
    return SPIFFS_OK;
}
/*********************************************************************************************************
** ��������: spiffsCacheWrite
** ��������: дCache�����WriteThru�Ͳ���ҪдCache��Ҫд���ʣ�����Write Not Alloc����
** �䡡��  : pfs          �ļ�ͷ
**          blkIX         �߼����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsCacheWrite(PSPIFFS_VOLUME pfs, UINT8 uiOps, SPIFFS_FILE fileHandler, 
                       UINT32 uiAddr, UINT32 uiLen, PUCHAR pSrc){
    (VOID)fileHandler;
    SPIFFS_PAGE_IX pageIX = SPIFFS_PADDR_TO_PAGE(pfs, uiAddr);
    PSPIFFS_CACHE pCache = SPIFFS_GET_CACHE_HDR(pfs);
    PSPIFFS_CACHE_PAGE pCachePage = __spiffsCachePageHit(pfs, pageIX);
    PUCHAR pPageContent;
    if(pCachePage && ((uiOps & SPIFFS_OP_COM_MASK) != SPIFFS_OP_C_WRTHRU)){
        if((uiOps & SPIFFS_OP_COM_MASK) == SPIFFS_OP_C_DELE &&          /* ��Ҫ��ɾ�� */
           (uiOps & SPIFFS_OP_TYPE_MASK) != SPIFFS_OP_T_OBJ_LU){        /* �Ҳ���Look Up Page */
            __spiffsCachePageFree(pfs, pCachePage->uiIX, 0);
            return write_nor(uiAddr, pSrc, uiLen, WRITE_KEEP);
        }
        pCache->uiLastAccess++;
        pCachePage->uiLastAccess = pCache->uiLastAccess;

        pPageContent = SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, pCachePage->uiIX);
        lib_memcpy(pPageContent + SPIFFS_PADDR_TO_PAGE_OFFSET(pfs, uiAddr), pSrc, uiLen);

        if(pCachePage->flags & SPIFFS_CACHE_FLAG_WRTHRU){
            return write_nor(uiAddr, pSrc, uiLen, WRITE_KEEP);
        }
        else {
            return SPIFFS_OK;
        }
    }
    else {
        return write_nor(uiAddr, pSrc, uiLen, WRITE_KEEP);
    }
}
/*********************************************************************************************************
** ��������: spiffsEraseBlk
** ��������: ����һ���飬��ΪCache����Ӳ����ӣ���ˣ����ú�����������
** �䡡��  : pfs          �ļ�ͷ
**          blkIX         �߼����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsEraseBlk(PSPIFFS_VOLUME pfs, SPIFFS_BLOCK_IX blkIX){
    INT32 iRes;
    UINT uiAddr = SPIFFS_BLOCK_TO_PADDR(pfs, blkIX);
    UINT uiSize = SPIFFS_CFG_LOGIC_PAGE_SZ(pfs);
    
    UINT uiSectorSize;
    UINT uiSectorNo;

    SPIFFS_OBJ_ID objIdMagic;
    
    while (uiSize > 0) {
        SPIFFS_DBG("erase "_SPIPRIad":"_SPIPRIi"\n", uiAddr,  SPIFFS_CFG_PHYS_ERASE_SZ(pfs));
        erase_nor(uiAddr, ERASE_SECTOR);

        uiSectorNo = GET_SECTOR_NO(uiAddr);    
        uiSectorSize = GET_SECTOR_SIZE(uiSectorNo);

        uiAddr += uiSectorSize;
        uiSize -= uiSectorSize;
    }
    pfs->uiFreeBlks++;

    /* ������д��uiMaxEraseCount */
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_WRTHRU, 0, 
                            SPIFFS_ERASE_COUNT_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID),
                            (PUCHAR)&pfs->uiMaxEraseCount);
    SPIFFS_CHECK_RES(iRes);

    /* ������д��Magic*/
    objIdMagic = SPIFFS_MAGIC(pfs, blkIX);
    iRes = spiffsCacheWrite(pfs, SPIFFS_OP_T_OBJ_LU2 | SPIFFS_OP_C_WRTHRU, 0, 
                            SPIFFS_MAGIC_PADDR(pfs, blkIX), sizeof(SPIFFS_OBJ_ID),
                            (PUCHAR)&objIdMagic);

    SPIFFS_CHECK_RES(iRes);

    pfs->uiMaxEraseCount++;
    /* ���λ����Ϊ 1 */
    if(pfs->uiMaxEraseCount == SPIFFS_OBJ_ID_IX_FLAG) {
        pfs->uiMaxEraseCount = 0;
    }
    return iRes;
}
/*********************************************************************************************************
** ��������: spiffsCachePageGetByFd
** ��������: �����ļ�������ָ���Cache Page�����û���򷵻�LW_NULL
** �䡡��  : pfs          �ļ�ͷ
**          fileHandler    �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PSPIFFS_CACHE_PAGE spiffsCachePageGetByFd(PSPIFFS_VOLUME pfs, PSPIFFS_FD pFd){
    PSPIFFS_CACHE pCache = SPIFFS_GET_CACHE_HDR(pfs);
    PSPIFFS_CACHE_PAGE pCachePage;
    INT i;
    if(!SPIFFS_CHECK_CACHE_EXIST(pCache)){  /* ����Cache Page���ǿ��еģ�û��ҳ����Ա������ObjId */
        return LW_NULL;
    }
    for (i = 0; i < pCache->uiCpageCount; i++)
    {
        pCachePage = SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i);
        if(SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, i) &&
           (pCachePage->flags & SPIFFS_CACHE_FLAG_TYPE_WR) &&
           pCachePage->objId == pFd->objId){
            return pCachePage;
        }
    }
    return LW_NULL;
}

/*********************************************************************************************************
** ��������: spiffsCacheFdRelease
** ��������:  unrefers all fds that this cache page refers to and releases the cache page
** �䡡��  : pfs          �ļ�ͷ
**          pCachePage    ����ҳ
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID spiffsCacheFdRelease(PSPIFFS_VOLUME pfs, PSPIFFS_CACHE_PAGE pCachePage){
    UINT i;
    PSPIFFS_FD *pFds;
    PSPIFFS_FD  pFdCur;
    if(pCachePage == LW_NULL)
        return;
    pFds = (PSPIFFS_FD)pfs->pucFdSpace;
    for (i = 0; i < pfs->uiFdCount; i++) {
        pFdCur = &pFds[i];
        if (pFdCur->fileN != 0 && pFdCur->pCachePage == pCachePage) {
            pFdCur->pCachePage = LW_NULL;
        }
    }
    __spiffsCachePageFree(pfs, pCachePage->uiIX, 0);
    pCachePage->objId = 0;
    return;
}

/*********************************************************************************************************
** ��������: spiffsCacheFflush
** ��������: �����ļ�fileHandler��ص�CacheWrite��д�ؽ���
** �䡡��  : pfs          �ļ�ͷ
**          fileHandler    �ļ����
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 spiffsCacheFflush(PSPIFFS_VOLUME pfs, SPIFFS_FILE fileHandler){
    INT32 iRes = SPIFFS_OK;
    PSPIFFS_FD pFd;

    iRes = spiffsFdGet(pfs, fileHandler, &pFd);
    SPIFFS_CHECK_RES(iRes);

    if((pFd->flags & SPIFFS_O_DIRECT) == 0){
        if(pFd->pCachePage == LW_NULL){
            pFd->pCachePage = spiffsCachePageGetByFd(pfs, pFd);
        }

        if(pFd->pCachePage){
            SPIFFS_CACHE_DBG("CACHE_WR_DUMP: dumping cache page "_SPIPRIi" for fd "_SPIPRIfd":"_SPIPRIid", flush, offs:"_SPIPRIi" size:"_SPIPRIi"\n",
                              pFd->pCachePage->uiIX, pFd->fileN,  pFd->objId, pFd->pCachePage->uiOffset, pFd->pCachePage->uiSize);
            //TODO: ��δ�ҿ���ɴ
        }
    }
}
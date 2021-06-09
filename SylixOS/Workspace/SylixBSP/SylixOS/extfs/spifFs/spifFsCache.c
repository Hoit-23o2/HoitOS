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
#include "../SylixOS/driver/mtd/nor/nor.h"
#define SPIFFS_CHECK_CACHE_EXIST(pCache)            !((pCache->uiCpageUseMap & pCache->uiCpageUseMask) == 0)  
#define SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, i)    (pCache->uiCpageUseMap & (1 << i))
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
        //TODO��SPIFFS_CACHE_FLAG_TYPE_WR������ʲô��
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
            read_nor(uiAddr, pDst, uiLen);
        }
    }
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
    spiffsCache.uiCpageUseMap   = (UINT32)0;
    spiffsCache.uiCpageUseMask  = uiCacheMask;

    lib_memcpy(pfs->pCache, &spiffsCache, sizeof(SPIFFS_CACHE));
    
    
    pCache = SPIFFS_GET_CACHE_HDR(pfs);
    lib_memset(pCache->Cpages, 0 ,
               pCache->uiCpageCount * SPIFFS_GET_CACHE_PAGE_SIZE(pfs));
    
    for (i = 0; i < pCache->uiCpageCount; i++)
    {
        SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i)->uiIX = i;
    }
    return SPIFFS_OK;
}

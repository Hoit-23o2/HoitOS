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
** 文   件   名: spifFsCache.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统缓存层
*********************************************************************************************************/
#include "spifFsCache.h"
#include "../SylixOS/driver/mtd/nor/nor.h"
#define SPIFFS_CHECK_CACHE_EXIST(pCache)            !((pCache->uiCpageUseMap & pCache->uiCpageUseMask) == 0)  
#define SPIFFS_CHECK_CACHE_PAGE_VALID(pCache, i)    (pCache->uiCpageUseMap & (1 << i))
/*********************************************************************************************************
** 函数名称: __spiffsCachePageHit
** 功能描述: 检查页面pageIX是否在Cache中命中，如果命中，则返回页面
** 输　入  : pfs          文件头
**          pageIX         待检查页面
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PSPIFFS_CACHE_PAGE __spiffsCachePageHit(PSPIFFS_VOLUME pfs, SPIFFS_PAGE_IX pageIX){
    PSPIFFS_CACHE pCache = SPIFFS_GET_CACHE_HDR(pfs);
    INT i;
    PSPIFFS_CACHE_PAGE pCachePage;
    if(!SPIFFS_CHECK_CACHE_EXIST(pCache)){     /* 检查Cache中是否有可用页面 */
        return LW_NULL;
    }    
    for (i = 0; i < pCache->uiCpageCount; i++)
    {
        pCachePage = SPIFFS_GET_CACHE_PAGE_HDR(pfs, pCache, i);
        //TODO：SPIFFS_CACHE_FLAG_TYPE_WR代表着什么？
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
** 函数名称: spiffsCacheRead
** 功能描述: 读Cache，未命中就替换或者读入
** 输　入  : pfs          文件头
**          pageIX         待检查页面
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT32 spiffsCacheRead(PSPIFFS_VOLUME pfs, UINT8 uiOps, SPIFFS_FILE fileHandler, 
                     UINT32 uiAddr, UINT32 uiLen, PUCHAR pDst){
    (VOID)fileHandler;      /* 不使用 */
    PSPIFFS_CACHE       pCache = SPIFFS_GET_CACHE_HDR(pfs);
    PSPIFFS_CACHE_PAGE  pCachePage = __spiffsCachePageHit(pfs, SPIFFS_PADDR_TO_PAGE(pfs, uiAddr));
    PUCHAR              pPageContent;
    
    if(uiLen > SPIFFS_CFG_LOGIC_PAGE_SZ(pfs)){      /* 不允许读超过逻辑页面大小 */
        return SPIFFS_ERR_CACHE_OVER_RD;
    }

    pCache->uiLastAccess++;

    if(pCachePage){         /* 命中 */
        pfs->uiCacheHits++;
        pCachePage->uiLastAccess = pCache->uiLastAccess;
        pPageContent = SPIFFS_GET_CACHE_PAGE_CONTENT(pfs, pCache, pCachePage->uiIX);
        lib_memcpy(pDst, pPageContent + SPIFFS_PADDR_TO_PAGE_OFFSET(pfs, uiAddr), uiLen);
    }
    else {                  /* 未命中 */
        if((uiOps & SPIFFS_OP_TYPE_MASK) == SPIFFS_OP_T_OBJ_LU2){   /* 不Cache */
            read_nor(uiAddr, pDst, uiLen);
        }
    }
}
/*********************************************************************************************************
** 函数名称: spiffsCacheInit
** 功能描述: 初始化pfs->pCache段结构，结构如下：

----------------------------------------------------------------------------------------------------------
                |                   |                  |                    |              |   ...
SPIFFS_Cache    |   SPIFFS_Page_HDR |                  |    SPIFFS_Page_HDR |              |   ...
                |                   |                  |                    |              |   ...
----------------------------------------------------------------------------------------------------------

** 输　入  : pfs          文件头
** 输　出  : None
** 全局变量:
** 调用模块:
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
    iCacheEntries   = (uiSize - sizeof(SPIFFS_CACHE)) / SPIFFS_CACHE_PAGE_SIZE(pfs);    /* 减去缓存头部长度 */
    
    if(iCacheEntries <= 0){
        return SPIFFS_ERR_CACHE_NO_MEM;
    }
    /* 记录哪些位可用 */
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

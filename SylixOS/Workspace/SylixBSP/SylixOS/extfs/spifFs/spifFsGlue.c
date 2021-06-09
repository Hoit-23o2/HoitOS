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
** ��������: __spiffs_mount
** ��������: �����ļ�
** �䡡��  : pfs          �ļ�ͷ
**          pConfig         �����ļ�
**          pucWorkBuffer   ����WorkBuffer
**          puiFdSpace      ������ļ�����������
**          uiFdSpaceSize   ������ļ������������С
**          pCache          ����Ļ���ռ�
**          uiCacheSize     �����Cached��С
** �䡡��  : None
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT32 __spiffs_mount(PSPIFFS_VOLUME pfs, PSPIFFS_CONFIG pConfig, PUCHAR pucWorkBuffer,
                     UINT8 *puiFdSpace, UINT32 uiFdSpaceSize,
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
    lib_memset(puiFdSpace, 0, uiFdSpaceSize);
    lib_memcpy(&pfs->cfg, pConfig, sizeof(SPIFFS_CONFIG));

    pfs->pUserData              = pUserData;
    pfs->uiBlkCount             = SPIFFS_CFG_PHYS_SZ(pfs) / SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);
    pfs->pucWorkBuffer          = pucWorkBuffer;
    pfs->pucLookupWorkBuffer    = pucWorkBuffer + SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);

    /* ����puiFdSpace��8�ֽڶ��� */
    __spiffsAlign8Byte(puiFdSpace, uiFdSpaceSize);
    pfs->puiFdSpace = puiFdSpace;
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
}


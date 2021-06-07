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
#include "spifFsType.h"
#include "spifFsLib.h"
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
                     VOID *pCache, UINT32 uiCacheSize,
                     spiffsCheckCallback checkCallbackFunc){
    //TODO:ʲô��˼��
    PVOID           pUserData;      
    UINT8           uiAddrLSB;                                  /* ����ָ����� */
    pUserData     = LW_NULL;
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

    pfs->pUserData  = pUserData;
    pfs->uiBlkCount = SPIFFS_CFG_PHYS_SZ(pfs) / SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);
    pfs->pucWorkBuffer = pucWorkBuffer;
    pfs->pucLookupWorkBuffer = pucWorkBuffer + SPIFFS_CFG_LOGIC_BLOCK_SZ(pfs);

    /* ����puiFdSpace */
}


/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: HoitFsLib.c
**
** ��   ��   ��: Hoit Group
**
** �ļ���������: 2021 �� 03 �� 20 ��
**
** ��        ��: Hoit�ļ�ϵͳ�ڲ�����.
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "../driver/mtd/nor/nor.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0
#include "hoitFsLib.h"
#ifndef HOITFSLIB_DISABLE
/*********************************************************************************************************
** ��������: __hoit_just_open
** ��������: ��ĳ���Ѵ򿪵�Ŀ¼�ļ������һ���ļ�
**           ע��pcName�Ǹ�Ŀ¼�ļ��µ�һ���ļ���(���·��)��Ҫ�򿪵��ļ�������Ŀ¼�ļ�pdir��ֱ�����ļ������򷵻�NULL
** �䡡��  :
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_INODE_INFO  __hoit_just_open(PHOIT_INODE_INFO  pdir,
    PCHAR       pName)
{

    if (pdir == LW_NULL || !S_ISDIR(pdir->HOITN_mode)) {
        printk("Error in hoit_just_open\n");
        return (LW_NULL);
    }
    UINT newHash = __hoit_name_hash(pName);
    PHOIT_FULL_DIRENT pfile = pdir->HOITN_dents;
    while (pfile != LW_NULL) {
        if (pfile->HOITFD_nhash == newHash && lib_strcmp(pfile->HOITFD_file_name, pName) == 0) {
            return __hoit_get_full_file(pdir->HOITN_volume, pfile->HOITFD_ino);
        }
        else {
            pfile = pfile->HOITFD_next;
        }
    }

    return  (LW_NULL);                                                  /*  �޷��ҵ��ڵ�                */
}

/*********************************************************************************************************
** ��������: __hoit_name_hash
** ��������: �����ļ����������hashֵ
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT __hoit_name_hash(CPCHAR pcName) {
    UINT ret = 0;
    while (*pcName != PX_EOS) {
        ret += *pcName;
    }
    return ret;
}
/*********************************************************************************************************
** ��������: __hoit_free_full_dirent
** ��������: �ͷ�FullDirent�����ļ���
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT __hoit_free_full_dirent(PHOIT_FULL_DIRENT pDirent) {
    __SHEAP_FREE(pDirent->HOITFD_file_name);
    __SHEAP_FREE(pDirent);
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_get_full_file
** ��������: ����inode number��������Ӧfull_xxx�ṹ�壨Ŀ¼�ļ�������������ͨ�ļ��������������
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_INODE_INFO __hoit_get_full_file(PHOIT_VOLUME pfs, UINT ino) {
    if (pfs == LW_NULL) {
        printk("Error in hoit_get_full_file\n");
    }

    PHOIT_INODE_CACHE pcache = pfs->HOITFS_cache_list;

    while (pcache && pcache->HOITC_ino != ino) {
        pcache = pcache->HOITC_next;
    }
    //************************************ TODO ************************************


    //************************************ END  ************************************
    return LW_NULL;
}

/*********************************************************************************************************
** ��������: __hoit_get_inode_cache
** ��������: ����inode number������inode_cache��û�оͷ���NULL
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_INODE_CACHE __hoit_get_inode_cache(PHOIT_VOLUME pfs, UINT ino) {
    if (pfs == LW_NULL) {
        printk("Error in hoit_get_full_file\n");
    }

    PHOIT_INODE_CACHE pcache = pfs->HOITFS_cache_list;

    while (pcache && pcache->HOITC_ino != ino) {
        pcache = pcache->HOITC_next;
    }
    
    return pcache;
}

/*********************************************************************************************************
** ��������: __hoit_add_dirent
** ��������: ��Ŀ¼�ļ������һ��dirent���漰nhash��
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_add_dirent(PHOIT_INODE_INFO  pFatherInode,
    PHOIT_FULL_DIRENT pSonDirent)
{

    PHOIT_RAW_DIRENT    pRawDirent = (PHOIT_RAW_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_RAW_DIRENT));
    PHOIT_RAW_INFO      pRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));
    if (pRawDirent == LW_NULL || pRawInfo == LW_NULL || pFatherInode == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return;
    }
    PHOIT_VOLUME pfs = pFatherInode->HOITN_volume;
    lib_bzero(pRawDirent, sizeof(HOIT_RAW_DIRENT));
    lib_bzero(pRawInfo, sizeof(HOIT_RAW_INFO));

    pRawDirent->file_type = pSonDirent->HOITFD_file_type;
    pRawDirent->ino = pSonDirent->HOITFD_ino;
    pRawDirent->magic_num = HOIT_MAGIC_NUM;
    pRawDirent->pino = pSonDirent->HOITFD_pino;
    pRawDirent->totlen = sizeof(HOIT_RAW_DIRENT) + lib_strlen(pSonDirent->HOITFD_file_name);

    pRawInfo->phys_addr = pfs->HOITFS_now_sector->HOITS_offset + pfs->HOITFS_now_sector->HOITS_addr;
    pRawInfo->totlen = pRawDirent->totlen;

    __hoit_write_flash(pfs, (PVOID)pRawDirent, sizeof(HOIT_RAW_DIRENT), LW_NULL);
    __hoit_write_flash(pfs, (PVOID)(pSonDirent->HOITFD_file_name), lib_strlen(pSonDirent->HOITFD_file_name), LW_NULL);

    PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pFatherInode->HOITN_ino);
    pRawInfo->next_phys = pInodeCache->HOITC_nodes;
    pInodeCache->HOITC_nodes = pRawInfo;
    pSonDirent->HOITFD_raw_info = pRawInfo;
    __hoit_add_to_dents(pFatherInode, pSonDirent);
    __SHEAP_FREE(pRawDirent);
}
/*********************************************************************************************************
** ��������: __hoit_alloc_ino
** ��������: ���ļ�ϵͳ����һ���µ�inode number
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT __hoit_alloc_ino(PHOIT_VOLUME pfs) {
    if (pfs == LW_NULL) {
        printk("Error in hoit_get_full_file\n");
    }
    return pfs->HOITFS_highest_ino++;
}
/*********************************************************************************************************
** ��������: __hoit_write_flash
** ��������: д�������豸�������Լ�ѡ�����ַ
** �䡡��  :
** �䡡��  : <0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_write_flash(PHOIT_VOLUME pfs, PVOID pdata, UINT length, UINT* phys_addr) {
    write_nor(pfs->HOITFS_now_sector->HOITS_offset + pfs->HOITFS_now_sector->HOITS_addr, (PCHAR)(pdata), length, WRITE_KEEP);
    pfs->HOITFS_now_sector->HOITS_offset += length;
    if (phys_addr != LW_NULL) {
        *phys_addr = pfs->HOITFS_now_sector->HOITS_offset + pfs->HOITFS_now_sector->HOITS_addr;
    }
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_write_flash_thru
** ��������: д�������豸�������Լ�ѡ�����ַ
** �䡡��  :
** �䡡��  : <0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_write_flash_thru(PHOIT_VOLUME pfs, PVOID pdata, UINT length, UINT phys_addr) {
    write_nor(phys_addr, (PCHAR)(pdata), length, WRITE_KEEP);
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_add_to_inode_cache
** ��������: ��һ��raw_info���뵽inode_cache��
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_add_to_inode_cache(PHOIT_INODE_CACHE pInodeCache, PHOIT_RAW_INFO pRawInfo) {
    if (pInodeCache == LW_NULL || pRawInfo == LW_NULL) {
        printk("Error in hoit_add_to_inode_cache\n");
        return HOIT_ERROR;
    }
    pRawInfo->next_phys = pInodeCache->HOITC_nodes;
    pInodeCache->HOITC_nodes = pRawInfo;
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_add_to_cache_list
** ��������: ��һ��raw_info���뵽inode_cache��
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_add_to_cache_list(PHOIT_VOLUME pfs, PHOIT_INODE_CACHE pInodeCache) {
    if (pfs == LW_NULL || pInodeCache == LW_NULL) {
        printk("Error in hoit_add_to_inode_cache\n");
        return HOIT_ERROR;
    }
    
    pInodeCache->HOITC_next = pfs->HOITFS_cache_list;
    pfs->HOITFS_cache_list = pInodeCache->HOITC_next;
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_add_to_dents
** ��������: ��һ��full_dirent���뵽��Ŀ¼�ļ��е�dents��
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_add_to_dents(PHOIT_INODE_INFO pInodeFather, PHOIT_FULL_DIRENT pFullDirent) {
    if (pInodeFather == LW_NULL || pFullDirent == LW_NULL) {
        printk("Error in hoit_add_to_dents\n");
        return HOIT_ERROR;
    }

    pFullDirent->HOITFD_next = pInodeFather->HOITN_dents;
    pInodeFather->HOITN_dents = pFullDirent;
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_search_in_dents
** ��������: ��һ����dents�������ö��ַ�����һ��ָ��ino���ļ�������FullDirent
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FULL_DIRENT __hoit_search_in_dents(PHOIT_INODE_INFO pInodeFather, UINT ino) {
    if (pInodeFather == LW_NULL) {
        printk("Error in hoit_search_in_dents\n");
        return LW_NULL;
    }
    PHOIT_FULL_DIRENT pDirent = pInodeFather->HOITN_dents;
    while (pDirent && pDirent->HOITFD_ino != ino) {
        pDirent = pDirent->HOITFD_next;
    }
    if (pDirent) 
        return pDirent;
    return LW_NULL;
}
/*********************************************************************************************************
** ��������: __hoit_del_raw_info
** ��������: ��һ��RawInfo�Ӷ�Ӧ��InodeCache������ɾ��������free��Ӧ�ڴ�ռ�
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_del_raw_info(PHOIT_INODE_CACHE pInodeCache, PHOIT_RAW_INFO pRawInfo) {
    if (pInodeCache == LW_NULL || pRawInfo == LW_NULL) {
        printk("Error in hoit_add_to_dents\n");
        return HOIT_ERROR;
    }
    if (pInodeCache->HOITC_nodes == pRawInfo) {
        pInodeCache->HOITC_nodes = pRawInfo->next_phys;
        return 0;
    }
    else {
        PHOIT_RAW_INFO pRawTemp = pInodeCache->HOITC_nodes;
        while (pRawTemp && pRawTemp->next_phys != pRawInfo) {
            pRawTemp = pRawTemp->next_phys;
        }
        if (pRawTemp) {
            pRawTemp->next_phys = pRawInfo->next_phys;
            return 0;
        }
        else {
            return HOIT_ERROR;
        }
    }
    
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_del_raw_data
** ��������: ��һ��RawDirent��RawInode�ڶ�Ӧ�Ĵ����б��Ϊ����,�����ͷ�RawInfo�ڴ�
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_del_raw_data(PHOIT_RAW_INFO pRawInfo) {
    if (pRawInfo == LW_NULL) {
        printk("Error in hoit_del_raw_data\n");
        return HOIT_ERROR;
    }

    PCHAR buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);
    lib_bzero(buf, pRawInfo->totlen);
    read_nor(pRawInfo->phys_addr, buf, pRawInfo->totlen);

    PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)buf;
    if (pRawHeader->magic_num != HOIT_MAGIC_NUM || (pRawHeader->flag & HOIT_FLAG_OBSOLETE) == 0) {
        printk("Error in hoit_del_raw_data\n");
        return HOIT_ERROR;
    }
    pRawHeader->flag &= (~HOIT_FLAG_OBSOLETE);      //��obsolete��־��Ϊ0���������
    
    __hoit_write_flash_thru(LW_NULL, (PVOID)pRawHeader, pRawInfo->totlen, pRawInfo->phys_addr);
    __SHEAP_FREE(buf);
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_del_full_dirent
** ��������: ��һ��FullDirent�Ӷ�Ӧ��InodeInfo��dents������ɾ��������free��Ӧ�ڴ�ռ�
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_del_full_dirent(PHOIT_INODE_INFO pInodeInfo, PHOIT_FULL_DIRENT pFullDirent) {
    if (pInodeInfo == LW_NULL || pFullDirent == LW_NULL) {
        printk("Error in hoit_del_full_dirent\n");
        return HOIT_ERROR;
    }
    if (pInodeInfo->HOITN_dents == pFullDirent) {
        pInodeInfo->HOITN_dents = pFullDirent->HOITFD_next;
        return 0;
    }
    else {
        PHOIT_FULL_DIRENT pFullTemp = pInodeInfo->HOITN_dents;
        while (pFullTemp && pFullTemp->HOITFD_next != pFullDirent) {
            pFullTemp = pFullTemp->HOITFD_next;
        }
        if (pFullTemp) {
            pFullTemp->HOITFD_next = pFullDirent->HOITFD_next;
            return 0;
        }
        else {
            return HOIT_ERROR;
        }
    }
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_del_inode_cache
** ��������: ��һ��InodeCache�ӹ��ص��ļ�ϵͳ��ɾ��������free��Ӧ�ڴ�ռ�
** �䡡��  :
** �䡡��  : !=0 �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_del_inode_cache(PHOIT_VOLUME pfs, PHOIT_INODE_CACHE pInodeCache) {
    PHOIT_INODE_CACHE pTemp = pfs->HOITFS_cache_list;
    if (pTemp == pInodeCache) {
        pfs->HOITFS_cache_list = pInodeCache->HOITC_next;
        return 0;
    }
    while (pTemp && pTemp->HOITC_next != pInodeCache) {
        pTemp = pTemp->HOITC_next;
    }
    if (pTemp == LW_NULL) return HOIT_ERROR;
    pTemp->HOITC_next = pInodeCache->HOITC_next;
    return 0;
}


/*********************************************************************************************************
** ��������: __hoit_open
** ��������: hoitfs ��һ���ļ�
** �䡡��  : pfs              �ļ�ϵͳ
**           pcName           �ļ���
**           ppinodeFather     ���޷��ҵ��ڵ�ʱ������ӽ���һ��,
                              ��Ѱ�ҵ��ڵ�ʱ���游ϵ�ڵ�.
                              LW_NULL ��ʾ��
             pbRoot           �Ƿ�Ϊ���ڵ�
**           pbLast           ��ƥ��ʧ��ʱ, �Ƿ������һ���ļ�ƥ��ʧ��
**           ppcTail          ������������ļ�, ָ�������ļ����·��
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_INODE_INFO  __hoit_open(PHOIT_VOLUME  pfs,
    CPCHAR       pcName,
    PHOIT_INODE_INFO* ppinodeFather,
    BOOL* pbRoot,
    BOOL* pbLast,
    PCHAR* ppcTail)
{
    CHAR                pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;

    PHOIT_INODE_INFO    pinodeTemp;

    if (ppinodeFather == LW_NULL) {
        ppinodeFather = &pinodeTemp;                                      /*  ��ʱ����                    */
    }
    *ppinodeFather = LW_NULL;
    UINT inodeFatherIno = 0;
    if (*pcName == PX_ROOT) {                                           /*  ���Ը�����                  */
        lib_strlcpy(pcTempName, (pcName + 1), PATH_MAX);
    }
    else {
        lib_strlcpy(pcTempName, pcName, PATH_MAX);
    }

    if (pcTempName[0] == PX_EOS) {
        if (pbRoot) {
            *pbRoot = LW_TRUE;                                          /*  pcName Ϊ��                 */
        }
        if (pbLast) {
            *pbLast = LW_FALSE;
        }
        return  (LW_NULL);
    }
    else {
        if (pbRoot) {
            *pbRoot = LW_FALSE;                                         /*  pcName ��Ϊ��               */
        }
    }
    PHOIT_INODE_INFO    pInode;
    PHOIT_FULL_DIRENT   pDirentTemp;

    pcNext = pcTempName;
    pInode = pfs->HOITFS_pRootDir;                               /*  �Ӹ�Ŀ¼��ʼ����            */

    do {
        pcNode = pcNext;
        pcNext = lib_index(pcNode, PX_DIVIDER);                         /*  �ƶ����¼�Ŀ¼              */
        if (pcNext) {                                                   /*  �Ƿ���Խ�����һ��          */
            *pcNext = PX_EOS;
            pcNext++;                                                   /*  ��һ���ָ��                */
        }

        for (pDirentTemp = pInode->HOITN_dents;
            pDirentTemp != LW_NULL;
            pDirentTemp = pDirentTemp->HOITFD_next) {

            if (pDirentTemp == LW_NULL) {                                     /*  �޷���������                */
                goto    __find_error;
            }
            if (S_ISLNK(pDirentTemp->HOITFD_file_type)) {                            /*  �����ļ�                    */
                if (lib_strcmp(pDirentTemp->HOITFD_file_name, pcNode) == 0) {
                    goto    __find_ok;                                  /*  �ҵ�����                    */
                }

            }
            else if (S_ISDIR(pDirentTemp->HOITFD_file_type)) {
                if (lib_strcmp(pDirentTemp->HOITFD_file_name, pcNode) == 0) {      /*  �Ѿ��ҵ�һ��Ŀ¼            */
                    break;
                }
            }
            else {
                if (lib_strcmp(pDirentTemp->HOITFD_file_name, pcNode) == 0) {
                    if (pcNext) {                                       /*  �������¼�, �������ΪĿ¼  */
                        goto    __find_error;                           /*  ����Ŀ¼ֱ�Ӵ���            */
                    }
                    break;
                }
            }
        }

        inodeFatherIno = pDirentTemp->HOITFD_ino;                       /*  �ӵ�ǰ�ڵ㿪ʼ����          */
        pInode = __hoit_get_full_file(pfs, inodeFatherIno);             /*  �ӵ�һ�����ӿ�ʼ            */
    } while (pcNext);                                                   /*  �������¼�Ŀ¼              */

__find_ok:
    *ppinodeFather = __hoit_get_full_file(pfs, pDirentTemp->HOITFD_pino);                            /*  ��ϵ�ڵ�                    */
    /*
     *  ���� tail ��λ��.
     */
    if (ppcTail) {
        if (pcNext) {
            INT   iTail = pcNext - pcTempName;
            *ppcTail = (PCHAR)pcName + iTail;                           /*  ָ��û�б������ / �ַ�     */
        }
        else {
            *ppcTail = (PCHAR)pcName + lib_strlen(pcName);              /*  ָ����ĩβ                  */
        }
    }
    return  (pInode);

__find_error:
    if (pbLast) {
        if (pcNext == LW_NULL) {                                        /*  ���һ������ʧ��            */
            *pbLast = LW_TRUE;
        }
        else {
            *pbLast = LW_FALSE;
        }
    }
    return  (LW_NULL);                                                  /*  �޷��ҵ��ڵ�                */
}


/*********************************************************************************************************
** ��������: __hoit_maken
** ��������: HoitFs ����һ���ļ�
** �䡡��  : pfs              �ļ�ϵͳ
**           pcName           �ļ��������ܺ����ϼ�Ŀ¼������
**           pInodeFather     ����, NULL ��ʾ��Ŀ¼
**           mode             mode_t
**           pcLink           ���Ϊ�����ļ�, ����ָ������Ŀ��.
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_INODE_INFO  __hoit_maken(PHOIT_VOLUME  pfs,
    CPCHAR       pcName,
    PHOIT_INODE_INFO    pInodeFather,
    mode_t       mode,
    CPCHAR       pcLink)
{
    PHOIT_RAW_INODE     pRawInode   = (PHOIT_RAW_INODE)__SHEAP_ALLOC(sizeof(HOIT_RAW_INODE));
    PHOIT_RAW_INFO     pRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));
    PHOIT_INODE_CACHE   pInodeCache = (PHOIT_INODE_CACHE)__SHEAP_ALLOC(sizeof(HOIT_INODE_CACHE));
    PHOIT_FULL_DIRENT   pFullDirent = (PHOIT_FULL_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_FULL_DIRENT));
    CPCHAR      pcFileName;

    if (pRawInfo == LW_NULL || pRawInode == LW_NULL || pInodeCache == LW_NULL || pFullDirent == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    lib_bzero(pRawInode, sizeof(HOIT_RAW_INODE));
    lib_bzero(pRawInfo, sizeof(HOIT_RAW_INFO));
    lib_bzero(pInodeCache, sizeof(HOIT_INODE_CACHE));
    lib_bzero(pFullDirent, sizeof(HOIT_FULL_DIRENT));

    pcFileName = lib_rindex(pcName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    }
    else {
        pcFileName = pcName;
    }

    pRawInode->file_type = mode;
    pRawInode->ino = __hoit_alloc_ino(pfs);
    pRawInode->magic_num = HOIT_MAGIC_NUM;
    pRawInode->totlen = sizeof(HOIT_RAW_INODE);
    pRawInode->flag = HOIT_FLAG_TYPE_INODE | HOIT_FLAG_OBSOLETE;

    UINT phys_addr;
    __hoit_write_flash(pfs, (PVOID)pRawInode, sizeof(HOIT_RAW_INODE), &phys_addr);
   
    pRawInfo->phys_addr = phys_addr;
    pRawInfo->totlen = sizeof(HOIT_RAW_INODE);

    pInodeCache->HOITC_ino = pRawInode->ino;

    __hoit_add_to_inode_cache(pInodeCache, pRawInfo);
    __hoit_add_to_cache_list(pfs, pInodeCache);


    pFullDirent->HOITFD_file_name = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);
    if (pFullDirent->HOITFD_file_name == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_strcpy(pFullDirent->HOITFD_file_name, pcFileName);
    pFullDirent->HOITFD_file_type = mode;
    pFullDirent->HOITFD_ino = pRawInode->ino;
    pFullDirent->HOITFD_nhash = __hoit_name_hash(pcFileName);
    pFullDirent->HOITFD_pino = pInodeFather->HOITN_ino;

    __hoit_add_dirent(pInodeFather, pFullDirent);

    __SHEAP_FREE(pRawInode);
    /*
    *   �Ѿ������ļ����ó���һ���Ѿ����ڵ��ļ�������ֻ�����get_full_file����
    */
    return  __hoit_get_full_file(pfs, pFullDirent->HOITFD_ino);
}
/*********************************************************************************************************
** ��������: __hoit_unlink_regular
** ��������: HoitFs ����ͨ�ļ���������1������Ӧ��FullDirent���Ϊ���ڣ��������������Ϊ0���ļ���RawInodeҲ������ǹ���
**           �൱�ڱ�����ֻɾ����Ŀ¼�ļ�
**           ע�������������pDirent�����ڸú����ڱ��ͷţ�Ӧ���ɵ��øú������ϼ����������ͷ�
** �䡡��  : pramn            �ļ��ڵ�
** �䡡��  : ɾ�����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoit_unlink_regular(PHOIT_INODE_INFO pInodeFather, PHOIT_FULL_DIRENT  pDirent)
{
    if (pDirent == LW_NULL || S_ISDIR(pDirent->HOITFD_file_type)) {
        _ErrorHandle(ENOTEMPTY);
        return  (PX_ERROR);
    }
    PHOIT_VOLUME pfs = pInodeFather->HOITN_volume;
    PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pDirent->HOITFD_ino);
    pInodeCache->HOITC_nlink -= 1;

    PHOIT_INODE_CACHE pFatherInodeCache = __hoit_get_inode_cache(pfs, pInodeFather->HOITN_ino);
    /*
    *����ɾ����FullDirent��Ӧ��RawInfo��Flash�ϵ�RawDirentɾ��
    */
    PHOIT_RAW_INFO pRawInfo = pDirent->HOITFD_raw_info;
    __hoit_del_raw_info(pFatherInodeCache, pRawInfo);     //��RawInfo��InodeCache��������ɾ��
    __hoit_del_raw_data(pRawInfo);
    __SHEAP_FREE(pRawInfo);
    /*
    *����FullDirent�Ӹ�Ŀ¼�ļ��е�dents����ɾ��
    */
    __hoit_del_full_dirent(pInodeFather, pDirent);

    /*
    *���nlink��Ϊ0���򽫸�InodeCache��Ӧ���ļ�������Flash�ϵ����ݱ��Ϊ���ڲ��ͷŵ��ڴ��е�InodeCache
    */
    if (pInodeCache->HOITC_nlink == 0) {
        PHOIT_RAW_INFO pRawTemp = pInodeCache->HOITC_nodes;
        PHOIT_RAW_INFO pRawNext = LW_NULL;
        while (pRawTemp) {
            __hoit_del_raw_data(pRawTemp);
            pRawNext = pRawTemp->next_phys;
            __SHEAP_FREE(pRawTemp);
            pRawTemp = pRawNext;
        }
    }
    __hoit_del_inode_cache(pfs, pInodeCache);
    __SHEAP_FREE(pInodeCache);
    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __hoit_truncate
** ��������: hoitfs �ض�һ���ļ�
** �䡡��  : pInodeInfo       �ļ��ڵ�
**           offset            �ضϵ�
** �䡡��  : ���̽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_truncate(PHOIT_INODE_INFO  pInodeInfo, size_t  offset)
{
    //************************************ TODO ************************************


    //************************************ END  ************************************
}

/*********************************************************************************************************
** ��������: __hoit_unlink_dir
** ��������: ��һ��Ŀ¼�ļ�ɾ�������������������ļ�����ɾ������ͨ�ļ�����__hoit_unlink_regular����������ļ���Ŀ¼�ļ���ݹ����__hoit_unlink_dir��
**           �൱�ڱ�����ֻɾ��Ŀ¼�ļ�����ramfs��ͬ����������ɾ��Ŀ¼�ļ��µ����ļ�����ɾ��Ŀ¼�ļ�����
**           ע�������������pDirent�����ڸú����ڱ��ͷţ�Ӧ���ɵ��øú������ϼ����������ͷ�
** �䡡��  : pramn            �ļ��ڵ�
** �䡡��  : !=0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoit_unlink_dir(PHOIT_INODE_INFO pInodeFather, PHOIT_FULL_DIRENT  pDirent) {
    if (pDirent == LW_NULL || !S_ISDIR(pDirent->HOITFD_file_type)) {
        _ErrorHandle(ENOTEMPTY);
        return  (PX_ERROR);
    }
    PHOIT_VOLUME pfs = pInodeFather->HOITN_volume;
    PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pDirent->HOITFD_ino);
    

    PHOIT_INODE_CACHE pFatherInodeCache = __hoit_get_inode_cache(pfs, pInodeFather->HOITN_ino);
    /*
    *����ɾ����FullDirent��Ӧ��RawInfo��Flash�ϵ�RawDirentɾ��
    */
    PHOIT_RAW_INFO pRawInfo = pDirent->HOITFD_raw_info;
    __hoit_del_raw_info(pFatherInodeCache, pRawInfo);     //��RawInfo��InodeCache��������ɾ��
    __hoit_del_raw_data(pRawInfo);
    __SHEAP_FREE(pRawInfo);
    /*
    *����FullDirent�Ӹ�Ŀ¼�ļ��е�dents����ɾ�������Ž�FullDirent�ڴ��ͷŵ�
    */
    __hoit_del_full_dirent(pInodeFather, pDirent);
;
    /*
    *Ŀ¼�ļ�nlinkΪ1���ټ�1�ͱ�Ϊ0�ˣ������ȳ���unlink���ļ�����ɾ��Ŀ¼�ļ����������
    */
    if (pInodeCache->HOITC_nlink == 1) {
        //�ȴ�Ŀ¼�ļ�
        PHOIT_INODE_INFO pDirFileInode = __hoit_get_full_file(pfs, pInodeCache->HOITC_ino);
        if (!S_ISDIR(pDirFileInode->HOITN_mode)) return HOIT_ERROR;

        //��һ��unlinkĿ¼�ļ��µ�ÿ�����ļ�
        PHOIT_FULL_DIRENT pFullDirent = pDirFileInode->HOITN_dents;
        PHOIT_FULL_DIRENT pFullDirentNext = LW_NULL;
        while (pFullDirent) {
            pFullDirentNext = pFullDirent->HOITFD_next;
            if (!S_ISDIR(pFullDirent->HOITFD_file_type)) {
                __hoit_unlink_regular(pDirFileInode, pFullDirent);
            }
            else {
                __hoit_unlink_dir(pDirFileInode, pFullDirent);
            }
            __hoit_free_full_dirent(pFullDirent);
            pFullDirent = pFullDirentNext;
        }

        //ÿ��Ŀ¼�ļ���һ���Լ���RawInode��Ҫ�����Լ�ɾ��
        PHOIT_RAW_INFO pRawTemp = pInodeCache->HOITC_nodes;
        PHOIT_RAW_INFO pRawNext = LW_NULL;
        while (pRawTemp) {
            __hoit_del_raw_data(pRawTemp);
            pRawNext = pRawTemp->next_phys;
            __SHEAP_FREE(pRawTemp);
            pRawTemp = pRawNext;
        }

        __hoit_del_inode_cache(pfs, pInodeCache);
        __SHEAP_FREE(pDirFileInode);
        __SHEAP_FREE(pInodeCache);
    }
    pInodeCache->HOITC_nlink -= 1;
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __hoit_move_check
** ��������: HoitFs ���ڶ����ڵ��Ƿ�Ϊ��һ���ڵ������
** �䡡��  : pInode1       ��һ���ڵ�
**           pInode2       �ڶ����ڵ�
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoit_move_check(PHOIT_INODE_INFO  pInode1, PHOIT_INODE_INFO  pInode2)
{
    PHOIT_VOLUME pfs = pInode1->HOITN_volume;
    PHOIT_INODE_CACHE pCache1 = pInode1->HOITN_inode_cache;
    PHOIT_INODE_CACHE pCache2 = pInode2->HOITN_inode_cache;
    do {
        if (pCache1->HOITC_ino == pCache2->HOITC_ino) {
            return  (PX_ERROR);
        }
        pCache2 = __hoit_get_inode_cache(pfs, pCache2->HOITC_ino);
    } while (pCache2);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __hoit_move
** ��������: HoitFs �ƶ�����������һ���ļ�
** �䡡��  : pInodeFather     �ļ��ĸ�Ŀ¼�ڵ㣨move֮ǰ�ģ�
**           pInodeInfo       �ļ��ڵ�
**           pcNewName        �µ�����
** �䡡��  : ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoit_move(PHOIT_INODE_INFO pInodeFather, PHOIT_INODE_INFO  pInodeInfo, PCHAR  pcNewName)
{
    INT         iRet;
    PHOIT_VOLUME pfs;
    PHOIT_INODE_INFO   pInodeTemp;
    PHOIT_INODE_INFO   pInodeNewFather;
    BOOL        bRoot;
    BOOL        bLast;
    PCHAR       pcTail;
    PCHAR       pcTemp;
    PCHAR       pcFileName;

    pfs = pInodeInfo->HOITN_volume;

    pInodeTemp = __hoit_open(pfs, pcNewName, &pInodeNewFather, &bRoot, &bLast, &pcTail);
    if (!pInodeTemp && (bRoot || (bLast == LW_FALSE))) {                 /*  ������ָ�������û��Ŀ¼    */
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pInodeInfo == pInodeTemp) {                                           /*  ��ͬ                        */
        return  (ERROR_NONE);
    }

    if (S_ISDIR(pInodeInfo->HOITN_mode) && pInodeNewFather) {
        if (__hoit_move_check(pInodeInfo, pInodeNewFather)) {                  /*  ���Ŀ¼�Ϸ���              */
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }
    }
    pcFileName = lib_rindex(pcNewName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    }
    else {
        pcFileName = pcNewName;
    }

    pcTemp = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);          /*  Ԥ�������ֻ���              */
    if (pcTemp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_strcpy(pcTemp, pcFileName);

    if (pInodeTemp) {
        if (!S_ISDIR(pInodeInfo->HOITN_mode) && S_ISDIR(pInodeTemp->HOITN_mode)) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(EISDIR);
            return  (PX_ERROR);
        }
        if (S_ISDIR(pInodeInfo->HOITN_mode) && !S_ISDIR(pInodeTemp->HOITN_mode)) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }

        PHOIT_FULL_DIRENT pFullDirent = __hoit_search_in_dents(pInodeNewFather, pInodeTemp->HOITN_ino);
        if (pFullDirent == LW_NULL) {
            __SHEAP_FREE(pcTemp);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        if (S_ISDIR(pInodeTemp->HOITN_mode)) {
            iRet = __hoit_unlink_dir(pInodeNewFather, pFullDirent);     /*  ɾ��Ŀ��                    */
        }
        else {
            iRet = __hoit_unlink_regular(pInodeNewFather, pFullDirent); /*  ɾ��Ŀ��                    */
        }
        __hoit_free_full_dirent(pFullDirent);
        

        if (iRet) {
            __SHEAP_FREE(pcTemp);
            return  (PX_ERROR);
        }
    }

    if (pInodeFather != pInodeNewFather) {                              /*  Ŀ¼�����ı�                */
        PHOIT_FULL_DIRENT pFullDirent = (PHOIT_FULL_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_FULL_DIRENT));
        pFullDirent->HOITFD_file_name = pcTemp;
        pFullDirent->HOITFD_file_type = pInodeInfo->HOITN_mode;
        pFullDirent->HOITFD_ino = pInodeInfo->HOITN_ino;
        pFullDirent->HOITFD_nhash = __hoit_name_hash(pcTemp);
        pFullDirent->HOITFD_pino = pInodeNewFather->HOITN_ino;
        __hoit_add_dirent(pInodeNewFather, pFullDirent);

        PHOIT_FULL_DIRENT pOldDirent = __hoit_search_in_dents(pInodeFather, pInodeInfo->HOITN_ino);
        __hoit_del_full_dirent(pInodeFather, pOldDirent);
    }
    else {
        PHOIT_FULL_DIRENT pOldDirent = __hoit_search_in_dents(pInodeFather, pInodeInfo->HOITN_ino);
        __hoit_del_full_dirent(pInodeFather, pOldDirent);

        __SHEAP_FREE(pOldDirent->HOITFD_file_name);
        pOldDirent->HOITFD_file_name = pcTemp;

        __hoit_add_to_dents(pInodeFather, pOldDirent);
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __hoit_stat
** ��������: ��һ���򿪵��ļ��ж�ȡ��Ӧ���ݵ�stat�ṹ��
** �䡡��  : pInodeInfo       �ļ��ڵ�
** �䡡��  : !=0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoit_stat(PHOIT_INODE_INFO pInodeInfo, PHOIT_VOLUME  pfs, struct stat* pstat) {
    if (pInodeInfo) {
        pstat->st_dev = LW_DEV_MAKE_STDEV(&pfs->HOITFS_devhdrHdr);
        pstat->st_ino = (ino_t)pInodeInfo;
        pstat->st_mode = pInodeInfo->HOITN_mode;
        pstat->st_nlink = pInodeInfo->HOITN_inode_cache->HOITC_ino;
        pstat->st_uid = pInodeInfo->HOITN_uid;
        pstat->st_gid = pInodeInfo->HOITN_gid;
        pstat->st_rdev = 1;
        // pstat->st_size = (off_t)pInodeInfo->HOITN_stSize;
        // pstat->st_atime = pInodeInfo->HOITN_timeAccess;
        // pstat->st_mtime = pInodeInfo->HOITN_timeChange;
        // pstat->st_ctime = pInodeInfo->HOITN_timeCreate;
        pstat->st_blksize = 0;
        pstat->st_blocks = 0;
    }
    else {
        pstat->st_dev = LW_DEV_MAKE_STDEV(&pfs->HOITFS_devhdrHdr);
        pstat->st_ino = (ino_t)0;
        pstat->st_mode = pfs->HOITFS_mode;
        pstat->st_nlink = 1;
        pstat->st_uid = pfs->HOITFS_uid;
        pstat->st_gid = pfs->HOITFS_gid;
        pstat->st_rdev = 1;
        pstat->st_size = 0;
        pstat->st_atime = pfs->HOITFS_time;
        pstat->st_mtime = pfs->HOITFS_time;
        pstat->st_ctime = pfs->HOITFS_time;
        pstat->st_blksize = 0;
        pstat->st_blocks = 0;
    }

    pstat->st_resv1 = LW_NULL;
    pstat->st_resv2 = LW_NULL;
    pstat->st_resv3 = LW_NULL;
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __hoit_statfs
** ��������: ��ȡ�ļ�ϵͳ�����Ϣ��pstatfs
** �䡡��  : pInodeInfo       �ļ��ڵ�
** �䡡��  : !=0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoit_statfs(PHOIT_VOLUME  pfs, struct statfs* pstatfs) {
    pstatfs->f_type = TMPFS_MAGIC;  //��Ҫ�޸�
    pstatfs->f_bsize = 0;
    pstatfs->f_blocks = 0;
    pstatfs->f_bfree = 0;
    pstatfs->f_bavail = 1;

    pstatfs->f_files = 0;
    pstatfs->f_ffree = 0;

#if LW_CFG_CPU_WORD_LENGHT == 64
    pstatfs->f_fsid.val[0] = (int32_t)((addr_t)pfs >> 32);
    pstatfs->f_fsid.val[1] = (int32_t)((addr_t)pfs & 0xffffffff);
#else
    pstatfs->f_fsid.val[0] = (int32_t)pfs;
    pstatfs->f_fsid.val[1] = 0;
#endif

    pstatfs->f_flag = 0;
    pstatfs->f_namelen = PATH_MAX;
    return ERROR_NONE;
}
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
#endif //HOITFSLIB_DISABLE

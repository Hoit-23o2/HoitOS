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
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 03 �� 20 ��
**
** ��        ��: Hoit�ļ�ϵͳ�ڲ�����.
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
//#include "../SylixOS/kernel/include/k_kernel.h"
//#include "../SylixOS/system/include/s_system.h"
//#include "../SylixOS/fs/include/fs_fs.h"
//#include "hoitFsTree.h"
#include "hoitFsLib.h"
#include "hoitFsTree.h"
#include "hoitFsFDLib.h"
#include "hoitFsCache.h"
#include "hoitFsCmd.h"
#include "hoitFsLog.h"
#include "hoitMergeBuffer.h"
#include "../../driver/mtd/nor/nor.h"
#include "../tools/crc/crc32.h"
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
#if LW_CFG_MAX_VOLUMES > 0

#ifndef HOITFSLIB_DISABLE
#define NS hoitFsLib
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
        pcName++;
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
        return LW_NULL;
    }

    if (ino == HOIT_ROOT_DIR_INO && pfs->HOITFS_pRootDir) {
        return pfs->HOITFS_pRootDir;
    }

    PHOIT_INODE_CACHE pcache = pfs->HOITFS_cache_list;

    while (pcache && pcache->HOITC_ino != ino) {
        pcache = pcache->HOITC_next;
    }
    if (pcache == LW_NULL) {
        return LW_NULL;
    }
    PHOIT_FULL_DIRENT   pDirentList = LW_NULL;
    PHOIT_FULL_DNODE    pDnodeList  = LW_NULL;
    PHOIT_FULL_DIRENT*  ppDirentList = &pDirentList;
    PHOIT_FULL_DNODE*   ppDnodeList  = &pDnodeList;

    __hoit_get_inode_nodes(pfs, pcache, ppDirentList, ppDnodeList);
    if (*ppDnodeList == LW_NULL) {
        return LW_NULL;
    }
    if (S_ISDIR((*ppDnodeList)->HOITFD_file_type)) {
        PHOIT_INODE_INFO pNewInode = (PHOIT_INODE_INFO)__SHEAP_ALLOC(sizeof(HOIT_INODE_INFO));
        pNewInode->HOITN_dents = *ppDirentList;
        pNewInode->HOITN_metadata = *ppDnodeList;
        pNewInode->HOITN_ino = ino;
        pNewInode->HOITN_inode_cache = pcache;
        pNewInode->HOITN_mode = (*ppDnodeList)->HOITFD_file_type;
        pNewInode->HOITN_rbtree = (PVOID)LW_NULL;
        pNewInode->HOITN_volume = pfs;
        pNewInode->HOITN_pcLink = LW_NULL;
        pNewInode->HOITN_pMergeBuffer = LW_NULL;
        return pNewInode;
    }
    else if (S_ISLNK((*ppDnodeList)->HOITFD_file_type)) {   /* �����ļ� */
        PHOIT_INODE_INFO pNewInode = (PHOIT_INODE_INFO)__SHEAP_ALLOC(sizeof(HOIT_INODE_INFO));
        pNewInode->HOITN_dents = LW_NULL;
        pNewInode->HOITN_metadata = *ppDnodeList;
        pNewInode->HOITN_ino = ino;
        pNewInode->HOITN_inode_cache = pcache;
        pNewInode->HOITN_mode = (*ppDnodeList)->HOITFD_file_type;
        pNewInode->HOITN_rbtree = (PVOID)LW_NULL;
        pNewInode->HOITN_volume = pfs;
        pNewInode->HOITN_pcLink = __hoit_get_data_after_raw_inode(pfs, pNewInode->HOITN_metadata->HOITFD_raw_info);
        pNewInode->HOITN_pMergeBuffer = LW_NULL;
        return pNewInode;
    }
    else {
        PHOIT_INODE_INFO pNewInode = (PHOIT_INODE_INFO)__SHEAP_ALLOC(sizeof(HOIT_INODE_INFO));
        pNewInode->HOITN_rbtree = hoitInitFragTree(pfs);
        PHOIT_FULL_DNODE pTempDnode = *ppDnodeList;
        PHOIT_FULL_DNODE pTempNext = LW_NULL;
        while (pTempDnode) {
            /*�����*/
            PHOIT_FRAG_TREE_NODE pTreeNode = newHoitFragTreeNode(pTempDnode, pTempDnode->HOITFD_length, pTempDnode->HOITFD_offset, pTempDnode->HOITFD_offset);
            hoitFragTreeInsertNode(pNewInode->HOITN_rbtree, pTreeNode);
            pTempNext = pTempDnode->HOITFD_next;
            pTempDnode->HOITFD_next = LW_NULL;
            pTempDnode = pTempNext;
        }
        hoitFragTreeOverlayFixUp(pNewInode->HOITN_rbtree);
        pNewInode->HOITN_dents = LW_NULL;
        pNewInode->HOITN_ino = ino;
        pNewInode->HOITN_inode_cache = pcache;
        pNewInode->HOITN_metadata = LW_NULL;
        pNewInode->HOITN_mode = (*ppDnodeList)->HOITFD_file_type;
        pNewInode->HOITN_volume = pfs;
        pNewInode->HOITN_pcLink = LW_NULL;
        pNewInode->HOITN_pMergeBuffer = LW_NULL;
        __hoit_new_merge_buffer(pNewInode); /* 07-18 ���ӳ�ʼ��WriteBuffer By HZS */

        PHOIT_FRAG_TREE_LIST_HEADER pFTlistHeader = hoitFragTreeCollectRange(pNewInode->HOITN_rbtree, 0, INT_MAX);
        PHOIT_FRAG_TREE_LIST_NODE pFTlistNode = pFTlistHeader->pFTlistHeader->pFTlistNext;
        size_t HOITN_stSize = 0;
        while (pFTlistNode)
        {
           HOITN_stSize = __MAX(HOITN_stSize, pFTlistNode->pFTn->pFDnode->HOITFD_offset + pFTlistNode->pFTn->pFDnode->HOITFD_length);
           pFTlistNode = pFTlistNode->pFTlistNext;
        }

        pNewInode->HOITN_stSize = HOITN_stSize;
        return pNewInode;
    }

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
        printk("Error in hoit_get_inode_cache\n");
        return LW_NULL;
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
    PHOIT_FULL_DIRENT pSonDirent, UINT needLog)
{
    
    UINT totlen = sizeof(HOIT_RAW_DIRENT) + lib_strlen(pSonDirent->HOITFD_file_name);
    PCHAR pWriteBuf = (PCHAR)__SHEAP_ALLOC(totlen);
    PHOIT_RAW_DIRENT    pRawDirent = (PHOIT_RAW_DIRENT)pWriteBuf;
    PHOIT_RAW_INFO      pRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));

    if (pWriteBuf == LW_NULL || pRawInfo == LW_NULL || pFatherInode == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return;
    }
    PHOIT_VOLUME pfs = pFatherInode->HOITN_volume;

    lib_bzero(pWriteBuf, totlen);
    lib_bzero(pRawInfo, sizeof(HOIT_RAW_INFO));

    pRawDirent->file_type = pSonDirent->HOITFD_file_type;
    pRawDirent->ino = pSonDirent->HOITFD_ino;
    pRawDirent->magic_num = HOIT_MAGIC_NUM;
    pRawDirent->pino = pSonDirent->HOITFD_pino;
    pRawDirent->totlen = sizeof(HOIT_RAW_DIRENT) + lib_strlen(pSonDirent->HOITFD_file_name);
    pRawDirent->flag = HOIT_FLAG_NOT_OBSOLETE | HOIT_FLAG_TYPE_DIRENT;
    pRawDirent->version = pfs->HOITFS_highest_version++;

    PCHAR pFileName = pWriteBuf + sizeof(HOIT_RAW_DIRENT);
    lib_memcpy(pFileName, pSonDirent->HOITFD_file_name, lib_strlen(pSonDirent->HOITFD_file_name));

    UINT phys_addr = 0;

    pRawDirent->crc = 0;
    pRawDirent->crc = crc32_le(pWriteBuf, totlen);
    __hoit_write_flash(pfs, (PVOID)pWriteBuf, totlen, &phys_addr, needLog);

    pRawInfo->phys_addr = phys_addr;
    pRawInfo->totlen = pRawDirent->totlen;
    pRawInfo->next_logic = LW_NULL;
    pRawInfo->next_phys = LW_NULL;
    pRawInfo->is_obsolete = HOIT_FLAG_NOT_OBSOLETE;

    PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pFatherInode->HOITN_ino);

    __hoit_add_to_inode_cache(pInodeCache, pRawInfo);
    __hoit_add_raw_info_to_sector(pfs->HOITFS_now_sector, pRawInfo);
    pSonDirent->HOITFD_raw_info = pRawInfo;
    if (__hoit_add_to_dents(&(pFatherInode->HOITN_dents), pSonDirent)) {
        __SHEAP_FREE(pSonDirent->HOITFD_file_name);
        __SHEAP_FREE(pSonDirent);
        __SHEAP_FREE(pWriteBuf);
        return;
    }

    PHOIT_INODE_CACHE pSonInodeCache = __hoit_get_inode_cache(pfs, pSonDirent->HOITFD_ino);
    pSonInodeCache->HOITC_nlink++;

    __SHEAP_FREE(pWriteBuf);
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
        printk("Error in hoit_alloc_ino\n");
        return HOIT_ERROR;
    }
    return pfs->HOITFS_highest_ino++;
}
/*********************************************************************************************************
** ��������: __hoit_read_flash
** ��������: ��ȡ�����豸
** �䡡��  :
** �䡡��  : !=0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_read_flash(PHOIT_VOLUME pfs, UINT phys_addr, PVOID pdata, UINT length) {
    hoitReadFromCache(pfs->HOITFS_cacheHdr, phys_addr, pdata, length);
}

/*********************************************************************************************************
** ��������: __hoit_write_flash
** ��������: д�������豸�������Լ�ѡ�����ַ
** �䡡��  :
** �䡡��  : !=0�������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_write_flash(PHOIT_VOLUME pfs, PVOID pdata, UINT length, UINT* phys_addr, UINT needLog) {
    /*
    write_nor(pfs->HOITFS_now_sector->HOITS_offset + pfs->HOITFS_now_sector->HOITS_addr, (PCHAR)(pdata), length, WRITE_KEEP);
    if (phys_addr != LW_NULL) {
        *phys_addr = pfs->HOITFS_now_sector->HOITS_offset + pfs->HOITFS_now_sector->HOITS_addr;
    }
    pfs->HOITFS_now_sector->HOITS_offset += length;
    */
    if(needLog)
        hoitLogAppend(pfs, pdata, length);
    UINT temp_addr = hoitWriteToCache(pfs->HOITFS_cacheHdr, pdata, length);
    if(phys_addr){
        *phys_addr = temp_addr;
    }
    if (temp_addr == PX_ERROR) {
        printf("Error in write flash\n");
        return PX_ERROR;
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
    //write_nor(phys_addr, (PCHAR)(pdata), length, WRITE_KEEP);
    hoitWriteThroughCache(pfs->HOITFS_cacheHdr, phys_addr, pdata, length);
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
        printk("Error in %s\n", __func__);
        return HOIT_ERROR;
    }
    pRawInfo->next_logic = pInodeCache->HOITC_nodes;
    pInodeCache->HOITC_nodes = pRawInfo;
    return 0;
}
/*********************************************************************************************************
** ��������: __hoit_add_to_cache_list
** ��������: ��һ��inode cache���뵽cache_list��
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
    //pInodeCache->HOITC_nodes = LW_NULL;
    pInodeCache->HOITC_next = pfs->HOITFS_cache_list;
    pfs->HOITFS_cache_list = pInodeCache;
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
UINT8 __hoit_add_to_dents(PHOIT_FULL_DIRENT* ppFatherDents, PHOIT_FULL_DIRENT pFullDirent) {
    if (ppFatherDents == LW_NULL || pFullDirent == LW_NULL) {
        printk("Error in hoit_add_to_dents\n");
        return HOIT_ERROR;
    }

    PHOIT_FULL_DIRENT pDirent = *ppFatherDents;
    while (pDirent) {
        if ((pDirent->HOITFD_ino == pFullDirent->HOITFD_ino) && (lib_strcmp(pDirent->HOITFD_file_name, pFullDirent->HOITFD_file_name) == 0)) {
            if (pDirent->HOITFD_version > pFullDirent->HOITFD_version) {
                return HOIT_ERROR;
            }
            else {
                __hoit_del_full_dirent(ppFatherDents, pDirent);
                break;
            }
        }
        pDirent = pDirent->HOITFD_next;
    }

    pFullDirent->HOITFD_next = *ppFatherDents;
    *ppFatherDents = pFullDirent;
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
PHOIT_FULL_DIRENT __hoit_search_in_dents(PHOIT_INODE_INFO pInodeFather, UINT ino, PCHAR pName) {
    if (pInodeFather == LW_NULL) {
        printk("Error in hoit_search_in_dents\n");
        return LW_NULL;
    }

    PCHAR pcFileName = lib_rindex(pName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    }
    else {
        pcFileName = pName;
    }

    UINT nHash = __hoit_name_hash(pcFileName);
    PHOIT_FULL_DIRENT pDirent = pInodeFather->HOITN_dents;
    while (pDirent) {
        if (pDirent->HOITFD_ino == ino) {
            UINT nTempHash = __hoit_name_hash(pDirent->HOITFD_file_name);
            if (nTempHash == nHash) {
                break;
            }
        }
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
        printk("Error in hoit_del_raw_info\n");
        return HOIT_ERROR;
    }
    if (pInodeCache->HOITC_nodes == pRawInfo) {
        pInodeCache->HOITC_nodes = pRawInfo->next_logic;
        return 0;
    }
    else {
        PHOIT_RAW_INFO pRawTemp = pInodeCache->HOITC_nodes;
        while (pRawTemp && pRawTemp->next_logic != pRawInfo) {
            pRawTemp = pRawTemp->next_logic;
        }
        if (pRawTemp) {
            pRawTemp->next_logic = pRawInfo->next_logic;
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
UINT8 __hoit_del_raw_data(PHOIT_VOLUME pfs, PHOIT_RAW_INFO pRawInfo) {
    if (pRawInfo == LW_NULL) {
        printk("Error in hoit_del_raw_data\n");
        return HOIT_ERROR;
    }

    PCHAR buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);
    lib_bzero(buf, pRawInfo->totlen);

    __hoit_read_flash(pfs, pRawInfo->phys_addr, buf, pRawInfo->totlen);

    PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)buf;
    crc32_check(pRawHeader);

    if (pRawHeader->magic_num != HOIT_MAGIC_NUM || (pRawHeader->flag & HOIT_FLAG_NOT_OBSOLETE) == 0) {
        printk("Error in hoit_del_raw_data\n");
        return HOIT_ERROR;
    }
    //!pRawHeader->flag &= (~HOIT_FLAG_NOT_OBSOLETE);      //��obsolete��־��Ϊ0���������
    __hoit_mark_obsolete(pfs, pRawHeader, pRawInfo);
    // /* ��obsolete��־λ��0��д��ԭ��ַ */
    // pRawHeader->crc = 0;
    // pRawHeader->crc = crc32_le(pRawHeader, pRawInfo->totlen);
    // __hoit_write_flash_thru(pfs, (PVOID)pRawHeader, pRawInfo->totlen, pRawInfo->phys_addr);

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
UINT8 __hoit_del_full_dirent(PHOIT_FULL_DIRENT* ppFatherDents, PHOIT_FULL_DIRENT pFullDirent) {
    if (ppFatherDents == LW_NULL || pFullDirent == LW_NULL) {
        printk("Error in hoit_del_full_dirent\n");
        return HOIT_ERROR;
    }
    if (*ppFatherDents == pFullDirent) {
        *ppFatherDents = pFullDirent->HOITFD_next;
        return 0;
    }
    else {
        PHOIT_FULL_DIRENT pFullTemp = *ppFatherDents;
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
** ��������: __hoit_get_inode_nodes
** ��������: �������ͨ�ļ����򷵻�һ��FullDnode��������δ��ɺ������
**           �����Ŀ¼�ļ����򷵻�һ��FullDirent������Ҳ�ǻ�δ�����ʽ�ṹ��
** �䡡��  :
** �䡡��  : !=0 �������, pDirentList�Ƿ��ص�FullDirent����, pDnodeList�Ƿ��ص�FullDnode����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
UINT8 __hoit_get_inode_nodes(PHOIT_VOLUME pfs, PHOIT_INODE_CACHE pInodeInfo, PHOIT_FULL_DIRENT* ppDirentList, PHOIT_FULL_DNODE* ppDnodeList) {
    PHOIT_RAW_INFO pRawInfo = pInodeInfo->HOITC_nodes;
    while (pRawInfo) {
        PCHAR pBuf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);
        
        __hoit_read_flash(pfs, pRawInfo->phys_addr, pBuf, pRawInfo->totlen);
        PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)pBuf;
        crc32_check(pRawHeader);

        if (!__HOIT_IS_OBSOLETE(pRawHeader)) {
            if (__HOIT_IS_TYPE_DIRENT(pRawHeader)) {
                PHOIT_RAW_DIRENT pRawDirent = (PHOIT_RAW_DIRENT)pRawHeader;
                PHOIT_FULL_DIRENT pFullDirent = (PHOIT_FULL_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_FULL_DIRENT));

                pFullDirent->HOITFD_file_type = pRawDirent->file_type;
                pFullDirent->HOITFD_ino = pRawDirent->ino;
                pFullDirent->HOITFD_pino = pRawDirent->pino;
                pFullDirent->HOITFD_raw_info = pRawInfo;
                pFullDirent->HOITFD_version = pRawDirent->version;
                pFullDirent->HOITFD_file_name = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen - sizeof(HOIT_RAW_DIRENT) + 1);   // ����Ҫ��1���'\0', ��Ϊ��flash�д洢���ļ���'
                lib_bzero(pFullDirent->HOITFD_file_name, pRawInfo->totlen - sizeof(HOIT_RAW_DIRENT) + 1);

                PCHAR pRawName = ((PCHAR)pRawDirent) + sizeof(HOIT_RAW_DIRENT);
                lib_memcpy(pFullDirent->HOITFD_file_name, pRawName, pRawInfo->totlen - sizeof(HOIT_RAW_DIRENT));
                if (__hoit_add_to_dents(ppDirentList, pFullDirent)) {
                    __SHEAP_FREE(pFullDirent->HOITFD_file_name);
                    __SHEAP_FREE(pFullDirent);
                }
            }
            else {
                PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)pRawHeader;
                PHOIT_FULL_DNODE pFullDnode = (PHOIT_FULL_DNODE)__SHEAP_ALLOC(sizeof(HOIT_FULL_DNODE));
                pFullDnode->HOITFD_length = pRawInfo->totlen - sizeof(HOIT_RAW_INODE);
                pFullDnode->HOITFD_offset = pRawInode->offset;
                pFullDnode->HOITFD_raw_info = pRawInfo;
                pFullDnode->HOITFD_next = *ppDnodeList;
                pFullDnode->HOITFD_file_type = pRawInode->file_type;
                pFullDnode->HOITFD_version = pRawInode->version;
                *ppDnodeList = pFullDnode;
            }
        }
        __SHEAP_FREE(pBuf);
        pRawInfo = pRawInfo->next_logic;
    }
    return ERROR_NONE;
}
/*********************************************************************************************************
** ��������: __hoit_add_to_sector_list
** ��������: ���һ��sector��volume
** �䡡��  :
** �䡡��  : 
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_add_to_sector_list(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector) {
    // TODO:��Ȼ������ԭʼ�б�������µ�sector�б�
    pErasableSector->HOITS_next = pfs->HOITFS_erasableSectorList;
    pfs->HOITFS_erasableSectorList = pErasableSector;
    //!TODO: mount��ʼ��������װ����ת�ƺ���
    __hoit_fix_up_sector_list(pfs, pErasableSector);
    return LW_TRUE;
}
/*********************************************************************************************************
** ��������: __hoit_scan_single_sector
** ��������: ɨ��һ��������
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_scan_single_sector(ScanThreadAttr* pThreadAttr) {
    PHOIT_VOLUME pfs    = pThreadAttr->pfs;
    UINT8 sector_no     = pThreadAttr->sector_no;

    UINT                    uiSectorSize;         
    UINT                    uiSectorOffset;
    UINT                    uiFreeSize;
    UINT                    uiUsedSize;
    PHOIT_ERASABLE_SECTOR   pErasableSector;
    UINT                    uiSectorNum;


    uiSectorSize            = pfs->HOITFS_cacheHdr->HOITCACHE_blockSize;
    uiSectorOffset          = hoitGetSectorOffset(sector_no);
    uiFreeSize              = uiSectorSize;
    uiUsedSize              = 0;

    /* �ȴ���sector�ṹ�� */
    pErasableSector = (PHOIT_ERASABLE_SECTOR)__SHEAP_ALLOC(sizeof(HOIT_ERASABLE_SECTOR));
    pErasableSector->HOITS_bno              = sector_no;
    pErasableSector->HOITS_length           = uiSectorSize;
    pErasableSector->HOITS_addr             = uiSectorOffset;
    pErasableSector->HOITS_offset           = 0;
    pErasableSector->HOITS_pRawInfoFirst    = LW_NULL;
    pErasableSector->HOITS_pRawInfoLast     = LW_NULL;
    pErasableSector->HOITS_pRawInfoCurGC    = LW_NULL;
    pErasableSector->HOITS_pRawInfoPrevGC   = LW_NULL;
    pErasableSector->HOITS_pRawInfoLastGC   = LW_NULL;
    pErasableSector->HOITS_tBornAge         = API_TimeGet(); 
    
    LW_SPIN_INIT(&pErasableSector->HOITS_lock);         /* ��ʼ��SPIN LOCK */

    // ugly now
    if(pfs->HOITFS_now_sector == LW_NULL){
        pfs->HOITFS_now_sector = pErasableSector;
    }

    /* �����������ɨ�� */
    PCHAR pReadBuf = (PCHAR)__SHEAP_ALLOC(uiSectorSize);
    lib_bzero(pReadBuf, uiSectorSize);

    __hoit_read_flash(pfs, uiSectorOffset, pReadBuf, uiSectorSize);

    /* 2021-07-10 Modified by HZS */
    size_t pageAmount       = uiSectorSize / (HOIT_FILTER_EBS_ENTRY_SIZE + HOIT_FILTER_PAGE_SIZE);
    size_t EBSStartAddr     = HOIT_FILTER_PAGE_SIZE * pageAmount;

    PCHAR pNow              = pReadBuf;

    BOOL EBSMode = 1;   /* EBSMode��ʾ�Ƿ�����EBS�ṹ, Sector��EBS��CRCУ�鲻ͨ����궨��ȡ����EBSModeΪ0 */

#ifndef EBS_ENABLE
    EBSMode=0;
#endif

    if(hoitCheckSectorCRC(pfs->HOITFS_cacheHdr,sector_no) == LW_FALSE) EBSMode = 0;

    BOOL stopFlag   = 0;
    INT sectorIndex = 0;
    UINT32 obsoleteFlag = 0;
    while(1){
        if(EBSMode){
            /* EBSģʽ��, ������溯������ȫ1�����sectorɨ�������ǰ����, obsoleteFlag�����Entry�Ƿ񱻱�ǹ��� */
            UINT32 uSectorOffset = hoitSectorGetNextAddr(pfs->HOITFS_cacheHdr, sector_no, sectorIndex++, &obsoleteFlag);
            pNow = pReadBuf + uSectorOffset;
            if(obsoleteFlag == HOIT_FLAG_OBSOLETE) continue;
            if(uSectorOffset == -1) break;
            if(sectorIndex > pfs->HOITFS_cacheHdr->HOITCACHE_PageAmount) break;
        }else{
            if(pNow > pReadBuf + uiSectorSize) break;
        }


        PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)pNow;
        if(sector_no == 0 
        && pRawHeader->ino != -1
        && pRawHeader->magic_num == HOIT_MAGIC_NUM){
            printf("offs: %d ino: %d\n", uiSectorOffset + (pNow - pReadBuf), pRawHeader->ino);
        }
        if(pRawHeader->magic_num == HOIT_MAGIC_NUM){
            uiUsedSize += pRawHeader->totlen;
            uiFreeSize -= pRawHeader->totlen;
        }
        if (pRawHeader->magic_num == HOIT_MAGIC_NUM && !__HOIT_IS_OBSOLETE(pRawHeader)) {
            /* //TODO:�������ﻹ�����CRCУ�� */
            crc32_check(pRawHeader);
            PHOIT_RAW_INFO pRawInfo = LW_NULL;
            crc32_check(pRawHeader);
            if (__HOIT_IS_TYPE_INODE(pRawHeader)) {
                PHOIT_RAW_INODE     pRawInode   = (PHOIT_RAW_INODE)pNow;

                __HOITFS_VOL_LOCK(pfs);
                PHOIT_INODE_CACHE   pInodeCache = __hoit_get_inode_cache(pfs, pRawInode->ino);
                __HOITFS_VOL_UNLOCK(pfs);

                if (pInodeCache == LW_NULL) {                  /* ����һ��Inode Cache */
                    pInodeCache = (PHOIT_INODE_CACHE)__SHEAP_ALLOC(sizeof(HOIT_INODE_CACHE));
                    if (pInodeCache == LW_NULL) {
                        _ErrorHandle(ENOMEM);
                        return  (PX_ERROR);
                    }
                    pInodeCache->HOITC_ino = pRawInode->ino;
                    pInodeCache->HOITC_nlink = 0;
                    pInodeCache->HOITC_nodes = LW_NULL;

                    __HOITFS_VOL_LOCK(pfs);
                    __hoit_add_to_cache_list(pfs, pInodeCache);
                    __HOITFS_VOL_UNLOCK(pfs);
                }
                pRawInfo                    = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));
                pRawInfo->phys_addr         = uiSectorOffset + (pNow - pReadBuf);
                pRawInfo->totlen            = pRawInode->totlen;
                pRawInfo->is_obsolete       = HOIT_FLAG_NOT_OBSOLETE;
                pRawInfo->next_logic = LW_NULL;
                pRawInfo->next_phys = LW_NULL;

                __hoit_add_to_inode_cache(pInodeCache, pRawInfo);
                __hoit_add_raw_info_to_sector(pErasableSector, pRawInfo);

                if (pRawInode->ino == HOIT_ROOT_DIR_INO) {     /* ���ɨ�赽���Ǹ�Ŀ¼��Ψһ��RawInode */
                    PHOIT_FULL_DNODE pFullDnode = __hoit_bulid_full_dnode(pfs, pRawInfo);
                    PHOIT_INODE_INFO pInodeInfo = (PHOIT_INODE_INFO)__SHEAP_ALLOC(sizeof(HOIT_INODE_INFO));
                    pInodeInfo->HOITN_dents = LW_NULL;
                    pInodeInfo->HOITN_gid = pfs->HOITFS_gid;
                    pInodeInfo->HOITN_ino = HOIT_ROOT_DIR_INO;
                    pInodeInfo->HOITN_inode_cache = pInodeCache;
                    pInodeInfo->HOITN_metadata = pFullDnode;
                    pInodeInfo->HOITN_mode = S_IFDIR;
                    pInodeInfo->HOITN_rbtree = LW_NULL;
                    pInodeInfo->HOITN_uid = pfs->HOITFS_uid;
                    pInodeInfo->HOITN_volume = pfs;

                    pfs->HOITFS_pRootDir = pInodeInfo;

                    if (pfs->HOITFS_pTempRootDirent != LW_NULL) {
                        pInodeInfo->HOITN_dents = pfs->HOITFS_pTempRootDirent;
                        pfs->HOITFS_pTempRootDirent = LW_NULL;
                    }
                }
            }
            else if (__HOIT_IS_TYPE_DIRENT(pRawHeader)) {
                PHOIT_RAW_DIRENT pRawDirent = (PHOIT_RAW_DIRENT)pNow;

                __HOITFS_VOL_LOCK(pfs);
                PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pRawDirent->pino);  /* �����pino����Ŀ¼�ļ��Լ���ino */
                __HOITFS_VOL_UNLOCK(pfs);

                if (pInodeCache == LW_NULL) {
                    pInodeCache = (PHOIT_INODE_CACHE)__SHEAP_ALLOC(sizeof(HOIT_INODE_CACHE));
                    if (pInodeCache == LW_NULL) {
                        _ErrorHandle(ENOMEM);
                        return  (PX_ERROR);
                    }
                    pInodeCache->HOITC_ino = pRawDirent->pino;  /* �����pino����Ŀ¼�ļ��Լ���ino */
                    pInodeCache->HOITC_nlink = 0;
                    pInodeCache->HOITC_nodes = LW_NULL;

                    __HOITFS_VOL_LOCK(pfs);
                    __hoit_add_to_cache_list(pfs, pInodeCache);
                    __HOITFS_VOL_UNLOCK(pfs);
                }
                pRawInfo                = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));
                pRawInfo->phys_addr     = uiSectorOffset + (pNow - pReadBuf);
                pRawInfo->totlen        = pRawDirent->totlen;
                pRawInfo->is_obsolete   = HOIT_FLAG_NOT_OBSOLETE;
                pRawInfo->next_logic = LW_NULL;
                pRawInfo->next_phys = LW_NULL;

                __hoit_add_to_inode_cache(pInodeCache, pRawInfo);
                __hoit_add_raw_info_to_sector(pErasableSector, pRawInfo);

                if (pRawDirent->pino == HOIT_ROOT_DIR_INO) {    /* ���ɨ�赽���Ǹ�Ŀ¼��Ŀ¼�� */
                    __HOITFS_VOL_LOCK(pfs);
                    PHOIT_FULL_DIRENT pFullDirent = __hoit_bulid_full_dirent(pfs, pRawInfo);
                    INT addFail = 0;
                    if (pfs->HOITFS_pRootDir == LW_NULL) {      /* �����Ŀ¼��ΨһRawInode��δɨ�赽 */
                        addFail = __hoit_add_to_dents(&(pfs->HOITFS_pTempRootDirent), pFullDirent);
                    }
                    else {
                        addFail = __hoit_add_to_dents(&(pfs->HOITFS_pRootDir->HOITN_dents), pFullDirent);
                    }

                    if (addFail) {
                        __SHEAP_FREE(pFullDirent->HOITFD_file_name);
                        __SHEAP_FREE(pFullDirent);
                    }
                    __HOITFS_VOL_UNLOCK(pfs);
                }
            }
            
            __HOITFS_VOL_LOCK(pfs);
            if (pRawHeader->ino > pfs->HOITFS_highest_ino)
                pfs->HOITFS_highest_ino = pRawHeader->ino;
            if (pRawHeader->version > pfs->HOITFS_highest_version)
                pfs->HOITFS_highest_version = pRawHeader->version;
            __HOITFS_VOL_UNLOCK(pfs);
            //!��ʼ��pErasableSector�ĸ�����Ϣ, Added by PYQ 2021-04-26
            if (pRawInfo != LW_NULL){

            }
            if(EBSMode){
                ;
            }else{
                pNow += __HOIT_MIN_4_TIMES(pRawHeader->totlen);
            }
        }
        else {
            if(EBSMode){
                ;
            }else{
                pNow += 4;   /* ÿ���ƶ�4�ֽ� */
            }
        }
    }
    pErasableSector->HOITS_uiUsedSize = uiUsedSize;
    pErasableSector->HOITS_uiFreeSize = uiFreeSize;
    pErasableSector->HOITS_offset = uiUsedSize;     /*! Modified By PYQ ����д��ƫ�� */

    __HOITFS_VOL_LOCK(pfs);
    __hoit_add_to_sector_list(pfs, pErasableSector);
    __HOITFS_VOL_UNLOCK(pfs);

    __SHEAP_FREE(pReadBuf);
    __SHEAP_FREE(pThreadAttr);
    return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_new_inode_info
** ��������: hoitfs ����һ���µ��ļ�����Flash��д��һ���򵥵�RawInode
** �䡡��  : 
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_INODE_INFO __hoit_new_inode_info(PHOIT_VOLUME pfs, mode_t mode, CPCHAR pcLink) {
    PHOIT_RAW_INODE     pRawInode = LW_NULL;
    UINT32 totlen = sizeof(HOIT_RAW_INODE);
    CHAR cFullPathName[MAX_FILENAME_LENGTH];

    if (S_ISLNK(mode)) {    /* �����ļ� */
        if (_PathCat(_PathGetDef(), pcLink, cFullPathName) != ERROR_NONE) { /*  ��ôӸ�Ŀ¼��ʼ��·��      */
            _ErrorHandle(ENAMETOOLONG);
            return  LW_NULL;
        }

        pRawInode = (PHOIT_RAW_INODE)__SHEAP_ALLOC(sizeof(HOIT_RAW_INODE) + lib_strlen(cFullPathName));
        totlen = sizeof(HOIT_RAW_INODE) + lib_strlen(cFullPathName);
    }
    else {
        pRawInode = (PHOIT_RAW_INODE)__SHEAP_ALLOC(sizeof(HOIT_RAW_INODE));
    }
    PHOIT_RAW_INFO     pRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));
    PHOIT_INODE_CACHE   pInodeCache = (PHOIT_INODE_CACHE)__SHEAP_ALLOC(sizeof(HOIT_INODE_CACHE));


    if (pRawInfo == LW_NULL || pRawInode == LW_NULL || pInodeCache == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    
    lib_bzero(pRawInode, totlen);
    lib_bzero(pRawInfo, sizeof(HOIT_RAW_INFO));
    lib_bzero(pInodeCache, sizeof(HOIT_INODE_CACHE));


    pRawInode->file_type = mode;
    pRawInode->ino = __hoit_alloc_ino(pfs);
    pRawInode->magic_num = HOIT_MAGIC_NUM;
    pRawInode->totlen = totlen;
    pRawInode->flag = HOIT_FLAG_TYPE_INODE | HOIT_FLAG_NOT_OBSOLETE;
    pRawInode->offset = 0;
    pRawInode->version = pfs->HOITFS_highest_version++;
    pRawInode->crc = 0;
    

    UINT phys_addr;
    if (S_ISLNK(mode)) {
        PCHAR pLink = ((PCHAR)pRawInode) + sizeof(HOIT_RAW_INODE);
        lib_memcpy(pLink, cFullPathName, lib_strlen(cFullPathName));  /* �����ļ� */
        pRawInode->crc = crc32_le(pRawInode, totlen);
        __hoit_write_flash(pfs, (PVOID)pRawInode, totlen, &phys_addr, 1);
    }
    else {
        pRawInode->crc = crc32_le(pRawInode, sizeof(HOIT_RAW_INODE));
        __hoit_write_flash(pfs, (PVOID)pRawInode, sizeof(HOIT_RAW_INODE), &phys_addr, 1);
    }
    
    


    pRawInfo->phys_addr = phys_addr;
    pRawInfo->totlen = totlen;
    pRawInfo->is_obsolete = HOIT_FLAG_NOT_OBSOLETE;

    pInodeCache->HOITC_ino = pRawInode->ino;
    pInodeCache->HOITC_nlink = 0;
    pInodeCache->HOITC_nodes = LW_NULL;
    __hoit_add_to_inode_cache(pInodeCache, pRawInfo);
    __hoit_add_raw_info_to_sector(pfs->HOITFS_now_sector, pRawInfo);
    __hoit_add_to_cache_list(pfs, pInodeCache);

    /*
    *   �Ѿ������ļ����ó���һ���Ѿ����ڵ��ļ�������ֻ�����get_full_file����
    */
    __SHEAP_FREE(pRawInode);

    return  __hoit_get_full_file(pfs, pInodeCache->HOITC_ino);
    
}
/*********************************************************************************************************
** ��������: __hoit_get_nlink
** ��������: hoitfs �������νṹ�ݹ��ȥͳ��ÿ���ļ���������
** �䡡��  :
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoit_get_nlink(PHOIT_INODE_INFO pInodeInfo){
    if (!S_ISDIR(pInodeInfo->HOITN_mode)) return;
    PHOIT_VOLUME pfs = pInodeInfo->HOITN_volume;

    PHOIT_FULL_DIRENT pTempDirent = pInodeInfo->HOITN_dents;
    for (; pTempDirent != LW_NULL; pTempDirent = pTempDirent->HOITFD_next) {
        PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pTempDirent->HOITFD_ino);
        if (pInodeCache == LW_NULL) {
            continue;
        }
        pInodeCache->HOITC_nlink++;
        if (S_ISDIR(pTempDirent->HOITFD_file_type)) {   /* ���ļ���Ŀ¼�ļ�, ��ݹ���ȥ */
            PHOIT_INODE_INFO pTempInode = __hoit_get_full_file(pfs, pInodeCache->HOITC_ino);
            __hoit_get_nlink(pTempInode);
            __hoit_close(pTempInode, 0);
        }
    }
}

/*********************************************************************************************************
** ��������: __hoit_get_data_after_raw_inode
** ��������: hoitfs �õ�Flash�Ͻ�����raw_inode�ĺ��������, ע������ĩβ�����'\0'
** �䡡��  :
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PCHAR __hoit_get_data_after_raw_inode(PHOIT_VOLUME pfs, PHOIT_RAW_INFO pInodeInfo) {
    if (pInodeInfo == LW_NULL) {
        return LW_NULL;
    }

    INT iDataLen = pInodeInfo->totlen - sizeof(HOIT_RAW_INODE);
    PCHAR pTempBuf = (PCHAR)__SHEAP_ALLOC(pInodeInfo->totlen);

    __hoit_read_flash(pfs, pInodeInfo->phys_addr, pTempBuf, pInodeInfo->totlen);
    crc32_check(pTempBuf);

    PCHAR pData = (PCHAR)__SHEAP_ALLOC(iDataLen + 1);
    lib_bzero(pData, iDataLen + 1);
    PCHAR pTempData = pTempBuf + sizeof(HOIT_RAW_INODE);
    lib_memcpy(pData, pTempData, iDataLen);
    __SHEAP_FREE(pTempBuf);
    return pData;
}

/*********************************************************************************************************
** ��������: __hoit_add_raw_info_to_sector
** ��������: hoitfs ��rawInfo���뵽���raw_info������
** �䡡��  :
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoit_add_raw_info_to_sector(PHOIT_ERASABLE_SECTOR pSector, PHOIT_RAW_INFO pRawInfo) {
    INTREG iregInterLevel;
    //API_SpinLockQuick(&pSector->HOITS_lock, &iregInterLevel);
    //TODO: Sector��HOITS_uiObsoleteEntityCount��HOITS_uiAvailableEntityCount��ʼ��λ�ã�
    /*                      
                          |--------------------------------|
                          |     Sector                     |      Sector
                          |    /     \                     |    /       \
        InodeCache----RawInfo(ino)-----RawInfo(ino2)-----RawInfo(ino)-----RawInfo(ino)
                        |                   |               |               |
                ---------------------------------------------------------------------
                   | [H][Entity]       [H][Entity]  |    [H][Entity]   [H][Entity]  |
                ------------------------------------------------------------------
    */
    pRawInfo->next_phys = LW_NULL;
    if(pRawInfo->is_obsolete){
        pSector->HOITS_uiObsoleteEntityCount++;
    }
    else {
        pSector->HOITS_uiAvailableEntityCount++;
    }

    //����ת�ƺ���
#ifdef LIB_DEBUG
    printf("[%s]:add raw info at %p for sector %d\n", __func__, pRawInfo, pSector->HOITS_bno);
#endif // LIB_DEBUG
    if (pSector->HOITS_pRawInfoFirst == LW_NULL) {
        pSector->HOITS_pRawInfoFirst = pRawInfo;
        pSector->HOITS_pRawInfoLast = pRawInfo;
        //API_SpinUnlockQuick(&pSector->HOITS_lock, iregInterLevel);
        return;
    }
    else {
        if (pSector->HOITS_pRawInfoLast == LW_NULL) {
            //API_SpinUnlockQuick(&pSector->HOITS_lock, iregInterLevel);
            return;
        }
        pSector->HOITS_pRawInfoLast->next_phys = pRawInfo;
        pSector->HOITS_pRawInfoLast = pRawInfo;
    }

    //API_SpinUnlockQuick(&pSector->HOITS_lock, iregInterLevel);
}

/*********************************************************************************************************
** ��������: __hoit_move_home
** ��������: hoitfs �������պ�������������Ч���ݵ�rawInfo�Ӿ�sector�ᵽ��sector�ϣ�
**          ע�⣬���ﲻ���޸�ĳ������ʵ���Prevָ�룬����ע�Ᵽ��now_sectorָ�룬
**          �����޸ĸ�ָ�룻
** �䡡��  : pSector����sector
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_move_home(PHOIT_VOLUME pfs, PHOIT_RAW_INFO pRawInfo) {
    //! 2021-05-16 �޸� move home By PYQ������Ӧ����һ���¿���д
    PCHAR                   pReadBuf;
    PHOIT_RAW_LOG           pRawLog;
    pReadBuf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);
    /* �ȶ��������� */
    lib_bzero(pReadBuf, pRawInfo->totlen);
    crc32_check(pReadBuf);

    __hoit_read_flash(pfs, pRawInfo->phys_addr, pReadBuf, pRawInfo->totlen);

    PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)pReadBuf;
    if (pRawHeader->magic_num != HOIT_MAGIC_NUM 
    || (pRawHeader->flag & HOIT_FLAG_NOT_OBSOLETE) == 0) {
        //printf("Error in hoit_move_home\n");
        return LW_FALSE;
    }
    //!pRawHeader->flag &= (~HOIT_FLAG_NOT_OBSOLETE);      //��obsolete��־��Ϊ0���������
    __hoit_mark_obsolete(pfs, pRawHeader, pRawInfo);
    // /* ��obsolete��־λ��0��д��ԭ��ַ */
    // pRawHeader->crc = 0;
    // pRawHeader->crc = crc32_le(pRawHeader, pRawInfo->totlen);
    // __hoit_write_flash_thru(pfs, (PVOID)pRawHeader, pRawInfo->totlen, pRawInfo->phys_addr);
    
    /* ��obsolete��־λ�ָ���д���µ�ַ */
    pRawHeader->flag |= HOIT_FLAG_NOT_OBSOLETE;         //��obsolete��־��Ϊ1������δ����
    UINT phys_addr = 0;

    //!2021-05-16 �޸�now_sectorָ�� modified by PYQ 
    //pfs->HOITFS_now_sector = hoitFindNextToWrite(pfs->HOITFS_cacheHdr, HOIT_CACHE_TYPE_DATA, pRawInfo->totlen);
    
    pRawHeader->crc = 0;
    pRawHeader->crc = crc32_le(pRawHeader, pRawInfo->totlen);
    __hoit_write_flash(pfs, pReadBuf, pRawInfo->totlen, &phys_addr, 1);
    pRawInfo->phys_addr = phys_addr;

    if(__HOIT_IS_TYPE_LOG(pRawHeader)){                         /* �����LOG HDR����ôҪ����logInfo�����Ϣ */
        pRawLog = (PHOIT_RAW_LOG)pRawHeader;
        if (pRawLog->uiLogFirstAddr != -1) {                    /* LOG HDR */
            pfs->HOITFS_logInfo->uiRawLogHdrAddr = pRawInfo->phys_addr;
        }
    }
    /* ��RawInfo�Ӿɿ�ᵽ�¿� */
    __hoit_add_raw_info_to_sector(pfs->HOITFS_now_sector, pRawInfo);
    return LW_TRUE;
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
    PHOIT_INODE_INFO* ppInodeFather,
    PHOIT_FULL_DIRENT* ppFullDirent,
    BOOL* pbRoot,
    BOOL* pbLast,
    PCHAR* ppcTail)
{
    CHAR                pcTempName[MAX_FILENAME_LENGTH];
    PCHAR               pcNext;
    PCHAR               pcNode;

    PHOIT_INODE_INFO    pinodeTemp;

    if (ppInodeFather == LW_NULL) {
        ppInodeFather = &pinodeTemp;                                      /*  ��ʱ����                    */
    }
    *ppInodeFather = LW_NULL;
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
    pInode = pfs->HOITFS_pRootDir;                                      /*  �Ӹ�Ŀ¼��ʼ����            */

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
                    goto    __find_ok;
                }
            }
        }

        if(pDirentTemp == LW_NULL)
            goto __find_error;
        if(S_ISDIR(pDirentTemp->HOITFD_file_type) && pcNext == NULL){
            goto    __find_ok;
        }
        inodeFatherIno = pDirentTemp->HOITFD_ino;                       /*  �ӵ�ǰ�ڵ㿪ʼ����          */
        if (pInode != pfs->HOITFS_pRootDir) {
            __hoit_close(pInode, 0);
        }
        pInode = __hoit_get_full_file(pfs, inodeFatherIno);             /*  �ӵ�һ�����ӿ�ʼ            */
    } while (pcNext);                                                   /*  �������¼�Ŀ¼              */

__find_ok:
    if (ppFullDirent) *ppFullDirent = pDirentTemp;
    if (ppInodeFather) *ppInodeFather = pInode;                            /*  ��ϵ�ڵ�                    */
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
    return  __hoit_get_full_file(pfs, pDirentTemp->HOITFD_ino);

__find_error:
    if (ppFullDirent) *ppFullDirent = pDirentTemp;
    if (ppInodeFather) *ppInodeFather = pInode;                            /*  ��ϵ�ڵ�                    */
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
    PHOIT_INODE_INFO pInodeInfo = LW_NULL;

    // ����socket����Ӳ�����ļ�
    if (S_ISSOCK(mode)) {
        pInodeInfo = __hoit_open(pfs, pcLink, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    }
    else {
        pInodeInfo = __hoit_new_inode_info(pfs, mode, pcLink);
    }
    

    PHOIT_FULL_DIRENT   pFullDirent = (PHOIT_FULL_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_FULL_DIRENT));
    CPCHAR      pcFileName;

    if (pFullDirent == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }

    lib_bzero(pFullDirent, sizeof(HOIT_FULL_DIRENT));

    pcFileName = lib_rindex(pcName, PX_DIVIDER);
    if (pcFileName) {
        pcFileName++;
    }
    else {
        pcFileName = pcName;
    }

    pFullDirent->HOITFD_file_name = (PCHAR)__SHEAP_ALLOC(lib_strlen(pcFileName) + 1);
    lib_bzero(pFullDirent->HOITFD_file_name, lib_strlen(pcFileName) + 1);
    if (pFullDirent->HOITFD_file_name == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (LW_NULL);
    }
    lib_memcpy(pFullDirent->HOITFD_file_name, pcFileName, lib_strlen(pcFileName));

    // ����socket����Ӳ�����ļ�
    if (S_ISSOCK(mode)) {
        pFullDirent->HOITFD_file_type = pInodeInfo->HOITN_mode;
    }
    else {
        pFullDirent->HOITFD_file_type = mode;
    }
    pFullDirent->HOITFD_ino = pInodeInfo->HOITN_ino;
    pFullDirent->HOITFD_nhash = __hoit_name_hash(pcFileName);
    pFullDirent->HOITFD_pino = pInodeFather->HOITN_ino;

    __hoit_add_dirent(pInodeFather, pFullDirent, 1);

    return pInodeInfo;
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
    __hoit_del_raw_data(pfs, pRawInfo);
    pRawInfo->is_obsolete = 1;

    /*
    *����FullDirent�Ӹ�Ŀ¼�ļ��е�dents����ɾ��
    */
    __hoit_del_full_dirent(&(pInodeFather->HOITN_dents), pDirent);

    /*
    *���nlink��Ϊ0���򽫸�InodeCache��Ӧ���ļ�������Flash�ϵ����ݱ��Ϊ���ڲ��ͷŵ��ڴ��е�InodeCache
    */
    if (pInodeCache->HOITC_nlink == 0) {
        PHOIT_RAW_INFO pRawTemp = pInodeCache->HOITC_nodes;
        PHOIT_RAW_INFO pRawNext = LW_NULL;
        while (pRawTemp) {
            __hoit_del_raw_data(pfs, pRawTemp);
            pRawNext = pRawTemp->next_logic;
            pRawTemp->is_obsolete = HOIT_FLAG_OBSOLETE;
            pRawTemp = pRawNext;
        }
        __hoit_del_inode_cache(pfs, pInodeCache);
        __SHEAP_FREE(pInodeCache);
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __hoit_truncate
** ��������: hoitfs �ض�һ���ļ�(ֱ��ɾ������), ע���������ֻ�����ض���ͨ�����ļ�
** �䡡��  : pInodeInfo       �ļ��ڵ�
**           offset            �ضϵ�
** �䡡��  : �ضϽ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_truncate(PHOIT_INODE_INFO  pInodeInfo, size_t  offset)
{
    hoitFragTreeDeleteRange(pInodeInfo->HOITN_rbtree, offset, UINT_MAX, LW_TRUE);
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
    __hoit_del_raw_data(pfs, pRawInfo);
    pRawInfo->is_obsolete = HOIT_FLAG_OBSOLETE;
    /*
    *����FullDirent�Ӹ�Ŀ¼�ļ��е�dents����ɾ�������Ž�FullDirent�ڴ��ͷŵ�
    */
    __hoit_del_full_dirent(&(pInodeFather->HOITN_dents), pDirent);
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
            __hoit_del_raw_data(pfs, pRawTemp);
            pRawNext = pRawTemp->next_logic;
            pRawTemp->is_obsolete = HOIT_FLAG_OBSOLETE;
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
** ��������: __hoit_close
** ��������: hoitfs �ر�һ���ļ�, ���������full_xxx�ṹ��, �����ͷŴ����InodeInfo
** �䡡��  : pInodeInfo           �ļ��ڵ�
**           iFlag            ���ļ�ʱ�ķ���
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_close(PHOIT_INODE_INFO  pInodeInfo, INT  iFlag)
{
    if(pInodeInfo->HOITN_ino == HOIT_ROOT_DIR_INO || pInodeInfo == LW_NULL){ /* ��close��Ŀ¼ */
        return;
    }
    if (S_ISDIR(pInodeInfo->HOITN_mode)) {
        PHOIT_FULL_DIRENT pFullDirent = pInodeInfo->HOITN_dents;
        PHOIT_FULL_DIRENT pFullNext = LW_NULL;
        while (pFullDirent) {
            pFullNext = pFullDirent->HOITFD_next;
            __SHEAP_FREE(pFullDirent->HOITFD_file_name);
            __SHEAP_FREE(pFullDirent);
            pFullDirent = pFullNext;
        }
        if (pInodeInfo->HOITN_metadata != LW_NULL) __SHEAP_FREE(pInodeInfo->HOITN_metadata);
        __SHEAP_FREE(pInodeInfo);
    }
    else if(S_ISLNK(pInodeInfo->HOITN_mode)){
        if (pInodeInfo->HOITN_metadata != LW_NULL) __SHEAP_FREE(pInodeInfo->HOITN_metadata);
        __SHEAP_FREE(pInodeInfo);
    }
    else {
        hoitFragTreeDeleteTree(pInodeInfo->HOITN_rbtree, LW_FALSE);
        __hoit_free_merge_buffer(pInodeInfo);
        if (pInodeInfo->HOITN_metadata != LW_NULL) __SHEAP_FREE(pInodeInfo->HOITN_metadata);
        __SHEAP_FREE(pInodeInfo);
    }
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

    if (!S_ISDIR(pInode1->HOITN_mode)) {
        return (PX_ERROR);
    }
    PHOIT_FULL_DIRENT pFullDirent = pInode1->HOITN_dents;
    while (pFullDirent) {
        if (S_ISDIR(pFullDirent->HOITFD_file_type)) {
            if (pFullDirent->HOITFD_ino == pInode2->HOITN_ino)
                return PX_ERROR;
            PHOIT_INODE_INFO pTempInfo = __hoit_get_full_file(pfs, pFullDirent->HOITFD_ino);
            int checkResult = __hoit_move_check(pTempInfo, pInode2);

            __hoit_close(pTempInfo, 0);
            if (checkResult != ERROR_NONE)
                return PX_ERROR;
        }
        else {
            if (pFullDirent->HOITFD_ino == pInode2->HOITN_ino)
                return PX_ERROR;
        }
    }
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

    pInodeTemp = __hoit_open(pfs, pcNewName, &pInodeNewFather, LW_NULL, &bRoot, &bLast, &pcTail);

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
    lib_bzero(pcTemp, lib_strlen(pcFileName) + 1);
    if (pcTemp == LW_NULL) {
        _ErrorHandle(ENOMEM);
        return  (PX_ERROR);
    }
    lib_memcpy(pcTemp, pcFileName, lib_strlen(pcFileName));
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

        PHOIT_FULL_DIRENT pFullDirent = __hoit_search_in_dents(pInodeNewFather, pInodeTemp->HOITN_ino, pcTemp);
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
        __hoit_add_dirent(pInodeNewFather, pFullDirent, 1);

        PHOIT_FULL_DIRENT pOldDirent = __hoit_search_in_dents(pInodeFather, pInodeInfo->HOITN_ino, pcTemp);
        __hoit_del_full_dirent(&(pInodeFather->HOITN_dents), pOldDirent);
    }
    else {
        PHOIT_FULL_DIRENT pOldDirent = __hoit_search_in_dents(pInodeFather, pInodeInfo->HOITN_ino, pcTemp);
        __hoit_del_full_dirent(&(pInodeFather->HOITN_dents), pOldDirent);

        __SHEAP_FREE(pOldDirent->HOITFD_file_name);
        pOldDirent->HOITFD_file_name = pcTemp;

        if (__hoit_add_to_dents(&(pInodeFather->HOITN_dents), pOldDirent)) {
            __SHEAP_FREE(pOldDirent->HOITFD_file_name);
            __SHEAP_FREE(pOldDirent);
        }

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
        pstat->st_nlink = pInodeInfo->HOITN_inode_cache->HOITC_nlink;
        pstat->st_uid = pInodeInfo->HOITN_uid;
        pstat->st_gid = pInodeInfo->HOITN_gid;
        pstat->st_rdev = 1;
        pstat->st_size = (off_t)pInodeInfo->HOITN_stSize;
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

/*********************************************************************************************************
** ��������: __hoit_read
** ��������: hoitfs ��ȡ�ļ�����
** �䡡��  : 
** �䡡��  : ��ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __hoit_read(PHOIT_INODE_INFO  pInodeInfo, PVOID  pvBuffer, size_t  stSize, size_t  stOft)
{
    ssize_t readSize = 0;
    if (pInodeInfo->HOITN_stSize <= stOft) {                                 /*  �Ѿ����ļ�ĩβ              */
        return  (0);
    }

    readSize = pInodeInfo->HOITN_stSize - stOft;                           /*  ����ʣ��������              */
    readSize = __MIN(readSize, stSize);
    hoitFragTreeRead(pInodeInfo->HOITN_rbtree, stOft, stSize, pvBuffer);
    return readSize;
}
/*********************************************************************************************************
** ��������: __hoit_write
** ��������: hoitfs д���ļ�����(�ú���ֻ����raw_inode���͵�����ʵ��)
** �䡡��  : pInodeInfo            �ļ��ڵ�
**           pvBuffer         ������
**           stNBytes         ��Ҫ��ȡ�Ĵ�С
**           stOft            ƫ����
** �䡡��  : ��ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ssize_t  __hoit_write(PHOIT_INODE_INFO  pInodeInfo, CPVOID  pvBuffer, size_t  stNBytes, size_t  stOft, UINT needLog) {
    PHOIT_VOLUME pfs = pInodeInfo->HOITN_volume;
    if (stNBytes > HOIT_MAX_DATA_SIZE) {
        UINT uBufOffset = 0;
        UINT uOldNBytes = stNBytes;
        while(stNBytes > HOIT_MAX_DATA_SIZE){
            __hoit_write(pInodeInfo, pvBuffer+uBufOffset, HOIT_MAX_DATA_SIZE, stOft + uBufOffset, needLog);
            stNBytes -= HOIT_MAX_DATA_SIZE;
            uBufOffset += HOIT_MAX_DATA_SIZE;
        }
        if(stNBytes > 0){
            __hoit_write(pInodeInfo, pvBuffer+uBufOffset, stNBytes, stOft + uBufOffset, needLog);
        }
        return uOldNBytes;
    }

    if(stNBytes == 0){
        return stNBytes;
    }

    if (pInodeInfo->HOITN_rbtree != LW_NULL) {
        PHOIT_FULL_DNODE pFullDnode = __hoit_write_full_dnode(pInodeInfo, stOft, stNBytes, pvBuffer, needLog);
        PHOIT_FRAG_TREE_NODE pTreeNode = newHoitFragTreeNode(pFullDnode, stNBytes, stOft, stOft);
        hoitFragTreeInsertNode(pInodeInfo->HOITN_rbtree, pTreeNode);
        hoitFragTreeOverlayFixUp(pInodeInfo->HOITN_rbtree);
#ifdef WRITE_BUFFER_ENABLE
        if (stNBytes < HOIT_MERGE_BUFFER_FRAGSIZE) {
            __hoit_new_merge_entry(pInodeInfo, pInodeInfo->HOITN_pMergeBuffer, pTreeNode);
        }
#endif
        return stNBytes;
    }
    else {
        PHOIT_FULL_DNODE pFullDnode = __hoit_write_full_dnode(pInodeInfo, stOft, stNBytes, pvBuffer, needLog);
        pInodeInfo->HOITN_metadata = pFullDnode;
        return stNBytes;
    }
}

/*********************************************************************************************************
** ��������: __hoit_ummount
** ��������: hoitfs ж��
** �䡡��  : pfs               �ļ�ϵͳ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_unmount(PHOIT_VOLUME pfs)
{
    /* TODO �ͷ�RAW INFO��Ҫ��GC�ȹ���*/
    //API_SpinDestory()
    if (pfs == LW_NULL) {
        printf("Error in unmount.\n");
        return;
    }
    hoitFlushCache(pfs->HOITFS_cacheHdr, (PHOIT_CACHE_BLK)-1);
    hoitGCClose(pfs);
    __hoit_close(pfs->HOITFS_pRootDir, 0);  /* ��ɾ����Ŀ¼ */
    hoitFreeCache(pfs->HOITFS_cacheHdr);    /* �ͷŻ���� */

    if (pfs->HOITFS_pTempRootDirent != LW_NULL) {   /* ɾ��TempDirent���� */
        PHOIT_FULL_DIRENT pTempDirent = pfs->HOITFS_pTempRootDirent;
        PHOIT_FULL_DIRENT pNextDirent = LW_NULL;
        while (pTempDirent) {
            pNextDirent = pTempDirent->HOITFD_next;
            __SHEAP_FREE(pTempDirent);
            pTempDirent = pNextDirent;
        }
    }

    /* ֻ��ɾ�����е�inode cache, raw info�ȵ�ɾ��sector��ʱ����ɾ�� */
    PHOIT_INODE_CACHE pTempCache = pfs->HOITFS_cache_list;
    PHOIT_INODE_CACHE pNextCache = LW_NULL;
    while (pTempCache) {
        pNextCache = pTempCache->HOITC_next;
        __SHEAP_FREE(pTempCache);
        pTempCache = pNextCache;
    }

    /* �ͷ�����sector */
    PHOIT_ERASABLE_SECTOR pTempSector = pfs->HOITFS_erasableSectorList;
    PHOIT_ERASABLE_SECTOR pNextSector = LW_NULL;
    while (pTempSector) {
        pNextSector = pTempSector->HOITS_next;
        /* ɾ��RAW INFO, С��GC */
        PHOIT_RAW_INFO pTempRaw = pTempSector->HOITS_pRawInfoFirst;
        PHOIT_RAW_INFO pNextRaw = LW_NULL;
        while (pTempRaw) {
            pNextRaw = pTempRaw->next_phys;
            __SHEAP_FREE(pTempRaw);
            pTempRaw = pNextRaw;
        }
        pTempSector = pNextSector;
    }
    FreeIterator(pfs->HOITFS_sectorIterator);
}
/*********************************************************************************************************
** ��������: __hoit_mount
** ��������: hoitfs ����
** �䡡��  : pfs           �ļ�ϵͳ
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_mount(PHOIT_VOLUME  pfs)
{
    pfs->HOITFS_highest_ino = 0;
    pfs->HOITFS_highest_version = 0;

    INT                 i;
    INT                 hasLog      = 0;
    UINT                phys_addr   = 0;
    UINT8               sector_no   = hoitGetSectorNo(phys_addr);
    PHOIT_RAW_LOG       pRawLogHdr  = LW_NULL;
    
    LW_CLASS_THREADATTR scThreadAttr;
    LW_OBJECT_HANDLE ulObjectHandle[NOR_FLASH_NSECTOR];
    INT handleSize = 0;

    while (hoitGetSectorSize(sector_no) != -1) {

        ScanThreadAttr* pThreadAttr = (ScanThreadAttr*)lib_malloc(sizeof(ScanThreadAttr));
        pThreadAttr->pfs = pfs;
        pThreadAttr->sector_no = sector_no;
#ifndef MULTI_THREAD_ENABLE
        __hoit_scan_single_sector(pThreadAttr);
#else
        API_ThreadAttrBuild(&scThreadAttr,
             4 * LW_CFG_KB_SIZE,
             LW_PRIO_NORMAL,
             LW_OPTION_THREAD_STK_CHK,
             (VOID*)pThreadAttr);

        ulObjectHandle[handleSize++] = API_ThreadCreate("t_scan_thread",
            (PTHREAD_START_ROUTINE)__hoit_scan_single_sector,
            &scThreadAttr,
            LW_NULL);
#endif
        sector_no++;
    }

#ifdef MULTI_THREAD_ENABLE
     for (i = 0; i < handleSize; i++) {
         API_ThreadJoin(ulObjectHandle[i], LW_NULL);
     }
#endif
    pfs->HOITFS_highest_ino++;
    pfs->HOITFS_highest_version++;
    printf("now sector offs: %d \n", pfs->HOITFS_now_sector->HOITS_offset);
    
   if (!hasLog) {
       hoitLogInit(pfs, hoitGetSectorSize(8), 1);
   }
   if (pRawLogHdr != LW_NULL){
       hoitLogOpen(pfs, pRawLogHdr);
   }

#ifdef LOG_ENABLE
//    __hoit_redo_log(pfs);
#endif // LOG_ENABLE

    if (pfs->HOITFS_highest_ino == HOIT_ROOT_DIR_INO) {    /* ϵͳ��һ������, ������Ŀ¼�ļ� */
        mode_t mode = S_IFDIR;
        PHOIT_INODE_INFO pRootDir = __hoit_new_inode_info(pfs, mode, LW_NULL);
        pfs->HOITFS_pRootDir = pRootDir;
    }
    /* ϵͳ���ǵ�һ�����еĻ�����ɨ��ʱ���ҵ�pRootDir */


    /* ������inode_cache��raw_info�������  */
    /* ������Ҫ�ݹ�ͳ�������ļ���nlink          */
    __hoit_get_nlink(pfs->HOITFS_pRootDir);
    register_hoitfs_cmd(pfs);
}

/*********************************************************************************************************
** ��������: __hoit_redo_log
** ��������: hoitfs �ָ�log
** �䡡��  :
** �䡡��  : �򿪽��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  __hoit_redo_log(PHOIT_VOLUME  pfs) {
    PHOIT_LOG_INFO pLogInfo = pfs->HOITFS_logInfo;
    UINT i;
    for (i = 0; i < pLogInfo->uiLogEntityCnt; i++) {
        PCHAR p = hoitLogEntityGet(pfs, i);
        PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)p;
        if(pRawHeader->ino == HOIT_ROOT_DIR_INO){
            continue;
        }
        if (__HOIT_IS_TYPE_INODE(pRawHeader)) {
            PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)pRawHeader;
            PCHAR pData = p + sizeof(HOIT_RAW_INODE);
            PHOIT_INODE_INFO pInodeInfo = __hoit_get_full_file(pfs, pRawInode->ino);

            pRawHeader->version = pfs->HOITFS_highest_version++;

            __hoit_write(pInodeInfo, pData, pRawInode->totlen - sizeof(HOIT_RAW_INODE), pRawInode->offset, 0);

            __hoit_close(pInodeInfo, 0);
        }
        else if (__HOIT_IS_TYPE_DIRENT(pRawHeader)) {
            PHOIT_RAW_DIRENT pRawDirent = (PHOIT_RAW_DIRENT)pRawHeader;
            PCHAR pFileName = p + sizeof(HOIT_RAW_DIRENT);
            UINT uiNameLength = pRawDirent->totlen - sizeof(HOIT_RAW_DIRENT);
            PHOIT_FULL_DIRENT pFullDirent = (PHOIT_FULL_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_FULL_DIRENT));
            pFullDirent->HOITFD_file_name = (PCHAR)__SHEAP_ALLOC(uiNameLength + 1);
            pRawHeader->version = pfs->HOITFS_highest_version++;

            lib_bzero(pFullDirent->HOITFD_file_name, uiNameLength + 1);
            if (pFullDirent->HOITFD_file_name == LW_NULL) {
                _ErrorHandle(ENOMEM);
                return  (LW_NULL);
            }
            lib_memcpy(pFullDirent->HOITFD_file_name, pFileName, uiNameLength);

            pFullDirent->HOITFD_file_type = pRawDirent->file_type;
            pFullDirent->HOITFD_ino = pRawDirent->ino;
            pFullDirent->HOITFD_nhash = __hoit_name_hash(pFullDirent->HOITFD_file_name);
            pFullDirent->HOITFD_pino = pRawDirent->pino;
      
            PHOIT_INODE_INFO pInodeFather = __hoit_get_full_file(pfs, pRawDirent->pino);

            __hoit_add_dirent(pInodeFather, pFullDirent, 0);

            __hoit_close(pInodeFather, 0);
        }

        __SHEAP_FREE(p);
    }
}

/*********************************************************************************************************
** ��������: __hoit_erasable_sector_list_check_exist
** ��������: ����������Ƿ��и�sector��
** �䡡��  :
**          HOITFS_sectorList   ��Ҫ�����б�
**          pErasableSector     ��Ҫ��ӵ��б��sector
** �䡡��  : ���ڣ�����LW_TRUEָ�룻�����ڣ�����LW_FALSE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_erasable_sector_list_check_exist(PHOIT_VOLUME pfs, List(HOIT_ERASABLE_SECTOR) HOITFS_sectorList, PHOIT_ERASABLE_SECTOR pErasableSector) {
    Iterator(HOIT_ERASABLE_SECTOR)      iter = pfs->HOITFS_sectorIterator;
    PHOIT_ERASABLE_SECTOR               psector;
    for(iter->begin(iter, HOITFS_sectorList); iter->isValid(iter); iter->next(iter)) {
        psector = iter->get(iter);
        if (psector == pErasableSector) {
            return LW_TRUE;
        }
    }
    
    return LW_FALSE;
}


//TODO: ֻ�ڳ�ʼ���׶ε��ã�����д���GC��������ҲӦ�õ���
/*********************************************************************************************************
** ��������: __hoit_fix_up_sector_list
** ��������: ���һ��sector�����ͣ�dirty��clean��free��������ӵ�volume��Ӧ�����ϡ�
**          ���sector�Ѿ��ڶ�Ӧ�����У���������
** �䡡��  :
**          pfs                 �ļ���
**          pErasableSector     ��Ҫ��ӵ��б��sector
** �䡡��  : 
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID __hoit_fix_up_sector_list(PHOIT_VOLUME pfs, PHOIT_ERASABLE_SECTOR pErasableSector) {
    if (pErasableSector->HOITS_uiFreeSize == GET_SECTOR_SIZE(pErasableSector->HOITS_bno)) { /* ��sector */
        if (!__hoit_erasable_sector_list_check_exist(pfs, GET_FREE_LIST(pfs), pErasableSector)) {
            GET_FREE_LIST(pfs)->insert(GET_FREE_LIST(pfs), pErasableSector, 0);
        }
    }
    if (pErasableSector->HOITS_uiObsoleteEntityCount != 0) {
        /* Ŀǰ��ֻҪ��������ʵ�壬�Ͱ�sector�ŵ�dirty list�� */
        if (!__hoit_erasable_sector_list_check_exist(pfs, GET_DIRTY_LIST(pfs), pErasableSector)) {
            GET_DIRTY_LIST(pfs)->insert(GET_DIRTY_LIST(pfs), pErasableSector, 0);
        }
    } else if (pErasableSector->HOITS_uiAvailableEntityCount != 0) {
        if (!__hoit_erasable_sector_list_check_exist(pfs, GET_CLEAN_LIST(pfs), pErasableSector)) {
            GET_CLEAN_LIST(pfs)->insert(GET_CLEAN_LIST(pfs), pErasableSector, 0);
        }
    }
}




#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
#endif //HOITFSLIB_DISABLE

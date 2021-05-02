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
** ��   ��   ��: HoitFsFDLib.c
**
** ��   ��   ��: Hu Zhisheng
**
** �ļ���������: 2021 �� 04 �� 10 ��
**
** ��        ��: Hoit�ļ�ϵͳ�ٿ�FullDnode�ĺ�����.
*********************************************************************************************************/
#include "hoitFsFDLib.h"
#include "hoitFsLib.h"
#include "../../driver/mtd/nor/nor.h"

/*********************************************************************************************************
** ��������: __hoit_delete_full_dnode
** ��������: ����flag����FullDnode����Ӧɾ��������1����ɾ���ײ����ݣ�0����ֻ�ͷ�FullDnode�ṹ��
** �䡡��  : 
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_delete_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFullDnode, INT flag) {
	if (flag == 1) {	/* ɾ�� */
		PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pFullDnode->HOITFD_raw_info->totlen);

		read_nor(pFullDnode->HOITFD_raw_info->phys_addr, read_buf, pFullDnode->HOITFD_raw_info->totlen);

		PHOIT_RAW_HEADER pRawHeader = (PHOIT_RAW_HEADER)read_buf;

		PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pRawHeader->ino);
		if (!pInodeCache) {
			return LW_FALSE;
		}
		__hoit_del_raw_data(pFullDnode->HOITFD_raw_info);
		__hoit_del_raw_info(pInodeCache, pFullDnode->HOITFD_raw_info);
		__SHEAP_FREE(pFullDnode->HOITFD_raw_info);
		pFullDnode->HOITFD_raw_info = LW_NULL;
		__SHEAP_FREE(pFullDnode);
		return LW_TRUE;
	}
	else {				/* ֻɾdnode */
		__SHEAP_FREE(pFullDnode);
		return LW_TRUE;
	}
}

/*********************************************************************************************************
** ��������: __hoit_update_full_dnode
** ��������: �ı�offset��size����
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL __hoit_update_full_dnode(PHOIT_FULL_DNODE pFullDnode, UINT offset, UINT length) {
	pFullDnode->HOITFD_offset = offset;
	pFullDnode->HOITFD_length = length;
	return LW_TRUE;
}

/*********************************************************************************************************
** ��������: __hoit_truncate_full_dnode
** ��������: ���Ѵ��ڵ�FullDnode�н�ȡһ�����ݲ�д��Flash�µĵط�������һ���µ�FullDnode
**			 offset����ָ���ļ��ڵ��߼���ַ������ָ����ھ�FullDnode��ʼλ�õ�offset
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FULL_DNODE __hoit_truncate_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFullDnode, UINT offset, UINT length) {
    PHOIT_RAW_INFO pRawInfo = pFullDnode->HOITFD_raw_info;
    if (sizeof(struct HOIT_RAW_INODE) + offset > pRawInfo->totlen) {
        return LW_NULL;
    }
    PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);     /* ע������ڴ�й¶ */
    if (!read_buf) {
        return LW_NULL;
    }
    lib_bzero(read_buf, pRawInfo->totlen);
    read_nor(pRawInfo->phys_addr, read_buf, pRawInfo->totlen);

    PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)read_buf;

    /* ��write_buf����װ��Ҫд������� */
    PCHAR write_buf = (PCHAR)__SHEAP_ALLOC(sizeof(struct HOIT_RAW_INODE) + length);  /* ע������ڴ�й¶ */
    if (!write_buf) {
        __SHEAP_FREE(read_buf);
        return LW_NULL;
    }
    lib_bzero(write_buf, sizeof(struct HOIT_RAW_INODE) + length);

    PHOIT_RAW_INODE pNewRawInode = (PHOIT_RAW_INODE)write_buf;
    pNewRawInode->file_type = pRawInode->file_type;
    pNewRawInode->flag = pRawInode->flag;
    pNewRawInode->ino = pRawInode->ino;
    pNewRawInode->magic_num = pRawInode->magic_num;
    pNewRawInode->totlen = sizeof(struct HOIT_RAW_INODE) + length;
    pNewRawInode->offset = pFullDnode->HOITFD_offset + offset;
    pNewRawInode->version = pfs->HOITFS_highest_version++;

    lib_memcpy(write_buf + sizeof(struct HOIT_RAW_INODE), read_buf + sizeof(struct HOIT_RAW_INODE) + offset, length);

    UINT phys_addr = 0;
    __hoit_write_flash(pfs, write_buf, sizeof(struct HOIT_RAW_INODE) + length, &phys_addr);

    PHOIT_RAW_INFO pNewRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(struct HOIT_RAW_INFO)); /* ע������ڴ�й¶ */
    if (!pNewRawInfo) {
        __SHEAP_FREE(write_buf);
        __SHEAP_FREE(read_buf);
        return LW_NULL;
    }
    pNewRawInfo->phys_addr = phys_addr;
    pNewRawInfo->totlen = sizeof(struct HOIT_RAW_INODE) + length;

    PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pRawInode->ino);
    __hoit_add_to_inode_cache(pInodeCache, pNewRawInfo);

    PHOIT_FULL_DNODE pNewFullDnode = (PHOIT_FULL_DNODE)__SHEAP_ALLOC(sizeof(struct HOIT_FULL_DNODE));  /* ע������ڴ�й¶ */
    if (!pNewFullDnode) {
        __SHEAP_FREE(write_buf);
        __SHEAP_FREE(read_buf);
        return LW_NULL;
    }
    pNewFullDnode->HOITFD_file_type = pFullDnode->HOITFD_file_type;
    pNewFullDnode->HOITFD_length = length;
    pNewFullDnode->HOITFD_offset = pFullDnode->HOITFD_offset + offset;
    pNewFullDnode->HOITFD_raw_info = pNewRawInfo;

    __SHEAP_FREE(write_buf);
    __SHEAP_FREE(read_buf);
    return pNewFullDnode;
}

/*********************************************************************************************************
** ��������: __hoit_write_full_dnode
** ��������: ��һ������д�뵽Flash, ������PHOIT_FULL_DNODE
**           offset��ָ�ļ��ڵ�ƫ�Ƶ�ַ
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FULL_DNODE __hoit_write_full_dnode(PHOIT_INODE_INFO pInodeInfo, UINT offset, UINT size, PCHAR pContent) {
    PHOIT_VOLUME pfs = pInodeInfo->HOITN_volume;
    PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)__SHEAP_ALLOC(sizeof(HOIT_RAW_INODE));     /* ע���ڴ�й¶ */
    if (pRawInode == LW_NULL) {
        return LW_NULL;
    }

    pRawInode->file_type = pInodeInfo->HOITN_mode;
    pRawInode->flag = HOIT_FLAG_OBSOLETE | HOIT_FLAG_TYPE_INODE;
    pRawInode->ino = pInodeInfo->HOITN_ino;
    pRawInode->magic_num = HOIT_MAGIC_NUM;
    pRawInode->totlen = sizeof(HOIT_RAW_INODE) + size;
    pRawInode->offset = offset;
    pRawInode->version = pfs->HOITFS_highest_version++;

    UINT phys_addr = 0;
    __hoit_write_flash(pfs, (PVOID)pRawInode, sizeof(HOIT_RAW_INODE), &phys_addr);
    __hoit_write_flash(pfs, (PVOID)pContent, size, LW_NULL);

    PHOIT_RAW_INFO pRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(HOIT_RAW_INFO));
    pRawInfo->phys_addr = phys_addr;
    pRawInfo->totlen = sizeof(HOIT_RAW_INODE) + size;
    
    __hoit_add_to_inode_cache(pInodeInfo->HOITN_inode_cache, pRawInfo);
    PHOIT_FULL_DNODE pFullDnode = (PHOIT_FULL_DNODE)__SHEAP_ALLOC(sizeof(HOIT_FULL_DNODE));
    pFullDnode->HOITFD_file_type = pInodeInfo->HOITN_mode;
    pFullDnode->HOITFD_length = size;
    pFullDnode->HOITFD_offset = offset;
    pFullDnode->HOITFD_raw_info = pRawInfo;
    
    __SHEAP_FREE(pRawInode);
    return pFullDnode;
}
/*********************************************************************************************************
** ��������: __hoit_bulid_full_dnode
** ��������: �ڴ��ļ�ʱ, ����pRawInfo����FullDnode
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FULL_DNODE __hoit_bulid_full_dnode(PHOIT_RAW_INFO pRawInfo) {
    PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);        /* ע���ڴ�й¶ */
    read_nor(pRawInfo->phys_addr, read_buf, pRawInfo->totlen);
    PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)read_buf;
    
    PHOIT_FULL_DNODE pFullDnode = (PHOIT_FULL_DNODE)__SHEAP_ALLOC(sizeof(HOIT_FULL_DNODE));
    pFullDnode->HOITFD_file_type = pRawInode->file_type;
    pFullDnode->HOITFD_length = pRawInode->totlen - sizeof(PHOIT_RAW_INODE);
    pFullDnode->HOITFD_offset = pRawInode->offset;
    pFullDnode->HOITFD_raw_info = pRawInfo;
    __SHEAP_FREE(read_buf);
    return pFullDnode;
}

/*********************************************************************************************************
** ��������: __hoit_bulid_full_dirent
** ��������: �ڴ��ļ�ʱ, ����pRawInfo����FullDirent
** �䡡��  :
** �䡡��  :
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PHOIT_FULL_DIRENT __hoit_bulid_full_dirent(PHOIT_RAW_INFO pRawInfo) {
    PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);        /* ע���ڴ�й¶ */
    read_nor(pRawInfo->phys_addr, read_buf, pRawInfo->totlen);
    PHOIT_RAW_DIRENT pRawDirent = (PHOIT_RAW_DIRENT)read_buf;

    PHOIT_FULL_DIRENT pFullDirent = (PHOIT_FULL_DIRENT)__SHEAP_ALLOC(sizeof(HOIT_FULL_DIRENT));
    PCHAR pFileName = read_buf + sizeof(HOIT_RAW_DIRENT);
    PCHAR pNewFileName = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen - sizeof(HOIT_RAW_INFO));
    lib_strncpy(pNewFileName, pFileName, pRawInfo->totlen - sizeof(HOIT_RAW_INFO));
    pFullDirent->HOITFD_file_name = pNewFileName;
    pFullDirent->HOITFD_file_type = pRawDirent->file_type;
    pFullDirent->HOITFD_ino = pRawDirent->ino;
    pFullDirent->HOITFD_next = LW_NULL;
    pFullDirent->HOITFD_nhash = __hoit_name_hash(pNewFileName);
    pFullDirent->HOITFD_pino = pRawDirent->pino;
    pFullDirent->HOITFD_raw_info = pRawInfo;
    pFullDirent->HOITFD_version = pRawDirent->version;
    

    __SHEAP_FREE(read_buf);
    return pFullDirent;
}

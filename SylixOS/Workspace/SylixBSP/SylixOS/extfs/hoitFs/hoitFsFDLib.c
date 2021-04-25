/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: HoitFsFDLib.c
**
** 创   建   人: Hoit Group
**
** 文件创建日期: 2021 年 04 月 10 日
**
** 描        述: Hoit文件系统操控FullDnode的函数库.
*********************************************************************************************************/
#include "hoitFsFDLib.h"
#include "../../driver/mtd/nor/nor.h"

/*********************************************************************************************************
** 函数名称: __hoit_delete_full_dnode
** 功能描述: 根据flag进行FullDnode的相应删除操作，1代表还删除底层数据，0代表只释放FullDnode结构体
** 输　入  : 
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoit_delete_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFullDnode, INT flag) {
	if (flag == 1) {	/* 删完 */
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
	else {				/* 只删dnode */
		__SHEAP_FREE(pFullDnode);
		return LW_TRUE;
	}
}

/*********************************************************************************************************
** 函数名称: __hoit_update_full_dnode
** 功能描述: 改变offset和size即可
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL __hoit_update_full_dnode(PHOIT_FULL_DNODE pFullDnode, UINT offset, UINT length) {
	pFullDnode->HOITFD_offset = offset;
	pFullDnode->HOITFD_length = length;
	return LW_TRUE;
}

/*********************************************************************************************************
** 函数名称: __hoit_truncate_full_dnode
** 功能描述: 从已存在的FullDnode中截取一段数据并写到Flash新的地方，返回一个新的FullDnode
**			 offset不是指在文件内的逻辑地址，而是指相对于旧FullDnode起始位置的offset
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FULL_DNODE __hoit_truncate_full_dnode(PHOIT_VOLUME pfs, PHOIT_FULL_DNODE pFullDnode, UINT offset, UINT length) {
    PHOIT_RAW_INFO pRawInfo = pFullDnode->HOITFD_raw_info;
    if (sizeof(struct HOIT_RAW_INODE) + offset > pRawInfo->totlen) {
        return LW_NULL;
    }
    PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);     /* 注意避免内存泄露 */
    if (!read_buf) {
        return LW_NULL;
    }
    lib_bzero(read_buf, pRawInfo->totlen);
    read_nor(pRawInfo->phys_addr, read_buf, pRawInfo->totlen);

    PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)read_buf;

    /* 在write_buf中组装好要写入的数据 */
    PCHAR write_buf = (PCHAR)__SHEAP_ALLOC(sizeof(struct HOIT_RAW_INODE) + length);  /* 注意避免内存泄露 */
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

    PHOIT_RAW_INFO pNewRawInfo = (PHOIT_RAW_INFO)__SHEAP_ALLOC(sizeof(struct HOIT_RAW_INFO)); /* 注意避免内存泄露 */
    if (!pNewRawInfo) {
        __SHEAP_FREE(write_buf);
        __SHEAP_FREE(read_buf);
        return LW_NULL;
    }
    pNewRawInfo->phys_addr = phys_addr;
    pNewRawInfo->totlen = sizeof(struct HOIT_RAW_INODE) + length;

    PHOIT_INODE_CACHE pInodeCache = __hoit_get_inode_cache(pfs, pRawInode->ino);
    __hoit_add_to_inode_cache(pInodeCache, pNewRawInfo);

    PHOIT_FULL_DNODE pNewFullDnode = (PHOIT_FULL_DNODE)__SHEAP_ALLOC(sizeof(struct HOIT_FULL_DNODE));  /* 注意避免内存泄露 */
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
** 函数名称: __hoit_write_full_dnode
** 功能描述: 将一段数据写入到Flash, 并返回PHOIT_FULL_DNODE
**           offset是指文件内的偏移地址
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FULL_DNODE __hoit_write_full_dnode(PHOIT_INODE_INFO pInodeInfo, UINT offset, UINT size, PCHAR pContent) {
    PHOIT_VOLUME pfs = pInodeInfo->HOITN_volume;
    PHOIT_RAW_INODE pRawInode = (PHOIT_RAW_INODE)__SHEAP_ALLOC(sizeof(HOIT_RAW_INODE));     /* 注意内存泄露 */
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
** 函数名称: __hoit_bulid_full_dnode
** 功能描述: 在打开文件时, 根据pRawInfo建立FullDnode
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FULL_DNODE __hoit_bulid_full_dnode(PHOIT_RAW_INFO pRawInfo) {
    PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);        /* 注意内存泄露 */
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
** 函数名称: __hoit_bulid_full_dirent
** 功能描述: 在打开文件时, 根据pRawInfo建立FullDirent
** 输　入  :
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
PHOIT_FULL_DIRENT __hoit_bulid_full_dirent(PHOIT_RAW_INFO pRawInfo) {
    PCHAR read_buf = (PCHAR)__SHEAP_ALLOC(pRawInfo->totlen);        /* 注意内存泄露 */
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

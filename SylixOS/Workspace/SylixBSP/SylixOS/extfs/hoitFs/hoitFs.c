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
** ��   ��   ��: HoitFs.c
**
** ��   ��   ��: Hoit Group
**
** �ļ���������: 2021 �� 03 �� 19 ��
**
** ��        ��: HoitFs
*********************************************************************************************************/

#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "hoitFs.h"
#include "hoitFsGC.h"
#include "hoitFsCache.h"

#ifndef HOITFS_DISABLE
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT                              _G_iHoitFsDrvNum = PX_ERROR;
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LONG     __hoitFsOpen();
static INT      __hoitFsRemove();
static INT      __hoitFsClose();
static ssize_t  __hoitFsRead();
static ssize_t  __hoitFsPRead();
static ssize_t  __hoitFsWrite();
static ssize_t  __hoitFsPWrite();
static INT      __hoitFsLStat();
static INT      __hoitFsIoctl();
static INT      __hoitFsSymlink();
static ssize_t  __hoitFsReadlink();
/*********************************************************************************************************
  �ü���
*********************************************************************************************************/
//#if LW_CFG_MAX_VOLUMES > 0 //&& LW_CFG_RAMFS_EN > 0
#include "HoitFsLib.h"
/*********************************************************************************************************
** ��������: API_HoitFsDrvInstall
** ��������: ��װ hoitfs �ļ�ϵͳ��������
** �䡡��  :
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_HoitFsDrvInstall(VOID)
{
    struct file_operations     fileop;

    if (_G_iHoitFsDrvNum > 0) {
        return  (ERROR_NONE);
    }

    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner = THIS_MODULE;
    fileop.fo_create = __hoitFsOpen;
    fileop.fo_release = __hoitFsRemove;
    fileop.fo_open = __hoitFsOpen;
    fileop.fo_close = __hoitFsClose;
    fileop.fo_read = __hoitFsRead;
    fileop.fo_read_ex = __hoitFsPRead;
    fileop.fo_write = __hoitFsWrite;
    fileop.fo_write_ex = __hoitFsPWrite;
    fileop.fo_lstat = __hoitFsLStat;
    fileop.fo_ioctl = __hoitFsIoctl;
    fileop.fo_symlink = __hoitFsSymlink;
    fileop.fo_readlink = __hoitFsReadlink;

    _G_iHoitFsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  ʹ�� NEW_1 ���豸��������   */

    DRIVER_LICENSE(_G_iHoitFsDrvNum, "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iHoitFsDrvNum, "HITSZ.HoitGroup");
    DRIVER_DESCRIPTION(_G_iHoitFsDrvNum, "norflash fs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "norflash file system installed.\r\n");

    __fsRegister("hoitfs", API_HoitFsDevCreate, LW_NULL, LW_NULL);        /*  ע���ļ�ϵͳ                */

    return  ((_G_iHoitFsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}

/*********************************************************************************************************
** ��������: API_HoitFsDevCreate
** ��������: ���� hoitfs �ļ�ϵͳ�豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ʹ�� pblkd->BLKD_pcName ��Ϊ ����С ��ʾ.
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
#define NAMESPACE   hoitFs
USE_LIST_TEMPLATE(NAMESPACE, HOIT_ERASABLE_SECTOR_REF);
LW_API
INT  API_HoitFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PHOIT_VOLUME     pfs;
    if (_G_iHoitFsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "hoitfs Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }
    if ((pblkd == LW_NULL) || (pblkd->BLKD_pcName == LW_NULL)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "block device invalidate.\r\n");
        _ErrorHandle(ERROR_IOS_DEVICE_NOT_FOUND);
        return  (PX_ERROR);
    }
    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mount name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    pfs = (PHOIT_VOLUME)lib_malloc(sizeof(HOIT_VOLUME));
    if (pfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pfs, sizeof(HOIT_VOLUME));                              /*  ��վ���ƿ�                */

    pfs->HOITFS_bValid = LW_TRUE;

    pfs->HOITFS_hVolLock = API_SemaphoreMCreate("hoit_volume_lock", LW_PRIO_DEF_CEILING,
        LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
        LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
        LW_NULL);
    if (!pfs->HOITFS_hVolLock) {                                      /*  �޷���������                */
        hoit_free(pfs, pfs, sizeof(HOIT_VOLUME));
        return  (PX_ERROR);
    }

    /* �������������� */
    pfs->HOITFS_dirtyLock = API_SemaphoreMCreate("hoit_dirty_lock", LW_PRIO_DEF_CEILING,
        LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
        LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
        LW_NULL);
    if (!pfs->HOITFS_dirtyLock) {                                      /*  �޷���������                */
        hoit_free(pfs, pfs, sizeof(HOIT_VOLUME));
        return  (PX_ERROR);
    }
    pfs->HOITFS_cleanLock = API_SemaphoreMCreate("hoit_clean_lock", LW_PRIO_DEF_CEILING,
        LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
        LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
        LW_NULL);
    if (!pfs->HOITFS_cleanLock) {                                      /*  �޷���������                */
        hoit_free(pfs, pfs, sizeof(HOIT_VOLUME));
        return  (PX_ERROR);
    }
    pfs->HOITFS_freeLock = API_SemaphoreMCreate("hoit_free_lock", LW_PRIO_DEF_CEILING,
        LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
        LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
        LW_NULL);
    if (!pfs->HOITFS_freeLock) {                                      /*  �޷���������                */
        hoit_free(pfs, pfs, sizeof(HOIT_VOLUME));
        return  (PX_ERROR);
    }

    pfs->HOITFS_mode            = S_IFDIR | DEFAULT_DIR_PERM;
    pfs->HOITFS_uid             = getuid();
    pfs->HOITFS_gid             = getgid();
    pfs->HOITFS_time            = lib_time(LW_NULL);
    //TODO �ڴ����Ŀռ����
    pfs->HOITFS_ulCurBlk        = 0ul;
    pfs->HOITFS_ulMaxBlk        = 0ul;
    pfs->HOITFS_now_sector      = LW_NULL;
    pfs->HOITFS_pRootDir        = LW_NULL;
    pfs->HOITFS_totalUsedSize   = 0;

    pfs->HOITFS_hGCThreadId     = LW_NULL;

                                                                        /* GC��� */
    pfs->HOITFS_curGCSector        = LW_NULL;
    pfs->HOITFS_GCMsgQ             = PX_ERROR;
    pfs->ulGCBackgroundTimes       = 0;
    pfs->ulGCForegroundTimes       = 0;
    pfs->HOITFS_erasableSectorList = LW_NULL;
    pfs->HOITFS_bShouldKillGC      = LW_FALSE;

    InitList(pfs->HOITFS_dirtySectorList,    NAMESPACE, HOIT_ERASABLE_SECTOR_REF); /* ��ʼ��ģ������ */
    InitList(pfs->HOITFS_cleanSectorList,    NAMESPACE, HOIT_ERASABLE_SECTOR_REF);
    InitList(pfs->HOITFS_freeSectorList,     NAMESPACE, HOIT_ERASABLE_SECTOR_REF);
    InitIterator(pfs->HOITFS_sectorIterator, NAMESPACE, HOIT_ERASABLE_SECTOR_REF);

                                                                        /* Log��� */
    pfs->HOITFS_logInfo            = LW_NULL;
    //__ram_mount(pramfs);

    hoitEnableCache(GET_SECTOR_SIZE(8), 8, pfs);
    //TODO �ļ��ܴ�С��ʱӲ����
    pfs->HOITFS_totalSize       = pfs->HOITFS_cacheHdr->HOITCACHE_blockSize * 28;
    __hoit_mount(pfs);

#ifdef BACKGOURND_GC_ENABLE 
    hoitStartGCThread(pfs, pfs->HOITFS_totalSize / 3);
#endif  /* BACKGOURND_GC_ENABLE */

    if (iosDevAddEx(&pfs->HOITFS_devhdrHdr, pcName, _G_iHoitFsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        API_SemaphoreMDelete(&pfs->HOITFS_hVolLock);
        hoit_free(pfs, pfs, sizeof(HOIT_VOLUME));
        return  (PX_ERROR);
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);



    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: API_HoitFsDevDelete
** ��������: ɾ��һ�� hoitfs �ļ�ϵͳ�豸, ����: API_HoitFsDevDelete("/mnt/ram0");
** �䡡��  : pcName            �ļ�ϵͳ�豸��(�����豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_HoitFsDevDelete(PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));

    }
    else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}

/*********************************************************************************************************
** ��������: __hoitFsOpen
** ��������: �򿪻��ߴ����ļ�
** �䡡��  : pfs              �ڴ���HoitFs�ļ�ϵͳ��super block
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            mode_t
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG __hoitFsOpen(PHOIT_VOLUME     pfs,
    PCHAR           pcName,
    INT             iFlags,
    INT             iMode)
{
    PLW_FD_NODE         pfdnode;
    PHOIT_INODE_INFO    phoitn;
    PHOIT_INODE_INFO    phoitFather;
    BOOL                bRoot;
    BOOL                bLast;
    BOOL                bIsNew;
    PCHAR               pcTail;
    BOOL                bCreate;
    struct stat         statGet;

    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    if (iFlags & O_CREAT) {                                             /*  ��������                    */
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (S_ISFIFO(iMode) ||
            S_ISBLK(iMode) ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ��֧��������Щ��ʽ          */
            return  (PX_ERROR);
        }
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    /************************************ TODO ************************************/
    phoitn = __hoit_open(pfs, pcName, &phoitFather, LW_NULL,&bRoot, &bLast, &pcTail);
    
    if (phoitn) {
        if (!S_ISLNK(phoitn->HOITN_mode)) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  ���������ļ�                */
                __HOIT_VOLUME_UNLOCK(pfs);
                _ErrorHandle(EEXIST);                                   /*  �Ѿ������ļ�                */
                return  (PX_ERROR);            
            } else if ((iFlags & O_DIRECTORY) && !S_ISDIR(phoitn->HOITN_mode)) {
                __HOIT_VOLUME_UNLOCK(pfs);
                _ErrorHandle(ENOTDIR);
                return  (PX_ERROR);
            } else {
                goto    __file_open_ok;
            }
        }

    } else if ((iFlags & O_CREAT) && bLast) {                           /*  �����ڵ�                    */
        phoitn = __hoit_maken(pfs, pcName, phoitFather, iMode, LW_NULL);
        iFlags &= !O_TRUNC;
        if (phoitn) {
            bCreate = LW_TRUE;
            goto    __file_open_ok;
        } else {
            return (PX_ERROR);
        }
    }

    if (phoitn) {                                                       /*  �������Ӵ���                */
        INT                 iError;
        PCHAR               pcPrefix;
        INT                 iFollowLinkType;
        PHOIT_FULL_DIRENT   pFullDirent = __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pcName);
        PCHAR               pcSymfile = pcTail - lib_strlen(pFullDirent->HOITFD_file_name) - 1; 
                                                                        /* pcSymfileָ��pcName���������ļ�������ĸǰ���'/' */

        if (*pcSymfile != PX_DIVIDER) {
            pcSymfile--;
        }
        if (pcSymfile == pcName) {
            pcPrefix = LW_NULL;                                         /*  û��ǰ׺                    */
        } else {
            pcPrefix = pcName;
            *pcSymfile = PX_EOS;
        }        
        if (pcTail && lib_strlen(pcTail)) {
            iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  ����Ŀ���ڲ��ļ�            */
        } else {
            iFollowLinkType = FOLLOW_LINK_FILE;                         /*  �����ļ�����                */
        }
        //TODO PHOIT_INODE_INFO��û�� symbol link
        iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                        LW_NULL, pcPrefix, phoitn->HOITN_pcLink, pcTail);

        if (iError) {
            __HOIT_VOLUME_UNLOCK(pfs);
            return  (PX_ERROR);                                         /*  �޷�����������Ŀ��Ŀ¼      */
        } else {
            __HOIT_VOLUME_UNLOCK(pfs);
            return  (iFollowLinkType);
        }                                        
    } else if (bRoot == LW_FALSE) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(ENOENT);                                           /*  û���ҵ��ļ�                */
        return  (PX_ERROR);
    } else if (bRoot == LW_TRUE){
        phoitn = pfs->HOITFS_pRootDir;
    }

__file_open_ok:
    __hoit_stat(phoitn, pfs, &statGet);
    pfdnode = API_IosFdNodeAdd(&pfs->HOITFS_plineFdNodeHeader,
                               statGet.st_dev,
                               (ino64_t)statGet.st_ino,
                               iFlags,
                               iMode,
                               statGet.st_uid,
                               statGet.st_gid,
                               statGet.st_size,
                               (PVOID)phoitn,
                               &bIsNew);    

    if (pfdnode == LW_NULL) {                                           /*  �޷����� fd_node �ڵ�       */
        __HOIT_VOLUME_UNLOCK(pfs);
        if (bCreate) {                                                  /*  ɾ���½��Ľڵ�              */
            //TOOPT hoit_unlink ������
            if (S_ISDIR(phoitn->HOITN_mode)) {
                __hoit_unlink_dir(phoitFather, 
                                    __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pcName));
            } else { //TODO �в���ʶ����ͨ�ļ�
                __hoit_unlink_regular(phoitFather,
                                    __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pcName));
            }             
        }
        return  (PX_ERROR);
    }

    pfdnode->FDNODE_pvFsExtern = (PVOID)pfs;                            /*  ��¼�ļ�ϵͳ��Ϣ            */

    if ((iFlags & O_TRUNC) && ((iFlags & O_ACCMODE) != O_RDONLY)) {     /*  ��Ҫ�ض�                    */
        if (phoitn) {
            __hoit_truncate(phoitn, 0);
            pfdnode->FDNODE_oftSize = 0;
        }
    }

    if(bRoot != LW_TRUE)
        LW_DEV_INC_USE_COUNT(&pfs->HOITFS_devhdrHdr);                     /*  ���¼�����                  */
    /************************************ END  ************************************/
    __HOIT_VOLUME_UNLOCK(pfs);

    return  ((LONG)pfdnode);                                            /*  �����ļ��ڵ�                */
}


/*********************************************************************************************************
** ��������: __hoitFsRemove
** ��������: hoitfs remove ����
** �䡡��  : pfs              ���豸
**           pcName           �ļ���
**           ע���ļ������Ϊ�վ���ж�ر��ļ�ϵͳ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsRemove(PHOIT_VOLUME   pfs,
    PCHAR         pcName)
{
    PHOIT_INODE_INFO    phoitn;
    PHOIT_INODE_INFO    phoitFather;
    BOOL                bRoot;
    PCHAR               pcTail;
    INT                 iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    //************************************ TODO ************************************
    phoitn = __hoit_open(pfs, pcName, &phoitFather, LW_NULL, &bRoot, LW_NULL, &pcTail);
    if (phoitn) {
        //TODO ���������Ӳ��֣�PHOIT_INODE_INFO��ȱ��pcLink
        if (S_ISLNK(phoitn->HOITN_mode)) {
            size_t  stLenTail = 0;
            PHOIT_FULL_DIRENT   pFullDirent = __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pcName);

            if (pcTail) {
                stLenTail = lib_strlen(pcTail);                         /*  ȷ�� tail ����              */
            }
            if (stLenTail) {                                            /*  ָ�������ļ�                */
                PCHAR   pcSymfile = pcTail - lib_strlen(pFullDirent->HOITFD_file_name) - 1;
                PCHAR   pcPrefix;

                if (*pcSymfile != PX_DIVIDER) {
                    pcSymfile--;
                }
                if (pcSymfile == pcName) {
                    pcPrefix = LW_NULL;                                 /*  û��ǰ׺                    */
                } else {
                    pcPrefix = pcName;
                    *pcSymfile = PX_EOS;
                }
                if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
                                   LW_NULL, pcPrefix, phoitn->HOITN_pcLink, pcTail) < ERROR_NONE) {
                    __HOIT_VOLUME_UNLOCK(pfs);
                    return  (PX_ERROR);                                 /*  �޷�����������Ŀ��Ŀ¼      */
                } else {
                    __HOIT_VOLUME_UNLOCK(pfs);
                    return  (FOLLOW_LINK_TAIL);
                }
            }
        }


        //TOOPT hoit_unlink ������
        if (S_ISDIR(phoitn->HOITN_mode)) {
            iError = __hoit_unlink_dir(phoitFather, 
                                __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pcName));
        } else { //TODO �в���ʶ����ͨ�ļ�
            iError = __hoit_unlink_regular(phoitFather,
                                __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pcName));
        }   
        __HOIT_VOLUME_UNLOCK(pfs);
        return (iError);
    } else if (bRoot) {
        if (pfs->HOITFS_bValid == LW_FALSE) {
            __HOIT_VOLUME_UNLOCK(pfs);
            return (ERROR_NONE);
        }
__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pfs) > 1) {
            if (!pfs->HOITFS_bForceDelete) {
                __HOIT_VOLUME_UNLOCK(pfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }

            pfs->HOITFS_bValid = LW_FALSE;

            __HOIT_VOLUME_UNLOCK(pfs);

            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&pfs->HOITFS_devhdrHdr);               /*  ����������ļ���Ϊ�쳣ģʽ  */

            __HOIT_VOLUME_LOCK(pfs);
            goto    __re_umount_vol;
        } else {
            pfs->HOITFS_bValid = LW_FALSE;
        }

        iosDevDelete((LW_DEV_HDR *)pfs);                             /*  IO ϵͳ�Ƴ��豸             */
        API_SemaphoreMDelete(&pfs->HOITFS_hVolLock);
        API_SemaphoreMDelete(&pfs->HOITFS_dirtyLock);
        API_SemaphoreMDelete(&pfs->HOITFS_cleanLock);
        API_SemaphoreMDelete(&pfs->HOITFS_freeLock); 

        __hoit_unmount(pfs);
        hoit_free(pfs, pfs, sizeof(HOIT_VOLUME));

        _DebugHandle(__LOGMESSAGE_LEVEL, "hoitfs unmount ok.\r\n");

        return (ERROR_NONE);
    } else {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);       
    }
    /************************************ END  ************************************/
}

/*********************************************************************************************************
** ��������: __hoitFsClose
** ��������: hoitfs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsClose(PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    BOOL                bRemove = LW_FALSE;

    if(phoitn->HOITN_ino == HOIT_ROOT_DIR_INO){
        return (ERROR_NONE);
    }
    /* �ȵõ�Ҫclose���ļ���hoitfs�ļ�ϵͳ�е����·��, �����hoitfs�ĸ�Ŀ¼ */
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH] = {0};
    CHAR           cRealFileName[MAX_FILENAME_LENGTH] = {0};
//    PCHAR pcFileName = lib_rindex(pfdentry->FDENTRY_pcName, PX_DIVIDER);
//    if (pcFileName) {
//        pcFileName++;
//    }
//    else {
//        pcFileName = pfdentry->FDENTRY_pcName;
//    }

    lib_memcpy(cFullFileName, pfdentry->FDENTRY_pcName, lib_strlen(pfdentry->FDENTRY_pcName));
    /*
     *  ��Ҫ�ڽ�β���������� / ����, ����: /aaa/bbb/ccc/ ӦΪ /aaa/bbb/ccc
     */
    UINT stFullLen = lib_strlen(cFullFileName);
    if (stFullLen > 1) {                                                /*  ������� "/" ������ȥ       */
        if (cFullFileName[stFullLen - 1] == PX_DIVIDER) {
            cFullFileName[stFullLen - 1] =  PX_EOS;                     /*  ȥ��ĩβ�� /                */
        }
        _PathCondense(cFullFileName);                                   /*  ȥ�� ../ ./                 */
    }

    PCHAR pcTail = LW_NULL;
    pdevhdrHdr = iosDevFind((cFullFileName), &pcTail);
    if (pdevhdrHdr == LW_NULL) {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }

    /*
     *  ��� ppdevhdr == rootfs dev header ��, pcTail ��ʼλ��û�� '/' �ַ�,
     *  �˴���Ҫ�������ڸ��ļ�ϵͳ�豸��Ϊ "/" ����������.
     */
    if (pcTail && ((*pcTail != PX_EOS) && (*pcTail != PX_DIVIDER))) {
        lib_strlcpy(&cRealFileName[1], pcTail, MAX_FILENAME_LENGTH - 1);
        cRealFileName[0] = PX_ROOT;

    } else {
        lib_strlcpy(cRealFileName, pcTail, MAX_FILENAME_LENGTH);
    }
    

    // PHOIT_INODE_INFO phoitFather;
    // PHOIT_INODE_INFO pTempInode = __hoit_open(pfs, cRealFileName, &phoitFather, LW_NULL, LW_NULL, LW_NULL, LW_NULL);

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (API_IosFdNodeDec(&pfs->HOITFS_plineFdNodeHeader, 
                         pfdnode, &bRemove) == 0) {
        if (phoitn) {
            __hoit_close(phoitn, pfdentry->FDENTRY_iFlag);
        }
    }

    LW_DEV_DEC_USE_COUNT(&pfs->HOITFS_devhdrHdr);


    // if (bRemove && phoitn) {
    //     if (S_ISDIR(phoitn->HOITN_mode)) {
    //         __hoit_unlink_dir(phoitFather, 
    //                             __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pfdentry->FDENTRY_pcName));
    //     } else { //TODO �в���ʶ����ͨ�ļ�
    //         __hoit_unlink_regular(phoitFather,
    //                             __hoit_search_in_dents(phoitFather, phoitn->HOITN_ino, pfdentry->FDENTRY_pcName));
    //     }
    // }

    // __hoit_close(phoitFather, 0);
    // __hoit_close(pTempInode, 0);
    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __hoitFsRead
** ��������: hoitfs read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __hoitFsRead(PLW_FD_ENTRY pfdentry,
    PCHAR        pcBuffer,
    size_t       stMaxBytes)
{
    PLW_FD_NODE         pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn      = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs         = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t             sstReadNum  = PX_ERROR;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);   
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (stMaxBytes) { //TODO __hoit_read��δ��Ӷ���
        sstReadNum = __hoit_read(phoitn, pcBuffer, stMaxBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstReadNum > 0) {
            pfdentry->FDENTRY_oftPtr +=(off_t)sstReadNum;
        }
    } else {
        sstReadNum = 0;
    }

    __HOIT_VOLUME_UNLOCK(pfs);
    return  (sstReadNum);
}

/*********************************************************************************************************
** ��������: __hoitFsPRead
** ��������: hoitfs pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __hoitFsPRead(PLW_FD_ENTRY pfdentry,
    PCHAR        pcBuffer,
    size_t       stMaxBytes,
    off_t        oftPos)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn      = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs         = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t             sstReadNum  = PX_ERROR;

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);   
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (stMaxBytes) {
        sstReadNum = __hoit_read(phoitn, pcBuffer, stMaxBytes, (size_t)oftPos);
        /* ���read���������ж��޸�pfdnode�ļ�ָ��Ĳ��� */
    } else {    
        sstReadNum = 0;
    }   

    __HOIT_VOLUME_UNLOCK(pfs);
    return  (sstReadNum);
}

/*********************************************************************************************************
** ��������: __hoitFsWrite
** ��������: hoitfs write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __hoitFsWrite(PLW_FD_ENTRY  pfdentry,
    PCHAR         pcBuffer,
    size_t        stNBytes)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn      = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs         = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t             sstWriteNum = PX_ERROR;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);       
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }

    if (stNBytes) { //TODO __hoit_write��δ��Ӷ���
        sstWriteNum = __hoit_write(phoitn, pcBuffer, stNBytes, (size_t)pfdentry->FDENTRY_oftPtr, 1);
        if (sstWriteNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;             /*  �����ļ�ָ��                */
            //TODO HOITN_stSize��δ����
            /*pfdnode->FDNODE_oftSize   = (off_t)phoitn->HOITN_stSize;*/
        }
    } else {
        sstWriteNum = 0;
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (sstWriteNum);
}

/*********************************************************************************************************
** ��������: __hoitFsPWrite
** ��������: hoitfs pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __hoitFsPWrite(PLW_FD_ENTRY  pfdentry,
    PCHAR         pcBuffer,
    size_t        stNBytes,
    off_t         oftPos)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t       sstWriteNum = PX_ERROR;

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (stNBytes) { //TODO __hoit_write��δ��Ӷ���
        /*sstWriteNum = __hoit_write(phoitn, pcBuffer, stNBytes, (size_t)oftPos, 1);*/
        if (sstWriteNum > 0) {
            //TODO HOITN_stSize��δ����
            /*pfdnode->FDNODE_oftSize   = (off_t)phoitn->HOITN_stSize;*/
        }
    } else {
        sstWriteNum = 0;
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (sstWriteNum);
}

/*********************************************************************************************************
** ��������: __hoitFsNRead
** ��������: hoitFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           piNRead          ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    } 

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    //TODO HOITN_stSize��δ����
    /* *piNRead = (INT)(phoitn->HOITN_stSize - (size_t)pfdentry->FDENTRY_oftPtr); */

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __hoitFsNRead64
** ��������: hoitFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           poftNRead        ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (poftNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    } 

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }   
    //TODO HOITN_stSize��δ����
    /* *poftNRead = (off_t)(phoitn->HOITN_stSize - (size_t)pfdentry->FDENTRY_oftPtr); */

    __HOIT_VOLUME_UNLOCK(pfs);

    return (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __ramFsRename
** ��������: ramFs rename ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcNewName        �µ�����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PHOIT_VOLUME        pfsNew;
    CHAR                cNewPath[PATH_MAX + 1];
    INT                 iError;

    
    PCHAR dirPath = (PCHAR)hoit_malloc(pfs, lib_strlen(pfdentry->FDENTRY_pcName) + 1);
    lib_bzero(dirPath, lib_strlen(pfdentry->FDENTRY_pcName) + 1);
    lib_memcpy(dirPath, pfdentry->FDENTRY_pcName, lib_strlen(pfdentry->FDENTRY_pcName));
    PCHAR pDivider = lib_rindex(dirPath, PX_DIVIDER);
    *pDivider = '\0';
    PHOIT_INODE_INFO pInodeFather = __hoit_open(pfs, dirPath, NULL, NULL, NULL, NULL, NULL);

    if (phoitn == LW_NULL) {                                             /*  ����Ƿ�Ϊ�豸�ļ�          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  ��֧���豸������            */
        return (PX_ERROR);
    }       

    if (pcNewName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return (PX_ERROR);
    }
    
    if (__STR_IS_ROOT(pcNewName)) {
        _ErrorHandle(ENOENT);
        return (PX_ERROR);
    }    

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pfsNew, 
                          cNewPath) != ERROR_NONE) {                    /*  �����Ŀ¼·��              */
        __HOIT_VOLUME_UNLOCK(pfs);
        return  (PX_ERROR);
    }

    if (pfsNew != pfs) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);        
    }

    //TODEBUG û����ȡ���׽ڵ�
    iError = __hoit_move(pInodeFather, phoitn, pcNewName);

    __HOIT_VOLUME_UNLOCK(pfs);
    hoit_free(pfs, dirPath, lib_strlen(pfdentry->FDENTRY_pcName) + 1);
    return  (iError);
}

/*********************************************************************************************************
** ��������: __hoitFsSeek
** ��������: hoitFs seek ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           oftOffset        ƫ����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (oftOffset > (size_t)~0) {
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    } 

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }   

    pfdentry->FDENTRY_oftPtr = oftOffset;
    //TODO HOITN_stSize��δ����

    if (phoitn->HOITN_stSize < (size_t)oftOffset) {
        phoitn->HOITN_stSize = (size_t)oftOffset;
    }


   __HOIT_VOLUME_UNLOCK(pfs);
   return   (ERROR_NONE);
}                         

/*********************************************************************************************************
** ��������: __hoitFsWhere
** ��������: hoitFs ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}

/*********************************************************************************************************
** ��������: __hoitFsStat
** ��������: hoitFs stat ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{/* ͨ���ļ����ƿ�fd��ȡ�ļ�״̬ */
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);        
    }

    __hoit_stat(phoitn, pfs, pstat);

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __hoitFsLStat
** ��������: hoitFs stat ����
** �䡡��  : pfs              hoitfs �ļ�ϵͳ
**           pcName           �ļ���
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsLStat(PHOIT_VOLUME  pfs, PCHAR  pcName, struct stat* pstat)
{/* ͨ���ļ�����ȡ�ļ�״̬ */
    BOOL                bRoot;
    PHOIT_INODE_INFO    phoitn;
    PHOIT_INODE_INFO    phoitnFather;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    phoitn = __hoit_open(pfs, pcName, &phoitnFather, LW_NULL, &bRoot, LW_NULL, LW_NULL);
    if (phoitn) {
        __hoit_stat(phoitn, pfs, pstat);
    } else if (bRoot) {
        __hoit_stat(LW_NULL, pfs, pstat);
    } else {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);        
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: __hoitFsStatfs
** ��������: hoitFs statfs ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstatfs          �ļ�ϵͳ״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }    

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    //TODO __hoit_statfs��δ�깤
    __hoit_statfs(pfs, pstatfs);

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE); 
}

/*********************************************************************************************************
** ��������: __hoitFsReadDir
** ��������: hoitFs ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    INT                 i;
    LONG                iStart;
    INT                 iError = ERROR_NONE;  
    PHOIT_INODE_INFO    pInodeInfo;
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }   

    //TODO ����֪���²���α���Ŀ¼����ʱ����
    PHOIT_FULL_DIRENT pFullDirent;
    if (phoitn == LW_NULL) {
        pFullDirent = pfs->HOITFS_pRootDir->HOITN_dents;
    } else {
        if (!S_ISDIR(phoitn->HOITN_mode)) {
            __HOIT_VOLUME_UNLOCK(pfs);
            _ErrorHandle(ENOTDIR);
            return  (PX_ERROR);
        }
        pFullDirent = phoitn->HOITN_dents;
    }

    iStart = dir->dir_pos;

    for (i = 0;
         (pFullDirent != LW_NULL) && (i < iStart);
         (pFullDirent = pFullDirent->HOITFD_next), (i++));         /*  ����                        */

    if (pFullDirent == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */

    } else {
        pInodeInfo = __hoit_get_full_file(pfs, pFullDirent->HOITFD_ino);
        dir->dir_pos++;

        lib_strlcpy(dir->dir_dirent.d_name,
                    pFullDirent->HOITFD_file_name,
                    sizeof(dir->dir_dirent.d_name));

        dir->dir_dirent.d_type = IFTODT(pInodeInfo->HOITN_mode);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  iError;
}

/*********************************************************************************************************
** ��������: __hoitFsTimeset
** ��������: hoitfs �����ļ�ʱ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           utim                utimbuf �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __hoitFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }    

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    } 

    if (phoitn) {
        //TODO PHOIT_INODE_INFOʱ����ر�����δ����

        /*phoitn->HOITN_timeAccess = utim->actime;
        phoitn->HOITN_timeChange = utim->modtime;*/
    } else {
        pfs->HOITFS_time = utim->modtime;
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);    
}

/*********************************************************************************************************
** ��������: __hoitFsTruncate
** ��������: hoitfs �����ļ���С
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __hoitFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    size_t              stTru;

    if (phoitn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }    

    if (oftSize < 0) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (oftSize > (size_t)~0) {
        _ErrorHandle(ENOSPC);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (S_ISDIR(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }  

    stTru = (size_t)oftSize;

    //TODO HOITN_stSize��δ���壬__hoit_increase��δʵ��
    /*if (stTru > phoitn->HOITN_stSize) {
        __hoit_increase();
    } else if (stTru < phoitn->HOITN_stSize) {
        __hoit_truncate(phoitn, stTru);
    }*/

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE); 
}

/*********************************************************************************************************
** ��������: __hoitFsChmod
** ��������: hoitfs chmod ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           iMode               �µ� mode
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __hoitFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;/* S_IFMT���ļ����͵�λ���룬����ζ�ű�����������ı��ļ����� */

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    if (phoitn) {
        phoitn->HOITN_mode &= S_IFMT;
        phoitn->HOITN_mode |= iMode;
    } else {
        pfs->HOITFS_mode &= S_IFMT;
        pfs->HOITFS_mode |= iMode;
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);     
}

/*********************************************************************************************************
** ��������: __hoitFsChown
** ��������: hoitfs chown ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pusr                �µ������û�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __hoitFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE         pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_INODE_INFO    phoitn  = (PHOIT_INODE_INFO)pfdnode->FDNODE_pvFile;
    PHOIT_VOLUME        pfs     = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }    
    if (phoitn) {
        phoitn->HOITN_uid = pusr->IOU_uid;
        phoitn->HOITN_gid = pusr->IOU_gid;
    } else {
        pfs->HOITFS_uid = pusr->IOU_uid;
        pfs->HOITFS_gid = pusr->IOU_gid;
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);   
}

/*********************************************************************************************************
** ��������: __hoitFsSymlink
** ��������: hoitFs �������������ļ�
** �䡡��  : pfs                 hoitfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsSymlink(PHOIT_VOLUME   pfs,
    PCHAR         pcName,
    CPCHAR        pcLinkDst)
{

    BOOL                bRoot;
    PHOIT_INODE_INFO    phoitn;
    PHOIT_INODE_INFO    phoitnFather;

    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    phoitn = __hoit_open(pfs, pcName, &phoitnFather, LW_NULL, &bRoot, LW_NULL, LW_NULL);
    if (phoitn || bRoot) { /* ͬ���ļ����ڻ�Ϊ���ڵ� */
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);        
    }
    
    //TODO �����ӽ���������δʵ��
    phoitn = __hoit_maken(pfs, pcName, phoitnFather, S_IFLNK | DEFAULT_SYMLINK_PERM, pcLinkDst);
    if (phoitn == LW_NULL) {
        __HOIT_VOLUME_UNLOCK(pfs);
        return  (PX_ERROR);
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __hoitFsReadlink
** ��������: hoitFs ��ȡ���������ļ�����
** �䡡��  : pfs                 hoitfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __hoitFsReadlink(PHOIT_VOLUME   pfs,
    PCHAR         pcName,
    PCHAR         pcLinkDst,
    size_t        stMaxSize)
{

    size_t      stLen;
    PHOIT_INODE_INFO    phoitn;

    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    phoitn = __hoit_open(pfs, pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if ((phoitn == LW_NULL) || !S_ISLNK(phoitn->HOITN_mode)) {
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    //TODO ��δ����HOITN_pcLink�������Ӳ��ֹ���Ҳ��δʵ��
    
    stLen = lib_strlen(phoitn->HOITN_pcLink);
    lib_strncpy(pcLinkDst, phoitn->HOITN_pcLink, stMaxSize);
    

    if (stLen > stMaxSize) {
        stLen = stMaxSize;
    }
    __HOIT_VOLUME_UNLOCK(pfs);

    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** ��������: __hoitFsIoctl
** ��������: hoitFs ioctl ����
** �䡡��  : pfdentry           �ļ����ƿ�
**           request,           ����
**           arg                �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __hoitFsIoctl(PLW_FD_ENTRY  pfdentry,
    INT           iRequest,
    LONG          lArg)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_VOLUME   pfs      = (PHOIT_VOLUME)pfdnode->FDNODE_pvFsExtern;
    off_t         oftTemp;
    INT           iError;

    switch (iRequest) {
    case FIOCONTIG:
    case FIOTRUNC:
    case FIOLABELSET:
    case FIOATTRIBSET:
        if ((pfdentry->FDENTRY_iFlag & O_ACCMODE) == O_RDONLY) {
            _ErrorHandle(ERROR_IO_WRITE_PROTECTED);
            return  (PX_ERROR);
        }
	}
    
	switch (iRequest) {
	
	case FIODISKINIT:                                                   /*  ���̳�ʼ��                  */
        return  (ERROR_NONE);
        
    case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
        oftTemp = *(off_t *)lArg;
        return  (__hoitFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __hoitFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__hoitFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __hoitFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  �ļ�������                  */
        return  (__hoitFsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  ��ȡ���                    */
    case FIOLABELSET:                                                   /*  ���þ��                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__hoitFsStat(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__hoitFsStatfs(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__hoitFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
        return  (__hoitFsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
        oftTemp = *(off_t *)lArg;
        return  (__hoitFsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIOFLUSH:
    case FIODATASYNC:
        hoitFlushCache(pfs->HOITFS_cacheHdr, (PHOIT_CACHE_BLK)-1);
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__hoitFsChmod(pfdentry, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */
    
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOCHOWN:                                                      /*  �޸��ļ�������ϵ            */
        return  (__hoitFsChown(pfdentry, (LW_IO_USR *)lArg));
    
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "RAM FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = pfs->HOITFS_bForceDelete;
        return  (ERROR_NONE);
        
#if LW_CFG_FS_SELECT_EN > 0
    case FIOSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            SEL_WAKE_UP((PLW_SEL_WAKEUPNODE)lArg);                      /*  ���ѽڵ�                    */
        }
        return  (ERROR_NONE);
         
    case FIOUNSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            LW_SELWUN_SET_READY((PLW_SEL_WAKEUPNODE)lArg);
        }
        return  (ERROR_NONE);
#endif                                                                  /*  LW_CFG_FS_SELECT_EN > 0     */
        
    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

}

/*********************************************************************************************************
** ��������: __hoitFsHardlink
** ��������: hoitFs ����Ӳ�����ļ�
** �䡡��  : pfs                 hoitfs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  __hoitFsHardlink(PHOIT_VOLUME   pfs,
    PCHAR         pcName,
    CPCHAR        pcLinkDst)
{

    BOOL                bRoot;
    PHOIT_INODE_INFO    phoitn;
    PHOIT_INODE_INFO    phoitnFather;

    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    PLW_DEV_HDR    pdevhdrHdr;
    CHAR           cFullFileName[MAX_FILENAME_LENGTH] = { 0 };
    CHAR           cFullLinkDst[MAX_FILENAME_LENGTH] = { 0 };

    API_IoFullFileNameGet(pcName, &pdevhdrHdr, cFullFileName);
    API_IoFullFileNameGet(pcLinkDst, &pdevhdrHdr, cFullLinkDst);

    if (__HOIT_VOLUME_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    phoitn = __hoit_open(pfs, cFullFileName, &phoitnFather, LW_NULL, &bRoot, LW_NULL, LW_NULL);
    if (phoitn || bRoot) { /* ͬ���ļ����ڻ�Ϊ���ڵ� */
        __HOIT_VOLUME_UNLOCK(pfs);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }

    //����Ӳ����
    phoitn = __hoit_maken(pfs, cFullFileName, phoitnFather, S_IFSOCK | DEFAULT_SYMLINK_PERM, cFullLinkDst);
    if (phoitn == LW_NULL) {
        __HOIT_VOLUME_UNLOCK(pfs);
        return  (PX_ERROR);
    }

    __HOIT_VOLUME_UNLOCK(pfs);

    return  (ERROR_NONE);
}

//#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */
#endif // HOITFS_DISABLE

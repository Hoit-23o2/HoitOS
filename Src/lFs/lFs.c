#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"

/*********************************************************************************************************
  ����ͷ�ļ�
*********************************************************************************************************/
#include "lFs.h"
#include "lFsLib.h"
#include "../driver/mtd/nor/fake_nor.h"
#include "../driver/mtd/nor/fake_nor_cmd.h"
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT                              _G_iLFsDrvNum = PX_ERROR;

/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __LFS_FILE_LOCK(plfsn)        API_SemaphoreMPend(plfsn->RAMN_pramfs->LFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __LFS_FILE_UNLOCK(plfsn)      API_SemaphoreMPost(plfsn->RAMN_pramfs->LFS_hVolLock)

#define __LFS_VOL_LOCK(plfs)        API_SemaphoreMPend(plfs->LFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __LFS_VOL_UNLOCK(plfs)      API_SemaphoreMPost(plfs->LFS_hVolLock)

/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LONG     __LFsOpen(PLFS_VOLUME     plfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __LFsRemove(PLFS_VOLUME   plfs,
                              PCHAR         pcName);
static INT      __LFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __LFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes);
static ssize_t  __LFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos);
static ssize_t  __LFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes);
static ssize_t  __LFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __LFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset);
static INT      __LFsStat (PLW_FD_ENTRY  pfdentry, 
                            struct stat *pstat);
static INT      __LFsLStat(PLFS_VOLUME   plfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __LFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __LFsSymlink(PLFS_VOLUME   plfs,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __LFsReadlink(PLFS_VOLUME   plfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
static INT  __LFsWhere (PLW_FD_ENTRY  pfdentry, 
                        off_t  *poftPos);
static INT  __LFsFsSeek(PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset);

/*********************************************************************************************************
** ��������: API_LFSDrvInstall
** ��������: ��װ lfs �ļ�ϵͳ��������
** �䡡��  :
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LFSDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iLFsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __LFsOpen;
    fileop.fo_release  = __LFsRemove;
    fileop.fo_open     = __LFsOpen;
    fileop.fo_close    = __LFsClose;
    fileop.fo_read     = __LFsRead;
    fileop.fo_read_ex  = __LFsPRead;
    fileop.fo_write    = __LFsWrite;
    fileop.fo_write_ex = __LFsPWrite;
    fileop.fo_lstat    = __LFsLStat;
    fileop.fo_ioctl    = __LFsIoctl;
    fileop.fo_symlink  = __LFsSymlink;
    fileop.fo_readlink = __LFsReadlink;

    _G_iLFsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  ʹ�� NEW_1 ���豸��������   */

    DRIVER_LICENSE(_G_iLFsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iLFsDrvNum,      "Hoit");
    DRIVER_DESCRIPTION(_G_iLFsDrvNum, "lfs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "log-structure file system installed.\r\n");
                                     
    __fsRegister("lfs", API_LFsDevCreate, LW_NULL, LW_NULL);        /*  ע���ļ�ϵͳ                */

    return  ((_G_iLFsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}

/*********************************************************************************************************
** ��������: API_LFsDevCreate
** ��������: ���� lfs �ļ�ϵͳ�豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ʹ�� pblkd->BLKD_pcName ��Ϊ ����С ��ʾ.
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PLFS_VOLUME     plfs;
    INT             i;
    if (_G_iLFsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "lfs Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    if ((pcName == LW_NULL) || __STR_IS_ROOT(pcName)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "mount name invalidate.\r\n");
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }

    plfs = (PLFS_VOLUME)__SHEAP_ALLOC(sizeof(LFS_VOLUME));
    if (plfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(plfs, sizeof(LFS_VOLUME));                              /*  ��վ���ƿ�                */
    
    plfs->LFS_bValid = LW_TRUE;
    plfs->LFS_bForceDelete = LW_FALSE;

    plfs->LFS_hVolLock = API_SemaphoreMCreate("lfsvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!plfs->LFS_hVolLock) {                                      /*  �޷���������                */
        __SHEAP_FREE(plfs);
        return  (PX_ERROR);
    }
    
    plfs->LFS_cpr = (PCHECKPOINT_REGION)__SHEAP_ALLOC(sizeof(CHECKPOINT_REGION));
    if (plfs->LFS_cpr == LW_NULL) {                                          /* ����CPR���� */
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }    

    lib_bzero(plfs->LFS_cpr,sizeof(CHECKPOINT_REGION));
    /* һ��ʼû��д��imap��summary��������ǵ�sector����Ϊ -1 */
    (plfs->LFS_cpr)->imap_sec = (UINT) -1;
    (plfs->LFS_cpr)->summary_sec = (UINT) -1;

    /* ������������Щ����Ҫ��ȡflash�ϵ���Ϣ���ܼ����������������� */
    plfs->LFS_availableFile = MAX_FILE;
    plfs->LFS_curBlockNo = 2;
    plfs->LFS_availableSector = 0;
    for ( i = 0; i < MAX_FILE+1; i++)
    {
        plfs->LFS_imap[i] = (UINT)-1;
    }
    

    plfs->LFS_mode          = S_IFDIR | DEFAULT_DIR_PERM;
    plfs->LFS_uid           = getuid();
    plfs->LFS_gid           = getgid();
    plfs->LFS_time          = lib_time(LW_NULL);
    plfs->LFS_ulCurSector   = 0;
    plfs->LFS_ulMaxSector   = (NOR_FLASH_NSECTOR-2*NOR_FLASH_SECTPBLK); /* ��ȥ����CPR_FN��Ŀռ� */
    
    printk("size of plfs:%d",sizeof(LFS_VOLUME));
    if (iosDevAddEx(&plfs->LFS_devhdrHdr, pcName, _G_iLFsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        API_SemaphoreMDelete(&plfs->LFS_hVolLock);
        __SHEAP_FREE(plfs);
        return  (PX_ERROR);
    }

    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}

/*********************************************************************************************************
** ��������: API_LFsDevDelete
** ��������: ɾ��һ�� lfs �ļ�ϵͳ�豸, ����: API_RamFsDevDelete("/mnt/ram0");
** �䡡��  : pcName            �ļ�ϵͳ�豸��(�����豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_LFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}

/*********************************************************************************************************
** ��������: __LFsOpen
** ��������: �򿪻��ߴ����ļ�
** �䡡��  : plfs           lfs �ļ�ϵͳ
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            mode_t
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LONG            __LFsOpen(PLFS_VOLUME     plfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode)
{
    PLW_FD_NODE pfdnode;
    PLFS_INODE  plfsn;
    BOOL        bCreate = LW_FALSE;
    BOOL        bIsNew;
    BOOL        bRoot;
    struct stat statGet;
    // if (!S_ISREG(iMode))
    // {
    //     printk("lfs only support file\r\n");
    //     return PX_ERROR;
    // }
    

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
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ��֧��������Щ��ʽ          */
            return  (PX_ERROR);
        }
    }
    
    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }    

    plfsn = __lfs_open(plfs, pcName, &bRoot);
    
    if (plfsn){
        if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  ���������ļ�                */
            __LFS_VOL_UNLOCK(plfs);
            _ErrorHandle(EEXIST);                                   /*  �Ѿ������ļ�                */
            return  (PX_ERROR);
        
        } else {
            goto    __file_open_ok;
        }
    } else if ((iFlags & O_CREAT)) {
        plfsn = __lfs_maken(plfs,pcName,iMode);
        if(plfsn) {
            bCreate = LW_TRUE;
            goto    __file_open_ok;
        }
        else {
            goto    __file_create_error;
        }
    }

    if(!plfsn && bRoot == LW_FALSE) {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(ENOENT);                                           /*  û���ҵ��ļ�                */
        return  (PX_ERROR);        
    }

__file_open_ok:
    __lfs_stat(plfsn,plfs,&statGet);          /* ��ȡ�ļ�״̬������ϵͳ�Ĵ��ļ��б�������µ�fdNode */
    
    pfdnode = API_IosFdNodeAdd(&plfs->LFS_plineFdNodeHeader,
                               statGet.st_dev,
                               (ino64_t)statGet.st_ino,
                               iFlags,
                               iMode,
                               statGet.st_uid,
                               statGet.st_gid,
                               statGet.st_size,
                               (PVOID)plfsn,
                               &bIsNew); 

    if (pfdnode == LW_NULL) {                                           /*  �޷����� fd_node �ڵ�       */
        __LFS_VOL_UNLOCK(plfs);
        if (bCreate) {
            //__ram_unlink(plfsn);                                        /*  ɾ���½��Ľڵ�              */
        }
        return  (PX_ERROR);
    }
    if(plfsn && bCreate)
    {
        plfs->LFS_inodeMap[plfsn->LFSN_inodeNo] = (UINT8)-1;
        writeInode(plfs,plfsn,plfsn->LFSN_inodeNo);
    }
        

    pfdnode->FDNODE_pvFsExtern = (PVOID)plfs;                         /*  ��¼�ļ�ϵͳ��Ϣ            */
    
    LW_DEV_INC_USE_COUNT(&plfs->LFS_devhdrHdr);                     /*  ���¼�����                  */

    __LFS_VOL_UNLOCK(plfs);

    return ((LONG)pfdnode);

__file_create_error:
    __SHEAP_FREE(plfsn->LFSN_pcname);
    __SHEAP_FREE(plfsn);
    __LFS_VOL_UNLOCK(plfs);
    return ((LONG)LW_NULL);
}

/*********************************************************************************************************
** ��������: __LFsRemove
** ��������: lfs remove ����
** �䡡��  : plfs           ���豸
**           pcName           �ļ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/

INT             __LFsRemove(PLFS_VOLUME   plfs,
                            PCHAR         pcName)
{
    PLFS_INODE  plfsn;
    BOOL        bRoot;
    INT         iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
        
    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }   
    plfsn = __lfs_open(plfs, pcName, &bRoot);
    if(plfsn) {
        if (S_ISLNK(plfsn->LFSN_mode)) { //��֧��Synth Link
            __LFS_VOL_UNLOCK(plfs);
            return  (PX_ERROR);
        }

        iError = __lfs_unlink(plfs, plfsn);
        __LFS_VOL_UNLOCK(plfs);
        return (iError);
    } else if (bRoot) {
        if (plfs->LFS_bValid == LW_FALSE) {
            __LFS_VOL_UNLOCK(plfs);
            return (ERROR_NONE);
        }

__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)plfs)) {
            if(!plfs->LFS_bForceDelete) {
                __LFS_VOL_UNLOCK(plfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);            
            }

            plfs->LFS_bValid = LW_FALSE;

            __LFS_VOL_UNLOCK(plfs);
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&plfs->LFS_devhdrHdr);             /*  ����������ļ���Ϊ�쳣ģʽ��֮���ǿ�ƹر����д��ļ�  */          

            __LFS_VOL_LOCK(plfs);
            goto    __re_umount_vol;

        } else {
            plfs->LFS_bValid = LW_FALSE;
        }

        /* ���豸�б���ɾ���豸 */
        iosDevDelete((LW_DEV_HDR *)plfs); 
        /* ɾ���ź��� */
        API_SemaphoreMDelete(&plfs->LFS_hVolLock);

        __lfs_unmount(plfs);
        __SHEAP_FREE(plfs);

        _DebugHandle(__LOGMESSAGE_LEVEL, "lfs unmount ok.\r\n");
        return  (ERROR_NONE);
    } else {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);        
    }
}

static INT      __LFsClose(PLW_FD_ENTRY   pfdentry)
{
    PLW_FD_NODE pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE  plfsn   = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME plfs    = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;
    BOOL        bRemove = LW_FALSE;

    if(__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);        
    }

    /*
        API_IosFdNodeDec����fd���������������Ϊ0����ɾ��fd�ڵ㡣
    */
    if(API_IosFdNodeDec(&plfs->LFS_plineFdNodeHeader, 
                            pfdnode, &bRemove) == 0) {
        if (plfsn)
        {
            __lfs_close(plfsn,pfdentry->FDENTRY_iFlag);
        }
    }

    LW_DEV_DEC_USE_COUNT(&plfs->LFS_devhdrHdr);

    if (bRemove && plfsn) {
        __lfs_unlink(plfs, plfsn);
    }

    __LFS_VOL_UNLOCK(plfs);
    return ERROR_NONE;
}
/*
    LFS read����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С    
*/
ssize_t         __LFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes)
{
    PLW_FD_NODE     pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE      plfsn       = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME     plfs        = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t         sstReadNum  = PX_ERROR;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (plfsn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }    

    if (S_ISDIR(plfsn->LFSN_mode)) {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    if (stMaxBytes) {
        sstReadNum = __lfs_read(plfs, plfsn, pcBuffer, stMaxBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstReadNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;              /*  �����ļ�ָ��                */
        }
    
    } else {
        sstReadNum = 0;
    }    

    __LFS_VOL_UNLOCK(plfs);
    return (sstReadNum);
}
ssize_t         __LFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos)
{
    return ERROR_NONE;
}
/*
    LFS write����
�䡡��  :   pfdentry         �ļ����ƿ�
            pcBuffer         ������
            stNBytes         ��Ҫд������ݵĸ���   
*/
ssize_t         __LFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes)
{
    PLW_FD_NODE     pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE      plfsn       = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME     plfs        = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t       sstWriteNum = PX_ERROR;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (plfsn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    /*
        LFS û��ͨ�� plfsn ��ȡ���ļ��� plfs����������ֻ�ܴ�pfdnode���ȡ��
        ��Ϊ�ļ�ϵͳ������
    */
    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(plfsn->LFSN_mode)) {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }  
    /* ԭ���ļ�ָ����pfdentry��ά�������������ļ�ϵͳ�ڲ�ά�� */
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }

    if (stNBytes) {
        sstWriteNum = __lfs_write(plfs, plfsn, pcBuffer, stNBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstWriteNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;             /*  �����ļ�ָ��                */
            pfdnode->FDNODE_oftSize   = (off_t)plfsn->LFSN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }

    __LFS_VOL_UNLOCK(plfs);

    return  (sstWriteNum);
}
ssize_t         __LFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos)
{
    return ERROR_NONE;
}
INT             __LFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset)
{
    return ERROR_NONE;
}

static INT  __LFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE     plfsn   = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME   plfs  = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }    

    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    __lfs_stat(plfsn, plfs, pstat);

    __LFS_VOL_UNLOCK(plfs);

    return  (ERROR_NONE);
}

INT             __LFsLStat(PLFS_VOLUME   plfs,
                             PCHAR         pcName,
                             struct stat  *pstat)
{
    return ERROR_NONE;
}
/*
    ����û��ʵ��Ŀ¼�ļ���ReadDirͳһ��ʾLFS��Ŀ¼�µ��ļ�
*/
static INT  __LFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    //PLFS_INODE    plfsn   = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME   plfs  = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;

    INT                 iError = ERROR_NONE;    
    LONG                iStart;    
    INT                i;
    PLFS_INODE          plfsnTemp;
    BOOL                bGet = LW_FALSE;


    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iStart = dir->dir_pos;          /* idr->dir_pos��¼Inode��� */

    for( i = iStart ; i < MAX_FILE ; i++)
    {
        if(plfs->LFS_inodeMap[i]){
            plfsnTemp = getInodeFromInumber(plfs,i);
            if(plfsnTemp == LW_NULL)
                continue;

            iError = getFileNameOfInodeNumber(plfs, dir->dir_dirent.d_name, i);
            if (iError == PX_ERROR )
                continue;

            dir->dir_pos++;
            dir->dir_dirent.d_type = IFTODT(plfsnTemp->LFSN_mode);
            dir->dir_dirent.d_shortname[0] = PX_EOS;
            bGet = LW_TRUE;
            break;
        }
        dir->dir_pos++;
    }

    if (bGet)
    {
        iError = ERROR_NONE;
    }
    else
    {
        _ErrorHandle(ENOTDIR);
        iError = PX_ERROR;
    }
    

    __LFS_VOL_UNLOCK(plfs);
    return (iError);
}
/*
    LFS �ļ�ϵͳ nread���������ص�ǰָ�뵽�ļ�β֮�����������
** �䡡��  : pfdentry         �ļ����ƿ�
**           piNRead          ʣ��������
*/
static INT  __LFsNRead(PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE     pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE      plfsn   = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME     plfs    = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (plfsn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }    

    if (S_ISDIR(plfs->LFS_mode)) {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }

    *piNRead = (INT)(plfsn->LFSN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);

    __LFS_VOL_UNLOCK(plfs);

    return (ERROR_NONE);
}

/*
**  LFS rename ����
**  �䡡��  : pfdentry         �ļ����ƿ�
**           pcNewName        �µ�����
*/
static INT  __LFsRename(PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    PLW_FD_NODE     pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE      plfsn   = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME     plfs    = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PLFS_VOLUME     plfsNew;
    CHAR            cNewPath[PATH_MAX + 1];
    INT             iError;

    if (plfsn == LW_NULL) {                                             /*  ����Ƿ�Ϊ�豸�ļ�          */
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

    /* ��һ����Ϊ��ȥ */
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&plfsNew, 
                          cNewPath) != ERROR_NONE) {                    /*  �����Ŀ¼·��              */
        __LFS_VOL_UNLOCK(plfs);
        return  (PX_ERROR);
    } 
    
    if (plfsNew != plfs) {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);        
    }

    iError = __lfs_move(plfs, plfsn, cNewPath);

    __LFS_VOL_UNLOCK(plfs);

    return (iError);
}


INT             __LFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg)
{
    PLW_FD_NODE     pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_VOLUME     plfs  = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;
    off_t           oftTemp;
    INT             iError;

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

    switch (iRequest)
    {
        case FIOSEEK:                                                       /*  �ļ��ض�λ                  */
            oftTemp = *(off_t *)lArg;
            return  (__LFsFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __LFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

        case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
            return  (__LFsNRead(pfdentry, (INT *)lArg));
            
        case FIORENAME:                                                     /*  �ļ�������                  */
            return  (__LFsRename(pfdentry, (PCHAR)lArg));

        case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
            return  (__LFsStat(pfdentry, (struct stat *)lArg));
        
        case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
            return  (__LFsReadDir(pfdentry, (DIR *)lArg)); 

        case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
            *(BOOL *)lArg = plfs->LFS_bForceDelete;
            return  (ERROR_NONE);  

        case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
            *(PCHAR *)lArg = "Log-structure FileSystem";
            return  (ERROR_NONE);
        default:
            _ErrorHandle(ENOSYS);
            return  (PX_ERROR);
    }
}

static INT  __LFsFsSeek(PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE     pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PLFS_INODE      plfsn   = (PLFS_INODE)pfdnode->FDNODE_pvFile;
    PLFS_VOLUME     plfs    = (PLFS_VOLUME)pfdnode->FDNODE_pvFsExtern;

    if (plfsn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset > (size_t)~0) {
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    }  

    if(__LFS_VOL_LOCK(plfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);       
    }  

    if (S_ISDIR(plfsn->LFSN_mode)) {
        __LFS_VOL_UNLOCK(plfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);        
    }

    pfdentry->FDENTRY_oftPtr = oftOffset;
    if(plfsn->LFSN_stVSize< (size_t)oftOffset) {
        plfsn->LFSN_stVSize = (size_t)oftOffset;
    }
    __LFS_VOL_UNLOCK(plfs);
    return (ERROR_NONE);
}
/*
    LFS ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
*/
static INT  __LFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);    
}

INT             __LFsSymlink(PLFS_VOLUME   plfs,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst)
{
    return ERROR_NONE;
}
ssize_t         __LFsReadlink(PLFS_VOLUME   plfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    return ERROR_NONE;
}

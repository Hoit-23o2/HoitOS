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
** ��   ��   ��: spifFs.c
**
** ��   ��   ��: ������
**
** �ļ���������: 2021 �� 06 �� 01��
**
** ��        ��: Spiffs�ļ�ϵͳ�ӿڲ�
*********************************************************************************************************/
#define SPIFFS_DISABLE
#ifndef SPIFFS_DISABLE
#include "spifFs.h"
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "spifFsLib.h"
#include "spifFsFDLib.h"
#include "spifFsCmd.h"
/*********************************************************************************************************
  �ڲ�ȫ�ֱ���
*********************************************************************************************************/
static INT                              _G_iSpiffsDrvNum = PX_ERROR;



/*********************************************************************************************************
  �ļ�����
*********************************************************************************************************/
#define __SPIFFS_FILE_TYPE_NODE          0                               /*  open ���ļ�               */
#define __SPIFFS_FILE_TYPE_DIR           1                               /*  open ��Ŀ¼               */
#define __SPIFFS_FILE_TYPE_DEV           2                               /*  open ���豸               */
/*********************************************************************************************************
  �����
*********************************************************************************************************/
#define __SPIFFS_VOL_LOCK(pspiffs)        API_SemaphoreMPend(pspiffs->SPIFFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __SPIFFS_VOL_UNLOCK(pspiffs)      API_SemaphoreMPost(pspiffs->SPIFFS_hVolLock)
/*********************************************************************************************************
  �ڲ�����
*********************************************************************************************************/
static LONG     __spifFsOpen(PSPIF_VOLUME     pfs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __spifFsRemove(PSPIF_VOLUME   pfs,
                              PCHAR         pcName);
static INT      __spifFsClose(PLW_FD_ENTRY   pfdentry);
static ssize_t  __spifFsRead(PLW_FD_ENTRY    pfdentry,
                            PCHAR           pcBuffer,
                            size_t          stMaxBytes);
static ssize_t  __spifFsPRead(PLW_FD_ENTRY    pfdentry,
                             PCHAR           pcBuffer,
                             size_t          stMaxBytes,
                             off_t           oftPos);
static ssize_t  __spifFsWrite(PLW_FD_ENTRY  pfdentry,
                             PCHAR         pcBuffer,
                             size_t        stNBytes);
static ssize_t  __spifFsPWrite(PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes,
                              off_t         oftPos);
static INT      __spifFsSeek(PLW_FD_ENTRY  pfdentry,
                            off_t         oftOffset);
static INT      __spifFsWhere(PLW_FD_ENTRY pfdentry,
                             off_t       *poftPos);
static INT      __spifFsStat(PLW_FD_ENTRY  pfdentry, 
                            struct stat  *pstat);
static INT      __spifFsLStat(PSPIF_VOLUME   pfs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __spifFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __spifFsSymlink(PSPIF_VOLUME   pfs,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __spifFsReadlink(PSPIF_VOLUME   pfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
/*********************************************************************************************************
** ��������: API_SpifFsDrvInstall
** ��������: ��װ spiffs �ļ�ϵͳ��������
** �䡡��  :
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_SpifFsDrvInstall (VOID)
{
    struct file_operations     fileop;
    
    if (_G_iSpiffsDrvNum > 0) {
        return  (ERROR_NONE);
    }
    
    lib_bzero(&fileop, sizeof(struct file_operations));

    fileop.owner       = THIS_MODULE;
    fileop.fo_create   = __spifFsOpen;
    fileop.fo_release  = __spifFsRemove;
    fileop.fo_open     = __spifFsOpen;
    fileop.fo_close    = __spifFsClose;
    fileop.fo_read     = __spifFsRead;
    fileop.fo_read_ex  = __spifFsPRead;
    fileop.fo_write    = __spifFsWrite;
    fileop.fo_write_ex = __spifFsPWrite;
    fileop.fo_lstat    = __spifFsLStat;
    fileop.fo_ioctl    = __spifFsIoctl;
    fileop.fo_symlink  = __spifFsSymlink;
    fileop.fo_readlink = __spifFsReadlink;
    
    _G_iSpiffsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  ʹ�� NEW_1 ���豸��������   */

    DRIVER_LICENSE(_G_iSpiffsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iSpiffsDrvNum,      "HITSZ.HoitGroup");
    DRIVER_DESCRIPTION(_G_iSpiffsDrvNum, "spiffs file system.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "spif file system installed.\r\n");
                                     
    __fsRegister("spiffs", API_SpifFsDevCreate, LW_NULL, LW_NULL);        /*  ע���ļ�ϵͳ                */

    return  ((_G_iSpiffsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: API_SpifFsDevCreate
** ��������: ���� spiffs �ļ�ϵͳ�豸.
** �䡡��  : pcName            �豸��(�豸�ҽӵĽڵ��ַ)
**           pblkd             ʹ�� pblkd->BLKD_pcName ��Ϊ ����С ��ʾ.
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_SpifFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PSPIF_VOLUME    pfs;
    size_t          stMax;

    if (_G_iSpiffsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "spiffs Driver invalidate.\r\n");
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
    if (sscanf(pblkd->BLKD_pcName, "%zu", &stMax) != 1) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "max size invalidate.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
   
    pfs                         = (PSPIF_VOLUME)__SHEAP_ALLOC(sizeof(SPIF_VOLUME));

    if (pfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pfs, sizeof(SPIF_VOLUME));                              /*  ��վ���ƿ�                */
    
    pfs->SPIFFS_bValid = LW_TRUE;
    
    pfs->SPIFFS_hVolLock = API_SemaphoreMCreate("spifvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!pfs->SPIFFS_hVolLock) {                                      /*  �޷���������                */
        __SHEAP_FREE(pfs);
        return  (PX_ERROR);
    }
    
    pfs->SPIFFS_mode     = S_IFDIR | DEFAULT_DIR_PERM;
    pfs->SPIFFS_uid      = getuid();
    pfs->SPIFFS_gid      = getgid();
    pfs->SPIFFS_time     = lib_time(LW_NULL);
    pfs->SPIFFS_ulCurBlk = 0ul;
    pfs->pfs             = (PSPIFFS_VOLUME)__SHEAP_ALLOC(sizeof(SPIFFS_VOLUME));
    
    //TODO ���غ���
    __spif_mount(pfs);

    register_spiffs_cmd(pfs);
    
    if (iosDevAddEx(&pfs->SPIFFS_devhdrHdr, pcName, _G_iSpiffsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        API_SemaphoreMDelete(&pfs->SPIFFS_hVolLock);
        __SHEAP_FREE(pfs);
        return  (PX_ERROR);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SpifFsDevDelete
** ��������: ɾ��һ�� spiffs �ļ�ϵͳ�豸, ����: API_SpifFsDevDelete("/mnt/spif0");
** �䡡��  : pcName            �ļ�ϵͳ�豸��(�����豸�ҽӵĽڵ��ַ)
** �䡡��  : < 0 ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
                                           API ����
*********************************************************************************************************/
LW_API
INT  API_SpifFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  ������豸, �����ж���豸  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __spifFsOpen
** ��������: �򿪻��ߴ����ļ�
** �䡡��  : pfs           spiffs �ļ�ϵͳ
**           pcName           �ļ���
**           iFlags           ��ʽ
**           iMode            mode_t
** �䡡��  : < 0 ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG __spifFsOpen (PSPIF_VOLUME     pfs,
                          PCHAR            pcName,
                          INT              iFlags,
                          INT              iMode)
{
    PLW_FD_NODE   pfdnode;
    PSPIFN_NODE   pspifn;

    BOOL          bIsNew;
    BOOL          bIsRoot;
    BOOL          bCreate;
    struct stat   statGet;
    
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
            S_ISCHR(iMode)  ||
            S_ISDIR(iMode) ) {                                          /*   ��֧��Ŀ¼��SPIFFS��ƽ���ļ�ϵͳ����ʱ��֧�֣��ܲ���֧���أ��� */
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  ��֧��������Щ��ʽ          */
            return  (PX_ERROR);
        }
    }
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    //TODO �ļ��򿪺���
    pspifn = __spif_open(pfs, pcName, iFlags, iMode, &bIsRoot);
    if(pspifn == LW_NULL && bIsRoot == LW_FALSE){
        __SPIFFS_VOL_UNLOCK(pfs);
        _ErrorHandle(ENOENT);                                           /*  û���ҵ��ļ�                */
        return  (PX_ERROR);
    }
    __spif_stat(pfs, pspifn, &statGet);
    
    pfdnode = API_IosFdNodeAdd(&pfs->SPIFFS_plineFdNodeHeader,
                               statGet.st_dev,
                               (ino64_t)statGet.st_ino,
                               iFlags,
                               iMode,
                               statGet.st_uid,
                               statGet.st_gid,
                               statGet.st_size,
                               (PVOID)pspifn,
                               &bIsNew);                                /*  ����ļ��ڵ�                */
    if (pfdnode == LW_NULL) {                                           /*  �޷����� fd_node �ڵ�       */
        __SPIFFS_VOL_UNLOCK(pfs);
        if (bCreate) {
            //__spif_unlink(pspifn);                                      /*  ɾ���½��Ľڵ�              */
            //spiffs
            __spif_remove(pfs, pspifn);
        }
        return  (PX_ERROR);
    }
    
    pfdnode->FDNODE_pvFsExtern = (PVOID)pfs;                         /*  ��¼�ļ�ϵͳ��Ϣ            */
    
    LW_DEV_INC_USE_COUNT(&pfs->SPIFFS_devhdrHdr);                     /*  ���¼�����                  */
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  ((LONG)pfdnode);                                           /*  �����ļ��ڵ�                */
}
/*********************************************************************************************************
** ��������: __spifFsRemove
** ��������: spiffs remove ����
** �䡡��  : pfs           ���豸
**           pcName           �ļ���
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsRemove (PSPIF_VOLUME   pfs,
                            PCHAR         pcName)
{
    PSPIFN_NODE  pspifn;
    BOOL         bIsRoot;
    PCHAR        pcTail;
    INT          iError;
    INT          iRes; 

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
        
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    pspifn = __spif_open(pfs, pcName, O_RDWR, 0, &bIsRoot);
    if (pspifn) {
        //TODO �ļ�ɾ������
        iRes = __spif_remove(pfs, pspifn);  
        if(iRes == SPIFFS_OK){
            iError = ERROR_NONE;
        }
        __SPIFFS_VOL_UNLOCK(pfs);
        return  (iError);
            
    } 
    else if (bIsRoot) {                                                 /*  ɾ�� spiffs �ļ�ϵͳ         */
        if (pfs->SPIFFS_bValid == LW_FALSE) {
            __SPIFFS_VOL_UNLOCK(pfs);
            return  (ERROR_NONE);                                       /*  ���ڱ���������ж��          */
        }
        
__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pfs)) {
            if (!pfs->SPIFFS_bForceDelete) {
                __SPIFFS_VOL_UNLOCK(pfs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            pfs->SPIFFS_bValid = LW_FALSE;
            
            __SPIFFS_VOL_UNLOCK(pfs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&pfs->SPIFFS_devhdrHdr);               /*  ����������ļ���Ϊ�쳣ģʽ  */
            
            __SPIFFS_VOL_LOCK(pfs);
            goto    __re_umount_vol;
        
        } 
        else {
            pfs->SPIFFS_bValid = LW_FALSE;
        }
        
        iosDevDelete((LW_DEV_HDR *)pfs);                             /*  IO ϵͳ�Ƴ��豸             */
        API_SemaphoreMDelete(&pfs->SPIFFS_hVolLock);
        //TODO �ļ�ϵͳж�غ���
        __spif_unmount(pfs);                                          /*  �ͷ������ļ�����            */
        __SHEAP_FREE(pfs);
        
        _DebugHandle(__LOGMESSAGE_LEVEL, "spiffs unmount ok.\r\n");
        
        return  (ERROR_NONE);
        
    } else {
        __SPIFFS_VOL_UNLOCK(pfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: __spifFsClose
** ��������: spiffs close ����
** �䡡��  : pfdentry         �ļ����ƿ�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME  pfs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    BOOL          bRemove = LW_FALSE;
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    if (API_IosFdNodeDec(&pfs->SPIFFS_plineFdNodeHeader, 
                         pfdnode, &bRemove) == 0) {
        if (pspifn) {
            __spif_close(pfs, pspifn);
        }   
    }
    
    LW_DEV_DEC_USE_COUNT(&pfs->SPIFFS_devhdrHdr);
    
    if (bRemove && pspifn) {
        //__spif_unlink(pspifn);
        __spif_remove(pfs, pspifn);
    }
        
    __SPIFFS_VOL_UNLOCK(pfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsRead
** ��������: spiffs read ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __spifFsRead (PLW_FD_ENTRY pfdentry,
                             PCHAR        pcBuffer,
                             size_t       stMaxBytes)
{
    PLW_FD_NODE   pfdnode       = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn        = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME  pfs           = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        //TODO �ļ�������
        sstReadNum = __spif_read(pfs, pspifn, pcBuffer, (size_t)pfdentry->FDENTRY_oftPtr, stMaxBytes);
        if (sstReadNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;              /*  �����ļ�ָ��                */
        }
    
    } else {
        sstReadNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** ��������: __spifFsPRead
** ��������: spiffs pread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ���ջ�����
**           stMaxBytes       ���ջ�������С
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __spifFsPRead (PLW_FD_ENTRY pfdentry,
                              PCHAR        pcBuffer,
                              size_t       stMaxBytes,
                              off_t        oftPos)
{
    PLW_FD_NODE   pfdnode       = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn        = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME  pfs           = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    SPIFFS_FILE   fileHandler   = pspifn->pFd->fileN;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        sstReadNum = __spif_read(pfs, pspifn, pcBuffer, oftPos, stMaxBytes);
    } else {
        sstReadNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** ��������: __spifFsWrite
** ��������: spiffs write ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __spifFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode       = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn        = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME  pfs           = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }
    
    if (stNBytes) {
        //TODO �ļ�д����
        sstWriteNum = __spif_write(pfs, pspifn, pcBuffer, (size_t)pfdentry->FDENTRY_oftPtr, stNBytes);
        if (sstWriteNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;             /*  �����ļ�ָ��                */
            pfdnode->FDNODE_oftSize   = (off_t)pspifn->SPIFN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** ��������: __spifFsPWrite
** ��������: spiffs pwrite ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcBuffer         ������
**           stNBytes         ��Ҫд�������
**           oftPos           λ��
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  __spifFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIF_VOLUME  pfs           = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PSPIFN_NODE   pspifn       = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stNBytes) {
        sstWriteNum = __spif_write(pfs, pspifn, pcBuffer, (size_t)oftPos, stNBytes);
        if (sstWriteNum > 0) {
            pfdnode->FDNODE_oftSize = (off_t)pspifn->SPIFN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** ��������: __spifFsNRead
** ��������: spifFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           piNRead          ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn  = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    
    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *piNRead = (INT)(pspifn->SPIFN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsNRead64
** ��������: spifFs nread ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           poftNRead        ʣ��������
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn  = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *poftNRead = (off_t)(pspifn->SPIFN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsRename
** ��������: spifFs rename ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pcNewName        �µ�����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE    pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs      = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PSPIF_VOLUME   pspiffsNew;
    CHAR           cNewPath[PATH_MAX + 1];
    //TODO: Old Name
    //PCHAR         pcOldName;
    INT           iError;
    
    if (pspifn == LW_NULL) {                                             /*  ����Ƿ�Ϊ�豸�ļ�          */
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
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pspiffsNew, 
                          cNewPath) != ERROR_NONE) {                    /*  �����Ŀ¼·��              */
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        return  (PX_ERROR);
    }
    
    if (pspiffsNew != pfs) {                                          /*  ����Ϊͬһ�豸�ڵ�          */
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    //TODO ��������������Ҫ�����ļ��ƶ�����
    __spif_rename(pfs, pspifn, pcNewName);

    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (iError);
}
/*********************************************************************************************************
** ��������: __spifFsSeek
** ��������: spifFs seek ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           oftOffset        ƫ����
** �䡡��  : �������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE    pfdnode  = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE    pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs      = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset > (size_t)~0) {
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    if (pspifn->SPIFN_stVSize < (size_t)oftOffset) {
        pspifn->SPIFN_stVSize = (size_t)oftOffset;
    }
    __spif_lseek(pfs, pspifn, oftOffset);
    __SPIFFS_VOL_UNLOCK(pspifn->pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsWhere
** ��������: spifFs ����ļ���ǰ��дָ��λ�� (ʹ�ò�����Ϊ����ֵ, �� FIOWHERE ��Ҫ�����в�ͬ)
** �䡡��  : pfdentry            �ļ����ƿ�
**           poftPos             ��дָ��λ��
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsWhere (PLW_FD_ENTRY  pfdentry, off_t  *poftPos)
{
    if (poftPos) {
        *poftPos = (off_t)pfdentry->FDENTRY_oftPtr;
        return  (ERROR_NONE);
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __spifFsStatGet
** ��������: spifFs stat ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE   pspifn  = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME  pfs     = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
//TODO: 
    __spif_stat(pfs, pspifn, pstat);
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsLStat
** ��������: spifFs stat ����
** �䡡��  : pfs           spiffs �ļ�ϵͳ
**           pcName           �ļ���
**           pstat            �ļ�״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsLStat (PSPIF_VOLUME  pfs, PCHAR  pcName, struct stat *pstat)
{
    PSPIFN_NODE     pspifn;
    BOOL            bIsRoot;
    
    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    //TODO: Open����Ҫ�ر���
    pspifn = __spif_open(pfs, pcName, SPIFFS_RDONLY, 0, &bIsRoot);
    if (pspifn) {
        __spif_stat(pspifn, pfs, pstat);
    
    } else if (bIsRoot) {
        __spif_stat(LW_NULL, pfs, pstat);
    
    } else {
        __SPIFFS_VOL_UNLOCK(pfs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    __SPIFFS_VOL_UNLOCK(pfs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsStatfs
** ��������: spifFs statfs ����
** �䡡��  : pfdentry         �ļ����ƿ�
**           pstatfs          �ļ�ϵͳ״̬
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIF_VOLUME   pfs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    //TODO �ļ�ϵͳ״̬��ȡ
    __spif_statfs(pfs, pstatfs);
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsReadDir
** ��������: spifFs ���ָ��Ŀ¼��Ϣ
** �䡡��  : pfdentry            �ļ����ƿ�
**           dir                 Ŀ¼�ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE    pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE    pspifn  = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs     = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    INT            i;
    LONG           iStart;
    INT            iError = ERROR_NONE;
    PLW_LIST_LINE  plineTemp;
    PLW_LIST_LINE  plineHeader;
    PSPIFN_NODE    pspifnTemp;

    SPIFFS_DIR     dirFile;
    PSPIFFS_DIRENT pDirent;
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    iStart = dir->dir_pos;

    //TODO ��ȡ��һ��Ŀ¼������֣�ע�����spiffs�޸ı���Ŀ¼��ķ���
    // for ((plineTemp  = plineHeader), (i = 0); 
    //      (plineTemp != LW_NULL) && (i < iStart); 
    //      (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  ����                        */
    lib_assert(pspifn == LW_NULL);
    __spif_opendir(pfs, "/", &dirFile);
    pDirent = (PSPIFFS_DIRENT)lib_malloc(sizeof(SPIFFS_DIRENT));
    i       = 0;
    /* ���� */
    while ((pDirent = __spif_readdir(&dirFile, pDirent))) {
        i++;
        if(i > iStart){
            break;
        }
    }
    if (pDirent == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  û�ж���Ľڵ�              */
    } 
    else {
        //TODO ��ȡĿ¼���Ӧ�Ľṹ�壬��ҪΪ�˻�ȡ��������
        dir->dir_pos++;
        lib_strlcpy(dir->dir_dirent.d_name, 
                    pDirent->ucName,                                     
                    sizeof(dir->dir_dirent.d_name));

        dir->dir_dirent.d_type = DT_REG;
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __spif_closedir(pfs, &dirFile);

    __SPIFFS_VOL_UNLOCK(pfs);
    return  (iError);
}
/*********************************************************************************************************
** ��������: __spifFsTimeset
** ��������: spiffs �����ļ�ʱ��
** �䡡��  : pfdentry            �ļ����ƿ�
**           utim                utimbuf �ṹ
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __spifFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pspifn) {
        pspifn->SPIFN_timeAccess = utim->actime;
        pspifn->SPIFN_timeChange = utim->modtime;
    
    } else {
        pfs->SPIFFS_time = utim->modtime;
    }
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsTruncate
** ��������: spiffs �����ļ���С
** �䡡��  : pfdentry            �ļ����ƿ�
**           oftSize             �ļ���С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __spifFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    size_t        stTru;
    
    if (pspifn == LW_NULL) {
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
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    stTru = (size_t)oftSize;
    
    // if (stTru > pspifn->SPIFN_stSize) {
    //     //TODO spif�ļ��������Ӻ���
    //     //__spif_increase(pspifn, stTru);
    //     return;
    // } else if (stTru < pspifn->SPIFN_stSize) {
    //     //__spif_truncate(pspifn, stTru);
        
    // }
    __spiffs_open(SYLIX_TO_SPIFFS_PFS(pfs), pspifn->SPIFN_pcName, SPIFFS_O_TRUNC, 0);
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsChmod
** ��������: spiffs chmod ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           iMode               �µ� mode
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __spifFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pspifn) {
        pspifn->SPIFN_mode &= S_IFMT;
        pspifn->SPIFN_mode |= iMode;
    } else {
        pfs->SPIFFS_mode &= S_IFMT;
        pfs->SPIFFS_mode |= iMode;
    }
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsChown
** ��������: spiffs chown ����
** �䡡��  : pfdentry            �ļ����ƿ�
**           pusr                �µ������û�
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���: 
** ����ģ��: 
*********************************************************************************************************/
static INT  __spifFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pfs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pspifn) {
        pspifn->SPIFN_uid = pusr->IOU_uid;
        pspifn->SPIFN_gid = pusr->IOU_gid;
    } else {
        pfs->SPIFFS_uid = pusr->IOU_uid;
        pfs->SPIFFS_gid = pusr->IOU_gid;
    }
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsSymlink
** ��������: spifFs �������������ļ�
** �䡡��  : pfs              spiffs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsSymlink (PSPIF_VOLUME   pfs,
                            PCHAR         pcName,
                            CPCHAR        pcLinkDst)
{
    PSPIFN_NODE     pspifn;
    PSPIFN_NODE     pspifnFather;
    BOOL          bRoot;
    
    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    //TODO: �����ӣ�ʵ�ֲ���
    // pspifn = __spif_open(pfs, pcName, &pspifnFather, &bRoot, LW_NULL, LW_NULL);
    // if (pspifn || bRoot) {
    //     __SPIFFS_VOL_UNLOCK(pfs);
    //     _ErrorHandle(EEXIST);
    //     return  (PX_ERROR);
    // }
    
    // pspifn = __spif_maken(pfs, pcName, pspifnFather, S_IFLNK | DEFAULT_SYMLINK_PERM, pcLinkDst);
    // if (pspifn == LW_NULL) {
    //     __SPIFFS_VOL_UNLOCK(pfs);
    //     return  (PX_ERROR);
    // }
    
    __SPIFFS_VOL_UNLOCK(pfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __spifFsReadlink
** ��������: spifFs ��ȡ���������ļ�����
** �䡡��  : pfs              spiffs �ļ�ϵͳ
**           pcName              ����ԭʼ�ļ���
**           pcLinkDst           ����Ŀ���ļ���
**           stMaxSize           �����С
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t __spifFsReadlink (PSPIF_VOLUME   pfs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize)
{
    PSPIFN_NODE   pspifn;
    size_t      stLen;
    
    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    //TODO: ��������
    // pspifn = __spif_open(pfs, pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    // if ((pspifn == LW_NULL) || !S_ISLNK(pspifn->SPIFN_mode)) {
    //     __SPIFFS_VOL_UNLOCK(pfs);
    //     _ErrorHandle(ENOENT);
    //     return  (PX_ERROR);
    // }
    
    // stLen = lib_strlen(pspifn->SPIFN_pcLink);
    
    // lib_strncpy(pcLinkDst, pspifn->SPIFN_pcLink, stMaxSize);
    
    // if (stLen > stMaxSize) {
    //     stLen = stMaxSize;                                              /*  ������Ч�ֽ���              */
    // }
    
    // __SPIFFS_VOL_UNLOCK(pfs);
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** ��������: __spifFsIoctl
** ��������: spifFs ioctl ����
** �䡡��  : pfdentry           �ļ����ƿ�
**           request,           ����
**           arg                �������
** �䡡��  : < 0 ��ʾ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __spifFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIF_VOLUME  pfs     = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
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
        return  (__spifFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  ����ļ���ǰ��дָ��        */
        iError = __spifFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  ����ļ�ʣ���ֽ���          */
        return  (__spifFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  ����ļ�ʣ���ֽ���          */
        iError = __spifFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  �ļ�������                  */
        return  (__spifFsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  ��ȡ���                    */
    case FIOLABELSET:                                                   /*  ���þ��                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  ����ļ�״̬                */
        return  (__spifFsStat(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  ����ļ�ϵͳ״̬            */
        return  (__spifFsStatfs(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  ��ȡһ��Ŀ¼��Ϣ            */
        return  (__spifFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  �����ļ�ʱ��                */
        return  (__spifFsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    case FIOTRUNC:                                                      /*  �ı��ļ���С                */
        oftTemp = *(off_t *)lArg;
        return  (__spifFsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  ���ļ������д              */
    case FIOFLUSH:
    case FIODATASYNC:
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__spifFsChmod(pfdentry, (INT)lArg));                    /*  �ı��ļ�����Ȩ��            */
    
    case FIOSETFL:                                                      /*  �����µ� flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOCHOWN:                                                      /*  �޸��ļ�������ϵ            */
        return  (__spifFsChown(pfdentry, (LW_IO_USR *)lArg));
    
    case FIOFSTYPE:                                                     /*  ����ļ�ϵͳ����            */
        *(PCHAR *)lArg = "Spi flash FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  ǿ��ж���豸�Ƿ�����      */
        *(BOOL *)lArg = pfs->SPIFFS_bForceDelete;
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
  END
*********************************************************************************************************/
#endif  /* SpifFs DISABLE */

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
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
#include "../SylixOS/fs/fsCommon/fsCommon.h"
#include "../SylixOS/fs/include/fs_fs.h"
#include "HoitFs.h"
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
#if LW_CFG_MAX_VOLUMES > 0 //&& LW_CFG_RAMFS_EN > 0
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
LW_API
INT  API_HoitFsDevCreate(PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PHOIT_SB     pfs;

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

    pfs = (PHOIT_SB)__SHEAP_ALLOC(sizeof(HOIT_SB));
    if (pfs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pfs, sizeof(HOIT_SB));                              /*  ��վ���ƿ�                */

    pfs->HOITFS_bValid = LW_TRUE;

    pfs->HOITFS_hVolLock = API_SemaphoreMCreate("hoit_sb_lock", LW_PRIO_DEF_CEILING,
        LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE |
        LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
        LW_NULL);
    if (!pfs->HOITFS_hVolLock) {                                      /*  �޷���������                */
        __SHEAP_FREE(pfs);
        return  (PX_ERROR);
    }

    pfs->HOITFS_mode = S_IFDIR | DEFAULT_DIR_PERM;
    pfs->HOITFS_uid = getuid();
    pfs->HOITFS_gid = getgid();
    pfs->HOITFS_time = lib_time(LW_NULL);
    pfs->HOITFS_ulCurBlk = 0ul;

    //__ram_mount(pramfs);

    if (iosDevAddEx(&pfs->HOITFS_devhdrHdr, pcName, _G_iHoitFsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  ��װ�ļ�ϵͳ�豸            */
        API_SemaphoreMDelete(&pfs->HOITFS_hVolLock);
        __SHEAP_FREE(pfs);
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
static LONG __hoitFsOpen(PHOIT_SB     pfs,
    PCHAR           pcName,
    INT             iFlags,
    INT             iMode)
{
    PLW_FD_NODE pfdnode;

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

    if (__HOITFS_SB_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    //************************************ TODO ************************************
    

    //************************************ END  ************************************
    __HOITFS_SB_UNLOCK(pfs);

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
static INT  __hoitFsRemove(PHOIT_SB   pfs,
    PCHAR         pcName)
{
    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (__HOITFS_SB_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }

    //************************************ TODO ************************************


    //************************************ END  ************************************
    __HOITFS_SB_UNLOCK(pfs);
    return 0;
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
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PHOIT_SB      pfs     = (PHOIT_SB)pfdnode->FDNODE_pvFsExtern;

    if (__HOITFS_SB_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  �豸����                    */
        return  (PX_ERROR);
    }
    
    LW_DEV_DEC_USE_COUNT(&pfs->HOITFS_devhdrHdr);

    __HOITFS_SB_UNLOCK(pfs);

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
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    
    ssize_t       sstReadNum = PX_ERROR;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

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
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    
    ssize_t       sstReadNum = PX_ERROR;

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

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
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    
    ssize_t       sstWriteNum = PX_ERROR;

    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  ׷��ģʽ                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  �ƶ���дָ�뵽ĩβ          */
    }

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

    ssize_t       sstWriteNum = PX_ERROR;

    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    return  (sstWriteNum);
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
static INT  __hoitFsLStat(PHOIT_SB  pfs, PCHAR  pcName, struct stat* pstat)
{
    BOOL          bRoot;

    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOITFS_SB_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    __HOITFS_SB_UNLOCK(pfs);

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
static INT  __hoitFsSymlink(PHOIT_SB   pfs,
    PCHAR         pcName,
    CPCHAR        pcLinkDst)
{

    BOOL          bRoot;

    if (!pcName || !pcLinkDst) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__fsCheckFileName(pcName)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOITFS_SB_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    

    __HOITFS_SB_UNLOCK(pfs);

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
static ssize_t __hoitFsReadlink(PHOIT_SB   pfs,
    PCHAR         pcName,
    PCHAR         pcLinkDst,
    size_t        stMaxSize)
{
    size_t      stLen;

    if (!pcName || !pcLinkDst || !stMaxSize) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (__HOITFS_SB_LOCK(pfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    __HOITFS_SB_UNLOCK(pfs);

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
    return  (ERROR_NONE);
}
#endif                                                                  /*  LW_CFG_MAX_VOLUMES > 0      */

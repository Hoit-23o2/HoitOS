/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: spifFs.c
**
** 创   建   人: 潘延麒
**
** 文件创建日期: 2021 年 06 月 01日
**
** 描        述: Spiffs文件系统接口层
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
/*********************************************************************************************************
  内部全局变量
*********************************************************************************************************/
static INT                              _G_iSpiffsDrvNum = PX_ERROR;



/*********************************************************************************************************
  文件类型
*********************************************************************************************************/
#define __SPIFFS_FILE_TYPE_NODE          0                               /*  open 打开文件               */
#define __SPIFFS_FILE_TYPE_DIR           1                               /*  open 打开目录               */
#define __SPIFFS_FILE_TYPE_DEV           2                               /*  open 打开设备               */
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __SPIFFS_VOL_LOCK(pspiffs)        API_SemaphoreMPend(pspiffs->SPIFFS_hVolLock, \
                                        LW_OPTION_WAIT_INFINITE)
#define __SPIFFS_VOL_UNLOCK(pspiffs)      API_SemaphoreMPost(pspiffs->SPIFFS_hVolLock)
/*********************************************************************************************************
  内部函数
*********************************************************************************************************/
static LONG     __spifFsOpen(PSPIF_VOLUME     pspiffs,
                            PCHAR           pcName,
                            INT             iFlags,
                            INT             iMode);
static INT      __spifFsRemove(PSPIF_VOLUME   pspiffs,
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
static INT      __spifFsLStat(PSPIF_VOLUME   pspiffs,
                             PCHAR         pcName,
                             struct stat  *pstat);
static INT      __spifFsIoctl(PLW_FD_ENTRY  pfdentry,
                             INT           iRequest,
                             LONG          lArg);
static INT      __spifFsSymlink(PSPIF_VOLUME   pspiffs,
                               PCHAR         pcName,
                               CPCHAR        pcLinkDst);
static ssize_t  __spifFsReadlink(PSPIF_VOLUME   pspiffs,
                                PCHAR         pcName,
                                PCHAR         pcLinkDst,
                                size_t        stMaxSize);
/*********************************************************************************************************
** 函数名称: API_SpifFsDrvInstall
** 功能描述: 安装 spiffs 文件系统驱动程序
** 输　入  :
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
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
    
    _G_iSpiffsDrvNum = iosDrvInstallEx2(&fileop, LW_DRV_TYPE_NEW_1);     /*  使用 NEW_1 型设备驱动程序   */

    DRIVER_LICENSE(_G_iSpiffsDrvNum,     "GPL->Ver 2.0");
    DRIVER_AUTHOR(_G_iSpiffsDrvNum,      "Han.hui");
    DRIVER_DESCRIPTION(_G_iSpiffsDrvNum, "spiffs driver.");

    _DebugHandle(__LOGMESSAGE_LEVEL, "spif file system installed.\r\n");
                                     
    __fsRegister("spiffs", API_SpifFsDevCreate, LW_NULL, LW_NULL);        /*  注册文件系统                */

    return  ((_G_iSpiffsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** 函数名称: API_SpifFsDevCreate
** 功能描述: 创建 spiffs 文件系统设备.
** 输　入  : pcName            设备名(设备挂接的节点地址)
**           pblkd             使用 pblkd->BLKD_pcName 作为 最大大小 标示.
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_SpifFsDevCreate (PCHAR   pcName, PLW_BLK_DEV  pblkd)
{
    PSPIF_VOLUME     pspiffs;
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
    
    pspiffs = (PSPIF_VOLUME)__SHEAP_ALLOC(sizeof(SPIF_VOLUME));
    if (pspiffs == LW_NULL) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }
    lib_bzero(pspiffs, sizeof(SPIF_VOLUME));                              /*  清空卷控制块                */
    
    pspiffs->SPIFFS_bValid = LW_TRUE;
    
    pspiffs->SPIFFS_hVolLock = API_SemaphoreMCreate("spifvol_lock", LW_PRIO_DEF_CEILING,
                                             LW_OPTION_WAIT_PRIORITY | LW_OPTION_DELETE_SAFE | 
                                             LW_OPTION_INHERIT_PRIORITY | LW_OPTION_OBJECT_GLOBAL,
                                             LW_NULL);
    if (!pspiffs->SPIFFS_hVolLock) {                                      /*  无法创建卷锁                */
        __SHEAP_FREE(pspiffs);
        return  (PX_ERROR);
    }
    
    pspiffs->SPIFFS_mode     = S_IFDIR | DEFAULT_DIR_PERM;
    pspiffs->SPIFFS_uid      = getuid();
    pspiffs->SPIFFS_gid      = getgid();
    pspiffs->SPIFFS_time     = lib_time(LW_NULL);
    pspiffs->SPIFFS_ulCurBlk = 0ul;
    
    //TODO 挂载函数
    __spif_mount(pspiffs);
    
    if (iosDevAddEx(&pspiffs->SPIFFS_devhdrHdr, pcName, _G_iSpiffsDrvNum, DT_DIR)
        != ERROR_NONE) {                                                /*  安装文件系统设备            */
        API_SemaphoreMDelete(&pspiffs->SPIFFS_hVolLock);
        __SHEAP_FREE(pspiffs);
        return  (PX_ERROR);
    }
    
    _DebugFormat(__LOGMESSAGE_LEVEL, "target \"%s\" mount ok.\r\n", pcName);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: API_SpifFsDevDelete
** 功能描述: 删除一个 spiffs 文件系统设备, 例如: API_SpifFsDevDelete("/mnt/spif0");
** 输　入  : pcName            文件系统设备名(物理设备挂接的节点地址)
** 输　出  : < 0 表示失败
** 全局变量:
** 调用模块:
                                           API 函数
*********************************************************************************************************/
LW_API
INT  API_SpifFsDevDelete (PCHAR   pcName)
{
    if (API_IosDevMatchFull(pcName)) {                                  /*  如果是设备, 这里就卸载设备  */
        return  (unlink(pcName));
    
    } else {
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __spifFsOpen
** 功能描述: 打开或者创建文件
** 输　入  : pspiffs           spiffs 文件系统
**           pcName           文件名
**           iFlags           方式
**           iMode            mode_t
** 输　出  : < 0 错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG __spifFsOpen (PSPIF_VOLUME     pspiffs,
                         PCHAR           pcName,
                         INT             iFlags,
                         INT             iMode)
{
    PLW_FD_NODE pfdnode;
    PSPIFN_NODE   pspifn;
    PSPIFN_NODE   pspifnFather;
    BOOL        bRoot;
    BOOL        bLast;
    PCHAR       pcTail;
    BOOL        bIsNew;
    BOOL        bCreate = LW_FALSE;
    struct stat statGet;
    
    if (pcName == LW_NULL) {
        _ErrorHandle(EFAULT);                                           /*  Bad address                 */
        return  (PX_ERROR);
    }
    
    if (iFlags & O_CREAT) {                                             /*  创建操作                    */
        if (__fsCheckFileName(pcName)) {
            _ErrorHandle(ENOENT);
            return  (PX_ERROR);
        }
        if (S_ISFIFO(iMode) || 
            S_ISBLK(iMode)  ||
            S_ISCHR(iMode)) {
            _ErrorHandle(ERROR_IO_DISK_NOT_PRESENT);                    /*  不支持以上这些格式          */
            return  (PX_ERROR);
        }
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    //TODO 文件打开函数
    /* 
        pspifnFather    父亲节点，用于之后的文件创建操作
        详细参考__ram_open
     */
    pspifn = __spif_open(pspiffs, pcName, &pspifnFather, &bRoot, &bLast, &pcTail);
    if (pspifn) {
        if (!S_ISLNK(pspifn->SPIFN_mode)) {
            if ((iFlags & O_CREAT) && (iFlags & O_EXCL)) {              /*  排他创建文件                */
                __SPIFFS_VOL_UNLOCK(pspiffs);
                _ErrorHandle(EEXIST);                                   /*  已经存在文件                */
                return  (PX_ERROR);
            
            } else if ((iFlags & O_DIRECTORY) && !S_ISDIR(pspifn->SPIFN_mode)) {
                __SPIFFS_VOL_UNLOCK(pspiffs);
                _ErrorHandle(ENOTDIR);
                return  (PX_ERROR);
            
            } else {
                goto    __file_open_ok;
            }
        }
    
    } else if ((iFlags & O_CREAT) && bLast) {                           /*  创建节点                    */
        //TODO 创建新的文件节点
        pspifn = __spif_maken(pspiffs, pcName, pspifnFather, iMode, LW_NULL);
        if (pspifn) {
            bCreate = LW_TRUE;
            goto    __file_open_ok;
        
        } else {
            return  (PX_ERROR);
        }
    }
    
    if (pspifn) {                                                        /*  符号链接处理                */
        //! 符号链接部分可忽略
        // INT     iError;
        // INT     iFollowLinkType;
        // PCHAR   pcSymfile = pcTail - lib_strlen(pspifn->SPIFN_pcName) - 1;
        // PCHAR   pcPrefix;
        
        // if (*pcSymfile != PX_DIVIDER) {
        //     pcSymfile--;
        // }
        // if (pcSymfile == pcName) {
        //     pcPrefix = LW_NULL;                                         /*  没有前缀                    */
        // } else {
        //     pcPrefix = pcName;
        //     *pcSymfile = PX_EOS;
        // }
        // if (pcTail && lib_strlen(pcTail)) {
        //     iFollowLinkType = FOLLOW_LINK_TAIL;                         /*  连接目标内部文件            */
        // } else {
        //     iFollowLinkType = FOLLOW_LINK_FILE;                         /*  链接文件本身                */
        // }
        
        // iError = _PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
        //                         LW_NULL, pcPrefix, pspifn->SPIFN_pcLink, pcTail);
        // if (iError) {
        //     __SPIFFS_VOL_UNLOCK(pspiffs);
        //     return  (PX_ERROR);                                         /*  无法建立被链接目标目录      */
        // } else {
        //     __SPIFFS_VOL_UNLOCK(pspiffs);
        //     return  (iFollowLinkType);
        // }
    
    } else if (bRoot == LW_FALSE) {                                     /*  不是打开根目录              */
        __SPIFFS_VOL_UNLOCK(pspiffs);
        _ErrorHandle(ENOENT);                                           /*  没有找到文件                */
        return  (PX_ERROR);
    }
    
__file_open_ok:
    //TODO 文件状态获取函数，可直接抄__ram_stat
    __spif_stat(pspifn, pspiffs, &statGet);
    pfdnode = API_IosFdNodeAdd(&pspiffs->SPIFFS_plineFdNodeHeader,
                               statGet.st_dev,
                               (ino64_t)statGet.st_ino,
                               iFlags,
                               iMode,
                               statGet.st_uid,
                               statGet.st_gid,
                               statGet.st_size,
                               (PVOID)pspifn,
                               &bIsNew);                                /*  添加文件节点                */
    if (pfdnode == LW_NULL) {                                           /*  无法创建 fd_node 节点       */
        __SPIFFS_VOL_UNLOCK(pspiffs);
        if (bCreate) {
            __spif_unlink(pspifn);                                        /*  删除新建的节点              */
        }
        return  (PX_ERROR);
    }
    
    pfdnode->FDNODE_pvFsExtern = (PVOID)pspiffs;                         /*  记录文件系统信息            */
    
    if ((iFlags & O_TRUNC) && ((iFlags & O_ACCMODE) != O_RDONLY)) {     /*  需要截断                    */
        if (pspifn) {
            //TODO 文件数据删减函数
            __spif_truncate(pspifn, 0);
            pfdnode->FDNODE_oftSize = 0;
        }
    }
    
    LW_DEV_INC_USE_COUNT(&pspiffs->SPIFFS_devhdrHdr);                     /*  更新计数器                  */
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  ((LONG)pfdnode);                                            /*  返回文件节点                */
}
/*********************************************************************************************************
** 函数名称: __spifFsRemove
** 功能描述: spiffs remove 操作
** 输　入  : pspiffs           卷设备
**           pcName           文件名
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsRemove (PSPIF_VOLUME   pspiffs,
                           PCHAR         pcName)
{
    PSPIFN_NODE  pspifn;
    BOOL       bRoot;
    PCHAR      pcTail;
    INT        iError;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }
        
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    pspifn = __spif_open(pspiffs, pcName, LW_NULL, &bRoot, LW_NULL, &pcTail);
    if (pspifn) {
        //! 符号链接忽略
        // if (S_ISLNK(pspifn->SPIFN_mode)) {                                /*  符号链接                    */
        //     size_t  stLenTail = 0;
        //     if (pcTail) {
        //         stLenTail = lib_strlen(pcTail);                         /*  确定 tail 长度              */
        //     }
        //     if (stLenTail) {                                            /*  指向其他文件                */
        //         PCHAR   pcSymfile = pcTail - lib_strlen(pspifn->SPIFN_pcName) - 1;
        //         PCHAR   pcPrefix;
                
        //         if (*pcSymfile != PX_DIVIDER) {
        //             pcSymfile--;
        //         }
        //         if (pcSymfile == pcName) {
        //             pcPrefix = LW_NULL;                                 /*  没有前缀                    */
        //         } else {
        //             pcPrefix = pcName;
        //             *pcSymfile = PX_EOS;
        //         }
                
        //         if (_PathBuildLink(pcName, MAX_FILENAME_LENGTH, 
        //                            LW_NULL, pcPrefix, pspifn->SPIFN_pcLink, pcTail) < ERROR_NONE) {
        //             __SPIFFS_VOL_UNLOCK(pspiffs);
        //             return  (PX_ERROR);                                 /*  无法建立被链接目标目录      */
        //         } else {
        //             __SPIFFS_VOL_UNLOCK(pspiffs);
        //             return  (FOLLOW_LINK_TAIL);
        //         }
        //     }
        // }
        //TODO 文件删除函数
        iError = __spif_unlink(pspifn);
        __SPIFFS_VOL_UNLOCK(pspiffs);
        return  (iError);
            
    } else if (bRoot) {                                                 /*  删除 spiffs 文件系统         */
        if (pspiffs->SPIFFS_bValid == LW_FALSE) {
            __SPIFFS_VOL_UNLOCK(pspiffs);
            return  (ERROR_NONE);                                       /*  正在被其他任务卸载          */
        }
        
__re_umount_vol:
        if (LW_DEV_GET_USE_COUNT((LW_DEV_HDR *)pspiffs)) {
            if (!pspiffs->SPIFFS_bForceDelete) {
                __SPIFFS_VOL_UNLOCK(pspiffs);
                _ErrorHandle(EBUSY);
                return  (PX_ERROR);
            }
            
            pspiffs->SPIFFS_bValid = LW_FALSE;
            
            __SPIFFS_VOL_UNLOCK(pspiffs);
            
            _DebugHandle(__ERRORMESSAGE_LEVEL, "disk have open file.\r\n");
            iosDevFileAbnormal(&pspiffs->SPIFFS_devhdrHdr);               /*  将所有相关文件设为异常模式  */
            
            __SPIFFS_VOL_LOCK(pspiffs);
            goto    __re_umount_vol;
        
        } else {
            pspiffs->SPIFFS_bValid = LW_FALSE;
        }
        
        iosDevDelete((LW_DEV_HDR *)pspiffs);                             /*  IO 系统移除设备             */
        API_SemaphoreMDelete(&pspiffs->SPIFFS_hVolLock);
        //TODO 文件系统卸载函数
        __spif_unmount(pspiffs);                                          /*  释放所有文件内容            */
        __SHEAP_FREE(pspiffs);
        
        _DebugHandle(__LOGMESSAGE_LEVEL, "spiffs unmount ok.\r\n");
        
        return  (ERROR_NONE);
        
    } else {
        __SPIFFS_VOL_UNLOCK(pspiffs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: __spifFsClose
** 功能描述: spiffs close 操作
** 输　入  : pfdentry         文件控制块
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsClose (PLW_FD_ENTRY    pfdentry)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    BOOL          bRemove = LW_FALSE;
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);                                            /*  设备出错                    */
        return  (PX_ERROR);
    }
    
    if (API_IosFdNodeDec(&pspiffs->SPIFFS_plineFdNodeHeader, 
                         pfdnode, &bRemove) == 0) {
        if (pspifn) {
            __spif_close(pspifn, pfdentry->FDENTRY_iFlag);
        }
    }
    
    LW_DEV_DEC_USE_COUNT(&pspiffs->SPIFFS_devhdrHdr);
    
    if (bRemove && pspifn) {
        __spif_unlink(pspifn);
    }
        
    __SPIFFS_VOL_UNLOCK(pspiffs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsRead
** 功能描述: spiffs read 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __spifFsRead (PLW_FD_ENTRY pfdentry,
                             PCHAR        pcBuffer,
                             size_t       stMaxBytes)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn      = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        //TODO 文件读函数
        sstReadNum = __spif_read(pspifn, pcBuffer, stMaxBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstReadNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstReadNum;              /*  更新文件指针                */
        }
    
    } else {
        sstReadNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** 函数名称: __spifFsPRead
** 功能描述: spiffs pread 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         接收缓冲区
**           stMaxBytes       接收缓冲区大小
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __spifFsPRead (PLW_FD_ENTRY pfdentry,
                              PCHAR        pcBuffer,
                              size_t       stMaxBytes,
                              off_t        oftPos)
{
    PLW_FD_NODE   pfdnode    = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn      = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstReadNum = PX_ERROR;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stMaxBytes) {
        sstReadNum = __spif_read(pspifn, pcBuffer, stMaxBytes, (size_t)oftPos);
    
    } else {
        sstReadNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (sstReadNum);
}
/*********************************************************************************************************
** 函数名称: __spifFsWrite
** 功能描述: spiffs write 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __spifFsWrite (PLW_FD_ENTRY  pfdentry,
                              PCHAR         pcBuffer,
                              size_t        stNBytes)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn       = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (pfdentry->FDENTRY_iFlag & O_APPEND) {                           /*  追加模式                    */
        pfdentry->FDENTRY_oftPtr = pfdnode->FDNODE_oftSize;             /*  移动读写指针到末尾          */
    }
    
    if (stNBytes) {
        //TODO 文件写函数
        sstWriteNum = __spif_write(pspifn, pcBuffer, stNBytes, (size_t)pfdentry->FDENTRY_oftPtr);
        if (sstWriteNum > 0) {
            pfdentry->FDENTRY_oftPtr += (off_t)sstWriteNum;             /*  更新文件指针                */
            pfdnode->FDNODE_oftSize   = (off_t)pspifn->SPIFN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** 函数名称: __spifFsPWrite
** 功能描述: spiffs pwrite 操作
** 输　入  : pfdentry         文件控制块
**           pcBuffer         缓冲区
**           stNBytes         需要写入的数据
**           oftPos           位置
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t  __spifFsPWrite (PLW_FD_ENTRY  pfdentry,
                               PCHAR         pcBuffer,
                               size_t        stNBytes,
                               off_t         oftPos)
{
    PLW_FD_NODE   pfdnode     = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn       = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    ssize_t       sstWriteNum = PX_ERROR;
    
    if (!pcBuffer || (oftPos < 0)) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (stNBytes) {
        sstWriteNum = __spif_write(pspifn, pcBuffer, stNBytes, (size_t)oftPos);
        if (sstWriteNum > 0) {
            pfdnode->FDNODE_oftSize = (off_t)pspifn->SPIFN_stSize;
        }
        
    } else {
        sstWriteNum = 0;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (sstWriteNum);
}
/*********************************************************************************************************
** 函数名称: __spifFsNRead
** 功能描述: spifFs nread 操作
** 输　入  : pfdentry         文件控制块
**           piNRead          剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsNRead (PLW_FD_ENTRY  pfdentry, INT  *piNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    
    if (piNRead == LW_NULL) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *piNRead = (INT)(pspifn->SPIFN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsNRead64
** 功能描述: spifFs nread 操作
** 输　入  : pfdentry         文件控制块
**           poftNRead        剩余数据量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsNRead64 (PLW_FD_ENTRY  pfdentry, off_t  *poftNRead)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    *poftNRead = (off_t)(pspifn->SPIFN_stSize - (size_t)pfdentry->FDENTRY_oftPtr);
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsRename
** 功能描述: spifFs rename 操作
** 输　入  : pfdentry         文件控制块
**           pcNewName        新的名称
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsRename (PLW_FD_ENTRY  pfdentry, PCHAR  pcNewName)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    PSPIF_VOLUME   pspiffsNew;
    CHAR          cNewPath[PATH_MAX + 1];
    INT           iError;
    
    if (pspifn == LW_NULL) {                                             /*  检查是否为设备文件          */
        _ErrorHandle(ERROR_IOS_DRIVER_NOT_SUP);                         /*  不支持设备重命名            */
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
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (ioFullFileNameGet(pcNewName, 
                          (LW_DEV_HDR **)&pspiffsNew, 
                          cNewPath) != ERROR_NONE) {                    /*  获得新目录路径              */
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        return  (PX_ERROR);
    }
    
    if (pspiffsNew != pspiffs) {                                          /*  必须为同一设备节点          */
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EXDEV);
        return  (PX_ERROR);
    }
    //TODO 重命名函数，需要做到文件移动功能
    iError = __spif_move(pspifn, cNewPath);
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __spifFsSeek
** 功能描述: spifFs seek 操作
** 输　入  : pfdentry         文件控制块
**           oftOffset        偏移量
** 输　出  : 驱动相关
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsSeek (PLW_FD_ENTRY  pfdentry,
                         off_t         oftOffset)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    
    if (pspifn == LW_NULL) {
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    if (oftOffset > (size_t)~0) {
        _ErrorHandle(EOVERFLOW);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspifn->SPIFN_pramfs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    pfdentry->FDENTRY_oftPtr = oftOffset;
    if (pspifn->SPIFN_stVSize < (size_t)oftOffset) {
        pspifn->SPIFN_stVSize = (size_t)oftOffset;
    }
    
    __SPIFFS_VOL_UNLOCK(pspifn->SPIFN_pramfs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsWhere
** 功能描述: spifFs 获得文件当前读写指针位置 (使用参数作为返回值, 与 FIOWHERE 的要求稍有不同)
** 输　入  : pfdentry            文件控制块
**           poftPos             读写指针位置
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
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
** 函数名称: __spifFsStatGet
** 功能描述: spifFs stat 操作
** 输　入  : pfdentry         文件控制块
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsStat (PLW_FD_ENTRY  pfdentry, struct stat *pstat)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    __spif_stat(pspifn, pspiffs, pstat);
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsLStat
** 功能描述: spifFs stat 操作
** 输　入  : pspiffs           spiffs 文件系统
**           pcName           文件名
**           pstat            文件状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsLStat (PSPIF_VOLUME  pspiffs, PCHAR  pcName, struct stat *pstat)
{
    PSPIFN_NODE     pspifn;
    BOOL          bRoot;
    
    if (!pcName || !pstat) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pspifn = __spif_open(pspiffs, pcName, LW_NULL, &bRoot, LW_NULL, LW_NULL);
    if (pspifn) {
        __spif_stat(pspifn, pspiffs, pstat);
    
    } else if (bRoot) {
        __spif_stat(LW_NULL, pspiffs, pstat);
    
    } else {
        __SPIFFS_VOL_UNLOCK(pspiffs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsStatfs
** 功能描述: spifFs statfs 操作
** 输　入  : pfdentry         文件控制块
**           pstatfs          文件系统状态
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsStatfs (PLW_FD_ENTRY  pfdentry, struct statfs *pstatfs)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pstatfs) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    //TODO 文件系统状态获取
    //! 与__spif_stat有区别，前者获取整个文件系统的当前状态，后者获取文件节点的状态
    __spif_statfs(pspiffs, pstatfs);
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsReadDir
** 功能描述: spifFs 获得指定目录信息
** 输　入  : pfdentry            文件控制块
**           dir                 目录结构
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsReadDir (PLW_FD_ENTRY  pfdentry, DIR  *dir)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    INT                i;
    LONG               iStart;
    INT                iError = ERROR_NONE;
    PLW_LIST_LINE      plineTemp;
    PLW_LIST_LINE      plineHeader;
    PSPIFN_NODE          pspifnTemp;
    
    if (!dir) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }

    //TODO  这部分判断目录文件pfdnode->FDNODE_pvFile是否存在，如果不存在则读取根目录  
    // if (pspifn == LW_NULL) {
    //     plineHeader = pspiffs->SPIFFS_plineSon;
    // } else {
    //     if (!S_ISDIR(pspifn->SPIFN_mode)) {
    //         __SPIFFS_VOL_UNLOCK(pspiffs);
    //         _ErrorHandle(ENOTDIR);
    //         return  (PX_ERROR);
    //     }
    //     plineHeader = pspifn->SPIFN_plineSon;
    // }
    
    iStart = dir->dir_pos;

    //TODO 读取下一条目录项的名字，注意根据spiffs修改遍历目录项的方法
    for ((plineTemp  = plineHeader), (i = 0); 
         (plineTemp != LW_NULL) && (i < iStart); 
         (plineTemp  = _list_line_get_next(plineTemp)), (i++));         /*  忽略                        */
    
    if (plineTemp == LW_NULL) {
        _ErrorHandle(ENOENT);
        iError = PX_ERROR;                                              /*  没有多余的节点              */
    
    } else {
        //TODO 获取目录项对应的结构体，主要为了获取它的名字
        //pspifnTemp = _LIST_ENTRY(plineTemp, RAM_NODE, SPIFN_lineBrother);
        dir->dir_pos++;
        
        lib_strlcpy(dir->dir_dirent.d_name, 
                    pspifnTemp->SPIFN_pcName, 
                    sizeof(dir->dir_dirent.d_name));
                    
        dir->dir_dirent.d_type = IFTODT(pspifnTemp->SPIFN_mode);
        dir->dir_dirent.d_shortname[0] = PX_EOS;
    }
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (iError);
}
/*********************************************************************************************************
** 函数名称: __spifFsTimeset
** 功能描述: spiffs 设置文件时间
** 输　入  : pfdentry            文件控制块
**           utim                utimbuf 结构
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __spifFsTimeset (PLW_FD_ENTRY  pfdentry, struct utimbuf  *utim)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!utim) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pspifn) {
        pspifn->SPIFN_timeAccess = utim->actime;
        pspifn->SPIFN_timeChange = utim->modtime;
    
    } else {
        pspiffs->SPIFFS_time = utim->modtime;
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsTruncate
** 功能描述: spiffs 设置文件大小
** 输　入  : pfdentry            文件控制块
**           oftSize             文件大小
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __spifFsTruncate (PLW_FD_ENTRY  pfdentry, off_t  oftSize)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
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
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (S_ISDIR(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspiffs);
        _ErrorHandle(EISDIR);
        return  (PX_ERROR);
    }
    
    stTru = (size_t)oftSize;
    
    if (stTru > pspifn->SPIFN_stSize) {
        //TODO spif文件长度增加函数
        __spif_increase(pspifn, stTru);
        
    } else if (stTru < pspifn->SPIFN_stSize) {
        __spif_truncate(pspifn, stTru);
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsChmod
** 功能描述: spiffs chmod 操作
** 输　入  : pfdentry            文件控制块
**           iMode               新的 mode
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __spifFsChmod (PLW_FD_ENTRY  pfdentry, INT  iMode)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    iMode |= S_IRUSR;
    iMode &= ~S_IFMT;
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pspifn) {
        pspifn->SPIFN_mode &= S_IFMT;
        pspifn->SPIFN_mode |= iMode;
    } else {
        pspiffs->SPIFFS_mode &= S_IFMT;
        pspiffs->SPIFFS_mode |= iMode;
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsChown
** 功能描述: spiffs chown 操作
** 输　入  : pfdentry            文件控制块
**           pusr                新的所属用户
** 输　出  : < 0 表示错误
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
static INT  __spifFsChown (PLW_FD_ENTRY  pfdentry, LW_IO_USR  *pusr)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIFN_NODE     pspifn   = (PSPIFN_NODE)pfdnode->FDNODE_pvFile;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
    
    if (!pusr) {
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    if (pspifn) {
        pspifn->SPIFN_uid = pusr->IOU_uid;
        pspifn->SPIFN_gid = pusr->IOU_gid;
    } else {
        pspiffs->SPIFFS_uid = pusr->IOU_uid;
        pspiffs->SPIFFS_gid = pusr->IOU_gid;
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsSymlink
** 功能描述: spifFs 创建符号链接文件
** 输　入  : pspiffs              spiffs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsSymlink (PSPIF_VOLUME   pspiffs,
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
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pspifn = __spif_open(pspiffs, pcName, &pspifnFather, &bRoot, LW_NULL, LW_NULL);
    if (pspifn || bRoot) {
        __SPIFFS_VOL_UNLOCK(pspiffs);
        _ErrorHandle(EEXIST);
        return  (PX_ERROR);
    }
    
    pspifn = __spif_maken(pspiffs, pcName, pspifnFather, S_IFLNK | DEFAULT_SYMLINK_PERM, pcLinkDst);
    if (pspifn == LW_NULL) {
        __SPIFFS_VOL_UNLOCK(pspiffs);
        return  (PX_ERROR);
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __spifFsReadlink
** 功能描述: spifFs 读取符号链接文件内容
** 输　入  : pspiffs              spiffs 文件系统
**           pcName              链接原始文件名
**           pcLinkDst           链接目标文件名
**           stMaxSize           缓冲大小
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static ssize_t __spifFsReadlink (PSPIF_VOLUME   pspiffs,
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
    
    if (__SPIFFS_VOL_LOCK(pspiffs) != ERROR_NONE) {
        _ErrorHandle(ENXIO);
        return  (PX_ERROR);
    }
    
    pspifn = __spif_open(pspiffs, pcName, LW_NULL, LW_NULL, LW_NULL, LW_NULL);
    if ((pspifn == LW_NULL) || !S_ISLNK(pspifn->SPIFN_mode)) {
        __SPIFFS_VOL_UNLOCK(pspiffs);
        _ErrorHandle(ENOENT);
        return  (PX_ERROR);
    }
    
    stLen = lib_strlen(pspifn->SPIFN_pcLink);
    
    lib_strncpy(pcLinkDst, pspifn->SPIFN_pcLink, stMaxSize);
    
    if (stLen > stMaxSize) {
        stLen = stMaxSize;                                              /*  计算有效字节数              */
    }
    
    __SPIFFS_VOL_UNLOCK(pspiffs);
    
    return  ((ssize_t)stLen);
}
/*********************************************************************************************************
** 函数名称: __spifFsIoctl
** 功能描述: spifFs ioctl 操作
** 输　入  : pfdentry           文件控制块
**           request,           命令
**           arg                命令参数
** 输　出  : < 0 表示错误
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __spifFsIoctl (PLW_FD_ENTRY  pfdentry,
                          INT           iRequest,
                          LONG          lArg)
{
    PLW_FD_NODE   pfdnode = (PLW_FD_NODE)pfdentry->FDENTRY_pfdnode;
    PSPIF_VOLUME   pspiffs  = (PSPIF_VOLUME)pfdnode->FDNODE_pvFsExtern;
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
	
	case FIODISKINIT:                                                   /*  磁盘初始化                  */
        return  (ERROR_NONE);
        
    case FIOSEEK:                                                       /*  文件重定位                  */
        oftTemp = *(off_t *)lArg;
        return  (__spifFsSeek(pfdentry, oftTemp));

    case FIOWHERE:                                                      /*  获得文件当前读写指针        */
        iError = __spifFsWhere(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }
        
    case FIONREAD:                                                      /*  获得文件剩余字节数          */
        return  (__spifFsNRead(pfdentry, (INT *)lArg));
        
    case FIONREAD64:                                                    /*  获得文件剩余字节数          */
        iError = __spifFsNRead64(pfdentry, &oftTemp);
        if (iError == PX_ERROR) {
            return  (PX_ERROR);
        } else {
            *(off_t *)lArg = oftTemp;
            return  (ERROR_NONE);
        }

    case FIORENAME:                                                     /*  文件重命名                  */
        return  (__spifFsRename(pfdentry, (PCHAR)lArg));
    
    case FIOLABELGET:                                                   /*  获取卷标                    */
    case FIOLABELSET:                                                   /*  设置卷标                    */
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    
    case FIOFSTATGET:                                                   /*  获得文件状态                */
        return  (__spifFsStat(pfdentry, (struct stat *)lArg));
    
    case FIOFSTATFSGET:                                                 /*  获得文件系统状态            */
        return  (__spifFsStatfs(pfdentry, (struct statfs *)lArg));
    
    case FIOREADDIR:                                                    /*  获取一个目录信息            */
        return  (__spifFsReadDir(pfdentry, (DIR *)lArg));
    
    case FIOTIMESET:                                                    /*  设置文件时间                */
        return  (__spifFsTimeset(pfdentry, (struct utimbuf *)lArg));
        
    case FIOTRUNC:                                                      /*  改变文件大小                */
        oftTemp = *(off_t *)lArg;
        return  (__spifFsTruncate(pfdentry, oftTemp));
    
    case FIOSYNC:                                                       /*  将文件缓存回写              */
    case FIOFLUSH:
    case FIODATASYNC:
        return  (ERROR_NONE);
        
    case FIOCHMOD:
        return  (__spifFsChmod(pfdentry, (INT)lArg));                    /*  改变文件访问权限            */
    
    case FIOSETFL:                                                      /*  设置新的 flag               */
        if ((INT)lArg & O_NONBLOCK) {
            pfdentry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pfdentry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);
    
    case FIOCHOWN:                                                      /*  修改文件所属关系            */
        return  (__spifFsChown(pfdentry, (LW_IO_USR *)lArg));
    
    case FIOFSTYPE:                                                     /*  获得文件系统类型            */
        *(PCHAR *)lArg = "RAM FileSystem";
        return  (ERROR_NONE);
    
    case FIOGETFORCEDEL:                                                /*  强制卸载设备是否被允许      */
        *(BOOL *)lArg = pspiffs->SPIFFS_bForceDelete;
        return  (ERROR_NONE);
        
#if LW_CFG_FS_SELECT_EN > 0
    case FIOSELECT:
        if (((PLW_SEL_WAKEUPNODE)lArg)->SELWUN_seltypType != SELEXCEPT) {
            SEL_WAKE_UP((PLW_SEL_WAKEUPNODE)lArg);                      /*  唤醒节点                    */
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

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
** 文   件   名: functionality.c
**
** 创   建   人: Pan Yanqi
**
** 文件创建日期: 2021 年 08 月 02 日
**
** 描        述: 功能实现函数
*********************************************************************************************************/
#include "fstester.h"
#include "../driver/mtd/nor/nor.h"
#define IO_SZ  256

#define RANDOM_RANGE(a, b)                                  (lib_rand() % (b - a) + a)
#define RANDOM_ALPHABET()                                   (CHAR)(lib_rand() % 26 + 'a')
#define HUNDRED_PERCENT                                     (100)
#define IOS_PER_LOOPS(uiTestRange, uiLoopTimes, uiIOSize)   ((uiTestRange / uiLoopTimes) / uiIOSize)
/*********************************************************************************************************
** 函数名称: __fstesterUtilSequentialWrite
** 功能描述: 顺序写工具，用于小数据连续写入与连续写入
** 输　入  : uiPerWriteSize  每次写数据大小
**            uiOccupyFactor  占满空间百分比，[0 ~ 100]
**            pMountPoint      挂载点： /mnt/hoitfs
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterUtilSequentialWrite(INT iFdTest, UINT uiPerWriteSize, UINT uiLoopTimes, UINT uiOccupyFactor, UINT uiAccurayWriteTotalSize, PCHAR pMountPoint){
    UINT    i, j;
    UINT    uiWriteSize;
    UINT    uiInnerLoopTimes;
    PCHAR   pWriteBuffer;
    struct  statfs stat;

    statfs(pMountPoint, &stat);
    if(uiOccupyFactor != -1){
        uiWriteSize     = (stat.f_bsize * stat.f_blocks * uiOccupyFactor) / HUNDRED_PERCENT;      /* 写满fs的 @uiOccupyFactor% */
    }
    else if(uiAccurayWriteTotalSize != -1){
        uiWriteSize = uiAccurayWriteTotalSize;
    }

    uiInnerLoopTimes  = IOS_PER_LOOPS(uiWriteSize, uiLoopTimes, uiPerWriteSize);

    pWriteBuffer      = (PCHAR)lib_malloc(uiPerWriteSize);
    
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        for (j = 0; j < uiPerWriteSize; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        write(iFdTest, pWriteBuffer, uiPerWriteSize);
    }
    
    
    lib_free(pWriteBuffer);
    return uiInnerLoopTimes * uiPerWriteSize;
}


/*********************************************************************************************************
** 函数名称: __fstesterRandomRead
** 功能描述: 随机读实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   外层循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterRandomRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    UINT    i;
    UINT    uiRandomReadOffset;
    PCHAR   pReadBuffer;
    UINT    uiInnerLoopTimes = 10 * IOS_PER_LOOPS(uiTestRange, uiLoopTimes, IO_SZ);
    
    pReadBuffer = (PCHAR)lib_malloc(IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        lib_memset(pReadBuffer, 0, IO_SZ);
        uiRandomReadOffset  = lib_random() % uiTestRange;                 /* [0 ~  size] */
        lseek(iFdTest, uiRandomReadOffset, SEEK_SET);
        read(iFdTest, pReadBuffer, IO_SZ);
    }
    lib_free(pReadBuffer);
    return uiInnerLoopTimes * IO_SZ;
}
/*********************************************************************************************************
** 函数名称: __fstesterSequentialRead
** 功能描述: 顺序读实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   外层循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSequentialRead(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){
    (VOID) uiTestRange, uiLoopTimes, pUserValue;

    UINT    i, uiReadIter;
    PCHAR   pReadBuffer;
    UINT    uiInnerLoopTimes = IOS_PER_LOOPS(uiTestRange, uiLoopTimes, IO_SZ);       /* 每次读 IO_SZ，读全部数据 */
    
    pReadBuffer = (PCHAR)lib_malloc(IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    { 
        lib_memset(pReadBuffer, 0, IO_SZ);
        read(iFdTest, pReadBuffer, IO_SZ);
    }
    lib_free(pReadBuffer);
    return uiInnerLoopTimes * IO_SZ;
}
/*********************************************************************************************************
** 函数名称: __fstesterRandomWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterRandomWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){     
    (VOID)  pUserValue,  uiLoopTimes;
    UINT    i, j;
    UINT    uiRandomWriteOffset;
    PCHAR   pWriteBuffer;
    UINT    uiInnerLoopTimes = 10;

    pWriteBuffer  = (PCHAR)lib_malloc(IO_SZ);
    for (i = 0; i < uiInnerLoopTimes; i++)
    {
        uiRandomWriteOffset  = lib_random() % uiTestRange;              /* [0 ~  size] */     
        for (j = 0; j < IO_SZ; j++)
        {
            *(pWriteBuffer + j) = RANDOM_ALPHABET();
        }
        lseek(iFdTest, uiRandomWriteOffset, SEEK_SET);
        write(iFdTest, pWriteBuffer, IO_SZ);
    }
    lib_free(pWriteBuffer);
    return uiInnerLoopTimes * IO_SZ;
}
/*********************************************************************************************************
** 函数名称: __fstesterSequentialWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   外层循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSequentialWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){ 
    (VOID)  iFdTest, uiTestRange;
    INT uiOccupyFactor = 60;
    if(pUserValue != LW_NULL)
        uiOccupyFactor = lib_atoi((PCHAR)pUserValue);
    return __fstesterUtilSequentialWrite(iFdTest, IO_SZ, uiLoopTimes, 
                                         uiOccupyFactor, -1, pMountPoint);
}
/*********************************************************************************************************
** 函数名称: __fstesterSmallWrite
** 功能描述: 随机写实现
** 输　入  : iFdTest        待测试文件描述符
**            uiTestRange   文件大小
**            uiLoopTimes   外层循环测量次数
** 输　出  : None
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT __fstesterSmallWrite(INT iFdTest, UINT uiTestRange, UINT uiLoopTimes, PCHAR pMountPoint, PVOID pUserValue){ 
    (VOID)  iFdTest, uiTestRange;
    INT uiAccurayWriteTotalSize = 1000;
    if(pUserValue != LW_NULL)
        uiAccurayWriteTotalSize = lib_atoi((PCHAR)pUserValue);
    return __fstesterUtilSequentialWrite(iFdTest, 1, uiLoopTimes, 
                                         -1, uiAccurayWriteTotalSize, pMountPoint);
}

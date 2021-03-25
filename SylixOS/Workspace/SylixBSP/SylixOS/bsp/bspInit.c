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
** ��   ��   ��: bspInit.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2006 �� 06 �� 25 ��
**
** ��        ��: BSP �û� C �������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
/*********************************************************************************************************
  ����ϵͳ���
*********************************************************************************************************/
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include "stdlib.h"                                                     /*  for system() function       */
#include "gdbmodule.h"                                                  /*  GDB ������                  */
#include "gdbserver.h"                                                  /*  GDB ������                  */
#include "sys/compiler.h"                                               /*  ���������                  */
/*********************************************************************************************************
  BSP �� ��������
*********************************************************************************************************/
#include "driver/pm/s3c2440a_pm.h"                                      /*  ��Դ��������                */
#include "driver/tty/uart.h"                                            /*  ���Զ˿�                    */
#include "driver/rtc/rtc.h"                                             /*  RTC ����                    */
#include "driver/tty/samsungtty.h"                                      /*  SIO ����                    */
#include "driver/dma/samsungdma.h"                                      /*  DMA ����                    */
#include "driver/mtd/nand/k9f1g08.h"                                    /*  nand flash mtd ����         */
#include "driver/lcd/s3c2440a_lcd.h"                                    /*  lcd ����                    */
#include "driver/i2c/samsungi2c.h"                                      /*  i2c ��������                */
#include "driver/gpio/s3c2440_gpio.h"
#ifdef MINI2440_PACKET
#include "driver/touchscr/touchscr.h"                                   /*  ����������                  */
#elif defined(MICRO2440_PACKET)
#include "driver/touchscr/s3c_onewire.h"                                /*  ����������                  */
#endif                                                                  /*  MICRO2440_PACKET            */
#include "driver/netif/dm9000x.h"                                       /*  DM9000 ����оƬ����         */
#include "driver/sdi/sdInit.h"                                          /*  SD �ӿ�                     */
/*********************************************************************************************************
  ����ϵͳ���ű�
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0 && defined(__GNUC__)
#include "symbol.h"
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
                                                                        /*  defined(__GNUC__)           */
/*********************************************************************************************************
  �ڴ��ʼ��ӳ���
*********************************************************************************************************/
#define  __BSPINIT_MAIN_FILE
#include "bspMap.h"
/*********************************************************************************************************
  GPIO ����
*********************************************************************************************************/
#define __DM9000_EINT_GPIO              (1)                             /*  DM9000 �ⲿ�ж� GPIO ���   */
/*********************************************************************************************************
  ���߳��������̶߳�ջ (t_boot ���Դ�һ��, startup.sh �п����кܶ����Ķ�ջ�Ĳ���)
*********************************************************************************************************/
#define  __LW_THREAD_BOOT_STK_SIZE      (16 * LW_CFG_KB_SIZE)
#define  __LW_THREAD_MAIN_STK_SIZE      (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  ���߳�����
*********************************************************************************************************/
VOID  t_main(VOID);
/*********************************************************************************************************
** ��������: halModeInit
** ��������: ��ʼ��Ŀ��ϵͳ���е�ģʽ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halModeInit (VOID)
{
}
/*********************************************************************************************************
** ��������: halTimeInit
** ��������: ��ʼ��Ŀ��ϵͳʱ��ϵͳ (ϵͳĬ��ʱ��Ϊ:��8��)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID  halTimeInit (VOID)
{
    PLW_RTC_FUNCS   prtcfuncs = rtcGetFuncs();

    rtcDrv();
    rtcDevCreate(prtcfuncs);                                            /*  ����Ӳ�� RTC �豸           */
    rtcToSys();                                                         /*  �� RTC ʱ��ͬ����ϵͳʱ��   */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** ��������: halIdleInit
** ��������: ��ʼ��Ŀ��ϵͳ����ʱ����ҵ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halIdleInit (VOID)
{
    API_SystemHookAdd(armWaitForInterrupt, 
                      LW_OPTION_THREAD_IDLE_HOOK);                      /*  ����ʱ��ͣ CPU              */
}
/*********************************************************************************************************
** ��������: halCacheInit
** ��������: Ŀ��ϵͳ CPU ���ٻ����ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

static VOID  halCacheInit (VOID)
{
    API_CacheLibInit(CACHE_COPYBACK, CACHE_COPYBACK, ARM_MACHINE_920);  /*  ��ʼ�� CACHE ϵͳ           */
    API_CacheEnable(INSTRUCTION_CACHE);
    API_CacheEnable(DATA_CACHE);                                        /*  ʹ�� CACHE                  */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: halFpuInit
** ��������: Ŀ��ϵͳ FPU �������㵥Ԫ��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  halFpuInit (VOID)
{
    API_KernelFpuInit(ARM_MACHINE_920, ARM_FPU_NONE);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: halPmInit
** ��������: ��ʼ��Ŀ��ϵͳ��Դ����ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

static VOID  halPmInit (VOID)
{
    PLW_PMA_FUNCS  pmafuncs;

    pmafuncs = pmGetFuncs();

    pmAdapterCreate("inner_pm", 21, pmafuncs);
}

#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
** ��������: halBusInit
** ��������: ��ʼ��Ŀ��ϵͳ����ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halBusInit (VOID)
{
    PLW_I2C_FUNCS    pi2cfuns  = i2cBusFuns();
    API_I2cLibInit();                                                   /*  ��ʼ�� i2c ��ϵͳ           */

    API_I2cAdapterCreate("/bus/i2c/0", pi2cfuns, 10, 1);                /*  ���� i2c ����������         */
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halDrvInit
** ��������: ��ʼ��Ŀ��ϵͳ��̬��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDrvInit (VOID)
{
    INT              i;
    ULONG            ulMaxBytes;
    PLW_DMA_FUNCS    pdmafuncs;

    /*
     *  standard device driver (rootfs and procfs need install first.)
     */
    rootFsDrv();                                                        /*  ROOT   device driver        */
    procFsDrv();                                                        /*  proc   device driver        */
    shmDrv();                                                           /*  shm    device driver        */
    randDrv();                                                          /*  random device driver        */
    ptyDrv();                                                           /*  pty    device driver        */
    ttyDrv();                                                           /*  tty    device driver        */
    memDrv();                                                           /*  mem    device driver        */
    pipeDrv();                                                          /*  pipe   device driver        */
    spipeDrv();                                                         /*  spipe  device driver        */
    tpsFsDrv();                                                         /*  TPS FS device driver        */
    fatFsDrv();                                                         /*  FAT FS device driver        */
    ramFsDrv();                                                         /*  RAM FS device driver        */
    romFsDrv();                                                         /*  ROM FS device driver        */
    nfsDrv();                                                           /*  nfs    device driver        */
    yaffsDrv();                                                         /*  yaffs  device driver        */
    canDrv();                                                           /*  CAN    device driver        */

    s3c2440GpioDrv();

#ifdef MINI2440_PACKET
    tsDrv();                                                            /*  touch  device driver        */
#elif defined(MICRO2440_PACKET)
    s3cOneWireTsDrv();
#endif

    /*
     *  BSP device driver
     */
    for (i = 0; i < 4; i++) {                                           /*  ��װ 2440 4 ��ͨ�� DMA ͨ�� */
        pdmafuncs = dmaGetFuncs(LW_DMA_CHANNEL0 + i, &ulMaxBytes);
        dmaDrv((UINT)i, pdmafuncs, (size_t)ulMaxBytes);                 /*  ��װ DMA ����������         */
    }

#ifdef MICRO2440_PACKET
    sdDrvInit();                                                        /*  ��װ SD �ӿ�����            */
#endif
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halDevInit
** ��������: ��ʼ��Ŀ��ϵͳ��̬�豸���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDevInit (VOID)
{
    SIO_CHAN    *psio0 = sioChanCreate(0);                              /*  �������� 0 ͨ��             */
    SIO_CHAN    *psio1 = sioChanCreate(1);                              /*  �������� 1 ͨ��             */
    SIO_CHAN    *psio2 = sioChanCreate(2);                              /*  �������� 2 ͨ��             */

#ifdef MICRO2440_PACKET
    S3C_ONEWIRE_TS  oneWireTs;
#endif

    /*
     *  �������ļ�ϵͳʱ, ���Զ����� dev, mnt, var Ŀ¼.
     */
    rootFsDevCreate();                                                  /*  �������ļ�ϵͳ              */
    procFsDevCreate();                                                  /*  ���� proc �ļ�ϵͳ          */
    shmDevCreate();                                                     /*  ���������ڴ��豸            */
    randDevCreate();                                                    /*  ����������ļ�              */

    ttyDevCreate("/dev/ttyS0", psio0, 30, 50);                          /*  add    tty   device         */
    ttyDevCreate("/dev/ttyS1", psio1, 30, 50);                          /*  add    tty   device         */
    ttyDevCreate("/dev/ttyS2", psio2, 30, 50);                          /*  add    tty   device         */

    yaffsDevCreate("/yaffs2");                                          /*  create yaffs device(only fs)*/
    lcdDevCreate();                                                     /*  create lcd device           */

#ifdef MINI2440_PACKET
    tsDevCreate("/dev/input/touch0");                                   /*  create touch device         */
#elif defined(MICRO2440_PACKET)
    oneWireTs.ucTimerNum        = 0;
    oneWireTs.uiOneWireGpio     = 0;
    oneWireTs.ulTimerIntVector  = LW_IRQ_10;
    oneWireTs.ulPclk            = PCLK;
    s3cOneWireTsDevCreate("/dev/input/touch0", &oneWireTs);             /*  create touch device         */
#endif
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halLogInit
** ��������: ��ʼ��Ŀ��ϵͳ��־ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static VOID  halLogInit (VOID)
{
    fd_set      fdLog;

    FD_ZERO(&fdLog);
    FD_SET(STD_OUT, &fdLog);
    API_LogFdSet(STD_OUT + 1, &fdLog);                                  /*  ��ʼ����־                  */
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** ��������: halStdFileInit
** ��������: ��ʼ��Ŀ��ϵͳ��׼�ļ�ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halStdFileInit (VOID)
{
    INT     iFd = open("/dev/ttyS0", O_RDWR, 0);

    if (iFd >= 0) {
        ioctl(iFd, FIOBAUDRATE,   SIO_BAUD_115200);
        ioctl(iFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_7_BIT)));       /*  system terminal 8 bit mode  */

        ioGlobalStdSet(STD_IN,  iFd);
        ioGlobalStdSet(STD_OUT, iFd);
        ioGlobalStdSet(STD_ERR, iFd);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halShellInit
** ��������: ��ʼ��Ŀ��ϵͳ shell ����, (getopt ʹ��ǰһ��Ҫ��ʼ�� shell ����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  halShellInit (VOID)
{
    API_TShellInit();

    /*
     *  ��ʼ�� appl �м�� shell �ӿ�
     */
    zlibShellInit();
    viShellInit();

    /*
     *  ��ʼ�� GDB ������
     */
    gdbInit();
    gdbModuleInit();
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: halVmmInit
** ��������: ��ʼ��Ŀ��ϵͳ�����ڴ�������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  halVmmInit (VOID)
{
    API_VmmLibInit(_G_physicalDesc, _G_virtualDesc, ARM_MACHINE_920);
    API_VmmMmuEnable();
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: halNetInit
** ��������: ���������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

static VOID  halNetInit (VOID)
{
    API_NetInit();                                                      /*  ��ʼ������ϵͳ              */
    API_NetSnmpInit();

    /*
     *  ��ʼ�����總�ӹ���
     */
#if LW_CFG_NET_PING_EN > 0
    API_INetPingInit();
    API_INetPing6Init();
#endif                                                                  /*  LW_CFG_NET_PING_EN > 0      */

#if LW_CFG_NET_NETBIOS_EN > 0
    API_INetNetBiosInit();
    API_INetNetBiosNameSet("sylixos");
#endif                                                                  /*  LW_CFG_NET_NETBIOS_EN > 0   */

#if LW_CFG_NET_TFTP_EN > 0
    API_INetTftpServerInit("/tmp");
#endif                                                                  /*  LW_CFG_NET_TFTP_EN > 0      */

#if LW_CFG_NET_FTPD_EN > 0
    API_INetFtpServerInit("/");
#endif                                                                  /*  LW_CFG_NET_FTP_EN > 0       */

#if LW_CFG_NET_TELNET_EN > 0
    API_INetTelnetInit(LW_NULL);
#endif                                                                  /*  LW_CFG_NET_TELNET_EN > 0    */

#if LW_CFG_NET_NAT_EN > 0
    API_INetNatInit();
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

#if LW_CFG_NET_NPF_EN > 0
    API_INetNpfInit();
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */

#if LW_CFG_NET_QOS_EN > 0
    API_INetQosInit();
#endif                                                                  /*  LW_CFG_NET_QOS_EN > 0       */
}
/*********************************************************************************************************
** ��������: halNetifAttch
** ��������: ����ӿ�����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halNetifAttch (VOID)
{
    dm9000NetInit("192.168.7.30", "255.255.255.0", "192.168.7.1");
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** ��������: halMonitorInit
** ��������: �ں˼�����ϴ���ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static VOID  halMonitorInit (VOID)
{
    /*
     *  ���������ﴴ���ں˼�����ϴ�ͨ��, Ҳ����ʹ�� shell �������.
     */
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** ��������: halPosixInit
** ��������: ��ʼ�� posix ��ϵͳ (���ϵͳ֧�� proc �ļ�ϵͳ, �������� proc �ļ�ϵͳ��װ֮��!)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

static VOID  halPosixInit (VOID)
{
    API_PosixInit();
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** ��������: halSymbolInit
** ��������: ��ʼ��Ŀ��ϵͳ���ű���, (Ϊ module loader �ṩ����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0

static VOID  halSymbolInit (VOID)
{
#ifdef __GNUC__
    void *__aeabi_read_tp();
#endif                                                                  /*  __GNUC__                    */

    API_SymbolInit();

#ifdef __GNUC__
    symbolAddAll();

    /*
     *  GCC will emit calls to this routine under -mtp=soft.
     */
    API_SymbolAdd("__aeabi_read_tp", (caddr_t)__aeabi_read_tp, LW_SYMBOL_FLAG_XEN);
#endif                                                                  /*  __GNUC__                    */
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** ��������: halLoaderInit
** ��������: ��ʼ��Ŀ��ϵͳ�����ģ��װ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  halLoaderInit (VOID)
{
    API_LoaderInit();
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** ��������: halBootThread
** ��������: ������״̬�µĳ�ʼ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  halBootThread (PVOID  pvBootArg)
{
    LW_CLASS_THREADATTR     threadattr = API_ThreadAttrGetDefault();    /*  ʹ��Ĭ������                */

    (VOID)pvBootArg;

#if LW_CFG_SHELL_EN > 0
    halShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_POWERM_EN > 0
    halPmInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#if LW_CFG_DEVICE_EN > 0
    halBusInit();
    halDrvInit();
    halDevInit();
    halStdFileInit();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

#if LW_CFG_LOG_LIB_EN > 0
    halLogInit();
    console_loglevel = default_message_loglevel;                        /*  ���� printk ��ӡ��Ϣ�ȼ�    */
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */

    /*
     *  ��Ϊ yaffs ���������ʱ, ��Ҫ stdout ��ӡ��Ϣ, ����� halDevInit() �б�����, ����û�д���
     *  ��׼�ļ�, ���Ի��ӡ���������Ϣ, ���Խ��˺�����������!
     *  ���δ��ʼ����׼�ļ�����ʾ������Ϣ
     */
#ifdef __GNUC__
    nand_init();
    mtdDevCreateEx("/n");                                               /*  mount mtddevice             */
#else
    nandDevCreateEx("/n");                                              /*  mount nandflash disk(yaffs) */
#endif

#if LW_CFG_DEVICE_EN > 0                                                /*  map rootfs                  */
    rootFsMap(LW_ROOTFS_MAP_LOAD_VAR | LW_ROOTFS_MAP_SYNC_TZ | LW_ROOTFS_MAP_SET_TIME);
#endif

    /*
     *  �����ʼ��һ����� shell ��ʼ��֮��, ��Ϊ��ʼ���������ʱ, ���Զ�ע�� shell ����.
     */
#if LW_CFG_NET_EN > 0
    halNetInit();
    halNetifAttch();                                                    /*  wlan ������Ҫ���ع̼�       */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

#if LW_CFG_POSIX_EN > 0
    halPosixInit();
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#if LW_CFG_SYMBOL_EN > 0
    halSymbolInit();
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */

#if LW_CFG_MODULELOADER_EN > 0
    halLoaderInit();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_MONITOR_EN > 0
    halMonitorInit();
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

#if LW_CFG_SHELL_EN > 0
    tshellStartup();                                                    /*  ִ�������ű�                */
#endif

    API_ThreadAttrSetStackSize(&threadattr, __LW_THREAD_MAIN_STK_SIZE); /*  ���� main �̵߳Ķ�ջ��С    */
    API_ThreadCreate("t_main",
                     (PTHREAD_START_ROUTINE)t_main,
                     &threadattr,
                     LW_NULL);                                          /*  Create "t_main()" thread    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: usrStartup
** ��������: ��ʼ��Ӧ��������, ��������ϵͳ�ĵ�һ������.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  usrStartup (VOID)
{
    LW_CLASS_THREADATTR     threadattr;

    /*
     *  ע��, ��Ҫ�޸ĸó�ʼ��˳�� (�����ȳ�ʼ�� vmm ������ȷ�ĳ�ʼ�� cache,
     *                              ������Ҫ������Դ��������ʼ��)
     */
    halIdleInit();
#if LW_CFG_CPU_FPU_EN > 0
    halFpuInit();
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_RTC_EN > 0
    halTimeInit();
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */

#if LW_CFG_VMM_EN > 0
    halVmmInit();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#if LW_CFG_CACHE_EN > 0
    halCacheInit();
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    API_ThreadAttrBuild(&threadattr,
                        __LW_THREAD_BOOT_STK_SIZE,
                        LW_PRIO_CRITICAL,
                        LW_OPTION_THREAD_STK_CHK,
                        LW_NULL);
    API_ThreadCreate("t_boot",
                     (PTHREAD_START_ROUTINE)halBootThread,
                     &threadattr,
                     LW_NULL);                                          /*  Create boot thread          */
}
/*********************************************************************************************************
** ��������: bspInit
** ��������: C ���
** �䡡��  : NONE
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT bspInit (VOID)
{
    /*
     *  ϵͳ�ں˶���ϵͳ��
     */
    extern UCHAR  __heap_start, __heap_end;

    halModeInit();                                                      /*  ��ʼ��Ӳ��                  */

    /*
     *  ����ĵ��Զ˿����������ϵͳ��, ������Ӧ�ò������ڲ���ϵͳ������.
     *  ��ϵͳ���ִ���ʱ, ����˿��Ե���Ϊ�ؼ�. (��Ŀ��������ͨ�����ùص�)
     */
    debugChannelInit(0);                                                /*  ��ʼ�����Խӿ�              */

    /*
     *  ����ʹ�� bsp ������������, ��� bootloader ֧��, ��ʹ�� bootloader ����.
     *  Ϊ�˼�����ǰ����Ŀ, ���� kfpu=yes �����ں���(�����ж�)ʹ�� FPU.
     */
    API_KernelStartParam("ncpus=1 kdlog=no kderror=yes kfpu=no heapchk=yes "
                         "rfsmap=/boot:/yaffs2/n0,/:/yaffs2/n1");
                                                                        /*  ����ϵͳ������������        */
    API_KernelStart(usrStartup,
                    (PVOID)&__heap_start,
                    (size_t)&__heap_end - (size_t)&__heap_start,
                    LW_NULL, 0);                                        /*  �����ں�                    */

    return  (ERROR_NONE);                                               /*  ����ִ�е�����              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

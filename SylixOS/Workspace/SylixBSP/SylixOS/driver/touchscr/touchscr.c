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
** ��   ��   ��: touchscr.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2014 �� 04 �� 29 ��
**
** ��        ��: 2440 �������ӿ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
#include "touchscr.h"
#include "mouse.h"
/*********************************************************************************************************
  touch screen �豸�ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                 TS_devhdr;                               /*  �豸ͷ                      */
    touchscreen_event_notify   TS_tData;                                /*  �ɼ���������                */
    BOOL                       TS_bIsReadRel;                           /*  �Ƿ��ȡ�� release ����     */
    LW_HANDLE                  TS_hThread;                              /*  ɨ���߳�                    */
    LW_SEL_WAKEUPLIST          TS_selwulList;                           /*  select() �ȴ���             */
    LW_SPINLOCK_DEFINE        (TS_slLock);                              /*  ������                      */
} TS_DEV;
/*********************************************************************************************************
  ����ȫ�ֱ���, ���ڱ���can������
*********************************************************************************************************/
static INT                  touch_dev_num = PX_ERROR;
static UINT32               xp = 0;
static UINT32               yp = 0;
static UINT32               abs_xp = 0;
static UINT32               abs_yp = 0;
static int                  send_press = 0;
static LW_OBJECT_HANDLE     adc_sem;
static UINT32               down_start = 0;

#define S3C2440_ADCTSC_YM_SEN       (1 << 7)
#define S3C2440_ADCTSC_YP_SEN       (1 << 6)
#define S3C2440_ADCTSC_XP_SEN       (1 << 4)
#define S3C2440_ADCTSC_AUTO_PST     (1 << 2)
#define S3C2440_ADCTSC_XY_PST(x)    (x << 0)
/*********************************************************************************************************
  �Զ�ת������
*********************************************************************************************************/
#define AUTOPST     (S3C2440_ADCTSC_YM_SEN | S3C2440_ADCTSC_YP_SEN | S3C2440_ADCTSC_XP_SEN | \
                     S3C2440_ADCTSC_AUTO_PST | S3C2440_ADCTSC_XY_PST(0))
/*********************************************************************************************************
  �ȴ���������
*********************************************************************************************************/
#define WAIT4INT(x) (((x)<<8) | S3C2440_ADCTSC_YM_SEN | S3C2440_ADCTSC_YP_SEN | \
                     S3C2440_ADCTSC_XP_SEN | S3C2440_ADCTSC_XY_PST(3))

#define S3C2440_ADCDAT0_UPDOWN      (1 << 15)
/*********************************************************************************************************
** ��������: getAbsX
** ��������: ��ȡѹ��
** �䡡��  : uiX     X �����ֵ
** �䡡��  : �������ѹ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static unsigned int getAbsX (unsigned int uiX)
{
    unsigned int    iPressure;

    iPressure = uiX;

    return  (iPressure);
}
/*********************************************************************************************************
** ��������: getAbsY
** ��������: ��ȡѹ��
** �䡡��  : uiY     Y �����ֵ
** �䡡��  : �������ѹ��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static unsigned int getAbsY (unsigned int uiY)
{
    unsigned int    iPressure;

    iPressure = uiY;

    return  (iPressure);
}
/*********************************************************************************************************
** ��������: touchIsr
** ��������: adc���жϷ������
** �䡡��  : NONE
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t touchIsr (void *arg)
{
    UINT32  x_data,y_data;
    INT     updown = 0;

    /*
     * ��ȡ��ǰ�Ƿ��Ǵ��������µ��µ��ж�
     */

    /*
     * ��ȡ��ǰ�Ƿ��ǰ���״̬
     */
    INTER_CLR_SUBSRCPND(BIT_SUB_ADC);
    INTER_CLR_SUBSRCPND(BIT_SUB_TC);
    INTER_CLR_PNDING(BIT_ADC);

    x_data = rADCDAT0;
    y_data = rADCDAT1;

    updown = (!(x_data & S3C2440_ADCDAT0_UPDOWN)) && (!(y_data & S3C2440_ADCDAT0_UPDOWN));

    if (updown) {
        if (down_start == 0) {
            rADCTSC  = AUTOPST;
            rADCCON |= 0x01;                                            /* ����ADת��                   */
            down_start++;

        } else {
            xp = (x_data & 0x3ff);
            yp = (y_data & 0x3ff);

                                                                        /* ����abs_x,abs_yֵ            */
            abs_xp = getAbsX(xp);
            abs_yp = getAbsY(yp);
            send_press = 1;
                                                                        /* �ͷ�һ���źŸ��ɼ�����       */
            API_SemaphoreBFlush(adc_sem, NULL);
            xp = 0;
            yp = 0;

                                                                        /* �������������ж�             */
            down_start = 0;
            rADCTSC = WAIT4INT(0);
        }
    } else {                                                            /* ��ʶ��ǰû�б����µı�׼     */
        send_press = 0;
        down_start = 0;
        xp = 0;
        yp = 0;

                                                                        /* �ͷ�һ���źŸ��ȴ�����       */
        API_SemaphoreBFlush(adc_sem, NULL);
        rADCTSC = WAIT4INT(0);
    }

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: touchInit
** ��������: ��ʼ�� ������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void  touchInit (void)
{
    API_SemaphoreBCreate("ad_thread_sem", 0, LW_OPTION_WAIT_FIFO, &adc_sem);

    rADCCON = ((1<<14) | (49 << 6));
    rADCDLY = 40000;

    /*
     * ��װADC �жϷ������,
     */
    API_InterVectorConnect(VIC_CHANNEL_ADC,
                           (PINT_SVR_ROUTINE)touchIsr,
                           (PVOID)NULL,
                           "touchscr");
    API_InterVectorEnable(VIC_CHANNEL_ADC);


    rINTSUBMSK     &= ~(BIT_SUB_TC) ;
    rINTSUBMSK     &= ~(BIT_SUB_ADC) ;

    /*
     * ���ó��жϵȴ�ģʽ���ȴ�������������
     */
    rADCTSC = WAIT4INT(0);
}
/*********************************************************************************************************
** ��������: touchGetXY
** ��������: ��ô����������ѹ X Y
** �䡡��  : pX              X �᷽���ѹ
             pY              Y �᷽���ѹ
** �䡡��  : 1: ��ʾ����Ч   0: ��ʾ����Ч
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static int  touchGetXY (int  *pX, int  *pY)
{
    API_SemaphoreBPend(adc_sem, LW_OPTION_WAIT_INFINITE);

    /*
     * qemu_2440 ����� x y ���෴��.
     */
    *pX = abs_yp;
    *pY = abs_xp;
    return send_press;
}
/*********************************************************************************************************
** ��������: touchThread
** ��������: ������ɨ���߳�
** �䡡��  : ptsDev        �豸
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void touchThread (TS_DEV  *ptsDev)
{
    int                  ix, iy;
    INTREG               iregInterLevel;

    while (1) {
        if (touchGetXY(&ix, &iy)) {
            /*
             *  ��ǰ�е������.
             */
            LW_SPIN_LOCK_QUICK(&ptsDev->TS_slLock, &iregInterLevel);
            ptsDev->TS_tData.kstat |= MOUSE_LEFT;
            ptsDev->TS_tData.xmovement = ix;
            ptsDev->TS_tData.ymovement = iy;
            LW_SPIN_UNLOCK_QUICK(&ptsDev->TS_slLock, iregInterLevel);
            SEL_WAKE_UP_ALL(&ptsDev->TS_selwulList,
                            SELREAD);                                   /*  �ͷ����еȴ������߳�        */
        } else {
            /*
             *  ��ǰû�е������.
             */
            if (ptsDev->TS_tData.kstat & MOUSE_LEFT) {
                LW_SPIN_LOCK_QUICK(&ptsDev->TS_slLock, &iregInterLevel);
                ptsDev->TS_tData.kstat &= (~MOUSE_LEFT);
                LW_SPIN_UNLOCK_QUICK(&ptsDev->TS_slLock, iregInterLevel);
            }

            if (ptsDev->TS_bIsReadRel == LW_FALSE) {                    /*  û�ж�ȡ���ͷŲ���          */
                SEL_WAKE_UP_ALL(&ptsDev->TS_selwulList,
                                SELREAD);                               /*  �ͷ����еȴ������߳�        */
            }
        }
    }
}
/*********************************************************************************************************
** ��������: touchIoctl
** ��������: ������ ioctl
** �䡡��  : ptsDev           �豸
**           iCmd             ��������
**           lArg             ����
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT touchIoctl (TS_DEV  *ptsDev, INT  iCmd, LONG  lArg)
{
    PLW_SEL_WAKEUPNODE   pselwunNode;
    INT                  iError = ERROR_NONE;

    switch (iCmd) {

    case FIOSELECT:
        pselwunNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&ptsDev->TS_selwulList, pselwunNode);
        break;

    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&ptsDev->TS_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;

    default:
        iError = PX_ERROR;
        errno  = ENOSYS;
        break;
    }

     return  (iError);
}
/*********************************************************************************************************
** ��������: touchOpen
** ��������: ������ open
** �䡡��  : ptsDev               �豸
**           pcName               �豸����
**           iFlags               ���豸ʱʹ�õı�־
**           iMode                �򿪵ķ�ʽ������
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  touchOpen (TS_DEV  *ptsDev,
                        PCHAR    pcName,
                        INT      iFlags,
                        INT      iMode)
{
    LW_CLASS_THREADATTR  threadattr;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return    (PX_ERROR);
    }

    if (LW_DEV_INC_USE_COUNT(&ptsDev->TS_devhdr) == 1) {
        touchInit();

        ptsDev->TS_tData.ctype   = MOUSE_CTYPE_ABS;
        ptsDev->TS_tData.kstat   = 0;
        ptsDev->TS_tData.xanalog = 0;
        ptsDev->TS_tData.yanalog = 0;
        ptsDev->TS_bIsReadRel    = LW_TRUE;

        threadattr = API_ThreadAttrGetDefault();
        threadattr.THREADATTR_pvArg      = (void *)ptsDev;
        threadattr.THREADATTR_ucPriority = LW_PRIO_T_SERVICE;
        threadattr.THREADATTR_ulOption  |= LW_OPTION_OBJECT_GLOBAL;

        ptsDev->TS_hThread = API_ThreadCreate("t_touch",
                                              (PTHREAD_START_ROUTINE)touchThread,
                                              &threadattr, LW_NULL);
        return  ((LONG)ptsDev);
    }

    _ErrorHandle(ERROR_IO_FILE_EXIST);
    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: touchClose
** ��������: ������ close
** �䡡��  : ptsDev               �豸
** �䡡��  : ERROR_NONE or PX_ERROR
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT touchClose (TS_DEV  *ptsDev)
{

    if (LW_DEV_GET_USE_COUNT(&ptsDev->TS_devhdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&ptsDev->TS_devhdr)) {
            SEL_WAKE_UP_ALL(&ptsDev->TS_selwulList,
                            SELEXCEPT);                                 /*  �����쳣�ȴ�                */
            SEL_WAKE_UP_ALL(&ptsDev->TS_selwulList,
                            SELWRITE);                                  /*  �����쳣�ȴ�                */
            SEL_WAKE_UP_ALL(&ptsDev->TS_selwulList,
                            SELREAD);                                   /*  �����쳣�ȴ�                */
            API_ThreadDelete(&ptsDev->TS_hThread, LW_NULL);
        }
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: touchClose
** ��������: ������ read
** �䡡��  : ptsDev           �豸
**           pnotify          ������ָ��
**           stNbyte          ��������С�ֽ���
** �䡡��  : ��ȡ�ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t touchRead (TS_DEV                      *ptsDev,
                          touchscreen_event_notify    *pnotify,
                          size_t                       stNbyte)
{
    INTREG  iregInterLevel;

    if (stNbyte == 0) {
        return  (0);
    }

    LW_SPIN_LOCK_QUICK(&ptsDev->TS_slLock, &iregInterLevel);

    pnotify->ctype   = ptsDev->TS_tData.ctype;
    pnotify->kstat   = ptsDev->TS_tData.kstat;
    pnotify->xanalog = ptsDev->TS_tData.xanalog;
    pnotify->yanalog = ptsDev->TS_tData.yanalog;

    if (ptsDev->TS_tData.kstat & MOUSE_LEFT) {                          /*  ��ȡ������¼�              */
        ptsDev->TS_bIsReadRel = LW_FALSE;                               /*  ��Ҫȷ��Ӧ�ö����ͷŲ���    */
    } else {
        ptsDev->TS_bIsReadRel = LW_TRUE;                                /*  �Ѿ���ȡ���ͷŲ���          */
    }

    LW_SPIN_UNLOCK_QUICK(&ptsDev->TS_slLock, iregInterLevel);

    return  (sizeof(touchscreen_event_notify));
}
/*********************************************************************************************************
** ��������: tsDrv
** ��������: ��װ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  tsDrv (void)
{
    if (touch_dev_num > 0) {
        return  (ERROR_NONE);
    }

    touch_dev_num = iosDrvInstall(touchOpen, LW_NULL, touchOpen, touchClose,
                                  touchRead, LW_NULL, touchIoctl);

    DRIVER_LICENSE(touch_dev_num,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(touch_dev_num,      "Li.xiaocheng");
    DRIVER_DESCRIPTION(touch_dev_num, "touch screen driver.");

    return  (touch_dev_num > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** ��������: tsDevCreate
** ��������: �����������豸
** �䡡��  : pcName    �豸��
** �䡡��  : �����Ƿ�ɹ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  tsDevCreate (PCHAR     pcName)
{
    TS_DEV   *ptsDev;
    INT       iTemp;

    if (!pcName) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return    (PX_ERROR);
    }

    if (touch_dev_num <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "can Driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return    (PX_ERROR);
    }

    ptsDev = (TS_DEV *)sys_malloc(sizeof(TS_DEV));
    if (!ptsDev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);
    }
    lib_bzero(ptsDev, sizeof(TS_DEV));

    SEL_WAKE_UP_LIST_INIT(&ptsDev->TS_selwulList);                      /*  ��ʼ�� select �ȴ���        */
    LW_SPIN_INIT(&ptsDev->TS_slLock);                                   /*  ��ʼ��������                */

    iTemp = (INT)iosDevAdd(&ptsDev->TS_devhdr,
                           pcName,
                           touch_dev_num);
    if (iTemp) {
        sys_free(ptsDev);
        _DebugHandle(__ERRORMESSAGE_LEVEL, "add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return    (PX_ERROR);

    } else {
        _ErrorHandle(ERROR_NONE);
        return    (ERROR_NONE);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

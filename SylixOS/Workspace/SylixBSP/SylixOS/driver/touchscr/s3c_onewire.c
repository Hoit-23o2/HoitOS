/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SyliXOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: s3c_onewire.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2014 �� 05 �� 11 ��
**
** ��        ��: һ�ߴ����豸����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include "driver/timer/timer.h"
#include "s3c_onewire.h"
#include "mouse.h"
/*********************************************************************************************************
  ɨ������Ĭ�����ȼ�
*********************************************************************************************************/
#define S3C_ONEWIRE_TS_THREAD_PRIO          (LW_PRIO_NORMAL - 1)
#define S3C_ONEWIRE_TS_THREAD_INTERVAL      10
/*********************************************************************************************************
  һ�ߴ����豸�ṹ
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  TSDEV_devHdr;                           /*  �豸ͷ                      */
    touchscreen_event_notify    TSDEV_tDownData;                        /*  �ɼ���������                */
    BOOL                        TSDEV_bDownData;
    touchscreen_event_notify    TSDEV_tUpData;                          /*  �ɼ���������                */
    BOOL                        TSDEV_bUpData;
    LW_HANDLE                   TSDEV_hThread;                          /*  ɨ���߳�                    */
    LW_SEL_WAKEUPLIST           TSDEV_selwulList;                       /*  select() �ȴ���             */
    LW_SPINLOCK_DEFINE         (TSDEV_slLock);                          /*  ������                      */

    LW_OBJECT_HANDLE            TSDEV_hLock;
    LW_OBJECT_HANDLE            TSDEV_hTimerSem;
    UINT32                      TSDEV_uiTimerCount;
    UCHAR                       TSDEV_ucTimerNum;
    UINT                        TSDEV_uiOneWireGpio;
    UINT64                      TSDEV_ulPclk;
    ULONG                       TSDEV_ulTimerIntVector;
} S3C_ONEWIRE_TS_DEV;
/*********************************************************************************************************
  ����ȫ�ֱ���
*********************************************************************************************************/
static INT  _G_iS3cOneWireTsDrvNum = PX_ERROR;                          /*  ���������                  */
/*********************************************************************************************************
  һ�ߴ������� ��ʱ�� ��غ���
*********************************************************************************************************/
/*********************************************************************************************************
** ��������: s3cOneWireTsTimerIsr
** ��������: һ�ߴ�����ʱ���жϷ������
** ��  ��  : pTsDev          һ�ߴ����豸
**           ulVector        �ж�����
** ��  ��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  s3cOneWireTsTimerIsr (S3C_ONEWIRE_TS_DEV  *pTsDev,
                                          ULONG                ulVector)
{
    INTER_CLR_PNDING(1 << pTsDev->TSDEV_ulTimerIntVector);              /*  ����ж�                    */

    API_SemaphoreBFlush(pTsDev->TSDEV_hTimerSem, LW_NULL);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsInitTimer
** ��������: ��ʼ��һ�ߴ�����ʱ��
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsInitTimer (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    timerConfig(pTsDev->TSDEV_ucTimerNum, TIMER_DISABLE);

    timerSetPrescaler(pTsDev->TSDEV_ucTimerNum, 0);
    timerSetMuxCnt(pTsDev->TSDEV_ucTimerNum, 0);

    pTsDev->TSDEV_uiTimerCount = (pTsDev->TSDEV_ulPclk / (0 + 1) / 2 / 9600 - 1);

    timerSetCnt(pTsDev->TSDEV_ucTimerNum, pTsDev->TSDEV_uiTimerCount);
    timerSetCmp(pTsDev->TSDEV_ucTimerNum, pTsDev->TSDEV_uiTimerCount / 2);

    timerConfig(pTsDev->TSDEV_ucTimerNum, TIMER_RELOAD | TIMER_MANUAL_UPDATE);

    INTER_CLR_PNDING(1 << pTsDev->TSDEV_ulTimerIntVector);              /*  ����ж�                    */

    API_InterVectorConnect(pTsDev->TSDEV_ulTimerIntVector,
                           (PINT_SVR_ROUTINE)s3cOneWireTsTimerIsr,
                           pTsDev,
                           "ts_timer_isr");

    API_InterVectorEnable(pTsDev->TSDEV_ulTimerIntVector);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsSetTimer
** ��������: ����һ�ߴ�����ʱ��
** ��  ��  : pTsDev           һ�ߴ����豸
**           iNum            ����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsSetTimer (S3C_ONEWIRE_TS_DEV  *pTsDev,
                                   INT                  iNum)
{
    pTsDev->TSDEV_uiTimerCount  = (pTsDev->TSDEV_ulPclk / (0 + 1)/ 2 / 9600 - 1);
    pTsDev->TSDEV_uiTimerCount *= iNum;

    timerSetCnt(pTsDev->TSDEV_ucTimerNum, pTsDev->TSDEV_uiTimerCount);
    timerSetCmp(pTsDev->TSDEV_ucTimerNum, pTsDev->TSDEV_uiTimerCount / 2);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsStartTimer
** ��������: ����һ�ߴ�����ʱ��
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsStartTimer (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    timerConfig(pTsDev->TSDEV_ucTimerNum,
            TIMER_DISABLE | TIMER_RELOAD | TIMER_MANUAL_UPDATE);

    timerConfig(pTsDev->TSDEV_ucTimerNum,
            TIMER_ENABLE | TIMER_RELOAD | TIMER_UNMANUAL_UPDATE);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsStopTimer
** ��������: ֹͣһ�ߴ�����ʱ��
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsStopTimer (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    timerConfig(pTsDev->TSDEV_ucTimerNum, TIMER_DISABLE);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsWaitTimer
** ��������: �ȴ�һ�ߴ�����ʱ��
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsWaitTimer (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    API_SemaphoreBPend(pTsDev->TSDEV_hTimerSem, LW_OPTION_WAIT_INFINITE);
}
/*********************************************************************************************************
  һ�ߴ�������
*********************************************************************************************************/
#define S3C_ONEWIRE_TS_REQ_INFO             0x60U
#define S3C_ONEWIRE_TS_REQ_TS               0x40U
/*********************************************************************************************************
** ��������: s3cOneWireTsCrc8
** ��������: ���� CRC8
** ��  ��  : uiValue      ����
**           uiLen        ����
** ��  ��  : CRC8
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static UCHAR  s3cOneWireTsCrc8 (UINT  uiValue,  UINT  uiLen)
{
    UCHAR  ucCrc = 0xACU;

    while (uiLen--) {
        if ((ucCrc & 0x80U) != 0) {
            ucCrc <<= 1;
            ucCrc  ^= 0x7U;
        } else {
            ucCrc <<= 1;
        }
        if ( (uiValue & (1U << 31)) != 0) {
            ucCrc ^= 0x7U;
        }
        uiValue <<= 1;
    }
    return  (ucCrc);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsSession
** ��������: ����һ�ߴ����豸�Ự
** ��  ��  : pTsDev          һ�ߴ����豸
**           ucReq           ����
**           ucRes           ��Ӧ
** ��  ��  : 1: ��ʾ����Ч   0: ��ʾ����Ч
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsSession (S3C_ONEWIRE_TS_DEV  *pTsDev,
                                 UCHAR                ucReq,
                                 UCHAR                ucRes[])
{
    UINT   uiReq;
    UINT  *uiRes;
    UINT   i;
    INT    iValue;

    uiReq = (ucReq << 24) | (s3cOneWireTsCrc8(ucReq << 24, 8) << 16);
    uiRes = (UINT *)ucRes;

    API_GpioDirectionOutput(pTsDev->TSDEV_uiOneWireGpio, 1);

    s3cOneWireTsSetTimer(pTsDev, 20);

    s3cOneWireTsStartTimer(pTsDev);

    for (i = 0; i < 3; i++) {
        s3cOneWireTsWaitTimer(pTsDev);
    }

    s3cOneWireTsStopTimer(pTsDev);

    API_GpioSetValue(pTsDev->TSDEV_uiOneWireGpio, 0);

    s3cOneWireTsSetTimer(pTsDev, 1);

    s3cOneWireTsStartTimer(pTsDev);

    for (i = 0; i < 2; i++) {
        s3cOneWireTsWaitTimer(pTsDev);
    }

    for (i = 0; i < 16; i++) {
        iValue  = (uiReq & (1U <<31));
        uiReq <<= 1;
        API_GpioSetValue(pTsDev->TSDEV_uiOneWireGpio, iValue);
        s3cOneWireTsWaitTimer(pTsDev);
    }

    s3cOneWireTsWaitTimer(pTsDev);

    API_GpioDirectionInput(pTsDev->TSDEV_uiOneWireGpio);

    s3cOneWireTsWaitTimer(pTsDev);

    for (i = 0; i < 32; i++) {
        *uiRes <<= 1;
        *uiRes  |= API_GpioGetValue(pTsDev->TSDEV_uiOneWireGpio);
        s3cOneWireTsWaitTimer(pTsDev);
    }

    s3cOneWireTsStopTimer(pTsDev);

    API_GpioDirectionOutput(pTsDev->TSDEV_uiOneWireGpio, 1);

    return  (s3cOneWireTsCrc8(*uiRes, 24) == ucRes[0]);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsTrySession
** ��������: ����һ��һ�ߴ����豸�Ự
** ��  ��  : pTsDev          һ�ߴ����豸
**           ucReq           ����
**           ucRes           ��Ӧ
** ��  ��  : 1: ��ʾ����Ч   0: ��ʾ����Ч
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsTrySession (S3C_ONEWIRE_TS_DEV  *pTsDev,
                                    UCHAR                ucReq,
                                    UCHAR                ucRes[])
{
    INT  i;

    API_SemaphoreBPend(pTsDev->TSDEV_hLock, LW_OPTION_WAIT_INFINITE);

    for (i = 0; i < 3; i++) {
        if (s3cOneWireTsSession(pTsDev, ucReq, ucRes)) {
            API_SemaphoreBPost(pTsDev->TSDEV_hLock);
            return  (1);
        }
    }

    API_SemaphoreBPost(pTsDev->TSDEV_hLock);

    return  (0);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsSetBacklight
** ��������: ���ñ�������
** ��  ��  : pTsDev          һ�ߴ����豸
**           ucBrightness    ��������(0 - 127)
** ��  ��  : 1: ��ʾ�ɹ�     0: ��ʾʧ��
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsSetBacklight (S3C_ONEWIRE_TS_DEV  *pTsDev,
                                      UINT8                ucBrightness)
{
    UCHAR  ucRes[4];

    if (ucBrightness > 127) {
        ucBrightness = 127;
    }

    return  (s3cOneWireTsTrySession(pTsDev, ucBrightness | 0x80U, ucRes));
}
/*********************************************************************************************************
** ��������: s3cOneWireTsGetXY
** ��������: ��ô����������ѹ X Y
** ��  ��  : pTsDev          һ�ߴ����豸
**           pX              X �᷽���ѹ
**           pY              Y �᷽���ѹ
** ��  ��  : 1: ��ʾ����Ч   0: ��ʾ����Ч
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsGetXY (S3C_ONEWIRE_TS_DEV  *pTsDev,
                               INT                 *pX,
                               INT                 *pY,
                               BOOL                *pDown)
{
    UCHAR  ucRes[4];
    INT    iRet;

    iRet = s3cOneWireTsTrySession(pTsDev, S3C_ONEWIRE_TS_REQ_TS, ucRes);
    if (iRet) {
        *pX = ((ucRes[3] >>   4U) << 8U) + ucRes[2];
        *pY = ((ucRes[3] &  0xFU) << 8U) + ucRes[1];
        if (!((*pX != 0xFFFU) && (*pY != 0xFFFU))) {
            *pDown = LW_FALSE;
        } else {
            *pDown = LW_TRUE;
        }
    }
    return  (iRet);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsInit
** ��������: ��ʼ��һ�ߴ����豸
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsInit (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    LW_OBJECT_HANDLE  hHandle;
    INT               iRet;

    hHandle = API_SemaphoreBCreate("ts_sem", 0, LW_OPTION_WAIT_FIFO, &pTsDev->TSDEV_hTimerSem);
    if (hHandle == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    hHandle = API_SemaphoreBCreate("ts_lock", 1, LW_OPTION_WAIT_FIFO, &pTsDev->TSDEV_hLock);
    if (hHandle == LW_OBJECT_HANDLE_INVALID) {
        return  (PX_ERROR);
    }

    iRet = API_GpioRequestOne(pTsDev->TSDEV_uiOneWireGpio, LW_GPIOF_OUT_INIT_HIGH, "ts_gpio");
    if (iRet != ERROR_NONE) {
        return  (iRet);
    }

    s3cOneWireTsInitTimer(pTsDev);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsExit
** ��������: ����һ�ߴ����豸
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsExit (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    s3cOneWireTsStopTimer(pTsDev);

    API_GpioFree(pTsDev->TSDEV_uiOneWireGpio);

    API_SemaphoreBDelete(&pTsDev->TSDEV_hTimerSem);

    API_SemaphoreBDelete(&pTsDev->TSDEV_hLock);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsThread
** ��������: һ�ߴ����豸ɨ���߳�
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  s3cOneWireTsThread (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    INT     iX, iY;
    BOOL    bDown;
    INTREG  iRegInterLevel;
    BOOL    bReportUp;

    bReportUp = LW_FALSE;

    while (1) {
        if (s3cOneWireTsGetXY(pTsDev, &iX, &iY, &bDown)) {
            if (bDown) {
                /*
                 *  ��ǰ�е������.
                 */
                LW_SPIN_LOCK_QUICK(&pTsDev->TSDEV_slLock, &iRegInterLevel);
                pTsDev->TSDEV_tDownData.kstat        = MOUSE_LEFT;
                pTsDev->TSDEV_tDownData.xmovement    = iX;
                pTsDev->TSDEV_tDownData.ymovement    = iY;
                pTsDev->TSDEV_bDownData              = LW_TRUE;
                LW_SPIN_UNLOCK_QUICK(&pTsDev->TSDEV_slLock, iRegInterLevel);

                SEL_WAKE_UP_ALL(&pTsDev->TSDEV_selwulList,
                                SELREAD);                               /*  �ͷ����еȴ������߳�        */

                bReportUp = LW_TRUE;

            } else if (bReportUp) {
                /*
                 *  ��ǰ�е������.
                 */
                LW_SPIN_LOCK_QUICK(&pTsDev->TSDEV_slLock, &iRegInterLevel);
                pTsDev->TSDEV_tDownData.kstat    = 0;
                pTsDev->TSDEV_tUpData.kstat      = 0;
                pTsDev->TSDEV_tUpData.xmovement  = pTsDev->TSDEV_tDownData.xmovement;
                pTsDev->TSDEV_tUpData.ymovement  = pTsDev->TSDEV_tDownData.ymovement;
                pTsDev->TSDEV_bUpData            = LW_TRUE;
                LW_SPIN_UNLOCK_QUICK(&pTsDev->TSDEV_slLock, iRegInterLevel);

                SEL_WAKE_UP_ALL(&pTsDev->TSDEV_selwulList,
                                SELREAD);                               /*  �ͷ����еȴ������߳�        */

                bReportUp = LW_FALSE;
            } else {
                /*
                 *  ��ǰû�е������.
                 */
                API_TimeMSleep(S3C_ONEWIRE_TS_THREAD_INTERVAL);
            }
        } else {
            /*
             *  ��ǰû�е������.
             */
            API_TimeMSleep(S3C_ONEWIRE_TS_THREAD_INTERVAL);
        }
    }
}
/*********************************************************************************************************
** ��������: s3cOneWireTsIoctl
** ��������: һ�ߴ����豸����
** ��  ��  : pTsDev           һ�ߴ����豸
**           iCmd             ��������
**           lArg             ����
** ��  ��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsIoctl (S3C_ONEWIRE_TS_DEV  *pTsDev,
                               INT                  iCmd,
                               LONG                 lArg)
{
    PLW_SEL_WAKEUPNODE  pSelNode;
    INT                 iError = ERROR_NONE;

    switch (iCmd) {

    case S3C_ONEWIRE_TS_SET_PRIO:
         API_ThreadSetPriority(pTsDev->TSDEV_hThread, *(UINT8 *)lArg);
         break;

    case S3C_ONEWIRE_TS_SET_BACKLIGHT:
         s3cOneWireTsSetBacklight(pTsDev, *(UINT8 *)lArg);
         break;

    case FIOSELECT:
        pSelNode = (PLW_SEL_WAKEUPNODE)lArg;
        SEL_WAKE_NODE_ADD(&pTsDev->TSDEV_selwulList, pSelNode);

        if (pTsDev->TSDEV_bUpData) {
            SEL_WAKE_UP_ALL(&pTsDev->TSDEV_selwulList,
                            SELREAD);                                   /*  �ͷ����еȴ������߳�        */
        }
        break;

    case FIOUNSELECT:
        SEL_WAKE_NODE_DELETE(&pTsDev->TSDEV_selwulList, (PLW_SEL_WAKEUPNODE)lArg);
        break;

    default:
        iError = PX_ERROR;
        errno  = ENOSYS;
        break;
    }

     return  (iError);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsOpen
** ��������: ��һ�ߴ����豸
** ��  ��  : pTsDev           һ�ߴ����豸
**           pcName           �豸����
**           iFlags           ���豸ʱʹ�õı�־
**           iMode            �򿪵ķ�ʽ������
** ��  ��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  s3cOneWireTsOpen (S3C_ONEWIRE_TS_DEV  *pTsDev,
                               PCHAR                pcName,
                               INT                  iFlags,
                               INT                  iMode)
{
    LW_CLASS_THREADATTR  threadAttr;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);
    }

    if (LW_DEV_INC_USE_COUNT(&pTsDev->TSDEV_devHdr) == 1) {

        if (s3cOneWireTsInit(pTsDev) != ERROR_NONE) {
            _ErrorHandle(ERROR_IO_DEVICE_ERROR);
            return  (PX_ERROR);
        }

        pTsDev->TSDEV_tDownData.ctype    = MOUSE_CTYPE_ABS;
        pTsDev->TSDEV_tDownData.kstat    = 0;
        pTsDev->TSDEV_tDownData.xanalog  = 0;
        pTsDev->TSDEV_tDownData.yanalog  = 0;
        pTsDev->TSDEV_bDownData          = LW_FALSE;

        pTsDev->TSDEV_tUpData.ctype      = MOUSE_CTYPE_ABS;
        pTsDev->TSDEV_tUpData.kstat      = 0;
        pTsDev->TSDEV_tUpData.xanalog    = 0;
        pTsDev->TSDEV_tUpData.yanalog    = 0;
        pTsDev->TSDEV_bUpData            = LW_FALSE;

        threadAttr = API_ThreadAttrGetDefault();
        threadAttr.THREADATTR_pvArg      = (PVOID)pTsDev;
        threadAttr.THREADATTR_ucPriority = S3C_ONEWIRE_TS_THREAD_PRIO;
        threadAttr.THREADATTR_ulOption  |= LW_OPTION_OBJECT_GLOBAL;

        pTsDev->TSDEV_hThread = API_ThreadCreate("t_touch",
                                                 (PTHREAD_START_ROUTINE)s3cOneWireTsThread,
                                                 &threadAttr, LW_NULL);
        return  ((LONG)pTsDev);
    } else {
        return  ((LONG)pTsDev);
    }
}
/*********************************************************************************************************
** ��������: s3cOneWireTsClose
** ��������: �ر�һ�ߴ����豸
** ��  ��  : pTsDev          һ�ߴ����豸
** ��  ��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  s3cOneWireTsClose (S3C_ONEWIRE_TS_DEV  *pTsDev)
{
    if (LW_DEV_GET_USE_COUNT(&pTsDev->TSDEV_devHdr)) {
        if (!LW_DEV_DEC_USE_COUNT(&pTsDev->TSDEV_devHdr)) {

            SEL_WAKE_UP_ALL(&pTsDev->TSDEV_selwulList,
                            SELEXCEPT);                                 /*  �����쳣�ȴ�                */
            SEL_WAKE_UP_ALL(&pTsDev->TSDEV_selwulList,
                            SELWRITE);                                  /*  �����쳣�ȴ�                */
            SEL_WAKE_UP_ALL(&pTsDev->TSDEV_selwulList,
                            SELREAD);                                   /*  �����쳣�ȴ�                */
            API_ThreadDelete(&pTsDev->TSDEV_hThread, LW_NULL);

            s3cOneWireTsExit(pTsDev);
        }
    }
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsRead
** ��������: ��һ�ߴ����豸
** ��  ��  : pTsDev           һ�ߴ����豸
**           pTouchData       ������ָ��
**           stNbyte          ��������С�ֽ���
** ��  ��  : �ɹ���ȡ���ֽ���
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static ssize_t  s3cOneWireTsRead (S3C_ONEWIRE_TS_DEV        *pTsDev,
                                  touchscreen_event_notify  *pTouchData,
                                  size_t                     stNbyte)
{
    INTREG  iRegInterLevel;

    if (stNbyte == 0) {
        return  (0);
    }

    LW_SPIN_LOCK_QUICK(&pTsDev->TSDEV_slLock, &iRegInterLevel);

    if (pTsDev->TSDEV_bDownData) {
        pTsDev->TSDEV_bDownData         = LW_FALSE;
        pTouchData->ctype               = pTsDev->TSDEV_tDownData.ctype;
        pTouchData->kstat               = MOUSE_LEFT;
        pTouchData->xanalog             = pTsDev->TSDEV_tDownData.xanalog;
        pTouchData->yanalog             = pTsDev->TSDEV_tDownData.yanalog;
    } else if (pTsDev->TSDEV_bUpData) {
        pTsDev->TSDEV_bUpData           = LW_FALSE;
        pTouchData->ctype               = pTsDev->TSDEV_tUpData.ctype;
        pTouchData->kstat               = 0;
        pTouchData->xanalog             = pTsDev->TSDEV_tUpData.xanalog;
        pTouchData->yanalog             = pTsDev->TSDEV_tUpData.yanalog;
    } else {
        LW_SPIN_UNLOCK_QUICK(&pTsDev->TSDEV_slLock, iRegInterLevel);
        return  (0);
    }

    LW_SPIN_UNLOCK_QUICK(&pTsDev->TSDEV_slLock, iRegInterLevel);

    return  (sizeof(touchscreen_event_notify));
}
/*********************************************************************************************************
** ��������: s3cOneWireTsDrv
** ��������: ��װһ�ߴ�����������
** ��  ��  : NONE
** ��  ��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  s3cOneWireTsDrv (VOID)
{
    if (_G_iS3cOneWireTsDrvNum > 0) {
        return  (ERROR_NONE);
    }

    _G_iS3cOneWireTsDrvNum = iosDrvInstall(s3cOneWireTsOpen,
                                           LW_NULL,
                                           s3cOneWireTsOpen,
                                           s3cOneWireTsClose,
                                           s3cOneWireTsRead,
                                           LW_NULL,
                                           s3cOneWireTsIoctl);

    DRIVER_LICENSE(_G_iS3cOneWireTsDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iS3cOneWireTsDrvNum,      "Jiao.jinxing");
    DRIVER_DESCRIPTION(_G_iS3cOneWireTsDrvNum, "s3c onewire touch screen driver.");

    return  (_G_iS3cOneWireTsDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR);
}
/*********************************************************************************************************
** ��������: s3cOneWireTsCreate
** ��������: ����һ�ߴ����豸
** ��  ��  : pcName             �豸��
**           pTs                һ�ߴ�����Ϣ
** ��  ��  : ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  s3cOneWireTsDevCreate (PCHAR  pcName, S3C_ONEWIRE_TS  *pTs)
{
    S3C_ONEWIRE_TS_DEV  *pTsDev;
    INT                  iTemp;

    if (!pcName || !pTs) {
        _DebugHandle(__ERRORMESSAGE_LEVEL,
                     "s3cOneWireTsDevCreate() error : parameter error.\r\n");
        _ErrorHandle(EINVAL);
        return  (PX_ERROR);
    }

    if (_G_iS3cOneWireTsDrvNum <= 0) {
        _DebugHandle(__ERRORMESSAGE_LEVEL,
                     "s3cOneWireTsDevCreate() error : touch screen driver invalidate.\r\n");
        _ErrorHandle(ERROR_IO_NO_DRIVER);
        return  (PX_ERROR);
    }

    pTsDev = (S3C_ONEWIRE_TS_DEV *)__SHEAP_ALLOC(sizeof(S3C_ONEWIRE_TS_DEV));
    if (!pTsDev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL,
                     "s3cOneWireTsDevCreate() error : system low memory.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    }

    lib_memset(pTsDev, 0, sizeof(S3C_ONEWIRE_TS_DEV));

    pTsDev->TSDEV_ulPclk            = pTs->ulPclk;
    pTsDev->TSDEV_ucTimerNum        = pTs->ucTimerNum;
    pTsDev->TSDEV_ulTimerIntVector  = pTs->ulTimerIntVector;
    pTsDev->TSDEV_uiOneWireGpio     = pTs->uiOneWireGpio;

    SEL_WAKE_UP_LIST_INIT(&pTsDev->TSDEV_selwulList);                   /*  ��ʼ�� select �ȴ���        */
    LW_SPIN_INIT(&pTsDev->TSDEV_slLock);                                /*  ��ʼ��������                */

    iTemp = (INT)iosDevAdd(&pTsDev->TSDEV_devHdr,
                           pcName,
                           _G_iS3cOneWireTsDrvNum);
    if (iTemp) {
        __SHEAP_FREE(pTsDev);
        _DebugHandle(__ERRORMESSAGE_LEVEL,
                     "s3cOneWireTsDevCreate() error : add device error.\r\n");
        _ErrorHandle(ERROR_SYSTEM_LOW_MEMORY);
        return  (PX_ERROR);
    } else {
        _ErrorHandle(ERROR_NONE);
        return  (ERROR_NONE);
    }
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

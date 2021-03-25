/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: s3csdi.c
**
** ��   ��   ��: Zeng.Bo(����)
**
** �ļ���������: 2010 �� 11 �� 24 ��
**
** ��        ��: s3c2440a sd����������Ӳ������Դ�ļ�

** BUG:
2010.12.01  ���� __SDI_REG_RSP0 ��ַ�������,�޸�֮.
2010.12.08  �Ż��ڲ�����.
2010.03.31  �޸�__s3csdPrepareData(),�Բ�ͬ�豸���ò�ͬ������λ��(��Ҫ�Ǽ���MMC).
2014.10.31  �޸�Ϊ���� SDM �ӿڵ�����
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
#include "mciLib.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __S3CSDHOST_NAME            "/bus/sd/0"

#ifdef  SDIO_OPS_DEBUG
#define  SDIO_DBG(fmt, args...)     _DebugFormat(__LOGMESSAGE_LEVEL, fmt, ##args)
#else
#define  SDIO_DBG(fmt, args...)
#endif
#define  SDIO_ERR(fmt, args...)     _DebugFormat(__ERRORMESSAGE_LEVEL, fmt, ##args)
/*********************************************************************************************************
  �ж���������
*********************************************************************************************************/
#define __NCD_SD                    (rGPGDAT & (1u << 8))
#define __SDCARD_INSERT             (__NCD_SD == 0)

#define __PIN_SET_IO()              rGPGCON = rGPGCON & ~(3 << 16)
#define __PIN_SET_INT()             rGPGCON = (rGPGCON & ~(3 << 16)) | (2 << 16)
/*********************************************************************************************************
  �������ڲ��ṹ
*********************************************************************************************************/
typedef struct {
    SD_HOST       sdhost;
    SD_CALLBACK   callbackChkDev;
    PVOID         pvCallBackArg;
    PVOID         pvSdmHost;
} __S3CSD_HOST;
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static INT __s3csdIoCtl(PLW_SD_ADAPTER          psdadapter,
                        INT                     iCmd,
                        LONG                    lArg);
static INT __s3csdTransfer(PLW_SD_ADAPTER       psdadapter,
                           PLW_SD_DEVICE        psddevice,
                           PLW_SD_MESSAGE       psdmsg,
                           INT                  iNum);
static INT  __s3csdCallBackInstall(SD_HOST     *pHost,
                                   INT          iCallbackType,
                                   SD_CALLBACK  callback,
                                   PVOID        pvCallbackArg);
static INT  __s3csdCallBackUnInstall(SD_HOST   *pHost,
                                    INT         iCallbackType);

static VOID __s3csdSdioIntEn(SD_HOST *pHost, BOOL bEnable);
static VOID __s3csdHotPlugInit(VOID);
static VOID __s3csdMciIrqHandle(ULONG ulArg);
static irqreturn_t __s3csdCdIntr(PVOID  pvArg, ULONG ulVec);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_SD_FUNCS        _G_sdfuncS3cSd;
static UINT32             _G_s3csdOCR = SD_VDD_32_33 |                  /*  OCR�а������������ĵ�ѹ֧�� */
                                        SD_VDD_33_34 |                  /*  ���,�����豸����֧�����   */
                                        SD_VDD_34_35 |
                                        SD_VDD_35_36;

static __S3CSD_HOST       _G_s3csdHost = {

        {
            .SDHOST_cpcName                = __S3CSDHOST_NAME,
            .SDHOST_iType                  = SDHOST_TYPE_SD,
            .SDHOST_iCapbility             = SDHOST_CAP_DATA_4BIT,
            .SDHOST_pfuncCallbackInstall   = __s3csdCallBackInstall,
            .SDHOST_pfuncCallbackUnInstall = __s3csdCallBackUnInstall,
            .SDHOST_pfuncSdioIntEn         = __s3csdSdioIntEn,
        },
};
/*********************************************************************************************************
  mciLibͨ�Ŷ���
*********************************************************************************************************/
static PSDIO_DAT            _G_pmciXfer = NULL;                         /*  mci ��ʹ�õĴ������        */
/*********************************************************************************************************
** ��������: __s3csdIoCtl
** ��������: SD����IO����
** ��    ��: psdadapter     ������
**           iCmd           ��������
**           lArg           ����
** ��    ��: NONE
** ��    ��: ERROR CODE
*********************************************************************************************************/
static INT __s3csdIoCtl (PLW_SD_ADAPTER  psdadapter,
                         INT             iCmd,
                         LONG            lArg)
{
    INT iError = ERROR_NONE;

    switch (iCmd) {
    case SDBUS_CTRL_POWEROFF:
        break;

    case SDBUS_CTRL_POWERUP:
    case SDBUS_CTRL_POWERON:
        sdioReset(_G_pmciXfer);
        bspDelayUs(50000);
        break;

    case SDBUS_CTRL_SETBUSWIDTH:
        iError = sdioSetBusWidth((INT)lArg);
        _G_pmciXfer->SDIO_iBusWidth = 1 << lArg;
        break;

    case SDBUS_CTRL_SETCLK:
        sdioSetClock((UINT32)lArg);
        break;

    case SDBUS_CTRL_DELAYCLK:
        break;

    case SDBUS_CTRL_GETOCR:
        *(UINT32 *)lArg = _G_s3csdOCR;
        iError = ERROR_NONE;
        break;

    default:
        _DebugHandle(__ERRORMESSAGE_LEVEL,"__s3csdIoCtl() error : can't support this cmd.\r\n");
        iError = PX_ERROR;
        break;
    }

    return  (iError);
}
/*********************************************************************************************************
** ��������: __s3csdTransfer
** ��������: SD���ش��亯��
** ��    ��: psdadapter   ������
**           psddevice    �豸
**           psdmsg       ������Ϣ
**           iNum         ��Ϣ����
** ��    ��: NONE
** ��    ��: ERROR CODE
*********************************************************************************************************/
static INT __s3csdTransfer (PLW_SD_ADAPTER      psdadapter,
                            PLW_SD_DEVICE       psddevice,
                            PLW_SD_MESSAGE      psdmsg,
                            INT                 iNum)
{
    PSDIO_DAT       pMciXfer = _G_pmciXfer;
    INT             i        = 0;

    while (i < iNum && psdmsg != NULL) {

        pMciXfer->SDIO_psdcmd  = psdmsg->SDMSG_psdcmdCmd;
        pMciXfer->SDIO_psddata = psdmsg->SDMSG_psddata;

        if (pMciXfer->SDIO_psddata) {
            BOOL bWrite;
            bWrite = SD_DAT_IS_WRITE(psdmsg->SDMSG_psddata) ? TRUE : FALSE;

            pMciXfer->SDIO_sdiopHeader.SDIOP_iBlkMode = SD_DAT_IS_STREAM(psdmsg->SDMSG_psddata) ?
                                                        FALSE : TRUE;
            pMciXfer->SDIO_sdiopHeader.SDIOP_iRdWr    = bWrite;

            pMciXfer->SDIO_puiBuf = bWrite ?  psdmsg->SDMSG_pucWrtBuffer : psdmsg->SDMSG_pucRdBuffer;
            pMciXfer->SDIO_bData  = TRUE;

        } else {
            pMciXfer->SDIO_puiBuf = NULL;
            pMciXfer->SDIO_bData  = FALSE;
        }

        if (pMciXfer->SDIO_request.MCIDRV_request) {
            pMciXfer->SDIO_request.MCIDRV_request(pMciXfer);
            if (pMciXfer->SDIO_uiCmdError != ERROR_NONE) {
                SDIO_DBG(" cmd(%d) error.\r\n", psdmsg->SDMSG_psdcmdCmd->SDCMD_uiOpcode);
                return  (PX_ERROR);
            }

            if (pMciXfer->SDIO_psddata && pMciXfer->SDIO_uiDataError != ERROR_NONE) {
                SDIO_DBG("data error.\r\n");
                return  (PX_ERROR);
            }

        } else {
            return  (PX_ERROR);
        }


        /*
         * �����ֹͣ����,�������
         */
        if (psdmsg->SDMSG_psdcmdStop) {
            pMciXfer->SDIO_psdcmd  = psdmsg->SDMSG_psdcmdStop;
            pMciXfer->SDIO_psddata = NULL;
            pMciXfer->SDIO_bData   = FALSE;
            pMciXfer->SDIO_request.MCIDRV_request(pMciXfer);
        }

        i++;
        psdmsg++;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __s3csdCallBackInstall
** ��������: ��װ�ص�����
** ��    ��: pHost            ����������
**           iCallbackType    �ص���������
**           callback         �ص�����ָ��
**           pvCallbackArg    �ص���������
** ��    ��: NONE
** ��    ��: ERROR CODE
*********************************************************************************************************/
static INT  __s3csdCallBackInstall (SD_HOST          *pHost,
                                    INT               iCallbackType,
                                    SD_CALLBACK       callback,
                                    PVOID             pvCallbackArg)
{
    __S3CSD_HOST *pS3cHost = (__S3CSD_HOST *)pHost;
    if (!pS3cHost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        pS3cHost->callbackChkDev = callback;
        pS3cHost->pvCallBackArg  = pvCallbackArg;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __s3csdCallBackUnInstall
** ��������: ж�ذ�װ�Ļص�����
** ��    ��: NONE
** ��    ��: NONE
** ��    ��: ERROR CODE
*********************************************************************************************************/
static INT  __s3csdCallBackUnInstall (SD_HOST          *pHost,
                                     INT               iCallbackType)
{
    __S3CSD_HOST *pS3cHost = (__S3CSD_HOST *)pHost;
    if (!pS3cHost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        pS3cHost->callbackChkDev = NULL;
        pS3cHost->pvCallBackArg  = NULL;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __s3csdSdioIntEn
** ��������: ʹ�� SDIO �ж�
** ��    ��: pHost      SD ����������
**           bEnable    �Ƿ�ʹ��
** ��    ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
static VOID __s3csdSdioIntEn (SD_HOST *pHost, BOOL bEnable)
{
    sdioIntEnable(_G_pmciXfer, bEnable);
}
/*********************************************************************************************************
** ��������: __s3csdHotPlugInit
** ��������:  �Ȳ��֧�ֳ�ʼ��
** ��    ��: NONE
** ��    ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
static VOID __s3csdHotPlugInit (VOID)
{
    __PIN_SET_INT();

    rEXTINT2  = (rEXTINT2 | 7);                                         /*  �ⲿ�ж�16Ϊ˫���ش����ж�  */
    rEINTPEND =  rEINTPEND | (1u << 16);                                /*  ����ⲿ�ж�16              */
    rEINTMASK =  rEINTMASK & ~(1u << 16);                               /*  ʹ���ⲿ�ж�16              */

    ClearPending(VIC_CHANNEL_EINT8_23);                                 /*  ����ⲿ�ж�2��־λ         */

    API_InterVectorConnect(VIC_CHANNEL_EINT8_23,
                           __s3csdCdIntr,
                           NULL,
                           "sdcd_isr");                                 /*  �����ж��Ӻ���              */

    API_InterVectorEnable(VIC_CHANNEL_EINT8_23);                        /*  ʹ���ⲿ�ж�2               */
}
/*********************************************************************************************************
** ��������: __s3csdCdIntr
** ��������: �Ȳ�������жϷ���
** ��    ��: pvArg     ����
**           ulVec     �ж�������
** ��    ��: NONE
** ��    ��: ERROR CODE
*********************************************************************************************************/
static irqreturn_t __s3csdCdIntr (PVOID  pvArg, ULONG ulVec)
{
    API_InterVectorDisable(VIC_CHANNEL_EINT8_23);

    rEINTPEND =  rEINTPEND | (1u << 16);                                /*  ����ⲿ�ж�16              */

    ClearPending(BIT_EINT8_23);                                         /*  ����ⲿ�ж�16��־λ        */

    __PIN_SET_IO();                                                     /*  ��ΪIO��                    */

    if (__SDCARD_INSERT) {
        API_SdmEventNotify(_G_s3csdHost.pvSdmHost, SDM_EVENT_DEV_INSERT);

    } else {
        if (_G_s3csdHost.callbackChkDev) {
            _G_s3csdHost.callbackChkDev(_G_s3csdHost.pvCallBackArg, SDHOST_DEVSTA_UNEXIST);
        }

        API_SdmEventNotify(_G_s3csdHost.pvSdmHost, SDM_EVENT_DEV_REMOVE);
    }

    __PIN_SET_INT();                                                    /*  ��Ϊ�жϿ�                  */

    API_InterVectorEnable(VIC_CHANNEL_EINT8_23);                        /*  ʹ���ⲿ�ж�2               */

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: __s3csdMciIrqHandle
** ��������: ��mci��ʹ��
** ��    ��: ulArg  ����
** ��    ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
static VOID __s3csdMciIrqHandle (ULONG ulArg)
{
    PVOID pvSdmHost = (PVOID)ulArg;

    API_SdmEventNotify(pvSdmHost, SDM_EVENT_SDIO_INTERRUPT);
}
/*********************************************************************************************************
** ��������: s3csdiDrvInstall
** ��������: ��ʼ�� SD ����, �������������������
** ��    ��: NONE
** ��    ��: NONE
** ��    ��: NONE
*********************************************************************************************************/
INT s3csdiDrvInstall (VOID)
{
    static UINT  uiCdDat;
           INT   iRet;
           PVOID pvSdmHost;

    (VOID)uiCdDat;
    /*
     * ��ʼ��MCI������ݽṹ
     */
    _G_pmciXfer = sdioAlloc();
    if (!_G_pmciXfer) {
        return  (PX_ERROR);
    }

    _G_pmciXfer->SDIO_userFunc = __s3csdMciIrqHandle;

    iRet = sdioInit(_G_pmciXfer);
    if (iRet != ERROR_NONE) {
        goto __err1;
    }

    /*
     * ����SD����������
     */
    _G_sdfuncS3cSd.SDFUNC_pfuncMasterXfer = __s3csdTransfer;
    _G_sdfuncS3cSd.SDFUNC_pfuncMasterCtl  = __s3csdIoCtl;
    iRet = API_SdAdapterCreate(__S3CSDHOST_NAME, &_G_sdfuncS3cSd);
    if (iRet != ERROR_NONE) {
        goto __err2;
    }

    /*
     * ��SDMע��HOST��Ϣ
     */
    pvSdmHost = API_SdmHostRegister(&_G_s3csdHost.sdhost);
    if (!pvSdmHost) {
        goto __err3;
    }
    _G_s3csdHost.pvSdmHost      = pvSdmHost;
    _G_pmciXfer->SDIO_ulFuncArg = (ULONG)pvSdmHost;

    /*
     * �����Ѿ���������
     */
    __PIN_SET_IO();
    uiCdDat = rGPGDAT;
    if (__SDCARD_INSERT) {
        API_SdmEventNotify(pvSdmHost, SDM_EVENT_DEV_INSERT);
    }

    __s3csdHotPlugInit();

    return  (ERROR_NONE);

__err3:
    API_SdAdapterDelete(__S3CSDHOST_NAME);

__err2:
    sdioDeInit(_G_pmciXfer);

__err1:
    sdioFree(_G_pmciXfer);

    return  (PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/

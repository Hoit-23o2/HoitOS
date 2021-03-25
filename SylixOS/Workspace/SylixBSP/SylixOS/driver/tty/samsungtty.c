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
** ��   ��   ��: samsungtty.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 20 ��
**
** ��        ��: tty ����.
**
** BUG:
2009.05.31  ���ڿ��жϵ�ʱ���д���.
2009.07.21  ���� 485 ����ǿ��ƴ���.
2012.12.14  ���� FIFO ���жϴ���.
2014.05.27  startup ������ Tx FIFO ���жϼ���.
2014.07.20  �����豸��Դ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
#include "uart.h"
#include "samsungtty.h"
/*********************************************************************************************************
  ��ָ���ܽŽ��в���
*********************************************************************************************************/
#define __COM1_RS485_SEND_START()       BIT_SET(rGPGDAT, 0)
#define __COM1_RS485_SEND_END()         BIT_CLR(rGPGDAT, 0)
/*********************************************************************************************************
  �ж� FIFO ��Ч����
*********************************************************************************************************/
#define __COM_FIFO_CNT                  64
#define __COM_FIFO_CNT_MASK             0x7F
/*********************************************************************************************************
  STREAM UART CHANNEL (SIO CHANNEL)
*********************************************************************************************************/
typedef struct {
    SIO_DRV_FUNCS             *pdrvFuncs;                               /*  SIO ����������              */
    LW_PM_DEV                  pmdev;                                   /*  ��Դ����ڵ�                */
    
    INT                      (*pcbGetTxChar)();                         /*  �жϻص�����                */
    INT                      (*pcbPutRcvChar)();
    
    PVOID                      pvGetTxArg;                              /*  �ص��������                */
    PVOID                      pvPutRcvArg;
    
    INT                        iChannelMode;                            /*  ͬ�� IO ͨ��ģʽ            */
    
    UCHAR                    (*pfuncHwInByte)(INT);                     /*  ����Ӳ������һ���ֽ�        */
    VOID                     (*pfuncHwOutByte)(INT, CHAR);              /*  ����Ӳ������һ���ֽ�        */
    
    INT                        iChannelNum;                             /*  ͨ����                      */
    INT                        iBaud;                                   /*  ������                      */
    
    INT                        iHwOption;                               /*  Ӳ��ѡ��                    */
    INT                        iRs485Flag;                              /*  �Դ����Ƿ�Ҫʹ��485����     */

} __SAMSUNGSIO_CHANNEL;
typedef __SAMSUNGSIO_CHANNEL  *__PSAMSUNGSIO_CHANNEL;                   /*  ָ������                    */
/*********************************************************************************************************
  SIO ͨ�����ƿ�
*********************************************************************************************************/
static __SAMSUNGSIO_CHANNEL     __GsamsungsiochanUart0;
static __SAMSUNGSIO_CHANNEL     __GsamsungsiochanUart1;
static __SAMSUNGSIO_CHANNEL     __GsamsungsiochanUart2;
/*********************************************************************************************************
  UART ��������
*********************************************************************************************************/
static INT   __uartIoctl(SIO_CHAN  *psiochanChan, INT  iCmd, LONG lArg);/*  �˿ڿ���                    */
static INT   __uartStartup(SIO_CHAN    *psiochanChan);                  /*  ����                        */
static INT   __uartcbInstall(SIO_CHAN          *psiochanChan,           /*  ��װ�ص�                    */
                             INT                iCallbackType,
                             VX_SIO_CALLBACK    callbackRoute,
                             PVOID              pvCallbackArg);
static INT   __uartPollRxChar(SIO_CHAN    *psiochanChan,
                              PCHAR        pcInChar);                   /*  ��ѯ����                    */
static INT   __uartPollTxChar(SIO_CHAN *, CHAR);                        /*  ��ѯ����                    */
static irqreturn_t  __uartIsr(SIO_CHAN  *psiochanChan);                 /*  �����ж�                    */
static VOID  __uartHwOptionAnalyse(INT     iHwOption, 
                                   PUCHAR  pucDataBits, 
                                   PUCHAR  pucStopBits, 
                                   PUCHAR  pucParity);                  /*  ����Ӳ������                */
/*********************************************************************************************************
  SIO ��������
*********************************************************************************************************/
static SIO_DRV_FUNCS    __GsiodrvUartDrvFunc = {
             (INT (*)(SIO_CHAN *,INT, PVOID))__uartIoctl,
             __uartStartup,
             __uartcbInstall,
             __uartPollRxChar,
             (INT (*)(SIO_CHAN *, CHAR))__uartPollTxChar
};
/*********************************************************************************************************
** Function name:           __uartHwOptionAnalyse
** Descriptions:            ���� SIO ͨ��Ӳ������
** input parameters:        iHwOption                   Ӳ������
** output parameters:       pucDataBits,                ����λ��
                            pucStopBits,                ����λ
                            pucParity                   У��λ
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static VOID   __uartHwOptionAnalyse (INT     iHwOption, 
                                     PUCHAR  pucDataBits, 
                                     PUCHAR  pucStopBits, 
                                     PUCHAR  pucParity)
{
    if ((iHwOption & CS8) == CS8) {                                     /*  ȷ������λ��                */
        *pucDataBits = 8;
    } else if (iHwOption & CS7) {
        *pucDataBits = 7;
    } else if (iHwOption & CS6) {
        *pucDataBits = 6;
    } else {
        *pucDataBits = 5;
    }
    
    if (iHwOption & STOPB) {                                            /*  ȷ������λ                  */
        *pucStopBits = TWO_STOPBIT;
    } else {
        *pucStopBits = ONE_STOPBIT;
    }
    
    if (iHwOption & PARENB) {                                           /*  ȷ��У��λ                  */
        if (iHwOption & PARODD) {
            *pucParity = CHK_ODD;
        } else {
            *pucParity = CHK_EVEN;
        }
    } else {
        *pucParity = CHK_NONE;
    }
}
/*********************************************************************************************************
** Function name:           sioChanCreate
** Descriptions:            ����һ�� sio ͨ��
** input parameters:        iChannelNum     Ӳ��ͨ����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/12/20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
SIO_CHAN    *sioChanCreate (INT   iChannelNum)
{
    static PLW_PM_ADAPTER    pmadapter = LW_NULL;
    __PSAMSUNGSIO_CHANNEL    psamsungsiochanUart;
    SIO_CHAN                *psiochan;
    
    UCHAR                    ucDataBits;
    UCHAR                    ucStopBits;
    UCHAR                    ucParity;
    
    if (pmadapter == LW_NULL) {
        pmadapter =  pmAdapterFind("inner_pm");
        if (pmadapter == LW_NULL) {
            printk(KERN_ERR "can not find power manager.\n");
        }
    }

    switch (iChannelNum) {
    
    case COM0:
        pmDevInit(&__GsamsungsiochanUart0.pmdev, pmadapter, 10, LW_NULL);
        __GsamsungsiochanUart0.pmdev.PMD_pcName = "uart0";
        psamsungsiochanUart              = &__GsamsungsiochanUart0;
        psamsungsiochanUart->pdrvFuncs   = &__GsiodrvUartDrvFunc;       /*  SIO FUNC                    */
        psamsungsiochanUart->iChannelNum = COM0;
        
        API_InterVectorConnect(VIC_CHANNEL_UART0, 
                               (PINT_SVR_ROUTINE)__uartIsr, 
                               (PVOID)&__GsamsungsiochanUart0,
                               "uart0_isr");                            /*  ��װ����ϵͳ�ж�������      */
        break;
    
    case COM1:
        pmDevInit(&__GsamsungsiochanUart1.pmdev, pmadapter, 11, LW_NULL);
        __GsamsungsiochanUart1.pmdev.PMD_pcName = "uart1";
        psamsungsiochanUart              = &__GsamsungsiochanUart1;
        psamsungsiochanUart->pdrvFuncs   = &__GsiodrvUartDrvFunc;       /*  SIO FUNC                    */
        psamsungsiochanUart->iChannelNum = COM1;
        
        API_InterVectorConnect(VIC_CHANNEL_UART1, 
                               (PINT_SVR_ROUTINE)__uartIsr, 
                               (PVOID)&__GsamsungsiochanUart1,
                               "uart1_isr");                            /*  ��װ����ϵͳ�ж�������      */
        break;
    
    case COM2:
        pmDevInit(&__GsamsungsiochanUart2.pmdev, pmadapter, 12, LW_NULL);
        __GsamsungsiochanUart2.pmdev.PMD_pcName = "uart2";
        psamsungsiochanUart              = &__GsamsungsiochanUart2;
        psamsungsiochanUart->pdrvFuncs   = &__GsiodrvUartDrvFunc;       /*  SIO FUNC                    */
        psamsungsiochanUart->iChannelNum = COM2;
        
        API_InterVectorConnect(VIC_CHANNEL_UART2, 
                               (PINT_SVR_ROUTINE)__uartIsr, 
                               (PVOID)&__GsamsungsiochanUart2,
                               "uart2_isr");                            /*  ��װ����ϵͳ�ж�������      */
        break;
    
    default:
        return  (LW_NULL);                                              /*  ͨ���Ŵ���                  */
    }
    
    psiochan = (SIO_CHAN *)psamsungsiochanUart;
    
    psamsungsiochanUart->iChannelMode = SIO_MODE_INT;                   /*  ʹ���ж�ģʽ                */
    psamsungsiochanUart->iBaud        = UART_DEFAULT_BAUD;              /*  ��ʼ��������                */
    psamsungsiochanUart->iHwOption    = UART_DEFAULT_OPT;               /*  Ӳ��״̬                    */
    psamsungsiochanUart->iRs485Flag   = RS485_DIS;                      /*  Ĭ�ϲ�ʹ��485����           */
    
    __uartHwOptionAnalyse(UART_DEFAULT_OPT,
                          &ucDataBits,
                          &ucStopBits,
                          &ucParity);                                   /*  ��þ������                */
                          
    uartInit(iChannelNum, UNUSE_INF, ucDataBits, ucStopBits, 
             ucParity, UART_DEFAULT_BAUD, LW_NULL);
             
    return  (psiochan);
}
/*********************************************************************************************************
** Function name:           __uartcbInstall
** Descriptions:            SIO ͨ����װ�ص�����
** input parameters:        psiochanChan                 ͨ��
**                          iCallbackType                �ص�����
**                          callbackRoute                �ص�����
**                          pvCallbackArg                �ص�����
** output parameters:       NONE
** Returned value:          �����
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT   __uartcbInstall(SIO_CHAN          *psiochanChan,
                             INT                iCallbackType,
                             VX_SIO_CALLBACK    callbackRoute,
                             PVOID              pvCallbackArg)
{
    __PSAMSUNGSIO_CHANNEL     psamsungsiochanUart = (__PSAMSUNGSIO_CHANNEL)psiochanChan;
    
    switch (iCallbackType) {
    
    case SIO_CALLBACK_GET_TX_CHAR:                                      /*  ���ͻص纯��                */
        psamsungsiochanUart->pcbGetTxChar = callbackRoute;
        psamsungsiochanUart->pvGetTxArg   = pvCallbackArg;
        return    (ERROR_NONE);
        
    case SIO_CALLBACK_PUT_RCV_CHAR:                                     /*  ���ջص纯��                */
        psamsungsiochanUart->pcbPutRcvChar = callbackRoute;
        psamsungsiochanUart->pvPutRcvArg   = pvCallbackArg;
        return    (ERROR_NONE);
        
    default:
        _ErrorHandle(ENOSYS);
        return    (PX_ERROR);
    }
}
/*********************************************************************************************************
** Function name:           __uart485Delay
** Descriptions:            uart1 485 ģʽ���ӳ�, ���� 485 �շ�������Ƶ������, �����ڷ���ǰ��, �������
                            ����ӳ�, �˳����� gcc -O3 -Os ����ͨ��
** input parameters:        iTimes          �ӳ�ѭ������ (һ��Ϊ 40)
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static VOID  __uart485Delay (INT  iTimes)
{
    volatile int    i;
    
    for (; iTimes > 0; iTimes--) {
        for (i = 0; i < 15; i++);
    }
}
/*********************************************************************************************************
** Function name:           __uartStartup
** Descriptions:            SIO ͨ������(û��ʹ���ж�)
** input parameters:        psiochanChan                 ͨ��
** output parameters:       NONE
** Returned value:          SIO ͨ�����ƿ�ָ��
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT   __uartStartup (SIO_CHAN    *psiochanChan)
{
    __PSAMSUNGSIO_CHANNEL     psamsungsiochanUart = (__PSAMSUNGSIO_CHANNEL)psiochanChan;
    CHAR                      cTx;
    
    switch (psamsungsiochanUart->iChannelNum) {
    
    case COM0:
        INTER_CLR_SUBMSK(BIT_SUB_TXD0);                                 /*  ʹ�� FIFO ���ж�            */
        break;
    
    case COM1:
        if (psamsungsiochanUart->iRs485Flag == RS485_EN) {              /*  485 ģʽʹ�ò�ѯ���ͷ�ʽ    */
            __COM1_RS485_SEND_START();                                  /*  485 ����(����ģʽ)          */
            __uart485Delay(40);                                         /*  ���ӳ�                      */
            do {
                if (psamsungsiochanUart->pcbGetTxChar(psamsungsiochanUart->pvGetTxArg, &cTx)
                    != ERROR_NONE) {
                    break;                                              /*  �������                    */
                }
                while (rUFSTAT1 & (1 << 14));                           /*  FIFO δ��                   */
                WrUTXH1(cTx);                                           /*  ��������                    */
            } while (1);
            while ((rUTRSTAT1 & (1 << 2)) == 0);                        /*  �ȴ����ͻ�����Ϊ��          */
            __COM1_RS485_SEND_END();                                    /*  485 ����(����ģʽ)          */
            __uart485Delay(40);                                         /*  ���ӳ�                      */
        
        } else {                                                        /*  ȫ˫��ģʽ�����жϷ���      */
            INTER_CLR_SUBMSK(BIT_SUB_TXD1);                             /*  ʹ�� FIFO ���ж�            */
        }
        break;
        
    case COM2:
        INTER_CLR_SUBMSK(BIT_SUB_TXD2);                                 /*  ʹ�� FIFO ���ж�            */
        break;
    }
    
    return    (ERROR_NONE);
}
/*********************************************************************************************************
** Function name:           __uartPollRxChar
** Descriptions:            SIO ͨ����ѯ����
** input parameters:        psiochanChan                 ͨ��
** output parameters:       pcInChar                     ���յ��ֽ�
** Returned value:          ���յĸ���
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT   __uartPollRxChar (SIO_CHAN    *psiochanChan, PCHAR  pcInChar)
{
    __PSAMSUNGSIO_CHANNEL     psamsungsiochanUart = (__PSAMSUNGSIO_CHANNEL)psiochanChan;
    
    switch (psamsungsiochanUart->iChannelNum) {
    
    case COM0:
        if (rUFSTAT0 & __COM_FIFO_CNT_MASK) {
            *pcInChar = RdURXH0();
        } else {
            _ErrorHandle(EAGAIN);
            return  (PX_ERROR);
        }
        break;
        
    case COM1:
        if (rUFSTAT1 & __COM_FIFO_CNT_MASK) {
            *pcInChar = RdURXH1();
        } else {
            _ErrorHandle(EAGAIN);
            return  (PX_ERROR);
        }
        break;
        
    case COM2:
        if (rUFSTAT2 & __COM_FIFO_CNT_MASK) {
            *pcInChar = RdURXH2();
        } else {
            _ErrorHandle(EAGAIN);
            return  (PX_ERROR);
        }
        break;
    }
    
    return    (ERROR_NONE);
}
/*********************************************************************************************************
** Function name:           __uartPollTxChar
** Descriptions:            SIO ͨ����ѯ����
** input parameters:        psiochanChan                 ͨ��
**                          cOutChar                     ���͵��ֽ�
** output parameters:       NONE
** Returned value:          ���͵ĸ���
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT   __uartPollTxChar (SIO_CHAN   *psiochanChan, CHAR  cOutChar)
{
    __PSAMSUNGSIO_CHANNEL     psamsungsiochanUart = (__PSAMSUNGSIO_CHANNEL)psiochanChan;
    
    switch (psamsungsiochanUart->iChannelNum) {
    
    case COM0:
        while (rUFSTAT0 & (1 << 14));                                   /*  FIFO δ��                   */
        WrUTXH0(cOutChar);                                              /*  ��������                    */
        break;
        
    case COM1:
        while (rUFSTAT1 & (1 << 14));                                   /*  FIFO δ��                   */
        WrUTXH1(cOutChar);                                              /*  ��������                    */
        break;
        
    case COM2:
        while (rUFSTAT2 & (1 << 14));                                   /*  FIFO δ��                   */
        WrUTXH2(cOutChar);                                              /*  ��������                    */
        break;
    }
    
    return    (ERROR_NONE);
}
/*********************************************************************************************************
** Function name:           __uartIsr
** Descriptions:            SIO ͨ���жϴ�����
** input parameters:        psiochanChan                 ͨ��
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static irqreturn_t  __uartIsr (SIO_CHAN  *psiochanChan)
{
    __PSAMSUNGSIO_CHANNEL     psamsungsiochanUart = (__PSAMSUNGSIO_CHANNEL)psiochanChan;
    UCHAR                     ucData;
    
    switch (psamsungsiochanUart->iChannelNum) {
    
    case COM0:
        if (rSUBSRCPND & BIT_SUB_TXD0) {
            INTER_CLR_SUBSRCPND(BIT_SUB_TXD0);                          /*  ��������ж�                */
        }
        if (rSUBSRCPND & BIT_SUB_RXD0) {
            INTER_CLR_SUBSRCPND(BIT_SUB_RXD0);                          /*  ��������ж�                */
        }
        INTER_CLR_PNDING(BIT_UART0);                                    /*  ����жϱ�־                */
        while (((rUFSTAT0 >> 8) & __COM_FIFO_CNT_MASK) < __COM_FIFO_CNT) {
                                                                        /*  TxFIFO δ��                 */
            if (psamsungsiochanUart->pcbGetTxChar(psamsungsiochanUart->pvGetTxArg, 
                (PCHAR)&ucData) != ERROR_NONE) {                        /*  ���ͽ���                    */
                INTER_SET_SUBMSK(BIT_SUB_TXD0);                         /*  ���ͽ���                    */
                break;
            } else {
                WrUTXH0(ucData);                                        /*  ��������                    */
            }
        }
        while (rUFSTAT0 & __COM_FIFO_CNT_MASK)  {                       /*  ��Ҫ��������                */
            ucData = RdURXH0();
            psamsungsiochanUart->pcbPutRcvChar(psamsungsiochanUart->pvPutRcvArg, ucData);
        }
        break;
        
    case COM1:
        if (rSUBSRCPND & BIT_SUB_TXD1) {
            INTER_CLR_SUBSRCPND(BIT_SUB_TXD1);                          /*  ��������ж�                */
        }
        if (rSUBSRCPND & BIT_SUB_RXD1) {
            INTER_CLR_SUBSRCPND(BIT_SUB_RXD1);                          /*  ��������ж�                */
        }
        INTER_CLR_PNDING(BIT_UART1);                                    /*  ����жϱ�־                */
        while (((rUFSTAT1 >> 8) & __COM_FIFO_CNT_MASK) < __COM_FIFO_CNT) {
                                                                        /*  TxFIFO δ��                 */
            if (psamsungsiochanUart->pcbGetTxChar(psamsungsiochanUart->pvGetTxArg, 
                (PCHAR)&ucData) != ERROR_NONE) {                        /*  ���ͽ���                    */
                INTER_SET_SUBMSK(BIT_SUB_TXD1);                         /*  ���ͽ���                    */
                break;
            } else {
                WrUTXH1(ucData);                                        /*  ��������                    */
            }
        }
        while (rUFSTAT1 & __COM_FIFO_CNT_MASK)  {                       /*  ��Ҫ��������                */
            ucData = RdURXH1();
            psamsungsiochanUart->pcbPutRcvChar(psamsungsiochanUart->pvPutRcvArg, ucData);
        }
        break;
        
    case COM2:
        if (rSUBSRCPND & BIT_SUB_TXD2) {
            INTER_CLR_SUBSRCPND(BIT_SUB_TXD2);                          /*  ��������ж�                */
        }
        if (rSUBSRCPND & BIT_SUB_RXD2) {
            INTER_CLR_SUBSRCPND(BIT_SUB_RXD2);                          /*  ��������ж�                */
        }
        INTER_CLR_PNDING(BIT_UART2);                                    /*  ����жϱ�־                */
        while (((rUFSTAT2 >> 8) & __COM_FIFO_CNT_MASK) < __COM_FIFO_CNT) {
                                                                        /*  TxFIFO δ��                 */
            if (psamsungsiochanUart->pcbGetTxChar(psamsungsiochanUart->pvGetTxArg, 
                (PCHAR)&ucData) != ERROR_NONE) {                        /*  ���ͽ���                    */
                INTER_SET_SUBMSK(BIT_SUB_TXD2);                         /*  ���ͽ���                    */
                break;
            } else {
                WrUTXH2(ucData);                                        /*  ��������                    */
            }
        }
        while (rUFSTAT2 & __COM_FIFO_CNT_MASK) {                        /*  ��Ҫ��������                */
            ucData = RdURXH2();
            psamsungsiochanUart->pcbPutRcvChar(psamsungsiochanUart->pvPutRcvArg, ucData);
        }
        break;
    }
    
    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** Function name:           __uartIoctl
** Descriptions:            SIO ͨ������
** input parameters:        psiochanChan                 ͨ��
**                          iCmd                         ����
**                          lArg                         ����
** output parameters:       NONE
** Returned value:          ����ִ�н��
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT   __uartIoctl (SIO_CHAN  *psiochanChan, INT  iCmd, LONG  lArg)
{
    __PSAMSUNGSIO_CHANNEL   psamsungsiochanUart = (__PSAMSUNGSIO_CHANNEL)psiochanChan;
    
    UCHAR                   ucDataBits;
    UCHAR                   ucStopBits;
    UCHAR                   ucParity;
    
    switch (iCmd) {
    
    case SIO_BAUD_SET:                                                  /*  ����ģ����                  */
        __uartHwOptionAnalyse(psamsungsiochanUart->iHwOption,
                              &ucDataBits,
                              &ucStopBits,
                              &ucParity);                               /*  ��þ������                */
        uartInit(psamsungsiochanUart->iChannelNum, UNUSE_INF, 
                 ucDataBits, ucStopBits, 
                 ucParity, (INT)lArg, LW_NULL);                         /*  ��ʼ������                  */
        psamsungsiochanUart->iBaud = (INT)lArg;
        break;
        
    case SIO_BAUD_GET:                                                  /*  ��ò�����                  */
        *((LONG *)lArg) = psamsungsiochanUart->iBaud;
        break;
    
    case SIO_HW_OPTS_SET:                                               /*  ����Ӳ������                */
        __uartHwOptionAnalyse((INT)lArg,
                              &ucDataBits,
                              &ucStopBits,
                              &ucParity);                               /*  ��þ������                */
        uartInit(psamsungsiochanUart->iChannelNum, UNUSE_INF, 
                 ucDataBits, ucStopBits, 
                 ucParity, psamsungsiochanUart->iBaud, LW_NULL);        /*  ��ʼ������                  */
        psamsungsiochanUart->iHwOption = (INT)lArg;                     /*  ��¼��Ϣ                    */
        break;
        
    case SIO_HW_OPTS_GET:                                               /*  ��ȡӲ������                */
        *((INT *)lArg) = psamsungsiochanUart->iHwOption;
        break;
    
    case SIO_OPEN:                                                      
        switch (psamsungsiochanUart->iChannelNum) {
        
        case COM0:
            pmDevOn(&psamsungsiochanUart->pmdev);
            INTER_CLR_PNDING((1u << VIC_CHANNEL_UART0));
            INTER_CLR_SUBMSK(BIT_SUB_RXD0);                             /*  �򿪽����ж�                */
            API_InterVectorEnable(VIC_CHANNEL_UART0);                   /*  ʹ�ܴ����ж�                */
            break;
            
        case COM1:
            pmDevOn(&psamsungsiochanUart->pmdev);
            INTER_CLR_PNDING((1u << VIC_CHANNEL_UART1));
            INTER_CLR_SUBMSK(BIT_SUB_RXD1);                             /*  �򿪽����ж�                */
            API_InterVectorEnable(VIC_CHANNEL_UART1);                   /*  ʹ�ܴ����ж�                */
            break;
            
        case COM2:
            pmDevOn(&psamsungsiochanUart->pmdev);
            INTER_CLR_PNDING((1u << VIC_CHANNEL_UART2));
            INTER_CLR_SUBMSK(BIT_SUB_RXD2);                             /*  �򿪽����ж�                */
            API_InterVectorEnable(VIC_CHANNEL_UART2);                   /*  ʹ�ܴ����ж�                */
            break;
        }
        break;
        
    case SIO_HUP:                                                       /*  �رմ���                    */
        switch (psamsungsiochanUart->iChannelNum) {
        
        case COM0:
            pmDevOff(&psamsungsiochanUart->pmdev);
            API_InterVectorDisable(VIC_CHANNEL_UART0);
            INTER_CLR_PNDING((1u << VIC_CHANNEL_UART0));
            break;
            
        case COM1:
            pmDevOff(&psamsungsiochanUart->pmdev);
            API_InterVectorDisable(VIC_CHANNEL_UART1);
            INTER_CLR_PNDING((1u << VIC_CHANNEL_UART1));
            break;
            
        case COM2:
            pmDevOff(&psamsungsiochanUart->pmdev);
            API_InterVectorDisable(VIC_CHANNEL_UART2);
            INTER_CLR_PNDING((1u << VIC_CHANNEL_UART2));
            break;
        }
        break;
        
    case RS485_SET:                                                     /*  ���� 485 ״̬               */
        if (psamsungsiochanUart->iChannelNum == COM1) {
            psamsungsiochanUart->iRs485Flag  =  (INT)lArg;
            if (lArg) {
                rGPGCON = (rGPGCON & ~(0x3)) | (0x1);                   /*  GPG0 ���                   */
                BIT_CLR(rGPGUP, 0);                                     /*  ʹ������                    */
                __COM1_RS485_SEND_END();                                /*  ��������״̬                */
            }
        }
        break;
        
    case RS485_GET:                                                     /*  ��ȡ 485 ״̬               */
        *((LONG *)lArg) = psamsungsiochanUart->iRs485Flag;
        break;
        
    default:
        _ErrorHandle(ENOSYS);
        return    (ENOSYS);
    }
    
    return    (ERROR_NONE);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

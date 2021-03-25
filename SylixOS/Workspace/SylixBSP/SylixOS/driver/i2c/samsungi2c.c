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
** ��   ��   ��: samsungi2c.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 19 ��
**
** ��        ��: S3C2440 I2C ����(������������)
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"                                                     /*  �������� & ���������       */
#include "SylixOS.h"
/*********************************************************************************************************
  S3C2440A I2C �����ʲ��� (ԼΪ100K)
*********************************************************************************************************/
#define __I2C_BPS_PARAM             (((PCLK / 512) / 100000) & 0xF)
/*********************************************************************************************************
  I2C CON �Ĵ�������ȡֵ����
*********************************************************************************************************/
#define __I2C_CON_DACK(iBpsParam)   ((1 << 7) | (1 << 6) | (1 << 5) | (iBpsParam))
#define __I2C_CON_DNACK(iBpsParam)  ((0 << 7) | (1 << 6) | (1 << 5) | (iBpsParam))
/*********************************************************************************************************
  I2C ����������ṹ
*********************************************************************************************************/
typedef struct {
    int                 iStatus;                                        /*  ״̬                        */
    int                 iBpsParam;                                      /*  �����ʲ���                  */
    PLW_I2C_MESSAGE     pi2cmsg;                                        /*  ��Ҫ�������Ϣ              */
    int                 iMsgPtr;                                        /*  ��Ϣ�ڲ�ָ��                */
    int                 iMsgNum;                                        /*  ��Ϣ����                    */
    int                 iMsgIndex;                                      /*  ��ǰ����� msg �±�         */
} __SAMSUNGI2C_CHANNEL;
typedef __SAMSUNGI2C_CHANNEL        *__PSAMSUNGI2C_CHANNEL;
/*********************************************************************************************************
  I2C ��Ϣ�����ж�
*********************************************************************************************************/
#define __I2C_BUS_IS_LASTMSG(psamsungi2c)   (psamsungi2c->iMsgIndex >= (psamsungi2c->iMsgNum - 1))
                                                                        /*  ���һ����Ϣ��û����Ϣ      */
#define __I2C_BUS_IS_MSGLAST(psamsungi2c)   (psamsungi2c->iMsgPtr ==    \
                                             (psamsungi2c->pi2cmsg->I2CMSG_usLen - 1))
                                                                        /*  ��Ϣ�����һ���ֽ�          */
#define __I2C_BUS_IS_MSGEND(psamsungi2c)    (psamsungi2c->iMsgPtr >= psamsungi2c->pi2cmsg->I2CMSG_usLen)
                                                                        /*  ��Ϣ����                    */
/*********************************************************************************************************
  I2C ����״̬
*********************************************************************************************************/
#define __I2C_BUS_STATE_IDLE        0                                   /*  ���߿���                    */
#define __I2C_BUS_STATE_START       1                                   /*  ��������                    */
#define __I2C_BUS_STATE_READ        2                                   /*  ������                      */
#define __I2C_BUS_STATE_WRITE       3                                   /*  д����                      */
#define __I2C_BUS_STATE_STOP        4                                   /*  ���߽���                    */
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_OBJECT_HANDLE             __GhI2cSignal;                      /*  i2c �ж�                    */
#define __I2C_BUS_SIGNAL()          API_SemaphoreBPost(__GhI2cSignal)
#define __I2C_BUS_WAIT(ulTimeout)   API_SemaphoreBPend(__GhI2cSignal, ulTimeout)
static LW_I2C_FUNCS                 __Gi2cfuncSamsung;                  /*  i2c ���ߺ���                */
static __SAMSUNGI2C_CHANNEL         __Gsamsungi2c;                      /*  i2c ������                  */
/*********************************************************************************************************
  ��������
*********************************************************************************************************/
static VOID  __samsungI2cStart(UINT16  usAddr, UINT16  usFlag);
static VOID  __samsungI2cStop(UINT16  usFlag);
/*********************************************************************************************************
** Function name:           __samsungI2cIsr
** Descriptions:            i2c �������жϴ�����
** input parameters:        psamsungi2c     ������
** output parameters:       NONE
** Returned value:          �жϷ��񷵻�ֵ
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static irqreturn_t  __samsungI2cIsr (__PSAMSUNGI2C_CHANNEL   psamsungi2c)
{
    BYTE        ucByte;
    ULONG       ulStatus = rIICSTAT;                                    /*  i2c ״̬                    */

    INTER_CLR_PNDING(BIT_IIC);                                          /*  ����жϱ�־                */

    switch (psamsungi2c->iStatus) {                                     /*  ����ͬ״̬                */
    
    case __I2C_BUS_STATE_IDLE:
        break;                                                          /*  ֱ���˳�                    */
    
    case __I2C_BUS_STATE_START:
        if ((ulStatus & 0x09) ||                                        /*  ���ִ���                    */
            (__I2C_BUS_IS_LASTMSG(psamsungi2c) &&                       /*  û�д��������Ϣ            */
             (psamsungi2c->pi2cmsg->I2CMSG_usLen == 0))) {
            errno = ENXIO;
            __samsungI2cStop(0);                                        /*  ֹͣ����                    */
            break;
        }

        if (psamsungi2c->pi2cmsg->I2CMSG_usFlag & LW_I2C_M_RD) {        /*  �����״̬                  */
            psamsungi2c->iStatus = __I2C_BUS_STATE_READ;
            goto    __prepare_read;
        } else {
            psamsungi2c->iStatus = __I2C_BUS_STATE_WRITE;               /*  ����д״̬                  */
            goto    __prepare_write;
        }
        break;
    
    case __I2C_BUS_STATE_READ:
        ucByte = (BYTE)rIICDS;                                          /*  ��ȡ����                    */
        psamsungi2c->pi2cmsg->I2CMSG_pucBuffer[psamsungi2c->iMsgPtr] = ucByte;
        psamsungi2c->iMsgPtr++;
        
__prepare_read:
        if (__I2C_BUS_IS_MSGLAST(psamsungi2c)) {                        /*  ���ǵ�ǰ��Ϣ�����һ���ֽ�  */
            if (__I2C_BUS_IS_LASTMSG(psamsungi2c)) {                    /*  �������һ����Ϣ            */
                rIICCON = __I2C_CON_DNACK(psamsungi2c->iBpsParam);      /*  �������߲���Ҫ ACK          */
            }
        } else if (__I2C_BUS_IS_MSGEND(psamsungi2c)) {                  /*  ��ǰ��Ϣ�Ѿ�����            */
            if (__I2C_BUS_IS_LASTMSG(psamsungi2c)) {                    /*  �������һ����Ϣ            */
                psamsungi2c->iMsgPtr = 0;
                psamsungi2c->iMsgIndex++;                               /*  ��֤����Ϣ������ͬ          */
                __samsungI2cStop(LW_I2C_M_RD);
            } else {
                psamsungi2c->iMsgPtr = 0;
                psamsungi2c->iMsgIndex++;
                psamsungi2c->pi2cmsg++;                                 /*  ��ʼ������һ����Ϣ          */
                rIICCON = __I2C_CON_DACK(psamsungi2c->iBpsParam);       /*  ������������ ACK            */
            }
        } else {
            rIICCON = __I2C_CON_DACK(psamsungi2c->iBpsParam);           /*  ������������ ACK            */
        }
        break;
    
    case __I2C_BUS_STATE_WRITE:
        if ((ulStatus & 0x01) &&                                        /*  ��Ҫ ACK ��û�н��յ� ACK   */
            !(psamsungi2c->pi2cmsg->I2CMSG_usFlag & 
              LW_I2C_M_IGNORE_NAK)) {
            errno = ECONNREFUSED;
            __samsungI2cStop(0);                                        /*  ֹͣ����                    */
            break;
        }
        
__prepare_write:
        if (!__I2C_BUS_IS_MSGEND(psamsungi2c)) {                        /*  ��ǰ��Ϣ��û�з�����        */
            ucByte = psamsungi2c->pi2cmsg->I2CMSG_pucBuffer[psamsungi2c->iMsgPtr];
            psamsungi2c->iMsgPtr++;
            rIICDS  = ucByte;
            rIICCON = __I2C_CON_DACK(psamsungi2c->iBpsParam);           /*  ����                        */
        
        } else if (!__I2C_BUS_IS_LASTMSG(psamsungi2c)) {                /*  ����ʣ�����Ϣû�з�����    */
            psamsungi2c->iMsgPtr = 0;
            psamsungi2c->iMsgIndex++;
            psamsungi2c->pi2cmsg++;                                     /*  ��ʼ������һ����Ϣ          */
            
            if (psamsungi2c->pi2cmsg->I2CMSG_usFlag & 
                LW_I2C_M_NOSTART) {                                     /*  ����Ҫ��ʼλ                */
                if (psamsungi2c->pi2cmsg->I2CMSG_usFlag &
                    LW_I2C_M_RD) {                                      /*  ������                      */
                    /*
                     *  ����������, ������������
                     */
                    __samsungI2cStop(0);                                /*  ���ܽ��ж�����              */
                }
                goto    __prepare_write;
                
            } else {
                __samsungI2cStart(psamsungi2c->pi2cmsg->I2CMSG_usAddr, 
                                  psamsungi2c->pi2cmsg->I2CMSG_usFlag); /*  ��������                    */
            }
        } else {
            psamsungi2c->iMsgPtr = 0;
            psamsungi2c->iMsgIndex++;                                   /*  ��֤����Ϣ������ͬ          */
            __samsungI2cStop(0);                                        /*  ���ͽ���                    */
        }
        break;
    
    case __I2C_BUS_STATE_STOP:
        API_InterVectorDisable(VIC_CHANNEL_IIC);                        /*  �ر������ж�                */
        break;
    }
    
    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** Function name:           __samsungI2cInit
** Descriptions:            i2c ��������ʼ��
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static VOID  __samsungI2cInit (VOID)
{
    rGPECON &= ~(0xFu << 28);
    rGPECON |=  (0xAu << 28);                                           /*  ���ùܽ�����                */

    rGPEUP   = rGPEUP | 0xC000;                                         /*  ��ֹ�ڲ���������            */

    rIICCON  = ((1 << 7) | (1 << 6) | (1 << 5) | (0));                  /*  ��ʼ��                      */
    rIICADD  = 0x10;                                                    /*  ���شӻ���ַ                */
    rIICSTAT =  (3 << 6) | (1 << 4);                                    /*  ʹ�� I2C ����               */
    rIICLC   =  (1 << 2) | (1);                                         /*  Filter enable 15 clocks     */
    
    __GhI2cSignal = API_SemaphoreBCreate("i2c_signal", LW_FALSE, LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    
    API_InterVectorConnect(VIC_CHANNEL_IIC, (PINT_SVR_ROUTINE)__samsungI2cIsr, 
                           (PVOID)&__Gsamsungi2c, "i2c_isr");
}
/*********************************************************************************************************
** Function name:           __samsungI2cStart
** Descriptions:            i2c ���������������ֽ�
** input parameters:        usAddr      ��ַ
**                          usFlag      ��־
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static VOID  __samsungI2cStart (UINT16  usAddr, UINT16  usFlag)
{
    __Gsamsungi2c.iStatus = __I2C_BUS_STATE_START;                      /*  ��������                    */

    API_InterVectorEnable(VIC_CHANNEL_IIC);                             /*  �������ж�                */

    if (usFlag & LW_I2C_M_RD) {                                         /*  ������                      */
        rIICDS    = (BYTE)(usAddr + 1);                                 /*  ������������ַ              */
        rIICSTAT  = (2 << 6) | (1 << 5) | (1 << 4);                     /*  ����ģʽ����                */
        rIICCON   = __I2C_CON_DACK(__Gsamsungi2c.iBpsParam);            /*  �������� ��Ҫ�д˲���       */
    } else {
        rIICDS    = (BYTE)usAddr;
        rIICSTAT  = (3 << 6) | (1 << 5) | (1 << 4);                     /*  ����ģʽ����                */
    }
}
/*********************************************************************************************************
** Function name:           __samsungI2cStop
** Descriptions:            i2c ����������ֹͣ����
** input parameters:        usFlag      ��־
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static VOID  __samsungI2cStop (UINT16  usFlag)
{
    if (usFlag & LW_I2C_M_RD) {                                         /*  ������                      */
        rIICSTAT  = (2 << 6) | (0 << 5) | (1 << 4);                     /*  ����ģʽֹͣ                */
    } else {                                                            /*  д����                      */
        rIICSTAT  = (3 << 6) | (0 << 5) | (1 << 4);                     /*  ����ģʽֹͣ                */
    }

    rIICCON = __I2C_CON_DACK(__I2C_BPS_PARAM);                          /*  ���� ACK                    */
    
    __Gsamsungi2c.iStatus = __I2C_BUS_STATE_STOP;                       /*  ֹͣ����                    */
    
    __I2C_BUS_SIGNAL();                                                 /*  ����ȴ�����                */
}
/*********************************************************************************************************
** Function name:           __samsungI2cTransferOne
** Descriptions:            i2c ���亯��
** input parameters:        pi2cadapter     i2c ������
**                          pi2cmsg         i2c ������Ϣ
**                          iNum            ��Ҫ����� msg ����
** output parameters:       NONE
** Returned value:          ��ɴ������Ϣ����
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT  __samsungI2cDoTransfer (PLW_I2C_ADAPTER   pi2cadapter,
                                    PLW_I2C_MESSAGE   pi2cmsg,
                                    INT               iNum)
{
    __Gsamsungi2c.iStatus    = __I2C_BUS_STATE_START;
    __Gsamsungi2c.iBpsParam  = __I2C_BPS_PARAM;
    __Gsamsungi2c.pi2cmsg    = pi2cmsg;
    __Gsamsungi2c.iMsgPtr    = 0;
    __Gsamsungi2c.iMsgNum    = iNum;
    __Gsamsungi2c.iMsgIndex  = 0;                                       /*  �ӵ�һ����ʼ����            */

    __samsungI2cStart(pi2cmsg->I2CMSG_usAddr, pi2cmsg->I2CMSG_usFlag);  /*  ��������                    */
    
    __I2C_BUS_WAIT(LW_OPTION_WAIT_A_SECOND * 3);                        /*  ���ȴ� 3 ����             */
    
    /*
     *  �˺����˳���������� i2c �ж�, ���ܵ��� msg[] �ֲ�������Ч. �������.
     */
    API_InterVectorDisable(VIC_CHANNEL_IIC);                            /*  �ر������ж�                */
    
    return  (__Gsamsungi2c.iMsgIndex);                                  /*  ���ش���ɹ�������          */
}
/*********************************************************************************************************
** Function name:           __samsungI2cTransfer
** Descriptions:            i2c ���亯��
** input parameters:        pi2cadapter     i2c ������
**                          pi2cmsg         i2c ������Ϣ��
**                          iNum            ��Ϣ����
** output parameters:       NONE
** Returned value:          ��ɴ������Ϣ����
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
static INT  __samsungI2cTransfer (PLW_I2C_ADAPTER   pi2cadapter,
                                  PLW_I2C_MESSAGE   pi2cmsg,
                                  INT               iNum)
{
    REGISTER INT        i;
    
    for (i = 0; i < pi2cadapter->I2CADAPTER_iRetry; i++) {
        if (__samsungI2cDoTransfer(pi2cadapter, pi2cmsg, iNum) == iNum) {
            return  (iNum);
        } else {
            API_TimeSleep(LW_OPTION_WAIT_A_TICK);                       /*  �ȴ�һ��������������        */
        }
    }
    
    return  (PX_ERROR);
}
/*********************************************************************************************************
** Function name:           i2cBusFuns
** Descriptions:            ��ʼ�� i2c ���߲���ȡ����������
** input parameters:        NONE
** output parameters:       NONE
** Returned value:          ���߲���������
** Created by:              Hanhui
** Created Date:            2009-10-20
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
PLW_I2C_FUNCS  i2cBusFuns (VOID)
{
    __Gi2cfuncSamsung.I2CFUNC_pfuncMasterXfer = __samsungI2cTransfer;
    
    __samsungI2cInit();
    
    return  (&__Gi2cfuncSamsung);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

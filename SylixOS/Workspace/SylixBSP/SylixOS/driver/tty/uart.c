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
** ��   ��   ��: uart.c
**
** ��   ��   ��: Han.Hui (����)
**
** �ļ���������: 2007 �� 09 �� 20 ��
**
** ��        ��: uart ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "uart.h"
#include "SylixOS.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __COM0_CLKBIT       (1 << 10)                                   /*  COM0 �� CLKCON �е�λ��     */
#define __COM1_CLKBIT       (1 << 11)                                   /*  COM1 �� CLKCON �е�λ��     */
#define __COM2_CLKBIT       (1 << 12)                                   /*  COM2 �� CLKCON �е�λ��     */

#define __COM0_GPIO         ((1 << 2) | (1 << 3))                       /*  COM0 �� IO �����е�λ��     */
#define __COM1_GPIO         ((1 << 4) | (1 << 5))                       /*  COM1 �� IO �����е�λ��     */
#define __COM2_GPIO         ((1 << 6) | (1 << 7))                       /*  COM2 �� IO �����е�λ��     */

#define __COM0_GPHCON       ((0x2 <<  4) | (0x2 <<  6))                 /*  COM0 �� GPHCON �е�����     */
#define __COM1_GPHCON       ((0x2 <<  8) | (0x2 << 10))                 /*  COM1 �� GPHCON �е�����     */
#define __COM2_GPHCON       ((0x2 << 12) | (0x2 << 14))                 /*  COM1 �� GPHCON �е�����     */

#define __COM0_MASK         ((0x3 <<  4) | (0x3 <<  6))                 /*  COM0 �� GPHCON �е�����     */
#define __COM1_MASK         ((0x3 <<  8) | (0x3 << 10))                 /*  COM1 �� GPHCON �е�����     */
#define __COM2_MASK         ((0x3 << 12) | (0x3 << 14))                 /*  COM2 �� GPHCON �е�����     */
/*********************************************************************************************************
** Function name:           uartInit
** Descriptions:            ��ʼ�� UART
** input parameters:        iCom                ���ں�
**                          iInFrared           �Ƿ�ʹ�ú���ģʽ
**                          iData               ����λ��
**                          iStopBit            ����λ
**                          iCheck              У�鷽��
**                          iBaud               ������
**                          pvIsrRoutine        �жϷ�����, ΪNULL��ʾ�������ж�
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int     uartInit (int   iCom,
                  int   iInFrared,
                  int   iData,
                  int   iStopBit,
                  int   iCheck,
                  int   iBaud,
                  void *pvIsrRoutine)
{
    unsigned int        uiUBRDIVn;                                      /*  �����ʷ�����ֵ              */
    unsigned int        uiULCONn;                                       /*  �߿�����ֵ                  */
    unsigned int        uiUCONn;                                        /*  ���ڿ��ƼĴ���ֵ            */
    unsigned int        uiUFCONn;                                       /*  FIFO ���ƼĴ���ֵ           */
    
    unsigned int        uiInterEn;                                      /*  �Ƿ������ж�                */
    
    if (iData < 5) {                                                    /*  ����λ������                */
        return  (-1);
    }
    iData -= 5;                                                         /*  ȷ���Ĵ����е�ֵ            */
    
    uiInterEn = (pvIsrRoutine == (void *)0) ? 0 : 1;                    /*  ȷ���Ƿ���Ҫ�ж�            */
    
    iBaud = (iBaud << 4);                                               /*  baud = baud * 16            */
    iBaud = PCLK / iBaud;
    iBaud = (int)(iBaud - 0.5);
    
    uiUBRDIVn = iBaud;                                                  /*  ������                      */
    
    uiULCONn  = ((iInFrared << 6)
              |  (iCheck    << 3)
              |  (iStopBit  << 2)
              |  (iData));                                              /*  ����������Ϣ                */
    
    uiUCONn   = ((0x00 << 10)                                           /*  PCLK                        */
              |  (1 << 9)                                               /*  Tx Interrupt Type LEVEL     */
              |  (1 << 8)                                               /*  Rx Interrupt Type LEVEL     */
              |  (1 << 7)                                               /*  Rx Time Out Enable          */
              |  (0 << 6)                                               /*  Rx Error Status             */
                                                                        /*  Interrupt Disable           */
              |  (0 << 5)                                               /*  Loopback Mode Disable       */
              |  (0 << 4)
              |  (1 << 2)                                               /*  Tx Interrupt or poll        */
              |  (1));                                                  /*  Rx Interrupt or poll        */
    
    uiUFCONn  = ((0x0 << 6)                                             /*  Tx FIFO Trigger Level 0     */
              |  (0x2 << 4)                                             /*  Rx FIFO Trigger Level 16    */
              |  (1 << 2)                                               /*  Tx FIFO Reset               */
              |  (1 << 1)                                               /*  Rx FIFO Reset               */
              |  (1));                                                  /*  FIFO Enable                 */

    if (iCom == COM0) {                                                 /*  ���� UART0 �� �ܽ�          */

        rGPHCON &= ~(__COM0_MASK);
        rGPHCON |=   __COM0_GPHCON;
        rGPHUP  &= ~(__COM0_GPIO);                                      /*  ʹ����������                */
        
        rCLKCON |=   __COM0_CLKBIT;                                     /*  ʱ�ӹҽ�                    */

        rUCON0   = 0;
        rUFCON0  = uiUFCONn;
        rUMCON0  = 0;                                                   /*  �ر�����                    */
        rULCON0  = uiULCONn;
        rUCON0   = uiUCONn;
        rUBRDIV0 = uiUBRDIVn;
        
        if (uiInterEn) {                                                /*  �����жϷ�����            */
            API_InterVectorConnect(VIC_CHANNEL_UART0, 
                                   (PINT_SVR_ROUTINE)pvIsrRoutine,
                                   LW_NULL,
                                   "uart0_isr");
            INTER_CLR_MSK((1u << VIC_CHANNEL_UART0));                   /*  ��������ж�                */
            INTER_CLR_SUBMSK(BIT_SUB_RXD0);                             /*  �򿪽����ж�                */
        }
    
    } else if (iCom == COM1) {                                          /*  ���� UART1 �� �ܽ�          */
    
        rGPHCON &= ~(__COM1_MASK);
        rGPHCON |=   __COM1_GPHCON;
        rGPHUP  &= ~(__COM1_GPIO);                                      /*  ʹ����������                */
        
        rCLKCON |=   __COM1_CLKBIT;                                     /*  ʱ�ӹҽ�                    */
        
        rUCON1   = 0;
        rUFCON1  = uiUFCONn;
        rUMCON1  = 0;                                                   /*  �ر�����                    */
        rULCON1  = uiULCONn;
        rUCON1   = uiUCONn;
        rUBRDIV1 = uiUBRDIVn;
        
        if (uiInterEn) {                                                /*  �����жϷ�����            */
            API_InterVectorConnect(VIC_CHANNEL_UART1, 
                                   (PINT_SVR_ROUTINE)pvIsrRoutine,
                                   LW_NULL,
                                   "uart1_isr");
            INTER_CLR_MSK((1u << VIC_CHANNEL_UART1));                   /*  ��������ж�                */
            INTER_CLR_SUBMSK(BIT_SUB_RXD1);                             /*  �򿪽����ж�                */
        }
    
    } else if (iCom == COM2) {                                          /*  ���� UART2 �� �ܽ�          */
        
        rGPHCON &= ~(__COM2_MASK);
        rGPHCON |=   __COM2_GPHCON;
        rGPHUP  &= ~(__COM2_GPIO);                                      /*  ʹ����������                */
        
        rCLKCON |=   __COM2_CLKBIT;                                     /*  ʱ�ӹҽ�                    */
        
        rUCON2   = 0;
        rUFCON2  = uiUFCONn;
        rUMCON2  = 0;                                                   /*  �ر�����                    */
        rULCON2  = uiULCONn;
        rUCON2   = uiUCONn;
        rUBRDIV2 = uiUBRDIVn;
        
        if (uiInterEn) {                                                /*  �����жϷ�����            */
            API_InterVectorConnect(VIC_CHANNEL_UART2, 
                                   (PINT_SVR_ROUTINE)pvIsrRoutine,
                                   LW_NULL,
                                   "uart2_isr");
            INTER_CLR_MSK((1u << VIC_CHANNEL_UART2));                   /*  ��������ж�                */
            INTER_CLR_SUBMSK(BIT_SUB_RXD2);                             /*  �򿪽����ж�                */
        }
    
    } else {                                                            /*  ���ڳ���                    */
        
        return  (-1);
    }
    
    return  (0);
}
/*********************************************************************************************************
** Function name:           uartSendByte
** Descriptions:            UART ����һ���ֽڵ�����
** input parameters:        iCom                ���ں�
**                          ucData              ����
** output parameters:       NONE
** Returned value:          ��ȷ���� 0,  ���󷵻� -1
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
int     uartSendByte (int   iCom, unsigned char  ucData)
{
    switch (iCom) {
    
    case COM0:
        while (rUFSTAT0 & (1 << 14));
        while (((rUFSTAT0) >> 8) & 0x3F);                               /*  Tx_FIFO IS EMPTY            */
        WrUTXH0(ucData);
        break;
        
    case COM1:
        while (rUFSTAT1 & (1 << 14));
        while (((rUFSTAT1) >> 8) & 0x3F);                               /*  Tx_FIFO IS EMPTY            */
        WrUTXH1(ucData);
        break;
    
    case COM2:
        while (rUFSTAT2 & (1 << 14));
        while (((rUFSTAT2) >> 8) & 0x3F);                               /*  Tx_FIFO IS EMPTY            */
        WrUTXH2(ucData);
        break;
    
    default:                                                            /*  ���ںŴ���                  */
        return  (1);
    }
    
    return  (0);
}
/*********************************************************************************************************
** Function name:           uartSendByteCnt
** Descriptions:            UART ����ָ�����ȵ�����
** input parameters:        iCom                ���ں�
**                          pucData             ���ݻ�����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void    uartSendByteCnt (int             iCom, 
                         unsigned char  *pucData,
                         int             iCnt)
{
    for (; iCnt != 0; iCnt--) {
        uartSendByte(iCom, *pucData);                                   /*  ��������                    */
        pucData++;
    }
}
/*********************************************************************************************************
** Function name:           uartSendString
** Descriptions:            UART ����һ���ַ���
** input parameters:        iCom                ���ں�
**                          pcData              �ַ���
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/18
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void     uartSendString (int   iCom, char  *pcData)
{
    if (!pcData) {                                                      /*  ָ��Ϊ��                    */
        return;
    }
    
    while (*pcData != '\0') {                                           /*  �����ַ���                  */
        uartSendByte(iCom, (unsigned char)*pcData);
        pcData++;
    }
}
/*********************************************************************************************************
** Function name:           debugChannelInit
** Descriptions:            ��ʼ�����Խӿ�
** input parameters:        iChannelNum                 ͨ����
** output parameters:       NONE
** Returned value:          NONE
** Created by:              Hanhui
** Created Date:            2007/09/24
**--------------------------------------------------------------------------------------------------------
** Modified by:
** Modified date:
**--------------------------------------------------------------------------------------------------------
*********************************************************************************************************/
void    debugChannelInit (int  iChannelNum)
{
    uartInit(iChannelNum, UNUSE_INF, 8, ONE_STOPBIT, CHK_NONE, 115200, (void *)0);
}
/*********************************************************************************************************
  END FILE
*********************************************************************************************************/

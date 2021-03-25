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
** ��   ��   ��: dm9000x.c
**
** ��   ��   ��: JiaoJinXing
**
** �ļ���������: 2012 �� 03 �� 03 ��
**
** ��        ��: DM9000X ��̫��оƬ����.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"
#include "driver/net/dm9000.h"
/*********************************************************************************************************
** ��������: dm9000IntIsr
** ��������: DM9000 �жϴ���
** �䡡��  : dm9000
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t  dm9000IntIsr (struct dm9000_netdev  *dm9000)
{
#define DM9000_INT_GPIO     1

    irqreturn_t ret;

    ret = API_GpioSvrIrq(DM9000_INT_GPIO);
    if (ret == LW_IRQ_HANDLED) {
        API_GpioClearIrq(DM9000_INT_GPIO);
        dm9000Isr(dm9000);
    }

    return  (ret);
}
/*********************************************************************************************************
** ��������: dm9000NetInit
** ��������: DM9000 ����, �жϳ�ʼ��
** �䡡��  : ip, netmask, gw   Ĭ�ϵ������ַ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
void  dm9000NetInit (const char *ip, const char *netmask, const char *gw)
{
    static struct dm9000_netdev  dm9000;
    INT  ret;

    /*
     * ���� GPA15 Ϊ nGCS4
     */
    rGPACON  |= 1 << 15;

    /*
     * ���� BANK4 ���߿��Ϊ 16 λ, ʹ�ܵȴ�
     */
    rBWSCON   = (rBWSCON & (~(0x07 << 16))) | (0x05 << 16);

    /*
     * ���� BANK4 �ķ���ʱ�����ʺ� DM9000 оƬ
     */
    rBANKCON4 = (1 << 13) | (1 << 11) | (6 << 8) | (1 << 6) | (1 << 4) | (0 << 2) | (0 << 0);

    dm9000.init = NULL;
    dm9000.base = (addr_t)API_VmmIoRemapNocache((PVOID)0x20000000, LW_CFG_VMM_PAGE_SIZE);
    dm9000.io   = dm9000.base + 0x00000300;
    dm9000.data = dm9000.base + 0x00000304;

    dm9000.mac[0] = 0x08;
    dm9000.mac[1] = 0x09;
    dm9000.mac[2] = 0x0a;
    dm9000.mac[3] = 0x0b;
    dm9000.mac[4] = 0xaa;
    dm9000.mac[5] = 0x5a;

    ret = API_GpioRequestOne(DM9000_INT_GPIO, LW_GPIOF_OUT_INIT_LOW, "dm9000_eint");
    if (ret != ERROR_NONE) {
        return;
    }

    dm9000.irq = API_GpioSetupIrq(DM9000_INT_GPIO, LW_TRUE, 1);

    API_InterVectorConnect(dm9000.irq, (PINT_SVR_ROUTINE)dm9000IntIsr, (PVOID)&dm9000, "dm9000_isr");

    dm9000Init(&dm9000, ip, netmask, gw);
}
/*********************************************************************************************************
** END FILE
*********************************************************************************************************/

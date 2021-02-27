# 2021-02-23 ~ 2021-03-02 SylixOS Practice

> 本周尝试研究将BSP烧录至mini2440开发板上；
>
> 根据蒋老师的回复，我们在虚拟机上模拟Norflash的想法破灭了（似乎），需要想个办法来模拟；
>
> 参考文献：
>
> 1. [mini2440 SST39VF1601 读、写与擦除](https://blog.csdn.net/lizuobin2/article/details/50381791?utm_source=app&app_version=4.5.2)
> 2. [NorFlash坏块管理？](https://www.amobbs.com/thread-5682965-1-1.html)
> 3. [NorFlash坏块检测？](https://bbs.csdn.net/topics/90214060)
> 4. [NorFlash寿命及失效模式](https://www.cnblogs.com/xilentz/archive/2010/05/27/1745634.html)
> 5. [NorFlash Bad Block Management？](https://community.cypress.com/t5/Knowledge-Base-Articles/Bad-Blocks-in-NOR-Flash-Devices-KBA219740/ta-p/247343)
> 6. [Bad Block on NorFlash](http://www.infradead.org/pipermail/linux-mtd/2002-August/005769.html)
> 7. [How-to-decide-whether-a-nor-flash-have-bad-segment](https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/523868?How-to-decide-whether-a-nor-flash-have-bad-segment)

## 蒋老师的回复

**Q：**咱们realevo-simulator中的mini2440中，如何模拟norflash设备呢?我们根据simulator手册新建了一个pflash(手册中说是norflash文件)文件，但是新建后，我用网上的0x00作为norflash base address并不能成功扫描到norflash，kernel报错是can not write。由于在虚拟机上模拟开发比较方便，所以想请问一下老师该怎么进行配置。

**A：**

pflash选项是qemu的标准参数，我们把它加到界面上，只会将其原样传给qemu，没有验证过是否在2440上可行噢。可能还是需要你们基于2440开发板开发，可以通过jlink调试bsp。

如果暂时没有开发板，建议你们在内存中根据norflash页规格模拟norflash，最终只是写入算法不一样。

## Fake Nor Flash

为了模拟NorFlash，我们只需在**内存中**模拟一个**NorFlash**的**读、写与擦除**操作。为了便于观察读写平衡，还要标记每个Sector的擦写次数。

模拟目标参考mini2440的一种NorFlash——**SST39VF1601**。
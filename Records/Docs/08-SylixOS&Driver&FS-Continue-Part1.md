# 2021-03-08 ~ 2021-03-015 SylixOS&Driver&FS-Continue

> 本周由于挑战杯事情耽搁了，不一定能够按时完成。
>
> 1. 主要研究如何访问mini2440上的Norflash；
> 3. 设计一个文件系统；
>
> 参考文献：
>
> 1. [裸机操作S3C2440 NorFlash](https://blog.csdn.net/Mculover666/article/details/104115535)
> 2. [mini2440 NorFlash芯片手册](../Files/Am29LV160DB.pdf)
>

## 利用U-boot访问Norflash

1. 利用和[07-SylixOS&Driver&FS-Part1.md](./07-SylixOS&Driver&FS-Part1.md)相同的方法——利用Jlink向NorFlash烧录U-boot；

2. 进入Shell；

3. 输入下述序列，即可查看NorFlash的CFI信息：

   ```shell
   mw.w aa 98			# 进入CFI模式，在aa处写入0x98
   md.w 20 3			# 查看Q.R.Y Magic，读出0x20、0x22、0x24三个字
   md.w 4e 1			# 查看大小：2^21 次方，从0x4e处读出一个字
   mw.w 00 f0			# 退出CFI模式，向00处写入f0
   ```

   ![image-20210311230459261](G:\MyProject\Project.HoitOS\HoitOS\Records\Docs\images\mini2440-nor-cfi.png)



## 利用NorFlash启动SylixOS

> 感谢蒋老师的回复：
>
> Q：
>
> 蒋老师，你好，咱们SylixOs是烧录到mini2440的nandflash上，但是从nand flash启动没办法看到norflash，如果直接把系统烧到norflash上又太大装不了，那还该如何访问norflash呢？
>
> A：
>
> 有一种方法，我不知道可不可行，就是你用nand起来，nand的前4k不是负责搬运镜像么。你把nand的前4k拷贝出来烧录到nor的前4k里面，然后从nor启动是不是就可以搬运了。
>
> Q：
>
> 那直接用u-boot呢？
>
> A：
>
> 那也可以，用uboot也就是占用一部分空间。然后你映射文件系统时跳过这段地址；

很简单，只需要修改bootcmd即可：

```shell
//设置启动环境变量：从nand的0x60000处读取0x4ce000字节，写入ram的0x30000000处，然后CPU跳转至0x30000000运行；
setenv bootcmd "nand read 0x30000000 0x60000 0x4ce000; go 0x30000000"
//保存环境变量
saveenv
```

原理很简单，因为上述命令可以在RAM中成功执行，而NorFlash正是RAM-Like器件，因此毫无疑问，它也能执行上述指令。

## 在SylixOS中访问mini2440的NorFlash

> 感谢陈老师的回复：
>
> Q：
>
> 流程是这样的
>
> 1. 在nor上烧录uboot
> 2. 修改nor上的uboot的bootcmd, 也就是用老师你之前的方法
> 3. bootcmd将位于nand的SylixOS加载到30000000处
>
> 4. 通过上面链接的代码，想要查看norflash的cfi，但是提示写错误
>
> A：
>
> 在SylixOS中，访问的地址是虚拟地址，需要做映射。`API_VmmIoRemap`用这个接口

关键在于通过`API_VmmIoRemap`将地址0到2MB的地址映射到某个虚拟地址上：

```c
VOID nor_show_cfi(){
    NOR_FLASH_BASE = (UINT32)API_VmmIoRemap2(0, 2 * 1024 * 1024);
    printf("### Show CFI\n");
    ...
}
```

结果如图所示：

![image-20210312151304969](G:\MyProject\Project.HoitOS\HoitOS\Records\Docs\images\mini2440-nor.png)

## NorFlash驱动设计

### 一个很棘手的问题

这里有一个疑问，我们在Uboot中利用如下指令序列向内存中写的时候，按道理来说地址`mw.w 00 f0`是U-boot的起始地址，然而断电后再次启动时，U-boot仍能正常工作，这是为什么呢？

```shell
mw.w aa 98			# 进入CFI模式
md.w 20 3			# 查看Q.R.Y Magic
md.w 4e 1			# 查看大小：2^21 次方
mw.w 00 f0			# 退出CFI模式
```

### mini2440 NorFlash进行探索

通过查阅mini2440手册可以发现，我们是Am29LV160DB NorFlash；

![image-20210312163601714](G:\MyProject\Project.HoitOS\HoitOS\Records\Docs\images\mini2440-nroflash-analysis.png)

另外，这里再P24的地方是芯片指令说明：

![image-20210312165016203](G:\MyProject\Project.HoitOS\HoitOS\Records\Docs\images\mini2440-instruction-set.png)

#### 读、擦除注意事项




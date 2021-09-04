# ~ HoitFS优化阶段-Part1

> 要准备做出更多成果了。我们的测试也能够在mini 2440上跑通了。现在为了更好地在mini 2440上进行实验测试，我们还需做更多的工作。

## 1. TODO List

- [ ] **宏控参数化**：

  宏控的目的在于减小镜像体积，这一点我们是芜锁胃的，因此，需要逐步将宏控转换为参数，避免我们不断烧录

- [ ] **EBS**：

  EBS的启用和关闭，通过传入相关参数进行

- [ ] **线程池机制**：

  挂载时不能无脑添加线程，否则会因为线程竞争导致更差的结果

- [ ] ***Merge Buffer Plus版**：

  控制红黑树高度？？？

  

## 2. 优化结果
### 2.1 宏控参数化

命令模式如下：

```shell
mount -t hoitfs [$crc [off/on]] [$ebs [off/on]] [$mt [on/off,tcnt=2]] \
      [$bgc [off/on,thr=50]] [$mtree [on/off,mdatasz=16,mbuffersz=16]] \
      [$tree [mnodesz=1024]  [$o [silence|s, ...]] /mnt/hoitfs
```

默认选项为：

```shell
$crc on										# 开启CRC
$ebs on										# 开启EBS
$mt  on,tcnt=2								# 开启扫描多线程，线程池大小为2
$bgc on,thr=50								# 开启后台GC，触发阈值为50%
$mtree on,mdatasz=16,mbuffersz=16			# 开启Mergeable Tree，小数据为16B，Merge Buffer大小为16
$tree mnodesz=1024							# 红黑树的最大节点大小为1024
$o   (no)silence							# 挂载静默？silence代表静默，默认不静默
```

一个例子：

```shell
mount -t hoitfs "$crc on $ebs on $mt on,tcnt=2 $bgc on,thr=50 $mtree on,mdatasz=16,mbuffersz=16 $tree mnodesz=1024" /mnt/hoitfs
```



  


# HoitOS
> 这里是项目简介

This is the repository of 2021 OS comp， 项目进度安排移步至[Worktile](https://vwpp20210125060242436.worktile.com/mission/projects/600e62ffd6e5f843a97f2182)

![worktile](./images/worktile.png)

## Log

> 这里记录了所有输出文档

1. [00-Prepare](./Records/Docs/00-Prepare.md)

   主要挑选合适的项目， 并对各项目进行再评估；

2. [01-SettleDown](./Records/Docs/01-SettleDown.md)

   定夺项目Proj32，开始着手预研工作；

3. [02-PreResearch-Part1](./Records/Docs/02-PreResearch-Part1.md)

   主要介绍Flash、基于Flash的文件系统：JFFS、JFFS2、YAFFS、SPIFFS等、SylixOS的IDE配置、TPSFS的掉电安全机制等；

4. [02-PreResearch-Part2](./Records/Docs/02-PreResearch-Part2.md)

   与Part1类似，额外添加了Github上提供的一种**写平衡** 算法；

5. [03-DeepResearch-Part1](./Records/Docs/03-DeepResearch-Part1.md)

   主要确定了Why Norflash、板子选型，研究了F2FS的基本结构以及其优势、研究了基于ramFs的SylixOS注册文件系统的一般方式、学习了关于**/mnt**、**/dev**目录的意义，以及驱动相关的概念，最后通过UCS的Lab7，深入理解了LFS的写操作、更新操作、GC操作等；

6. [03-DeepResearch-Part2](./Records/Docs/03-DeepResearch-Part2.md)

   主要研究了MTD的结构、详细方法、NorFlash驱动模板框架等；

7. [03-DeepResearch-Part3](./Records/Docs/03-DeepResearch-Part3.md)

   除了研究F2FS文件系统，还特别研究了SylixOS中Yaffs文件系统的装载工作；

8. [04-SylixOSResearch-Part1](./Records/Docs/04-SylixOSResearch-Part1.md)

   待补充

## ConfigurationControlBoard(CCB)

> 这里记录了会议纪要

1. [2021-01-12](./Records/CCB/2021-01-12.md)
2. [2021-01-14](./Records/CCB/2021-01-14.md)
3. [2021-01-24](./Records/CCB/2021-01-24.md)
4. [2021-01-31](./Records/CCB/2021-01-31.md)
5. [2021-02-07](./Records/CCB/2021-02-07.md)

## Basic Develop Method

> 这里记录了利用Git开发的基本流程

**1. Get Start**

```shell
# clone project
git clone https://github.com/Hoit-23o2/HoitOS.git
```

**2. Basic Development**

```shell
# pull project
git pull origin main

...

# add some modification
git add .
# commit
git commit -m "一些描述"
# REMEMBER MUST "pull" before "push"
git pull origin main
# push
git push origin main
```


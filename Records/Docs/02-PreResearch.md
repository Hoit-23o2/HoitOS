# 2021-01-14 ~ 2021-01-30 Pre Research

> 这个时间主要做预研工作
>
> 参考资料：
>
> 1. [File System Overview](https://kb.wisc.edu/helpdesk/page.php?id=11300#:~:text=A%20file%20system%20is%20a,systems%20when%20formatting%20such%20media.)
> 2. [文件系统的层次结构](https://www.cnblogs.com/caidi/p/6781759.html)
> 3. [OS之文件系统的层次结构](https://www.codetd.com/article/11977179)
> 4. [FAT32文件系统结构详解](https://blog.csdn.net/li_wen01/article/details/79929730/)
> 5. [EXT文件系统机制（知乎）](https://zhuanlan.zhihu.com/p/61749046)
> 6. [NTFS文件系统结构分析](http://www.360doc.com/content/10/1229/16/4573246_82348341.shtml)
> 7. [F2FS技术拆解](https://blog.csdn.net/feelabclihu/article/details/105534143)
> 8. [NorFlash结构与特性](http://www.yingtexin.net/news-2/54718.shtml)
> 9. [NorFlash的基本操作](https://blog.csdn.net/bkyy123/article/details/80167344?utm_medium=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-4.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-BlogCommendFromMachineLearnPai2-4.control)
> 10. [嵌入式系统FS](https://www.keil.com/pack/doc/mw/FileSystem/html/emb_fs.html#:~:text=The%20Embedded%20File%20System%20EFS,is%20optimized%20for%20maximum%20performance.&text=File%20Names%20%26%20Content%20are%20stored,provide%20optimal%20file%20access%20times.)
> 11. [几种嵌入式文件系统比较](http://blog.chinaunix.net/uid-20692983-id-1892749.html?utm_source=jiancool)
> 12. [XIP](https://www.embedded-computing.com/guest-blogs/execute-in-place-xip-an-external-flash-architecture-ideal-for-the-code-and-performance-requirements-of-edge-iot-and-ai#:~:text=XiP%20is%20a%20method%20of,the%20program%20from%20that%20RAM.&text=Serial%20flash%20memory%20is%20typically,as%20BIOS%20in%20a%20PC.)
> 13. [NandFlash](https://www.taodocs.com/p-4196750-1.html)
> 14. [NorFlash VS NandFlash](https://searchstorage.techtarget.com/definition/NOR-flash-memory)
> 15. [LFS](https://web.stanford.edu/~ouster/cgi-bin/papers/lfs.pdf)
> 16. [知乎：LFS](https://zhuanlan.zhihu.com/p/41358013)
> 17. [JFFS](https://sourceware.org/jffs2/jffs2.pdf)

[TOC]



## 文件系统的层次结构

### 文件系统的基本结构

![image-20210115123904655](G:\MyProject\Project.HoitOS\HoitOS\Records\Docs\images\basics-of-fs-structure)

其中，各个模块的 **宽泛含义** 如下：

- **用户接口模块** ：提供用户接口，例如：**open、write、read** 等；
- **文件目录模块** ：用户通过文件路径访问文件，这个模块需要**根据路径** 找到相应的**FCB** 。所有和目录、目录项相关的管理工作都在本模块内完成，例如：管理活跃的文件目录、管理打开文件表等；
- **存取控制模块** ：验证用户是否有访问权限；
- **逻辑文件系统与文件信息缓冲区** ：用户指明想要访问的文件记录号，该模块将记录好转换为对应的逻辑地址；
- **备份恢复模块** ：该模块用于解决掉电安全问题，在xv6中，对应于Logging Layer，在SylixOS中，tpsFS利用Transaction机制来保证；
- **物理文件系统** ：将逻辑地址转换为物理地址；
- **辅助分配模块** ：负责文件存储空间的管理，即负责分配和回收空间；
- **设备管理模块** ：直接与硬件交互，负责和硬件相关的一些管理工作：如分配设备、分配设备缓冲区、磁盘调度、启动设备、释放设备；

### 调研目前已有的FS结构

#### FAT FS



#### EXT FS



#### NT FS



#### F2 FS




/* ============================================================================
 * unistd.c - PicoC POSIX 系统调用库 (unistd.h)
 *
 * 为 PicoC 解释器提供 POSIX <unistd.h> 头文件中定义的系统调用封装，
 * 涵盖文件 I/O、进程控制、用户/组管理、目录操作等功能。
 *
 * 功能分组:
 *   - 文件 I/O:     open / close / read / write / lseek / dup / dup2 / fsync /
 *                   ftruncate / truncate / fdatasync / lockf
 *   - 文件系统:     access / chdir / chroot / link / unlink / readlink /
 *                   symlink / rmdir / remove
 *   - 进程控制:     fork / vfork / _exit / pause / alarm / sleep / usleep /
 *                   nice / setsid / setpgrp
 *   - 用户/组:      getuid / geteuid / getgid / getegid / setuid / setgid /
 *                   chown / fchown
 *   - 进程标识:     getpid / getppid / getpgrp / getsid / tcgetpgrp / tcsetpgrp
 *   - 终端:         isatty / ttyname / ctermid
 *   - 系统配置:     sysconf / pathconf / fpathconf / confstr / getpagesize
 *   - 其他:         getcwd / getwd / sbrk / sync / getlogin / gethostid
 *
 * 条件编译: WIN32 下部分函数行为不同。
 * 仅在未定义 BUILTIN_MINI_STDLIB 时编译 (嵌入式平台不包含)。
 * ============================================================================ */

#include "../interpreter.h"
#ifndef BUILTIN_MINI_STDLIB

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>

static int ZeroValue = 0;

/* =========================================================================
 * 文件/目录访问函数
 * ========================================================================= */

/* access: 检查文件的可访问性 (R_OK/W_OK/X_OK/F_OK) */
void UnistdAccess(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = access(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* chdir: 改变当前工作目录 */
void UnistdChdir(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = chdir(Param[0]->Val->Pointer);
}

/* chroot: 改变根目录 (需要 root 权限) */
void UnistdChroot(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = chroot(Param[0]->Val->Pointer);
}

/* chown: 改变文件所有者和组 */
void UnistdChown(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = chown(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* fchown: 通过文件描述符改变文件所有者和组 */
void UnistdFchown(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fchown(Param[0]->Val->Integer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* lchown: 改变符号链接本身的所有者 (不跟随链接) */
void UnistdLchown(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lchown(Param[0]->Val->Pointer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* fchdir: 通过文件描述符改变工作目录 */
void UnistdFchdir(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fchdir(Param[0]->Val->Integer);
}

/* link: 创建硬链接 */
void UnistdLink(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = link(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* unlink: 删除文件名 (硬链接) */
void UnistdUnlink(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = unlink(Param[0]->Val->Pointer);
}

/* readlink: 读取符号链接的目标路径 */
void UnistdReadlink(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = readlink(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* symlink: 创建符号链接 */
void UnistdSymlink(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = symlink(Param[0]->Val->Pointer, Param[1]->Val->Pointer);
}

/* rmdir: 删除空目录 */
void UnistdRmdir(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = rmdir(Param[0]->Val->Pointer);
}

/* =========================================================================
 * 文件 I/O 函数
 * ========================================================================= */

/* close: 关闭文件描述符 */
void UnistdClose(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = close(Param[0]->Val->Integer);
}

/* read: 从文件描述符读取数据 */
void UnistdRead(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = read(Param[0]->Val->Integer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* write: 向文件描述符写入数据 */
void UnistdWrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = write(Param[0]->Val->Integer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* lseek: 移动文件读写位置指针 */
void UnistdLseek(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lseek(Param[0]->Val->Integer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* dup: 复制文件描述符 */
void UnistdDup(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = dup(Param[0]->Val->Integer);
}

/* dup2: 复制文件描述符到指定编号 */
void UnistdDup2(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = dup2(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* fsync: 将文件描述符的缓冲区同步到磁盘 */
void UnistdFsync(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fsync(Param[0]->Val->Integer);
}

/* fdatasync: 将文件数据 (不含元数据) 同步到磁盘 */
void UnistdFdatasync(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
#ifndef F_FULLSYNC
    ReturnValue->Val->Integer = fdatasync(Param[0]->Val->Integer);
#else
    /* Mac OS X 等效实现 */
    ReturnValue->Val->Integer = fcntl(Param[0]->Val->Integer, F_FULLFSYNC);
#endif
}

/* ftruncate: 将文件截断为指定长度 */
void UnistdFtruncate(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ftruncate(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* truncate: 将文件 (按路径名) 截断为指定长度 */
void UnistdTruncate(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = truncate(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* lockf: 对文件区域加锁/解锁 */
void UnistdLockf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = lockf(Param[0]->Val->Integer, Param[1]->Val->Integer, Param[2]->Val->Integer);
}

/* =========================================================================
 * 进程控制函数
 * ========================================================================= */

/* alarm: 设置闹钟信号 (seconds 秒后发送 SIGALRM) */
void UnistdAlarm(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = alarm(Param[0]->Val->Integer);
}

/* ualarm: 微秒级别的闹钟 (useconds 后首次触发，之后每 interval 触发) */
void UnistdUalarm(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ualarm(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* sleep: 挂起进程指定秒数 */
void UnistdSleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = sleep(Param[0]->Val->Integer);
}

/* usleep: 挂起进程指定微秒数 */
void UnistdUsleep(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = usleep(Param[0]->Val->Integer);
}

/* pause: 挂起进程直到收到信号 */
void UnistdPause(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = pause();
}

/* fork: 创建子进程 (复制当前进程) */
void UnistdFork(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fork();
}

/* vfork: 创建子进程 (共享地址空间，不推荐使用) */
void UnistdVfork(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = vfork();
}

/* _exit: 立即终止进程 (不调用 atexit 处理函数) */
void Unistd_Exit(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    _exit(Param[0]->Val->Integer);
}

/* nice: 修改进程调度优先级 */
void UnistdNice(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = nice(Param[0]->Val->Integer);
}

/* setsid: 创建新会话并将进程设为会话组长 */
void UnistdSetsid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setsid();
}

/* setpgrp: 设置进程组 (无参数版本) */
void UnistdSetpgrp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setpgrp();
}

/* setpgid: 设置进程组 ID */
void UnistdSetpgid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setpgid(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* =========================================================================
 * 进程/用户标识函数
 * ========================================================================= */

/* getpid: 获取当前进程 ID */
void UnistdGetpid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getpid();
}

/* getppid: 获取父进程 ID */
void UnistdGetppid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getppid();
}

/* getpgrp: 获取进程组 ID */
void UnistdGetpgrp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getpgrp();
}

/* getuid: 获取实际用户 ID */
void UnistdGetuid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getuid();
}

/* geteuid: 获取有效用户 ID */
void UnistdGeteuid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = geteuid();
}

/* getgid: 获取实际组 ID */
void UnistdGetgid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getgid();
}

/* getegid: 获取有效组 ID */
void UnistdGetegid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getegid();
}

/* setuid: 设置用户 ID */
void UnistdSetuid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setuid(Param[0]->Val->Integer);
}

/* setgid: 设置组 ID */
void UnistdSetgid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setgid(Param[0]->Val->Integer);
}

/* setreuid: 设置实际和有效用户 ID */
void UnistdSetreuid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setreuid(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* setregid: 设置实际和有效组 ID */
void UnistdSetregid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = setregid(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* =========================================================================
 * 终端相关函数
 * ========================================================================= */

/* isatty: 检查文件描述符是否指向终端 */
void UnistdIsatty(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = isatty(Param[0]->Val->Integer);
}

/* ttyname: 获取终端设备名称 */
void UnistdTtyname(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = ttyname(Param[0]->Val->Integer);
}

/* ttyname_r: 获取终端设备名称 (可重入版本) */
void UnistdTtyname_r(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = ttyname_r(Param[0]->Val->Integer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* ctermid: 获取控制终端名称 */
void UnistdCtermid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = ctermid(Param[0]->Val->Pointer);
}

/* tcgetpgrp: 获取终端关联的前台进程组 ID */
void UnistdTcgetpgrp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = tcgetpgrp(Param[0]->Val->Integer);
}

/* tcsetpgrp: 设置终端关联的前台进程组 */
void UnistdTcsetpgrp(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = tcsetpgrp(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* =========================================================================
 * 目录和路径函数
 * ========================================================================= */

/* getcwd: 获取当前工作目录的绝对路径 */
void UnistdGetcwd(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = getcwd(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* getwd: 获取当前工作目录 (危险，无缓冲区溢出保护) */
void UnistdGetwd(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = getcwd(Param[0]->Val->Pointer, PATH_MAX);
}

/* =========================================================================
 * 系统和配置函数
 * ========================================================================= */

/* sysconf: 获取系统配置参数 (处理器数、页大小等) */
void UnistdSysconf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = sysconf(Param[0]->Val->Integer);
}

/* pathconf: 获取路径相关的配置参数 */
void UnistdPathconf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = pathconf(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* fpathconf: 获取文件描述符相关的配置参数 */
void UnistdFpathconf(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = fpathconf(Param[0]->Val->Integer, Param[1]->Val->Integer);
}

/* confstr: 获取字符串类型的系统配置值 */
void UnistdConfstr(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = confstr(Param[0]->Val->Integer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}

/* getpagesize: 获取系统页大小 (字节数) */
void UnistdGetpagesize(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getpagesize();
}

/* getdtablesize: 获取文件描述符表的最大大小 */
void UnistdGetdtablesize(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getdtablesize();
}

/* =========================================================================
 * 其他函数
 * ========================================================================= */

/* sbrk: 调整程序数据段大小 (增加 heap) */
void UnistdSbrk(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = sbrk(Param[0]->Val->Integer);
}

/* sync: 将所有文件系统缓冲区同步到磁盘 */
void UnistdSync(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    sync();
}

/* getlogin: 获取当前登录用户名 */
void UnistdGetlogin(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = getlogin();
}

/* getlogin_r: 获取当前登录用户名 (可重入版本) */
void UnistdGetlogin_r(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getlogin_r(Param[0]->Val->Pointer, Param[1]->Val->Integer);
}

/* gethostid: 获取当前主机的唯一标识符 */
void UnistdGethostid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = gethostid();
}

/* getpass: 从终端读取密码 (不回显) */
void UnistdGetpass(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = getpass(Param[0]->Val->Pointer);
}

#if 0
/* 以下函数暂时禁用 (未测试或平台不兼容) */

void UnistdCuserid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Pointer = cuserid(Param[0]->Val->Pointer);
}

void UnistdGetpgid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getpgid(Param[0]->Val->Integer);
}

void UnistdGetsid(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = getsid(Param[0]->Val->Integer);
}

void UnistdPread(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = pread(Param[0]->Val->Integer, Param[1]->Val->Pointer, Param[2]->Val->Integer, Param[3]->Val->Integer);
}

void UnistdPwrite(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = pwrite(Param[0]->Val->Integer, Param[1]->Val->Pointer, Param[2]->Val->Integer, Param[3]->Val->Integer);
}

void UnistdSwab(struct ParseState *Parser, struct Value *ReturnValue, struct Value **Param, int NumArgs)
{
    ReturnValue->Val->Integer = swab(Param[0]->Val->Pointer, Param[1]->Val->Pointer, Param[2]->Val->Integer);
}
#endif


/* POSIX 类型定义: uid_t, gid_t, pid_t, off_t, size_t, ssize_t 等 */
const char UnistdDefs[] = "\
typedef int uid_t; \
typedef int gid_t; \
typedef int pid_t; \
typedef int off_t; \
typedef int size_t; \
typedef int ssize_t; \
typedef int useconds_t;\
typedef int intptr_t;\
";

/* unistd 函数注册表: 将 PicoC 函数映射到 C 原型声明 */
struct LibraryFunction UnistdFunctions[] =
{
    { UnistdAccess,        "int access(char *, int);" },
    { UnistdAlarm,         "unsigned int alarm(unsigned int);" },
/*    { UnistdBrk,           "int brk(void *);" }, */
    { UnistdChdir,         "int chdir(char *);" },
    { UnistdChroot,        "int chroot(char *);" },
    { UnistdChown,         "int chown(char *, uid_t, gid_t);" },
    { UnistdClose,         "int close(int);" },
    { UnistdConfstr,       "size_t confstr(int, char *, size_t);" },
    { UnistdCtermid,       "char *ctermid(char *);" },
/*    { UnistdCuserid,       "char *cuserid(char *);" }, */
    { UnistdDup,           "int dup(int);" },
    { UnistdDup2,          "int dup2(int, int);" },
/*     { UnistdEncrypt,       "void encrypt(char[64], int);" }, */
/*    { UnistdExecl,         "int execl(char *, char *, ...);" }, */
/*    { UnistdExecle,        "int execle(char *, char *, ...);" }, */
/*    { UnistdExeclp,        "int execlp(char *, char *, ...);" }, */
/*    { UnistdExecv,         "int execv(char *, char *[]);" }, */
/*    { UnistdExecve,        "int execve(char *, char *[], char *[]);" }, */
/*    { UnistdExecvp,        "int execvp(char *, char *[]);" }, */
    { Unistd_Exit,         "void _exit(int);" },
    { UnistdFchown,        "int fchown(int, uid_t, gid_t);" },
    { UnistdFchdir,        "int fchdir(int);" },
    { UnistdFdatasync,     "int fdatasync(int);" },
    { UnistdFork,          "pid_t fork(void);" },
    { UnistdFpathconf,     "long fpathconf(int, int);" },
    { UnistdFsync,         "int fsync(int);" },
    { UnistdFtruncate,     "int ftruncate(int, off_t);" },
    { UnistdGetcwd,        "char *getcwd(char *, size_t);" },
    { UnistdGetdtablesize, "int getdtablesize(void);" },
    { UnistdGetegid,       "gid_t getegid(void);" },
    { UnistdGeteuid,       "uid_t geteuid(void);" },
    { UnistdGetgid,        "gid_t getgid(void);" },
/*    { UnistdGetgroups,     "int getgroups(int, gid_t []);" }, */
    { UnistdGethostid,     "long gethostid(void);" },
    { UnistdGetlogin,      "char *getlogin(void);" },
    { UnistdGetlogin_r,    "int getlogin_r(char *, size_t);" },
/*    { UnistdGetopt,        "int getopt(int, char * [], char *);" }, */
    { UnistdGetpagesize,   "int getpagesize(void);" },
    { UnistdGetpass,       "char *getpass(char *);" },
/*    { UnistdGetpgid,       "pid_t getpgid(pid_t);" }, */
    { UnistdGetpgrp,       "pid_t getpgrp(void);" },
    { UnistdGetpid,        "pid_t getpid(void);" },
    { UnistdGetppid,       "pid_t getppid(void);" },
/*    { UnistdGetsid,        "pid_t getsid(pid_t);" }, */
    { UnistdGetuid,        "uid_t getuid(void);" },
    { UnistdGetwd,         "char *getwd(char *);" },
    { UnistdIsatty,        "int isatty(int);" },
    { UnistdLchown,        "int lchown(char *, uid_t, gid_t);" },
    { UnistdLink,          "int link(char *, char *);" },
    { UnistdLockf,         "int lockf(int, int, off_t);" },
    { UnistdLseek,         "off_t lseek(int, off_t, int);" },
    { UnistdNice,          "int nice(int);" },
    { UnistdPathconf,      "long pathconf(char *, int);" },
    { UnistdPause,         "int pause(void);" },
/*    { UnistdPipe,          "int pipe(int [2]);" }, */
/*    { UnistdPread,         "ssize_t pread(int, void *, size_t, off_t);" }, */
/*    { UnistdPthread_atfork,"int pthread_atfork(void (*)(void), void (*)(void), void(*)(void));" }, */
/*    { UnistdPwrite,        "ssize_t pwrite(int, void *, size_t, off_t);" }, */
    { UnistdRead,          "ssize_t read(int, void *, size_t);" },
    { UnistdReadlink,      "int readlink(char *, char *, size_t);" },
    { UnistdRmdir,         "int rmdir(char *);" },
    { UnistdSbrk,          "void *sbrk(intptr_t);" },
    { UnistdSetgid,        "int setgid(gid_t);" },
    { UnistdSetpgid,       "int setpgid(pid_t, pid_t);" },
    { UnistdSetpgrp,       "pid_t setpgrp(void);" },
    { UnistdSetregid,      "int setregid(gid_t, gid_t);" },
    { UnistdSetreuid,      "int setreuid(uid_t, uid_t);" },
    { UnistdSetsid,        "pid_t setsid(void);" },
    { UnistdSetuid,        "int setuid(uid_t);" },
    { UnistdSleep,         "unsigned int sleep(unsigned int);" },
/*    { UnistdSwab,          "void swab(void *, void *, ssize_t);" }, */
    { UnistdSymlink,       "int symlink(char *, char *);" },
    { UnistdSync,          "void sync(void);" },
    { UnistdSysconf,       "long sysconf(int);" },
    { UnistdTcgetpgrp,     "pid_t tcgetpgrp(int);" },
    { UnistdTcsetpgrp,     "int tcsetpgrp(int, pid_t);" },
    { UnistdTruncate,      "int truncate(char *, off_t);" },
    { UnistdTtyname,       "char *ttyname(int);" },
    { UnistdTtyname_r,     "int ttyname_r(int, char *, size_t);" },
    { UnistdUalarm,        "useconds_t ualarm(useconds_t, useconds_t);" },
    { UnistdUnlink,        "int unlink(char *);" },
    { UnistdUsleep,        "int usleep(useconds_t);" },
    { UnistdVfork,         "pid_t vfork(void);" },
    { UnistdWrite,         "ssize_t write(int, void *, size_t);" },
    { NULL,                 NULL }
};

/* UnistdSetupFunc: 初始化 unistd 库
 * - 定义 NULL 常量
 * - 注册 optarg, optind, opterr, optopt 等 getopt 全局变量 */
void UnistdSetupFunc(Picoc *pc)
{
    /* 定义 NULL (如果尚未定义) */
    if (!VariableDefined(pc, TableStrRegister(pc, "NULL")))
        VariableDefinePlatformVar(pc, NULL, "NULL", &pc->IntType, (union AnyValue *)&ZeroValue, FALSE);

    /* 注册 getopt 相关的外部变量 */
    VariableDefinePlatformVar(pc, NULL, "optarg", pc->CharPtrType, (union AnyValue *)&optarg, TRUE);
    VariableDefinePlatformVar(pc, NULL, "optind", &pc->IntType, (union AnyValue *)&optind, TRUE);
    VariableDefinePlatformVar(pc, NULL, "opterr", &pc->IntType, (union AnyValue *)&opterr, TRUE);
    VariableDefinePlatformVar(pc, NULL, "optopt", &pc->IntType, (union AnyValue *)&optopt, TRUE);
}

#endif /* !BUILTIN_MINI_STDLIB */

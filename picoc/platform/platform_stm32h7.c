/* STM32H7 平台适配层 — PicoC 平台 I/O 接口实现
 *
 * 为 PicoC 解释器提供目标平台所需的底层 I/O 函数：
 *   - 串口输入：PlatformGetLine / PlatformGetLineQuiet / PlatformGetCharacter
 *   - 串口输出：PlatformPutc（通过 UART 发送单字节）
 *   - 文件操作：在此目标上不支持（返回错误）
 *   - 程序退出：PlatformExit（longjmp 回解释器）
 *
 * 所有串口收发最终通过 PicocApp_ConsoleGetCharBlocking() 和
 * SerialApp_Write() 完成，走 STM32H7 的 UART 硬件层。
 */

#include "../picoc.h"
#include "../interpreter.h"
#include "picoc_app.h"
#include "serial_app.h"

static void PlatformWriteString(const char *text);

/* 静默读取一行（无回显，无提示符），供调试器内部使用 */
char *PlatformGetLineQuiet(char *Buf, int MaxLen)
{
    int len = 0;

    if (Buf == NULL || MaxLen <= 1)
        return NULL;

    for (;;)
    {
        int ch = PicocApp_ConsoleGetCharBlocking();

        if (ch == '\r' || ch == '\n')
        {
            Buf[len++] = '\n';
            Buf[len] = '\0';
            return Buf;
        }

        if (ch == '\b' || ch == 0x7f)
        {
            if (len > 0)
                len--;
            continue;
        }

        if (len < MaxLen - 2)
            Buf[len++] = (char)ch;
    }
}

/* 平台初始化（当前无操作） */
void PlatformInit(Picoc *pc)
{
    (void)pc;
}

/* 平台清理（当前无操作） */
void PlatformCleanup(Picoc *pc)
{
    (void)pc;
}

/* 读取一行输入（带回显和提示符），供 REPL 交互使用 */
char *PlatformGetLine(char *Buf, int MaxLen, const char *Prompt)
{
    int len = 0;

    if (Buf == NULL || MaxLen <= 1)
    {
        return NULL;
    }

    if (Prompt != NULL)
    {
        PlatformWriteString(Prompt);
    }

    for (;;)
    {
        int ch = PicocApp_ConsoleGetCharBlocking();

        if (ch == '\r' || ch == '\n')
        {
            /* 回显换行 */
            PlatformPutc('\r', NULL);
            PlatformPutc('\n', NULL);
            Buf[len++] = '\n';
            Buf[len] = '\0';
            return Buf;
        }

        if (ch == '\b' || ch == 0x7f)
        {
            /* 回显退格：\b 空格 \b 擦除屏幕上的字符 */
            if (len > 0)
            {
                len--;
                PlatformPutc('\b', NULL);
                PlatformPutc(' ', NULL);
                PlatformPutc('\b', NULL);
            }
            continue;
        }

        if (len < MaxLen - 2)
        {
            Buf[len++] = (char)ch;
            PlatformPutc((unsigned char)ch, NULL);
        }
    }
}

/* 读取单个字符（阻塞） */
int PlatformGetCharacter(void)
{
    return PicocApp_ConsoleGetCharBlocking();
}

/* 通过 UART 串口输出单个字符
 * 调用 SerialApp_Write 直到字节成功发送 */
void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *Stream)
{
    uint8_t ch = OutCh;

    (void)Stream;

    while (SerialApp_Write(&ch, 1U) == 0U)
    {
    }
}

/* 从文件系统读取文件（此目标不支持） */
char *PlatformReadFile(Picoc *pc, const char *FileName)
{
    ProgramFailNoParser(pc, "file input not supported on this target: %s", FileName);
    return NULL;
}

/* 扫描文件（此目标不支持） */
void PicocPlatformScanFile(Picoc *pc, const char *FileName)
{
    ProgramFailNoParser(pc, "file input not supported on this target: %s", FileName);
}

/* 退出 PicoC 程序执行，通过 longjmp 回到解释器主循环 */
void PlatformExit(Picoc *pc, int RetVal)
{
    pc->PicocExitValue = RetVal;
    longjmp(pc->PicocExitBuf, 1);
}

/* 内部辅助：通过串口发送一个字符串 */
static void PlatformWriteString(const char *text)
{
    while (text != NULL && *text != '\0')
    {
        PlatformPutc((unsigned char)*text++, NULL);
    }
}

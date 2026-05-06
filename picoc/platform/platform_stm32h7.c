#include "../picoc.h"
#include "../interpreter.h"
#include "picoc_app.h"
#include "serial_app.h"

static void PlatformWriteString(const char *text);

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

void PlatformInit(Picoc *pc)
{
    (void)pc;
}

void PlatformCleanup(Picoc *pc)
{
    (void)pc;
}

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
            PlatformPutc('\r', NULL);
            PlatformPutc('\n', NULL);
            Buf[len++] = '\n';
            Buf[len] = '\0';
            return Buf;
        }

        if (ch == '\b' || ch == 0x7f)
        {
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

int PlatformGetCharacter(void)
{
    return PicocApp_ConsoleGetCharBlocking();
}

void PlatformPutc(unsigned char OutCh, union OutputStreamInfo *Stream)
{
    uint8_t ch = OutCh;

    (void)Stream;

    while (SerialApp_Write(&ch, 1U) == 0U)
    {
    }
}

char *PlatformReadFile(Picoc *pc, const char *FileName)
{
    ProgramFailNoParser(pc, "file input not supported on this target: %s", FileName);
    return NULL;
}

void PicocPlatformScanFile(Picoc *pc, const char *FileName)
{
    ProgramFailNoParser(pc, "file input not supported on this target: %s", FileName);
}

void PlatformExit(Picoc *pc, int RetVal)
{
    pc->PicocExitValue = RetVal;
    longjmp(pc->PicocExitBuf, 1);
}

static void PlatformWriteString(const char *text)
{
    while (text != NULL && *text != '\0')
    {
        PlatformPutc((unsigned char)*text++, NULL);
    }
}

/* picoc interactive debugger */

#ifndef NO_DEBUGGER

#include "picoc.h"
#include "interpreter.h"
#include <string.h>

#define BREAKPOINT_HASH(p) ( ((unsigned long)(p)->FileName) ^ ((p)->Line << 16) )

/* debug step state */
static int DebugStepNext = FALSE;

/* initialise the debugger by clearing the breakpoint table */
void DebugInit(Picoc *pc)
{
    TableInitTable(&pc->BreakpointTable, &pc->BreakpointHashTable[0], BREAKPOINT_TABLE_SIZE, TRUE);
    pc->BreakpointCount = 0;
}

/* free the contents of the breakpoint table */
void DebugCleanup(Picoc *pc)
{
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;
    int Count;

    for (Count = 0; Count < pc->BreakpointTable.Size; Count++)
    {
        for (Entry = pc->BreakpointHashTable[Count]; Entry != NULL; Entry = NextEntry)
        {
            NextEntry = Entry->Next;
            HeapFreeMem(pc, Entry);
        }
    }
}

/* search the table for a breakpoint */
static struct TableEntry *DebugTableSearchBreakpoint(struct ParseState *Parser, int *AddAt)
{
    struct TableEntry *Entry;
    Picoc *pc = Parser->pc;
    int HashValue = BREAKPOINT_HASH(Parser) % pc->BreakpointTable.Size;

    for (Entry = pc->BreakpointHashTable[HashValue]; Entry != NULL; Entry = Entry->Next)
    {
        if (Entry->p.b.FileName == Parser->FileName && Entry->p.b.Line == Parser->Line)
            return Entry;   /* found */
    }

    *AddAt = HashValue;    /* didn't find it in the chain */
    return NULL;
}

/* set a breakpoint in the table */
void DebugSetBreakpoint(struct ParseState *Parser)
{
    int AddAt;
    struct TableEntry *FoundEntry = DebugTableSearchBreakpoint(Parser, &AddAt);
    Picoc *pc = Parser->pc;

    if (FoundEntry == NULL)
    {
        /* add it to the table */
        struct TableEntry *NewEntry = HeapAllocMem(pc, sizeof(struct TableEntry));
        if (NewEntry == NULL)
            ProgramFailNoParser(pc, "out of memory");

        NewEntry->p.b.FileName = Parser->FileName;
        NewEntry->p.b.Line = Parser->Line;
        NewEntry->p.b.CharacterPos = Parser->CharacterPos;
        NewEntry->Next = pc->BreakpointHashTable[AddAt];
        pc->BreakpointHashTable[AddAt] = NewEntry;
        pc->BreakpointCount++;
    }
}

/* delete a breakpoint from the hash table */
int DebugClearBreakpoint(struct ParseState *Parser)
{
    struct TableEntry **EntryPtr;
    Picoc *pc = Parser->pc;
    int HashValue = BREAKPOINT_HASH(Parser) % pc->BreakpointTable.Size;

    for (EntryPtr = &pc->BreakpointHashTable[HashValue]; *EntryPtr != NULL; EntryPtr = &(*EntryPtr)->Next)
    {
        struct TableEntry *DeleteEntry = *EntryPtr;
        if (DeleteEntry->p.b.FileName == Parser->FileName && DeleteEntry->p.b.Line == Parser->Line)
        {
            *EntryPtr = DeleteEntry->Next;
            HeapFreeMem(pc, DeleteEntry);
            pc->BreakpointCount--;

            return TRUE;
        }
    }

    return FALSE;
}

/* handle a bkpt command: ":bkpt <filename> <line>" */
static void DebugHandleBkpt(Picoc *pc, const char *cmd, int set)
{
    const char *ptr;
    const char *filename_start;
    int filename_len;
    int line_no;
    char filename_buf[64];
    char *reg_file;
    struct ParseState bkpt_parser;

    ptr = cmd + 6; /* skip ":bkpt " or ":bkptc" then will skip "lear " */
    if (!set)
        ptr += 5; /* skip "lear " */

    /* skip leading space */
    while (*ptr == ' ')
        ptr++;

    /* extract filename */
    filename_start = ptr;
    while (*ptr != ' ' && *ptr != '\0')
        ptr++;
    filename_len = (int)(ptr - filename_start);
    if (filename_len <= 0 || filename_len >= (int)sizeof(filename_buf))
        return;

    memcpy(filename_buf, filename_start, (size_t)filename_len);
    filename_buf[filename_len] = '\0';

    /* extract line number */
    while (*ptr == ' ')
        ptr++;
    line_no = 0;
    while (*ptr >= '0' && *ptr <= '9')
    {
        line_no = line_no * 10 + (*ptr - '0');
        ptr++;
    }

    if (line_no <= 0)
        return;

    reg_file = TableStrRegister(pc, filename_buf);
    memset(&bkpt_parser, 0, sizeof(bkpt_parser));
    bkpt_parser.pc = pc;
    bkpt_parser.FileName = reg_file;
    bkpt_parser.Line = line_no;
    bkpt_parser.CharacterPos = 0;

    if (set)
        DebugSetBreakpoint(&bkpt_parser);
    else
        DebugClearBreakpoint(&bkpt_parser);

    PlatformPrintf(pc->CStdOut, ":ok %s\r\n", set ? "bkpt" : "bkptclear");
}

/* handle an eval command: ":eval <expression>" */
static void DebugHandleEval(Picoc *pc, const char *cmd)
{
    const char *expr;
    char exec_buf[512];
    int expr_len;
    int total_len;
    const char *prefix = "printf(\"%d\\n\",(";
    int prefix_len = (int)strlen(prefix);
    const char *suffix = "));\n";
    int suffix_len = (int)strlen(suffix);
    jmp_buf saved_exit_buf;

    expr = cmd + 6; /* skip ":eval " */
    while (*expr == ' ')
        expr++;

    expr_len = (int)strlen(expr);
    if (expr_len == 0)
    {
        PlatformPrintf(pc->CStdOut, ":err eval empty expression\r\n");
        return;
    }

    total_len = prefix_len + expr_len + suffix_len;
    if (total_len >= (int)sizeof(exec_buf))
    {
        PlatformPrintf(pc->CStdOut, ":err eval expression too long\r\n");
        return;
    }

    memcpy(exec_buf, prefix, (size_t)prefix_len);
    memcpy(exec_buf + prefix_len, expr, (size_t)expr_len);
    memcpy(exec_buf + prefix_len + expr_len, suffix, (size_t)suffix_len);
    exec_buf[total_len] = '\0';

    /* save and restore exit point so eval errors don't escape the debug loop */
    memcpy(&saved_exit_buf, &pc->PicocExitBuf, sizeof(jmp_buf));

    if (setjmp(pc->PicocExitBuf) == 0)
    {
        PicocParse(pc, "debug_eval", exec_buf, total_len, TRUE, TRUE, TRUE, FALSE);
        PlatformPrintf(pc->CStdOut, ":ok eval\r\n");
    }
    else
    {
        PlatformPrintf(pc->CStdOut, ":err eval failed\r\n");
    }

    memcpy(&pc->PicocExitBuf, &saved_exit_buf, sizeof(jmp_buf));
}

/* before we run a statement, check if there's anything we have to do with the debugger here */
void DebugCheckStatement(struct ParseState *Parser)
{
    int DoBreak = FALSE;
    int WasStep = FALSE;
    int AddAt;
    Picoc *pc = Parser->pc;
    char LineBuf[256];
    char *line;
    int len;

    /* check DebugStepNext first */
    if (DebugStepNext)
    {
        DoBreak = TRUE;
        DebugStepNext = FALSE;
        WasStep = TRUE;
    }

    /* has the user manually pressed break? */
    if (pc->DebugManualBreak)
    {
        DoBreak = TRUE;
        pc->DebugManualBreak = FALSE;
    }

    /* is this a breakpoint location? */
    if (pc->BreakpointCount != 0 && DebugTableSearchBreakpoint(Parser, &AddAt) != NULL)
        DoBreak = TRUE;

    /* handle a break */
    while (DoBreak)
    {
        PlatformPrintf(pc->CStdOut, "%s %s %d %d\r\n",
            WasStep ? ":step" : ":break",
            Parser->FileName ? Parser->FileName : "(none)",
            Parser->Line, Parser->CharacterPos);
        WasStep = FALSE;  /* reset for next break in loop */

        /* debug command loop */
        for (;;)
        {
            line = PlatformGetLineQuiet(LineBuf, sizeof(LineBuf));
            if (line == NULL)
                break;

            /* strip trailing newline */
            len = (int)strlen(line);
            while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
                line[--len] = '\0';

            /* skip empty lines (trailing \n from \r\n) */
            if (len == 0)
                continue;

            /* :cont - continue execution */
            if (len >= 5 && strncmp(line, ":cont", 5) == 0 && (len == 5 || line[5] == ' '))
                return;

            /* :step - single step */
            if (len >= 5 && strncmp(line, ":step", 5) == 0 && (len == 5 || line[5] == ' '))
            {
                DebugStepNext = TRUE;
                return;
            }

            /* :abort - abort execution */
            if (len == 6 && strncmp(line, ":abort", 6) == 0)
            {
                PlatformPrintf(pc->CStdOut, ":err load cancelled\r\n");
                longjmp(pc->PicocExitBuf, 1);
            }

            /* :bkptclear <filename> <line> - clear breakpoint */
            if (len >= 12 && strncmp(line, ":bkptclear ", 11) == 0)
            {
                DebugHandleBkpt(pc, line, FALSE);
                continue;
            }

            /* :bkpt <filename> <line> - set breakpoint */
            if (len >= 6 && strncmp(line, ":bkpt ", 6) == 0)
            {
                DebugHandleBkpt(pc, line, TRUE);
                continue;
            }

            /* :eval <expression> */
            if (len >= 6 && strncmp(line, ":eval ", 6) == 0)
            {
                DebugHandleEval(pc, line);
                continue;
            }

            /* unknown command - echo back */
            PlatformPrintf(pc->CStdOut, ":err debug unknown: %s\r\n", line);
        }

        /* PlatformGetLine returned NULL, exit debug loop */
        break;
    }
}

/* copy breakpoints from one Picoc instance to another */
void DebugCopyBreakpoints(Picoc *src, Picoc *dest)
{
    int Count;
    struct TableEntry *Entry;
    struct TableEntry *NextEntry;

    for (Count = 0; Count < src->BreakpointTable.Size; Count++)
    {
        for (Entry = src->BreakpointHashTable[Count]; Entry != NULL; Entry = NextEntry)
        {
            int DestHash;

            NextEntry = Entry->Next;

            struct TableEntry *NewEntry = HeapAllocMem(dest, sizeof(struct TableEntry));
            if (NewEntry == NULL)
                return;

            NewEntry->p.b.FileName = TableStrRegister(dest, Entry->p.b.FileName);
            NewEntry->p.b.Line = Entry->p.b.Line;
            NewEntry->p.b.CharacterPos = Entry->p.b.CharacterPos;

            DestHash = ((unsigned long)NewEntry->p.b.FileName ^
                        ((unsigned long)NewEntry->p.b.Line << 16)) %
                       (unsigned long)dest->BreakpointTable.Size;

            NewEntry->Next = dest->BreakpointHashTable[DestHash];
            dest->BreakpointHashTable[DestHash] = NewEntry;
            dest->BreakpointCount++;
        }
    }
}

void DebugStep()
{
    DebugStepNext = TRUE;
}

void DebugCancelStep(void)
{
    DebugStepNext = FALSE;
}

void DebugClearAllBreakpoints(Picoc *pc)
{
    DebugCleanup(pc);
    DebugInit(pc);
}
#endif /* !NO_DEBUGGER */

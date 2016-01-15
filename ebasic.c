/* ebasic.c - a simple basic interpreter
 *
 * Copyright (c) 2014 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include "db_compiler.h"
#include "db_image.h"
#include "db_vm.h"
#include "db_edit.h"

static uint8_t space[HEAPSIZE];

typedef struct {
    System *sys;
    ImageHdr *image;
} HandlerData;

/* command handlers */
static void DoRun(HandlerData *data);

/* command table */
UserCmd userCmds[] = {
{   "RUN",      (Handler *)DoRun    },
{   NULL,       NULL                }
};

void CompileAndExecute(HandlerData *data);

static int TermGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber);

int main(int argc, char *argv[])
{
    VMVALUE lineNumber = 0;
    HandlerData data;
    ImageHdr *image;
    System *sys;

    VM_printf("ebasic 0.003\n");

    VM_sysinit(argc, argv);

    sys = InitSystem(space, sizeof(space));
    sys->getLine = TermGetLine;
    sys->getLineCookie = &lineNumber;

    if (!(image = AllocateImage(sys, IMAGESIZE)))
        return 1;

    sys->freeMark = sys->freeNext;
     
    data.sys = sys;
    data.image = image;
    
    EditWorkspace(sys, userCmds, (Handler *)CompileAndExecute, &data);
    
    return 0;
}

static int EditGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber)
{
    return BufGetLine(pLineNumber, buf);
}

static void DoRun(HandlerData *data)
{
    System *sys = data->sys;
    ImageHdr *image = data->image;
    GetLineHandler *getLine;
    void *getLineCookie;
    VMVALUE code;

    getLine = sys->getLine;
    getLineCookie = sys->getLineCookie;
    
    sys->getLine = EditGetLine;

    BufSeekN(0);

    sys->freeNext = sys->freeMark;
    
    InitImage(image);

    if ((code = Compile(sys, image, VMFALSE)) != 0) {
        sys->freeNext = sys->freeMark;
        Execute(sys, image, code);
    }

    sys->getLine = getLine;
    sys->getLineCookie = getLineCookie;
}

void CompileAndExecute(HandlerData *data)
{
    System *sys = data->sys;
    ImageHdr *image = data->image;
    VMVALUE code;
    
    sys->freeNext = sys->freeMark;
    
    if ((code = Compile(sys, image, VMTRUE)) != 0) {
        sys->freeNext = sys->freeMark;
        Execute(sys, image, code);
    }
}

static int TermGetLine(void *cookie, char *buf, int len, VMVALUE *pLineNumber)
{
    VMVALUE *pLine = (VMVALUE *)cookie;
    *pLineNumber = ++(*pLine);
    return VM_getline(buf, len) != NULL;
}

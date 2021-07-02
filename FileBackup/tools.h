#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <fltKernel.h>

extern ULONG gTraceFlags;


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

UNICODE_STRING ExtractFileName(_In_ PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects);

#endif

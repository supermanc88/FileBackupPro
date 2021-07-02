#ifndef _BACKUP_FILE_H_
#define _BACKUP_FILE_H_

#include <fltKernel.h>

#define DOSDEVICE_LINK				L"\\DosDevices\\"
#define DOSDEVICE_LINK_LEN			12

#define BACKUP_MODE_WHEN_SAVE           1           // 当文件进行保存时，就备份
#define BACKUP_MODE_WHEN_INTERVAL       2           // 通过设置时间间隔来备份

NTSTATUS CopyFileToBackupPath(PWCHAR srcFilePath);

NTSTATUS CreateFileOrDir(PCFLT_RELATED_OBJECTS FltObjects, PWCHAR pwFilePath, BOOLEAN isFile);

NTSTATUS CreateDirectory(PCFLT_RELATED_OBJECTS FltObjects, PWCHAR pwFilePath);
#endif

#include "backup_file.h"
#include "setting.h"
#include "FileBackup.h"
#include "tools.h"


/**
* \brief CopyFileToBackupPath 将文件拷贝到指定的备份目录下，在备份同录下创建同文件路径层级一样的文件夹
* \param srcFilePath
* \return
*/
NTSTATUS CopyFileToBackupPath(PWCHAR srcFilePath)
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

/**
* \brief CreateFileOrDir 创建文件夹或文件
* \param FltObjects
* \param pwFilePath 文件或文件夹路径
* \param isFile TRUE创建文件，FALSE创建文件夹
* \return NTSTATUS
*/
NTSTATUS CreateFileOrDir(PCFLT_RELATED_OBJECTS FltObjects, PWCHAR pwFilePath, BOOLEAN isFile)
{
	HANDLE fileHandle = NULL;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uFilePath;
	NTSTATUS status = STATUS_SUCCESS;
	PWCHAR pwNameBuf = NULL;
	IO_STATUS_BLOCK ioStatus;

	pwNameBuf = (PWCHAR) ExAllocatePoolWithTag(NonPagedPool, PATH_MAX * 2, FILEBACKUP_TAG);

	if (pwNameBuf == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory(pwNameBuf, PATH_MAX * 2);
	RtlInitEmptyUnicodeString(&uFilePath, pwNameBuf, PATH_MAX);

	if (*(pwFilePath + 1) == L':')
	{
		RtlAppendUnicodeToString(&uFilePath, DOSDEVICE_LINK);
	}
	RtlAppendUnicodeToString(&uFilePath, pwFilePath);

	InitializeObjectAttributes(&oa, &uFilePath, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("!!! ---FileBackup.sys %s, pwFilePath = [ %ws ], isFile = [%x]\n", __FUNCTION__, pwFilePath, isFile));

	if (isFile)
	{
		status = FltCreateFile(FltObjects->FileObject,
								FltObjects->Instance,
								&fileHandle,
								SYNCHRONIZE | FILE_READ_ATTRIBUTES,
								&oa,
								&ioStatus,
								NULL,
								FILE_ATTRIBUTE_NORMAL,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								FILE_OPEN_IF,
								FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
								NULL,
								0,
								0);
	}
	else
	{
		status = FltCreateFile(FltObjects->FileObject,
								FltObjects->Instance,
								&fileHandle,
								SYNCHRONIZE | FILE_READ_ATTRIBUTES,
								&oa,
								&ioStatus,
								NULL,
								FILE_ATTRIBUTE_NORMAL,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								FILE_OPEN_IF,
								FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
								NULL,
								0,
								0);
	}
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("!!! ---FileBackup.sys %s, pwFilePath = [ %ws ], status = [%x]\n", __FUNCTION__, pwFilePath, status));

	ExFreePoolWithTag(pwNameBuf, FILEBACKUP_TAG);

	if (fileHandle != NULL)
	{
		FltClose(fileHandle);
	}

	return status;
}




/**
* \brief CreateDirectory 创建文件夹
* \param FltObjects
* \param pwFilePath pwFilePath pointer to the filepath, such as \??\C:\XXXX\YYY\
* \return NTSTATUS
*/
NTSTATUS CreateDirectory(PCFLT_RELATED_OBJECTS FltObjects, IN PWCHAR pwFilePath)
{
	NTSTATUS status = STATUS_SUCCESS;
	PWCHAR pwDir = NULL;

	PWCHAR pwDirIndex = pwFilePath;
	ULONG nPathLen = (ULONG) wcslen(pwFilePath);

	PWCHAR pwDirEnd = pwDirIndex + nPathLen;
	ULONG nSlashNum = 0;

	pwDir = (PWCHAR) ExAllocatePoolWithTag(NonPagedPool, PATH_MAX * 2, FILEBACKUP_TAG);

	if (pwDir == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// 从C:\XXXX\开始创建

	pwDirIndex++;

	while(pwDirIndex < pwDirEnd)
	{
		if (*pwDirIndex == L':')
		{
			nSlashNum = 0;
		}
		if (*pwDirIndex == L'\\')
		{
			nSlashNum++;
		}

		if (nSlashNum == 2)
		{
			break;
		}

		pwDirIndex++;
	}

	// 一层接一层的创建文件夹
	while(pwDirIndex < pwDirEnd)
	{
		if (*pwDirIndex == L'\\')
		{
			ULONG count = pwDirIndex - pwFilePath;
			RtlCopyMemory(pwDir,
						 pwFilePath,
						 count * sizeof(WCHAR));

			pwDir[count] = L'\0';
			status = CreateFileOrDir(FltObjects, pwFilePath, FALSE);

		}
		pwDirIndex++;

	}

	ExFreePoolWithTag(pwDir, FILEBACKUP_TAG);

	return status;
}

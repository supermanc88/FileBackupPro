#include "tools.h"
#include "FileBackup.h"


/**
* \brief ExtractFileName miniFilter 获取文件路径
* \param Data
* \return
*/
UNICODE_STRING ExtractFileName(_In_ PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects)
{
	NTSTATUS status;
	UNICODE_STRING ret = {0};


	KIRQL irql = KeGetCurrentIrql();
	if (irql > APC_LEVEL)
	{
		return ret;
	}

	if (Data)
	{
		PFLT_FILE_NAME_INFORMATION pNameInfo = NULL;

		status = FltGetFileNameInformation( Data,
											FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT,
											&pNameInfo);

		if (NT_SUCCESS(status) && pNameInfo)
		{
			status = FltParseFileNameInformation(pNameInfo);

			if (NT_SUCCESS(status))
			{
				/** Init(&ret, pNameInfo->Name.MaximumLength); */
				// 由于这块的内存是动态申请的，所以要在用完之后手动释放
				ret.MaximumLength = pNameInfo->Name.MaximumLength;
				ret.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, ret.MaximumLength, FILEBACKUP_TAG);
				/** RtlCopyUnicodeString(&ret, &pNameInfo->Name); */

				PDEVICE_OBJECT devObj = NULL;
				UNICODE_STRING dosName;

				status = FltGetDiskDeviceObject(FltObjects->Volume, &devObj);

				if (NT_SUCCESS(status))
				{
					status = IoVolumeDeviceToDosName(devObj, &dosName);
					PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("!!! FileBackup.sys --- %s, dosName = [%wZ]\n", __FUNCTION__, &dosName));

					RtlAppendUnicodeStringToString(&ret, &dosName);
					RtlAppendUnicodeToString(&ret, pNameInfo->Name.Buffer + pNameInfo->Volume.Length / 2);
				}

				if (devObj)
				{
					ObDereferenceObject(devObj);
				}

				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("!!! FileBackup.sys --- %s, volume = [%wZ], volume_len = [%d]\n", __FUNCTION__, &pNameInfo->Volume,
													pNameInfo->Volume.Length));
			}
			else
			{
				PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FltParseFileNameInformation Error : [0x%p]\n", status));
			}

			FltReleaseFileNameInformation(pNameInfo);
		}
	}

	return ret;
}

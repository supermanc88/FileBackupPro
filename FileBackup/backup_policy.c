#include "backup_policy.h"

#include <fltKernel.h>

// 可设置3种类型的策略

// 1. 备份间隔时间
ULONG g_interval = 0;

// 2. 备份指定目录, 以 | 分隔
char g_backup_path[4096] = { 0 };

// 3. 备份指定扩展名, 以 | 分隔
char g_file_ext[1024] = { 0 };

/**
* \brief ParseBackupPolicy 解析备份策略
* \param backup_policy 控制端传入的策略
* \return
*/
NTSTATUS ParseBackupPolicy(char *backup_policy)
{
	NTSTATUS status = STATUS_SUCCESS;



	return status;
}

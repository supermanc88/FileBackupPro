/*++

Module Name:

    FileBackup.c

Abstract:

    This is the main module of the FileBackup miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>



#include "FileBackup.h"
#include "tools.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

ULONG gTraceFlags = 1;

FILEBACKUP_DATA FileBackupData;

/*************************************************************************
    Prototypes
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
FileBackupInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
FileBackupInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
FileBackupInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
FileBackupUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
FileBackupInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FileBackupPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

VOID
FileBackupOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    );

FLT_POSTOP_CALLBACK_STATUS
FileBackupPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
FileBackupPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

BOOLEAN
FileBackupDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    );

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FileBackupUnload)
#pragma alloc_text(PAGE, FileBackupInstanceQueryTeardown)
#pragma alloc_text(PAGE, FileBackupInstanceSetup)
#pragma alloc_text(PAGE, FileBackupInstanceTeardownStart)
#pragma alloc_text(PAGE, FileBackupInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

    { IRP_MJ_CREATE,
      0,
      FileBackupPreCreate,
      FileBackupPostCreate },

    { IRP_MJ_CLOSE,
      0,
      FileBackupPreClose,
      FileBackupPostClose },

    { IRP_MJ_READ,
      0,
      FileBackupPreRead,
      FileBackupPostRead },

    { IRP_MJ_WRITE,
      0,
      FileBackupPreWrite,
      FileBackupPostWrite },

#if 0 // TODO - List all of the requests to filter.
    { IRP_MJ_CREATE_NAMED_PIPE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_QUERY_INFORMATION,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_SET_INFORMATION,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_QUERY_EA,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_SET_EA,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_FLUSH_BUFFERS,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_QUERY_VOLUME_INFORMATION,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_SET_VOLUME_INFORMATION,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_DIRECTORY_CONTROL,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_FILE_SYSTEM_CONTROL,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_DEVICE_CONTROL,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_INTERNAL_DEVICE_CONTROL,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_SHUTDOWN,
      0,
      FileBackupPreOperationNoPostOperation,
      NULL },                               //post operations not supported

    { IRP_MJ_LOCK_CONTROL,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_CLEANUP,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_CREATE_MAILSLOT,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_QUERY_SECURITY,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_SET_SECURITY,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_QUERY_QUOTA,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_SET_QUOTA,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_PNP,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_ACQUIRE_FOR_MOD_WRITE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_RELEASE_FOR_MOD_WRITE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_ACQUIRE_FOR_CC_FLUSH,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_RELEASE_FOR_CC_FLUSH,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_NETWORK_QUERY_OPEN,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_MDL_READ,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_MDL_READ_COMPLETE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_PREPARE_MDL_WRITE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_MDL_WRITE_COMPLETE,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_VOLUME_MOUNT,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

    { IRP_MJ_VOLUME_DISMOUNT,
      0,
      FileBackupPreOperation,
      FileBackupPostOperation },

#endif // TODO

    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    FileBackupUnload,                           //  MiniFilterUnload

    FileBackupInstanceSetup,                    //  InstanceSetup
    FileBackupInstanceQueryTeardown,            //  InstanceQueryTeardown
    FileBackupInstanceTeardownStart,            //  InstanceTeardownStart
    FileBackupInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
FileBackupInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    /** UNREFERENCED_PARAMETER( FltObjects ); */
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupInstanceSetup: Entered\n") );

	PDEVICE_OBJECT devObj = NULL;
	NTSTATUS status = STATUS_SUCCESS;
	UNICODE_STRING dosName;

	status = FltGetDiskDeviceObject(FltObjects->Volume, &devObj);

	if (NT_SUCCESS(status))
	{
		status = IoVolumeDeviceToDosName(devObj, &dosName);
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("!!! FileBackup.sys --- %s, dosName = [%wZ]\n", __FUNCTION__, &dosName));
	}

	if (devObj)
	{
		ObDereferenceObject(devObj);
	}

    return STATUS_SUCCESS;
}


NTSTATUS
FileBackupInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
FileBackupInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupInstanceTeardownStart: Entered\n") );
}


VOID
FileBackupInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupInstanceTeardownComplete: Entered\n") );
}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    NTSTATUS status;
	OBJECT_ATTRIBUTES oa;
	PSECURITY_DESCRIPTOR sd;
	UNICODE_STRING uniString;

    UNREFERENCED_PARAMETER( RegistryPath );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!DriverEntry: Entered\n") );

    //
    //  Register with FltMgr to tell it our callback routines
    //

    status = FltRegisterFilter( DriverObject,
                                &FilterRegistration,
                                &gFilterHandle );

	FileBackupData.Filter = gFilterHandle;

	status = FltBuildDefaultSecurityDescriptor( &sd, FLT_PORT_ALL_ACCESS);

	RtlInitUnicodeString( &uniString, FILEBACKUP_PORTNAME);

	if (NT_SUCCESS(status)) {
		InitializeObjectAttributes( &oa,
									&uniString,
									OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
									NULL,
									sd);

		status = FltCreateCommunicationPort( FileBackupData.Filter,
											 &FileBackupData.ServerPort,
											 &oa,
											 NULL,
											 FileBackupPortConnect,
											 FileBackupPortDisconnect,
											 FileBackupPortNotify,
											 1);

		FltFreeSecurityDescriptor( sd );
	}

    FLT_ASSERT( NT_SUCCESS( status ) );

    if (NT_SUCCESS( status )) {

        //
        //  Start filtering i/o
        //

        status = FltStartFiltering( gFilterHandle );

        if (!NT_SUCCESS( status )) {

			FltCloseCommunicationPort( FileBackupData.ServerPort );
            FltUnregisterFilter( gFilterHandle );
        }
    }


    return status;
}

NTSTATUS
FileBackupUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupUnload: Entered\n") );

	FltCloseCommunicationPort( FileBackupData.ServerPort );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
*************************************************************************/
FLT_PREOP_CALLBACK_STATUS
FileBackupPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupPreOperation: Entered\n") );

    //
    //  See if this is an operation we would like the operation status
    //  for.  If so request it.
    //
    //  NOTE: most filters do NOT need to do this.  You only need to make
    //        this call if, for example, you need to know if the oplock was
    //        actually granted.
    //

    if (FileBackupDoRequestOperationStatus( Data )) {

        status = FltRequestOperationStatusCallback( Data,
                                                    FileBackupOperationStatusCallback,
                                                    (PVOID)(++OperationStatusCtx) );
        if (!NT_SUCCESS(status)) {

            PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                          ("FileBackup!FileBackupPreOperation: FltRequestOperationStatusCallback Failed, status=%08x\n",
                           status) );
        }
    }

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_WITH_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}



VOID
FileBackupOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("FileBackup!FileBackupOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}


FLT_POSTOP_CALLBACK_STATUS
FileBackupPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupPostOperation: Entered\n") );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
FileBackupPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("FileBackup!FileBackupPreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
FileBackupDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}

/**
* \brief FileBackupPortConnect
* \param ClientPort
* \param ServerPortCookie
* \param SizeOfContext
* \param SizeOfContext
* \param
* \return
*/
NTSTATUS FileBackupPortConnect (
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionCookie
    )
{
	FileBackupData.ClientPort = ClientPort;

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- connect, port = [0x%p]\n", ClientPort) );

	return STATUS_SUCCESS;
}


/**
* \brief FileBackupPortDisconnect
* \param
* \return
*/
VOID FileBackupPortDisconnect(
     _In_opt_ PVOID ConnectionCookie
     )
{
	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("!!! FileBackup.sys --- disconnected, port = [0x%p]\n", FileBackupData.ClientPort) );


	FltCloseClientPort( FileBackupData.Filter, &FileBackupData.ClientPort );
}

/**
* \brief FileBackupPortNotify
* \param PortCookie
* \param InputBufferLength
* \param InputBufferLength
* \param OutputBufferLength,*ReturnOutputBufferLength
* \param OutputBufferLength
* \param
* \return
*/
NTSTATUS FileBackupPortNotify (
    _In_opt_ PVOID PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength,*ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
    )
{
	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return STATUS_SUCCESS;
}

/**
* \brief FileBackupPreCreate
* \param Data
* \param FltObjects
* \param
* \return
*/
FLT_PREOP_CALLBACK_STATUS FileBackupPreCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s start\n", __FUNCTION__ ) );

	if (FlagOn( Data->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {
		PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s current open dir = [%wZ]\n", __FUNCTION__, &FltObjects->FileObject->FileName) );
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	UNICODE_STRING fileName = ExtractFileName(Data, FltObjects);
	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s current open fileName = [%wZ]\n", __FUNCTION__, &fileName) );

	// 用完就释放掉
	RtlFreeUnicodeString(&fileName);

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


/**
* \brief FileBackupPostCreate
* \param Data
* \param FltObjects
* \param CompletionContext
* \param
* \return
*/
FLT_POSTOP_CALLBACK_STATUS FileBackupPostCreate (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return FLT_POSTOP_FINISHED_PROCESSING;
}

/**
* \brief FileBackupPreClose
* \param Data
* \param FltObjects
* \param
* \return
*/
FLT_PREOP_CALLBACK_STATUS FileBackupPreClose (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


/**
* \brief FileBackupPostClose
* \param Data
* \param FltObjects
* \param CompletionContext
* \param
* \return
*/
FLT_POSTOP_CALLBACK_STATUS FileBackupPostClose (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return FLT_POSTOP_FINISHED_PROCESSING;
}

/**
* \brief FileBackupPreRead
* \param Data
* \param FltObjects
* \param
* \return
*/
FLT_PREOP_CALLBACK_STATUS FileBackupPreRead (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


/**
* \brief FileBackupPostRead
* \param Data
* \param FltObjects
* \param CompletionContext
* \param
* \return
*/
FLT_POSTOP_CALLBACK_STATUS FileBackupPostRead (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return FLT_POSTOP_FINISHED_PROCESSING;
}

/**
* \brief FileBackupPreWrite
* \param Data
* \param FltObjects
* \param
* \return
*/
FLT_PREOP_CALLBACK_STATUS FileBackupPreWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;
	ULONG writeLen = iopb->Parameters.Write.Length;
	LARGE_INTEGER writeOffset = iopb->Parameters.Write.ByteOffset;

	PVOID writeBuffer = NULL;

	if (iopb->Parameters.Write.MdlAddress != NULL)
	{
		writeBuffer = MmGetSystemAddressForMdlSafe( iopb->Parameters.Write.MdlAddress, NormalPagePriority | MdlMappingNoExecute );
	}
	else
	{
		writeBuffer = iopb->Parameters.Write.WriteBuffer;
	}

	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s, writeLen = [%d], offset = [%d]\n", __FUNCTION__, writeLen, writeOffset.u.LowPart ) );

    // 打印写入内容
	// int i;
	// for (i = 0; i < writeLen; i++)
	// {
	// 	KdPrint(("0x%02x ", *((UCHAR *)writeBuffer + 1) ));
	// 	if ( i != 0 && i % 16 == 0 )
	// 	{
	// 		KdPrint(("\n"));
	// 	}
	// }
    
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("!!! FileBackup.sys --- %s, teststring\n", __FUNCTION__));
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


/**
* \brief FileBackupPostWrite
* \param Data
* \param FltObjects
* \param CompletionContext
* \param
* \return
*/
FLT_POSTOP_CALLBACK_STATUS FileBackupPostWrite (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
{
	PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ( "!!! FileBackup.sys --- %s\n", __FUNCTION__ ) );
	return FLT_POSTOP_FINISHED_PROCESSING;
}

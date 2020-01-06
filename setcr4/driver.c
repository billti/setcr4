// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file

#if !defined(_M_AMD64)
#error This file only targets the AMD64 architecture
#else
#define _AMD64_  // Needed for some of the includes
#endif

#include <ntddk.h>
#include <intrin.h>
// Ref: https://docs.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list

#include "driver.h"

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
DRIVER_DISPATCH DriverIoCtrl;
DRIVER_DISPATCH DriverCreateClose;

NTSTATUS DriverEntry(DRIVER_OBJECT* driver_object, UNICODE_STRING* registry_path) {
  NTSTATUS nt_status = STATUS_SUCCESS;
  UNICODE_STRING device_name, dos_device_name;
  DEVICE_OBJECT* device_object = NULL;

  DbgPrint("DriverEntry for " DRIVER_NAME " called\n");

  RtlInitUnicodeString(&device_name, L"\\Device\\" DRIVER_NAME);

  // See https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/initializing-a-device-object
  nt_status = IoCreateDevice(driver_object,
      0,  /* DeviceExtensionSize */
      &device_name,
      FILE_DEVICE_UNKNOWN,
      FILE_DEVICE_SECURE_OPEN,
      FALSE, /* Exclusive */
      &device_object);

  if (nt_status != STATUS_SUCCESS) {
    DbgPrint("IoCreateDevice failed with: %#08x\n", nt_status);
    return nt_status;
  }

  driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoCtrl;
  driver_object->MajorFunction[IRP_MJ_CLOSE]          = DriverCreateClose;
  driver_object->MajorFunction[IRP_MJ_CREATE]         = DriverCreateClose;
  driver_object->DriverUnload = DriverUnload;

  return nt_status;
}

NTSTATUS DriverIoCtrl(DEVICE_OBJECT* device_object, IRP* irp) {
  // https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/buffer-descriptions-for-i-o-control-codes

  NTSTATUS nt_status = STATUS_SUCCESS;

  PIO_STACK_LOCATION irp_loc = IoGetCurrentIrpStackLocation(irp);
  if (irp_loc->MajorFunction != IRP_MJ_DEVICE_CONTROL) {
    DbgPrint("Unexpected MajorFunction in DriverIoCtrl! %d\n", irp_loc->MajorFunction);
    // TODO: Should the early returns with errors also complete the IRP?
    return STATUS_INVALID_DEVICE_REQUEST;
  }
  void* buffer         = irp->AssociatedIrp.SystemBuffer;
  ULONG ctrl_code      = irp_loc->Parameters.DeviceIoControl.IoControlCode;
  ULONG out_buf_length = irp_loc->Parameters.DeviceIoControl.OutputBufferLength;

  const unsigned __int64 PCE_BIT = 1 << 8;
  unsigned __int64 cr4 = __readcr4();

  switch(ctrl_code) {
    case IOCTL_READ_CR4:
      // Reporting the value is done by default
      break;
    case IOCTL_ENABLE_PCE:
      cr4 |= PCE_BIT;
      __writecr4(cr4);
      break;
    case IOCTL_DISABLE_PCE:
      cr4 &= ~PCE_BIT;
      __writecr4(cr4);
      break;
    default:
      DbgPrint("Unexpected control code! %d\n", ctrl_code);
      return STATUS_INVALID_PARAMETER;
  }

  if (out_buf_length < sizeof(cr4)) return STATUS_BUFFER_TOO_SMALL;

  cr4 = __readcr4();
  *(unsigned __int64*)buffer = cr4;
  irp->IoStatus.Information = sizeof(cr4);
  irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(irp, IO_NO_INCREMENT);

  return nt_status;
}

NTSTATUS DriverCreateClose(DEVICE_OBJECT* device_object, IRP* irp) {
  irp->IoStatus.Status = STATUS_SUCCESS;
  irp->IoStatus.Information = 0;

  IoCompleteRequest(irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

void DriverUnload(DRIVER_OBJECT* driver_object) {
  DEVICE_OBJECT* device_object = driver_object->DeviceObject;
  if (device_object != NULL) IoDeleteDevice(device_object);
}

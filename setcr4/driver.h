// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file


// Drivers should include <ntddk.h> first. User-mode code <winioctl.h>
// See https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/defining-i-o-control-codes

#define DRIVER_NAME "setcr4"
#define SETCR4TYPE 0x8000

#define IOCTL_READ_CR4 \
    CTL_CODE(SETCR4TYPE, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_ENABLE_PCE \
    CTL_CODE(SETCR4TYPE, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DISABLE_PCE \
    CTL_CODE(SETCR4TYPE, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)

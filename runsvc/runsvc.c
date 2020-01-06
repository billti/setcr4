// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file

#if !defined(_M_AMD64)
#error This code only targets x64
#endif

#include <Windows.h>
#include <winioctl.h>
#include "..\setcr4\driver.h"

HANDLE std_out, std_in;
HANDLE hSCManager, hService, hDevice;

// 'Print' is needed as no CRT is linked in (i.e. 'printf', etc. are not available)
void Print(const char* str,...) {
  DWORD bytes_written;
  int buf_chars = 0;
  char buf[1024];  // 1024 is the max that wvsprintf will write.

  va_list ap;
  va_start(ap, str);
  buf_chars = wvsprintfA(buf, str, ap);
  va_end(ap);

  WriteFile(std_out, buf, (DWORD)buf_chars, &bytes_written, NULL);
}

BOOL LoadDriver() {
  if (!hSCManager) hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (hSCManager == NULL) {
    Print("Unable to open the service manager\n");
    return FALSE;
  }

  hService = CreateServiceA(hSCManager, DRIVER_NAME,
                            DRIVER_NAME,
                            SERVICE_ALL_ACCESS,
                            SERVICE_KERNEL_DRIVER,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_NORMAL,
                            "C:\\temp\\" DRIVER_NAME ".sys",  // TODO: Make relative to .exe
                            NULL, NULL, NULL, NULL, NULL);

  if(!hService)
  {
    hService = OpenServiceA(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
    if (!hService) {
      Print("Unable to create or open the setcr4 service\n");
      return FALSE;
    }
  }

  if(!StartServiceA(hService, 0, NULL)) {
    DeleteService(hService);
    CloseServiceHandle(hService);
    hService = NULL;
    Print("Unable to start the service\n");
    return FALSE;
  }

  return TRUE;
}

BOOL UnloadDriver() {
  if (hDevice) CloseHandle(hDevice);
  if (!hSCManager) hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
  if (!hSCManager) {
    Print("Unable to open the service manager\n");
    return FALSE;
  }
  if (!hService) {
    hService = OpenServiceA(hSCManager, DRIVER_NAME, SERVICE_ALL_ACCESS);
    if (!hService) {
      Print("Unable to open the setcr4 service\n");
      return FALSE;
    }
  }
  SERVICE_STATUS service_status;
  if (!ControlService(hService, SERVICE_CONTROL_STOP, &service_status)) {
    Print("ControlService failed to stop the service. Error: %d\n", GetLastError());
    return FALSE;
  }
  if (!DeleteService(hService)) {
    Print("Failed to delete the service\n");
  }

  CloseServiceHandle(hService);
  CloseServiceHandle(hSCManager);
  hService = NULL;
  hSCManager = NULL;
  return TRUE;
}

int main(void)
{
  hSCManager = NULL;
  hService = NULL;
  hDevice = NULL;
  char device_buffer[8];

  std_out = GetStdHandle(STD_OUTPUT_HANDLE);
  std_in  = GetStdHandle(STD_INPUT_HANDLE);
  if (std_out == INVALID_HANDLE_VALUE || std_in == INVALID_HANDLE_VALUE)
    return 1;  // Unable to report an error message with no STDOUT

  if (!LoadDriver()) {
    Print("Unable to load the kernel-mode driver!\n");
    return 2;
  }

  Sleep(1000);  // Give it a second to load. Should be plenty.

  hDevice = CreateFile("\\\\.\\GLOBALROOT\\Device\\" DRIVER_NAME,
                       GENERIC_READ | GENERIC_WRITE,
                       0, // dwShareMode - prevent other processes from opening
                       NULL, // lpSecurityAttributes
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
  if (hDevice == INVALID_HANDLE_VALUE) {
    Print("Unable to open the device. Error: %d\n", GetLastError());
    return 3;
  }

  Print("The SETCR4 kernel-mode driver is loaded\n");
  Print("Press 'r' to read CR4, 'e' to enable PCRs, 'd' to disable PCRs, and 'x' to exit\n");

  INPUT_RECORD input_records;
  DWORD records_read;
  while (1) {
    if (!ReadConsoleInputA(std_in, &input_records, 1, &records_read)) {
      Print("Error reading keyboard\n");
      ExitProcess(-2);
    }
    if (input_records.EventType != KEY_EVENT) continue;
    KEY_EVENT_RECORD* key_event = &input_records.Event.KeyEvent;
    if (!key_event->bKeyDown) continue;

    switch (key_event->uChar.AsciiChar) {
      case 'x':
        Print("x pressed, exiting\n");
        UnloadDriver();
        ExitProcess(0);
        break;
      case 'r':
        Print("r pressed, reading CR4...\n");
        if(!DeviceIoControl(hDevice, IOCTL_READ_CR4,
                            device_buffer, 8, device_buffer, 8,
                            &records_read, NULL)) {
          Print("Error calling DeviceIoControl: %d\n", GetLastError());
        } else {
          Print("CR4 value: %#08x\n", *(unsigned __int64*)device_buffer);
        }
        break;
      case 'e':
        Print("e pressed, enabling PCRs...\n");
        if(!DeviceIoControl(hDevice, IOCTL_ENABLE_PCE,
                            device_buffer, 8, device_buffer, 8,
                            &records_read, NULL)) {
          Print("Error calling DeviceIoControl: %d\n", GetLastError());
        } else {
          Print("CR4 value: %#08x\n", *(unsigned __int64*)device_buffer);
        }
        break;
      case 'd':
        Print("d pressed, disabling PCRs...\n");
        if(!DeviceIoControl(hDevice, IOCTL_DISABLE_PCE,
                            device_buffer, 8, device_buffer, 8,
                            &records_read, NULL)) {
          Print("Error calling DeviceIoControl: %d\n", GetLastError());
        } else {
          Print("CR4 value: %#08x\n", *(unsigned __int64*)device_buffer);
        }
        break;
      default:
        Print("Unknown key pressed\n");
        break;
    }
  }

  return -1;  // Should never get here
}

// Copyright 2020 Bill Ticehurst. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file

#if !defined(_M_AMD64)
#error This code only targets x64
#endif

#include <Windows.h>
#include <intrin.h>
// Ref: https://docs.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list

HANDLE std_out, std_in;

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

int main() {
  std_out = GetStdHandle(STD_OUTPUT_HANDLE);
  std_in  = GetStdHandle(STD_INPUT_HANDLE);
  if (std_out == INVALID_HANDLE_VALUE || std_in == INVALID_HANDLE_VALUE)
    return 1;  // Unable to report an error message with no STDOUT

  int cpuid[4];  // Register order is: EAX, EBX, ECX, EDX
  __cpuid(cpuid, 0);
  Print("cpuid(0): %#08x, %#08x, %#08x, %#08x\n", cpuid[0], cpuid[1], cpuid[2], cpuid[3]);
  Print("Maximum CPU leaf: %d\n", cpuid[0]);
  if (cpuid[2] == 0x6c65746e) {
    Print("GenuineIntel\n");
  }
  if (cpuid[2] == 0x444D4163) {
    Print("AuthenticAMD\n");
  }
  if (cpuid[0] < 1) return -1;  // Should never happen

  __cpuid(cpuid, 1);
  int family_id = (cpuid[0] >> 8) & 0x0F;
  int model = (cpuid[0] >> 4 ) & 0x0F;
  if (family_id == 0x0F || family_id == 0x06) model += ((cpuid[0] >> 12) & 0xF0);
  if (family_id == 0x0F) family_id += ((cpuid[0] >> 20) & 0xFF);
  Print("cpuid(1): %#08x, %#08x, %#08x, %#08x\n", cpuid[0], cpuid[1], cpuid[2], cpuid[3]);
  Print("DisplayFamily_DisplayModel: %02x_%02x\n", family_id, model);

  // See Intel Software Developer's Manual Vol 4: Model Specific Registers, Table 2-1.
  if (family_id == 0x06 && (model == 0x8e || model == 0x9e)) {
    Print("7th, 8th, or 9th gen Intel Core processor (Kaby Lake or Coffee Lake)\n");
  }
  if (family_id == 0x06 && (model == 0x7d || model == 0x7e)) {
    Print("10th gen Intel Core processor (Ice Lake)\n");
  }
  // TODO: Extract feature support from ECX and EDX.
  return 0;
}

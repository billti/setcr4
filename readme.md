# Experiments with CPU perf counters and device drivers

Sample code to allow reading and writing of Intel CPU performance registers.

This code takes inspiration from a number of sources, but mainly:

- The Intel Systems Programming Guide, Chapter 18 & 19 ("Performance Monitoring"
and "Performance Monitoring Events").
- Agner Fog's optimization resources, mainly the TestP package (<https://www.agner.org/optimize/#testp>)
- The Windows Driver Samples, mainly <https://github.com/microsoft/Windows-driver-samples/tree/master/general/ioctl/wdm>

## Code layout

There are 3 distinct pieces of code in this repository:

- `setcr4`: A kernel driver which configures the CPU's CR4 register to enable
user-mode code to read/write the performance counter registers.
- `runsvc`: A user-mode process to load the above driver, and set the right CPU flags.
- `testapp`: A sample program that uses the CPU perf counters enabled above.

## Building

Ensure the Visual Studio C++ compiler and the Windows DDK are installed. Then open
a 64-bit developer command prompt and run `nmake` from the root of the repo.

## Notes

By default, an unsigned driver cannot be loaded on Windows 10. Press F8 when
booting the OS and select to disable driver signature verification. (Alternately,
attach a kernel debugger to the system under test, which also disables it).

## Usage

- After building, copy the binary files from `.\bin\debug` to `C:\temp`
- Open an Admin command prompt and change the current directory to `C:\temp`.
- Run the `runsvc.exe` utility, which should indicate the driver was loaded.
- Press `e` to enable the Performance Counter Registers (PCRs).
  - (Bit 8 of the CR4 register should get set, e.g. from `0x370678` to `0x370778`).
- Run the code to test, which now should be able to make use of the `RDPMC` opcode.
- When done, disable the PCRs and unload the driver (press 'd' then 'x' in `runsvc.exe`).

## TODO

- Implement `testapp` using the counters.

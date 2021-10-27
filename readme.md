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
user-mode code to read/write the performance counter registers (via RDPMC).
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

## CPU perf counters

- RDTCP and RDTSCP are different in that the former does not imply any serialization
  of instructions (i.e. CPUID or *FENCE instructions should be used). RDTSCP waits
  until all previous instructions have completed and previous loads are globally
  visible.
- To have RDTSCP complete before any subsequent instructions, execute an LFENCE
  immediately after the RDTSCP.
- RDPMC does not imply any serialization; preceding instructions may not have completed,
  and subsequent instructions may have begun. Use CPUID of *FENCE to avoid this.
- Per the Intel manual: LFENCE does not execute until all prior instructions have
  completed, and no later instructions begin until LFENCE completes.
- CPUID serializes the instruction stream, i.e. any modifications to flags, registers,
  or memory for previous instructions, are completed before the next instruction
  is fetched and executed.
- CPUID, LFENCE, and RDTSC/RDTSCP can all be executed on Windows in a user-mode
  process by default. RDPMC can only be done by setting a bit on CR4 - which requires
  privilege level 0 (to call "mov cr4, /r").
- Support for RDTSCP is indicated by CPUID.80000001H:EDX[27].
- To configure the PMC events to monitor, model specific registers must be set via
  WRMSR, which requires privilege level 0.
- On recent processors, the TSC counter increments at a constant rate, is monotonically
  increasing, and will not wrap for at least 10 years.
- Processor's support for invariant TSC is indicated by CPUID.80000007H:EDX[8].
  The invariant TSC will run at a constant rate in all ACPI P-, C-. and T-states.
  This is the architectural behavior moving forward.
- Looks like RDPMC and RDTSCP should both take ~36 cycles.

## TODO

- Implement `testapp` using the counters.
- Add the code to read the architectural counters.
- Enable passing messages to the driver to configure the model-specific counters.
- Correct the order:
  - RDTSCP
  - LFENCE
  - save values
  - execute code under test
  - LFENCE
  - RDTSCP
- Use SetThreadAffinity and SetThreadPriority to keep it pinned to a core and unlikely
  to interrupt.

## From 18.2 of the Intel systems programming manual

Software queries the CPUID.0AH for the version identifier first; it then analyzes
the value returned in CPUID.0AH.EAX, CPUID.0AH.EBX to determine the facilities available.

IA32_PMCx MSRs start at address 0C1H and occupy a contiguous block of MSR address space; the number of
MSRs per logical processor is reported using CPUID.0AH:EAX[15:8].

IA32_PERFEVTSELx MSRs start at address 186H and occupy a contiguous block of MSR address space. Each
performance event select register is paired with a corresponding performance counter in the 0C1H address
block.

The bit width of an IA32_PMCx MSR is reported using the CPUID.0AH:EAX[23:16]. This the number of valid bits
for read operation. On write operations, the lower-order 32 bits of the MSR may be written with any value, and
the high-order bits are sign-extended from the value of bit 31.

Bit field layout of IA32_PERFEVTSELx MSRs is defined architecturally.


## From 18.7.3

For Intel processors in which the nominal core crystal clock frequency is enumerated in CPUID.15H.ECX and the
core crystal clock ratio is encoded in CPUID.15H (see Table 3-8 “Information Returned by CPUID Instruction”), the
nominal TSC frequency can be determined by using the following equation:
Nominal TSC frequency = ( CPUID.15H.ECX[31:0] * CPUID.15H.EBX[31:0] ) ÷ CPUID.15H.EAX[31:0]

## From CPUID instruction details

15H NOTES:
If EBX[31:0] is 0, the TSC/"core crystal clock" ratio is not enumerated.
If ECX is 0, the nominal core crystal clock frequency is not enumerated.
EBX[31:0]/EAX[31:0] indicates the ratio of the TSC frequency and the core crystal clock frequency.
"TSC frequency" = "core crystal clock frequency" * EBX/EAX.
The core crystal clock may differ from the reference clock, bus clock, or core clock frequencies.

EAX Bits 31 - 00: An unsigned integer which is the denominator of the TSC/"core crystal clock" ratio.
EBX Bits 31 - 00: An unsigned integer which is the numerator of the TSC/"core crystal clock" ratio.
ECX Bits 31 - 00: An unsigned integer which is the nominal frequency of the core crystal clock in Hz.

16H NOTES:
EAX Bits 15 - 00: Processor Base Frequency (in MHz).
EBX Bits 15 - 00: Maximum Frequency (in MHz).
ECX Bits 15 - 00: Bus (Reference) Frequency (in MHz).

NOTE: Data is returned from this interface in accordance with the processor's specification and does not reflect
actual values.

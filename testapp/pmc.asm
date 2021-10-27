; Routines to read the Performance Monitor Counters of Intel CPUs

.data

align 16
pi real4 3.141562564, 1.0, 0.5, 0.75
pi2 real4 1.0, 0.75, 0.5, 0.25

.code

; TODO: CPUID.0Ah:EAX[7:0] to determine capabilities of Architectural Performance Monitoring
; i.e. Number & width of counter registers, and events available.
; SkyLake and beyond support verisonID 4
; See section 18.2 and 18.3 of the Intel Systems Programming manual

sample PROC
  idiv BL
  idiv BX
  idiv EBX
  idiv RBX
  push rax
  push r8
db 0FFh, 0F0h
db 041h, 0FFh, 0F0h
  vmovaps xmm8, OWORD PTR pi
  movaps xmm8, OWORD PTR pi2

  mov eax, [rax + 8]

sample ENDP

; TODO Some CPU specific non-architectural events.

END

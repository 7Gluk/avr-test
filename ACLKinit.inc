;------------------------------
; XOSC init
  ldi r16,0b11_0_0_1011; ZQ range = 12-16MHz, no X32K LPM, low swing on X2, 16KC startup
  sts OSC_XOSCCTRL,r16
; XOSC and internal 32768 Hz start
  lds r16,OSC_CTRL
  ori r16,(0x08+0x04)
  sts OSC_CTRL,r16
; wait for XOSC
  lds r16,OSC_STATUS
  sbrs r16,3
  rjmp PC-3
; switch to XOSC
  ldi r16,0xD8
  ldi r17,3
  sts CPU_CCP,r16
  sts CLK_CTRL,r17; now ACLK = XOSC CLK
; OFF internal 2 MHz OSC
  lds r16,OSC_CTRL
  andi r16,~0x01
  sts OSC_CTRL,r16
; wait for 32768 Hz
  lds r16,OSC_STATUS
  sbrs r16,2
  rjmp PC-3
;----------------------------------------
;      ZQ frequency measuring
  ldi r16,0b0000_110_1; CLK in = 32768 Hz
  sts CLK_RTCCTRL,r16
  ldi r16,1; div = 1
  sts RTC_CTRL,r16

  ldi r16,0; TC0 MODE = normal
  sts TCC0_CTRLB,r16
  lxi r28,r29,RTC_CNT
  ldd r18,Y+0
  ldd r19,Y+1
  ldi r19,5
rtm01:; wait for first changing of RTC
    ldd r16,Y+0
    ldd r17,Y+1
    cp r16,r18
    breq rtm01
  sts TCC0_CTRLA,r19; start TC0 @ ACLK/64
  lxi r18,r19,(64*32768+500)/1000; real time = +64 ms
  add r18,r16
  adc r19,r17
rtm02:
	ldd r16,Y+0
	ldd r17,Y+1
	cp r16,r18
	brne rtm02
	cp r17,r19
	brne rtm02
  lds r16,TCC0_CNT+0
  lds r17,TCC0_CNT+1
  lds r22,TCC0_INTFLAGS
  sts TCC0_CTRLA,r9; stop TC0

  sbrc r22,0; TC0 OVF ?
HaltCPU:rjmp PC
  lxi r18,r19,2000-200; min ZQ
  cp  r16,r18
  cpc r17,r19
  brcs HaltCPU
  lxi r18,r19,16000+1600; max ZQ
  cp  r16,r18
  cpc r17,r19
  brcc HaltCPU

  lxi r18,r19,10000; max ZQ
  cp  r16,r18
  cpc r17,r19
  brcc rtm10
    lds r18,OSC_XOSCCTRL; change ZQ range = 2-9MHz
    andi r18,0x3F
	ori  r18,0x40
    sts OSC_XOSCCTRL,r18
rtm10:
  lxi r18,r19,48000; required PLL CLK
  clr r20
  clr r21
  ldi r22,17
rtm11:
    lsl r18
    rol r19
    rol r20
    rol r21
    cp  r20,r16
    cpc r21,r17
    brcs PC+4
      sub r20,r16
	  sbc r21,r17
	  inc r18
    dec r22
    brne rtm11
  lsr r18
  adc r18,r9; R18 = PLL divider

;==================================================
;        if CPU OSC is PLL

.if AVR_CLK==24;

; OFF internal 32768 Hz
  lds r16,OSC_CTRL
  andi r16,~0x04
  sts OSC_CTRL,r16

; set PRESCALER A = 1/2
  ldi r16,0xD8
  ldi r17,0b_0_00001_00
  sts CPU_CCP,r16
  sts CLK_PSCTRL,r17
;------------------------------
; PLL init for 48 MHz
  ldi r16,0b11_0_00000; OUTDIV=1, Fout=48 MHz
  add r16,r18
  sts OSC_PLLCTRL,r16
; PLL start
  lds r16,OSC_CTRL
  ori r16,0x10
  sts OSC_CTRL,r16
; wait for PLL
  lds r16,OSC_STATUS
  sbrs r16,4
  rjmp PC-3
; switch to PLL (48 MHz)
  ldi r16,0xD8
  ldi r17,4
  sts CPU_CCP,r16
  sts CLK_CTRL,r17; now ACLK = PLL CLK/2 = 24 MHz

;==================================================
;   if CPU OSC is internal 32 MHz with DFLL
.else
; PLL init for 48 MHz
  ldi r16,0b11_0_00000; OUTDIV=1, Fout=48 MHz
  add r16,r18
  sts OSC_PLLCTRL,r16
;----------------------------------------
; Start 32 MHz internal OSC and PLL
  lds r16,OSC_CTRL
  ori r16,0x02+0x10
  sts OSC_CTRL,r16
; wait for 32MHz will be stabil
     lds r16,OSC_STATUS
     sbrs r16,1
	 rjmp PC-3
; switch to 32 MHz
  ldi r16,0xD8
  ldi r17,1
  sts CPU_CCP,r16
  sts CLK_CTRL,r17; now ACLK = 32 MHz
; setup DFLLRC32M
  ldi r16,0b00000_00_0; DFLL32M from internal 32 kHz (1.024 kHz ref)
  sts OSC_DFLLCTRL,r16
  lxi r16,r17,31250; ouput frequence = 32000 MHz
  sts DFLLRC32M_COMP1,r16
  sts DFLLRC32M_COMP2,r17
  ldi r16,1; enable DFLL32M
  sts DFLLRC32M_CTRL,r16

; wait for PLL
  lds r16,OSC_STATUS
  sbrs r16,4
  rjmp PC-3

;==================================================
.endif
;==================================================
  ldi r16,0b00_000_00_1; enable USB module with Fin=48MHz
  sts CLK_USBCTRL,r16

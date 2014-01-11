#include <linux/linkage.h>
#include <mach/hipm.h>
#include <mach/memory.h>

/**physical address to virtal or virtual to physical**/
.macro	addr_proc, rx, rxt, p, v
	LDR	\rxt, =\p
	SUB	\rx, \rxt
	LDR	\rxt, =\v
	ADD	\rx, \rxt
.endm

/**physical to virtual**/
.macro	p2v, rx, rxt, p, v
	addr_proc \rx, \rxt, \p, \v
.endm

/**virtual to physical**/
.macro	v2p, rx, rxt, v, p
	addr_proc \rx, \rxt, \v, \p
.endm

/*
 *-------------------------------------------------------------------------------
 * Function: hilpm_cpu_godpsleep
 *
 * this function is the low level interface when deep sleep.
 *
 */

ENTRY (hilpm_cpu_godpsleep)

	@save current mode r0~r12, r14, so we can use them
	STMFD  sp!, {r0-r12, r14}

	LDR    r8, hi_cpu_godpsleep_phybase			@r8 store the PHY address of stored_ctx
	LDR    r9, =(hisi_se_p2v(A9_PRE_STORE_DATA_ADDR))	@r9 store the pre-store address which in securam

	/*
	 *Some A9 Contexts need be protected before MMU and cache disabled
	 *r9 store the address in securam, to store some critial cp15 register which has relationship with MMU operation
	 *Attention: For slave CPUs, to make sure not store the data to L2-Cache,
	 *we need store them to securam first, after MMU
	 *and L1-cache disabled,move them to DDR
	 */

	/* get the CPUID */
	MOV	r1, #0
	MCR	p15, 0, r1, c7, c5, 0	@ Invalidate entire instruction cache and flush branch predictor arrays
	MRC	p15, 0, r0, c0, c0, 5	@ Read CPU MPIDR
	AND	r0, r0, #0x03           @ Mask off, leaving the CPU ID field
	CMP	r0, #0                  @ IF (ID ==0)
	BEQ	save_ctx

	/* for each A9 CPU, a different offset of saving context data */
	ldr 	r1, =SUSPEND_STORE_OFFSET_UNIT
	mul 	r2, r1, r0        @R2 store the offset,different of each CPU
	ADD 	r8, r8, r2

	mov	r1,#A9_PRE_STORE_DATA_LEN
	mul 	r2, r1, r0
	ADD 	r9, r9, r2

save_ctx:

	/***
	save critial CP15 register before MMU Disabled
	CPU_ID save in R0
	save CTRL_Register in r1
	save Aux_Ctrl_register in r2
	TTBR0 in r3
	TTBR1 in r4
	TTBCR in r5
	DAC   in R6
	***/

	mrc     p15, 0, r1, c1, c0, 0	@ sctlr
	mrc     p15, 0, r2, c1, c0, 1	@ actlr
	mrc     p15, 0, r3, c2, c0, 0	@ TTBR0
	mrc     p15, 0, r4, c2, c0, 1	@ TTBR1
	mrc     p15, 0, r5, c2, c0, 2	@ TTBCR
	mrc     p15, 0, r6, c3, c0, 0	@ domain access control reg

	/***
	Notes: MMU is enabled, using the pre-store addree which stored in R9
	r0,[r9]             @offset0 store the CPU_ID
	r1,[r9,#0x4]        @CTRL_Register
	r2,[r9,#0x8]        @Aux_Ctrl_register
	r3,[r9,#0xc]        @TTBR0
	r4,[r9,#0x10]       @TTBR1
	r5,[r9,#0x14]       @TTBCR
	r6,[r9,#0x18]       @DAC
	***/

	STMIA  r9,{r0-r6}

	/* now Clean and Invalid D-Cache, and Disable Cache */
	mov     r0, #0
	mcr     p15, 0, r0, c7, c5, 4   @ Flush prefetch buffer
	mcr     p15, 0, r0, c7, c5, 6   @ Invalidate branch predictor array
	mcr     p15, 0, r0, c8, c5, 0   @ Invalidate instruction TLB
	mcr     p15, 0, r0, c8, c6, 0   @ Invalidate data TLB

	/*
	protect r9 to r6 while clean and invalid l1-cache
	now r9 can be released for free use 
	R8 is reserved which store the PHY address
	*/
	mov r6,r9

	/**
	now invalid and clean l1 cache, ARM defined operation
	Used Register: R0/R1/R2/R3/R4/R5/R7/R9/R10
	**/
clean_inv_l1:                           @ cache-v7.S routine used here
	dmb                             @ ensure ordering with previous accesses
	mrc     p15, 1, r0, c0, c0, 1   @ read clidr
	ands    r3, r0, #0x7000000      @ extract loc from clidr
	mov     r3, r3, lsr #23         @ left align loc bit field
	beq     finished                @ if loc is 0, then no need to clean
	mov     r10, #0                 @ start clean at cache level 0
loop1:
	add     r2, r10, r10, lsr #1    @ work out 3x current cache level
	mov     r1, r0, lsr r2          @ extract cache type bits from clidr
	and     r1, r1, #7              @ mask bits for current cache only
	cmp     r1, #2                  @ see what cache we have at this level
	blt     skip                    @ skip if no cache, or just i-cache
	mcr     p15, 2, r10, c0, c0, 0  @ select current cache level in cssr
	isb                             @ isb to sych the new cssr&csidr
	mrc     p15, 1, r1, c0, c0, 0   @ read the new csidr
	and     r2, r1, #7              @ extract the length of the cache lines
	add     r2, r2, #4              @ add 4 (line length offset)
	ldr     r4, =0x3ff
	ands    r4, r4, r1, lsr #3      @ find maximum number on the way size
	clz     r5, r4                  @ find bit position way size increment
	ldr     r7, =0x7fff
	ands    r7, r7, r1, lsr #13     @ extract max number of the index size
loop2:
	mov     r9, r4                  @ create working copy of max way size
loop3:
        orr     r11, r10, r9, lsl r5    @ factor way and cache number into r11
        orr     r11, r11, r7, lsl r2    @ factor index number into r11
        mcr     p15, 0, r11, c7, c14, 2 @ clean & invalidate by set/way
        subs    r9, r9, #1              @ decrement the way
        bge     loop3
        subs    r7, r7, #1              @ decrement the index
        bge     loop2
skip:
	add     r10, r10, #2            @ increment cache number
	cmp     r3, r10
	bgt     loop1
finished:
	mov     r10, #0                 @ swith back to cache level 0
	mcr     p15, 2, r10, c0, c0, 0  @ select current cache level in cssr
	dsb
	isb
	mov     r0, #0
	mcr     p15, 0, r0, c7, c1, 0   @ ICIALLUIS
	mcr     p15, 0, r0, c7, c1, 6   @ BPIALLIS
	mcr     p15, 0, r0, c8, c3, 0

	mov	r0, #0
	mcr	p15, 0, r0, c1, c0, 1	    @ A9 exit coherency now

	/***
	After clean and invalid cache, we need disable
	D-Cache immediately
	***/

	/*  Data Cache Disable  */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x04
	mcr	p15, 0, r0, c1, c0, 0

	/**
	save back Secruram stored data address
	r10 store the pre-store ctx address in securam
	r6 can be releaseed for free use
	**/
	mov	r10,	r6

	/** write domain access to get the domain access right  **/
	LDR	r0, 	=0xFFFFFFFF
	MCR	p15, 0, r0, c3, c0, 0


	/** R5 store the cpu resume address (both Master CPU and slave CPU) **/
	LDR	r5,	=(hisi_v2p(cpu_resume))

	/*
	R7 stroe the register address to fetch Mastber back address; 
	*/
	LDR    r7, =(hisi_sc_p2v(MASTER_SR_BACK_PHY_ADDR))

	/**
	master cpu and slave cpu use the same wake-up address
	each cpu write the suspend back address to their own place.
	Master CPU: MASTER_SR_BACK_PHY_ADDR
	Slave  CPU: hi_cpu_godpsleep_phybase + CPUID*SUSPEND_STORE_OFFSET_UNIT
	that is the first word of CPU context saved area.
	**/
	MRC	p15, 0, r0, c0, c0, 5   @ Read CPU MPIDR
	AND	r0, r0, #0x03           @ Mask off, leaving the CPU ID field
	CMP	r0, #0                  @IF (ID ==0)
	BNE	store_slave_bk_addr

	/* store master cpu bk addr */
	STR	r5,[r7]
	B	after_store_bkaddr

	/* slave cpus save their supspend back address to Context area(SECURAM)
	and will save back to context area in DDR*/
store_slave_bk_addr:
	STR	r5,[r10]

	/**
	* slave core do not need to 
	* disable mmu
	**/
	B	slave_ready_to_store

after_store_bkaddr:

	/*read TTBRC*/
	mrc	p15, 0, r7, c2, c0, 2
	and	r7, #0x7
	cmp	r7, #0x0
	beq	create_idmap
ttbr_error:
	@TTBR1 not supports
	b	ttbr_error

create_idmap:
	/**read TTBR0 registers**/
	mrc	p15, 0, r2, c2, c0, 0
	ldr	r5, =TTBRBIT_MASK
	and	r2, r5
	ldr	r4, =(hisi_v2p(disable_mmu))
	ldr	r5, =TABLE_INDEX_MASK
	and	r4, r5
	ldr	r1, =TABLE_ENTRY
	add	r1, r1, r4
	lsr	r4, #18
	add	r2, r4

	/**r2 virtual addr for TLB**/
	p2v r2, r4, PLAT_PHYS_OFFSET, PAGE_OFFSET

	/**read the TLB**/
	LDR	r7, [r2]

	/**config the identy mapping**/
	STR	r1, [r2]

	/**r9 virtual addr for tlb**/
	mov	r9, r2

	/**r11 virtual addr of the enable_mmu**/
	ADR     r11, mmu_enalbed

	LDR	r6, =(hisi_v2p(ready_to_store))
	LDR	pc, =(hisi_v2p(disable_mmu))

disable_mmu:

	/*disable MMU*/
	MRC  p15, 0, r0, c1, c0, 0
	BIC  r0, r0, #0x5
	MCR  p15, 0, r0, c1, c0, 0

	/*  invalidate I & D TLBs */
	LDR  r0,=0x0
	MCR  p15, 0, r0, c8, c7, 0
	NOP
	NOP
	NOP
	NOP

	MOV  pc, r6

	/**
	From this scratch , MMU is Disabled
	**/

ready_to_store:

	/** move critical data from securam to DDR (L1/l2 unaccessable)
	r0,[r10]         offset0 store the Slave CPU Return Addres
			 if offset0 is 0, that means this cpu has not booted up yet
	r1,[r10,#0x4]	@CTRL_Register
	r2,[r10,#0x8]	@Aux_Ctrl_register
	r3,[r10,#0xC]	@TTBR0
	r4,[r10,#0x10]	@TTBR1
	r5,[r10,#0x14]	@TTBCR
	r6,[r10,#0x18]	@DAC
	r7,[r10,#0x1C]  direct mapping first level descriptor
	r9,[r10,#0x20]  virtual addr for the first level descriptor
	r11,[r10, #0x24]  enable_mmu virtual addr
	**/

	/**r10 is the address of securam to save tmp data**/
	v2p	r10, r4, REG_BASE_SECRAM_VIRT, REG_BASE_SECRAM

	LDMIA   r10, {r0-r6}

	mrc     p15, 0, r3, c2, c0, 0	@ TTBR0
	mrc     p15, 0, r4, c2, c0, 1	@ TTBR1
	mrc     p15, 0, r5, c2, c0, 2	@ TTBCR

	/**r8  addr to store data in ddr**/
	STMIA   r8, {r0-r7, r9, r11}

	B	all_to_store

slave_ready_to_store:
	/** move critical data from securam to DDR (L1/l2 unaccessable)
	r0,[r10]         offset0 store the Slave CPU Return Addres
			 if offset0 is 0, that means this cpu has not booted up yet
	r1,[r10,#0x4]	@CTRL_Register
	r2,[r10,#0x8]	@Aux_Ctrl_register
	r3,[r10,#0xC]	@TTBR0
	r4,[r10,#0x10]	@TTBR1
	r5,[r10,#0x14]	@TTBCR
	r6,[r10,#0x18]	@DAC
	**/

	p2v	r8, r4, PLAT_PHYS_OFFSET, PAGE_OFFSET

	LDMIA   r10, {r0-r6}
	STMIA   r8, {r0-r6}

all_to_store:

	/** R6/R10 can be release now
	    R9/R8 is reserved */
	add r9, r8, #SUSPEND_STORE_RESEVED_UNIT   @stack_base start from ctx_base + 0x030

	/** save CP15 register **/
	mrc p15, 2, r0, c0, c0, 0  @ csselr
	mrc p15, 0, r4, c15, c0, 0 @ pctlr
	stmia r9!, {r0, r4}

	mrc p15, 0, r0, c15, c0, 1 @ diag
	mrc p15, 0, r1, c1, c0, 2  @ cpacr
	stmia	r9!, {r0-r1}
	mrc p15, 0, r4, c7, c4, 0  @ PAR
	mrc p15, 0, r5, c10, c2, 0 @ PRRR
	mrc p15, 0, r6, c10, c2, 1 @ NMRR
	mrc p15, 0, r7, c12, c0, 0 @ VBAR
	stmia r9!, {r4-r7}

	mrc p15, 0, r0, c13, c0, 1 @ CONTEXTIDR
	mrc p15, 0, r1, c13, c0, 2 @ TPIDRURW
	mrc p15, 0, r2, c13, c0, 3 @ TPIDRURO
	mrc p15, 0, r3, c13, c0, 4 @ TPIDRPRW
	stmia r9!, {r0-r3}

	/** to save normal register which including R9,so use R0 as stack pointer */
	MOV  r0,r9

	/**
	save 7 modes programmable registers
	save svc mode registers
	enter svc mode, no interrupts
	**/

	MOV     r2, #Mode_SVC | I_Bit | F_Bit
	MSR     cpsr_c, r2
	MRS     r1, spsr
	STMIA   r0!, {r1, r13, r14}

	/**
	save fiq mode registers
	enter fiq mode, no interrupts
	**/
	MOV     r2, #Mode_FIQ | I_Bit | F_Bit
	MSR     cpsr_c, r2
	MRS     r1,spsr
	STMIA   r0!, {r1, r8-r14}

	/**
	save irq mode registers
	enter irq mode, no interrupts
	**/
	MOV     r2, #Mode_IRQ | I_Bit | F_Bit
	MSR     cpsr_c, r2
	MRS     r1,spsr
	STMIA   r0!, {r1, r13, r14}

	/**
	save undefine mode registers
	enter undefine mode, no interrupts
	**/
	MOV     r2, #Mode_UND | I_Bit | F_Bit
	MSR     cpsr_c, r2
	MRS     r1,spsr
	STMIA   r0!, {r1, r13, r14}

	/**
	save abort mode registers
	enter abort mode, no interrupts
	**/
	MOV     r2, #Mode_ABT | I_Bit | F_Bit
	MSR     cpsr_c, r2
	MRS     r1,spsr
	STMIA   r0!, {r1, r13, r14}

	/**
	save system mode registers
	enter system mode, no interrupts
	**/
	MOV     r2, #Mode_SYS | I_Bit | F_Bit
	MSR     cpsr_c, r2
	STMIA   r0!, {r13, r14}

	/** back to SVC mode, no interrupts **/
	MOV     r2, #Mode_SVC | I_Bit | F_Bit
	MSR     cpsr_c, r2

	/** save the private timer  **/
	LDR	r4,= A9_PRV_TIMER_BASE

	/**
	Only master CPU protect NAND,eMMC,L2CC,SCU configuration
	CPU1/CPU2/CPU3 go directly to SKIP_SCU	for Master CPU,
	[R8,0]Store value is 0; 
	for Slave CPU, [R8,0]Store the go back address
	**/
	LDR r1,[r8,#0x0]
	CMP r1,#0x0
	BEQ save_prv_timer
	LDR	r4,=hisi_prv_p2v(A9_PRV_TIMER_BASE)
save_prv_timer:
	LDR	r2, [r4, #TIMER_Ld]   @timer load
	LDR	r3, [r4, #TIMER_Ctl]  @timer control
	STMIA   r0!, {r2-r3}

	BNE SKIP_MASTER_CPU_OPRATORATION

	/**
	Now Master CPU protect the Global timer.
	save the 64bit timer
	**/
	LDR r1,= A9_GLB_TIMER_BASE
	LDR r2, [r1, #TIM64_Ctl]  @64-bit timer control
	BIC	r3, r2, #0xF
	STR	r3, [r1, #TIM64_Ctl]  @disable the features

	/** the registers are now frozen for the context save  **/
	LDR	r3, [r1, #TIM64_AutoInc] @Autoincrement register
	LDR	r4, [r1, #TIM64_CmpLo]   @comparator - lo word
	LDR	r5, [r1, #TIM64_CmpHi]   @comparator - hi word
	STMIA	r0!, {r2-r5}

	LDR	r2, [r1, #TIM64_CntLo] @counter - lo word
	LDR	r3, [r1, #TIM64_CntHi] @counter - hi word
	STMIA	r0!, {r2-r3}


#ifdef CONFIG_CACHE_L2X0
	/**
	save L2CC Configuration
	**/
	dsb

	ldr     r6, =REG_BASE_L2CC
	ldr     r2, [r6, #L2X0_AUX_CTRL]
	ldr     r3, [r6, #L2X0_TAG_LATENCY_CTRL]
	ldr     r4, [r6, #L2X0_DATA_LATENCY_CTRL]
	ldr     r5, [r6, #L2X0_PREFETCH_OFFSET]
	stmia   r0!, {r2-r5}
#endif
	/** save SCU Configruation  **/
	LDR    r1,=A9_SCU_BASE
	LDR    r2,[r1,#SCU_FILTER_START_OFFSET]
	LDR    r3,[r1,#SCU_FILTER_END_OFFSET]
	LDR    r4,[r1,#SCU_ACCESS_CONTROL_OFFSET]
	LDR    r5,[r1,#SCU_NONSEC_CONTROL_OFFSET]
	LDR    r6,[r1,#SCU_CONTROL_OFFSET]
	LDR    r7,[r1,#SCU_POWER_STATE_OFFSET]
	STMIA  r0!,{r2-r7}

	/*
	Protect the NAND Configuration
	Since it is a risk to read NAND register when 
	we do not konw its clock and reset status
	so left this work to NAND driver.
	*/
	/* 
	LDR r4,=NAND_CTRLOR_BASE
	LDR r1,[r4,#0x0]
	LDR r2,[r4,#0x10]
	LDR r3,[r4,#0x14]
	STMIA  r0!,{r1-r3}
        */

	/**
	Need not protect the eMMC Configuration
	eMMC will re enumation
	**/

SKIP_MASTER_CPU_OPRATORATION:

	str  r0,[r8,#SUSPEND_STACK_ADDR]  @store the stack_top in offset 0x1C

	/* judge CPU_ID  */
	mrc p15, 0, r0, c0, c0, 5   @ Read CPU MPIDR
	and r0, r0, #0x03           @ Mask off, leaving the CPU ID field
	cmp r0, #0                  @IF (ID ==0)
	beq go_securam

	/**
	for slave CPUs, enter into WFI directory, Master CPU do the left.
	wait for interrupt
	**/
	DSB
	WFI
	NOP
	NOP
	NOP
	NOP
	/**
	slave back from WFI, system has not entered dpsleep state
	go director to cpu_resume
	**/
	b cpu_resume

go_securam:
	/* copy Data to SecuRAM DPSLEEP_CODE_ADDR,which defined in <mach/hipm.h> */
	LDR	r0, =DPSLEEP_CODE_ADDR
	ADR	r1, SecuramBegin
	ADD     r2, r1, #DPSLEEP_CODE_LEN

copy_securam_code:
	LDMIA   r1!, {r4-r7}
	STMIA   r0!, {r4-r7}
	CMP     r1, r2
	BLO     copy_securam_code

	/**
	jump to SecuRAM to execute
	R6 store the back address
	**/

	/*  Load the resume back address, if we back from Securam Code  **/
	LDR     r7,=MASTER_SR_BACK_PHY_ADDR
	LDR     r6,[r7]
	LDR     r0,=DPSLEEP_CODE_ADDR
	MOV     PC,r0

cpu_resume:

	LDR	r0, =REG_BASE_DDRC_CFG

	/**
	check SCBAKDATA4[bit 0]:
	0x1 means memory move successfully
	0x0 means memory move failed
	**/
	LDR	r4, =REG_BASE_SCTRL
	LDR	r1, [r4, #0x324]   @SCTRL SCBAKDATA4 register
	TST	r1, #0x1
	BEQ	cpu_resume_start

	/*resume CS1_RANK1 refresh*/
	LDR	r1, =0x001172
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_resume_cmd_done_1:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_resume_cmd_done_1

	/*resume CS1_RANK0 refresh*/
	LDR	r1, =0x0011B2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_resume_cmd_done_2:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_resume_cmd_done_2

	/*resume CS0_RANK1 refresh*/
	LDR	r1, =0x0011D2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_resume_cmd_done_3:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_resume_cmd_done_3

	/*resume CS0_RANK0 refresh*/
	LDR	r1, =0x0011E2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_resume_cmd_done_4:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_resume_cmd_done_4

cpu_resume_start:
	/* write domain access to get the domain access right */
	LDR	r0, =0xFFFFFFFF
	MCR     p15, 0, r0, c3, c0, 0

	mov     r0, #0
	mcr     p15, 0, r0, c7, c5, 4   @ Flush prefetch buffer
	mcr     p15, 0, r0, c7, c5, 6   @ Invalidate branch predictor array
	mcr     p15, 0, r0, c8, c5, 0   @ Invalidate instruction TLB
	mcr     p15, 0, r0, c8, c6, 0   @ Invalidate data TLB

	mov     r10, #0                 @ swith back to cache level 0
	mcr     p15, 2, r10, c0, c0, 0  @ select current cache level in cssr
	dsb
	isb
	mov     r0, #0
	mcr     p15, 0, r0, c7, c1, 0   @ ICIALLUIS
	mcr     p15, 0, r0, c7, c1, 6   @ BPIALLIS
	mcr     p15, 0, r0, c8, c3, 0

	/**
	restore_data
	R10 is reserved to store the PHY address of A9 stored_ctx
	**/
	LDR     r10, hi_cpu_godpsleep_phybase

	mov     r1, #0
	mcr	p15, 0, r1, c7, c5, 0	@ Invalidate entire instruction cache and flush branch predictor arrays
	mrc	p15, 0, r0, c0, c0, 5	@ Read CPU MPIDR
	and     r0, r0, #0x03           @ Mask off, leaving the CPU ID field
	cmp	r0, #0                  @IF (ID ==0)
	beq	master_start_load_ctx

	ldr     r1, =SUSPEND_STORE_OFFSET_UNIT
	mul     r2,r1,r0                @R2 store the offset,different of eahc CPU
	ADD     r10,r10,r2              @R10 is the ctx_base_addres of saved_context
	LDR     r0, [r10, #SUSPEND_STACK_ADDR]

	b       slave_start_load_ctx

master_start_load_ctx:

	/*  R0 is reserved to stored_ctx stack pointer  */
	LDR     r0, [r10, #SUSPEND_STACK_ADDR]

	/**
	Resume the NAND Configuration
	Since it is a risk to read NAND register when 
	we do not konw its clock and reset status
	so left this work to NAND driver.
	**/
	/*
	LDMDB   r0!, {r1-r3}
	LDR     r4,=REG_BASE_NANDC_CFG
	STR     r3,[r4,#0x14]
	STR     r2,[r4,#0x10]
	STR     r1,[r4,#0x0]
        */

	/*  Restore SCU Configruation  **/
	LDMDB   r0!, {r2-r7}
	LDR     r1,=A9_SCU_BASE
	STR     r2,[r1,#SCU_FILTER_START_OFFSET]
	STR     r3,[r1,#SCU_FILTER_END_OFFSET]
	STR     r4,[r1,#SCU_ACCESS_CONTROL_OFFSET]
	STR     r5,[r1,#SCU_NONSEC_CONTROL_OFFSET]

	LDR     r8, =0xFFFF
	STR     r8, [r1, #SCU_SEC_INVALID_REG_OFFSET]     @invalidate the duplicate TAG store

	STR     r6,[r1,#SCU_CONTROL_OFFSET]
	STR     r7,[r1,#SCU_POWER_STATE_OFFSET]    @restore CPU power statue

#ifdef CONFIG_CACHE_L2X0
	/* restore l2-cache configuration  */
	ldr     r6, =REG_BASE_L2CC
	LDMDB   r0!, {r2-r5}
	str     r3, [r6, #L2X0_TAG_LATENCY_CTRL]
	str     r4, [r6, #L2X0_DATA_LATENCY_CTRL]
	str     r5, [r6, #L2X0_PREFETCH_OFFSET]
	str     r2, [r6, #L2X0_AUX_CTRL]
#endif

	/*  restore 64bit global timer  */
	LDR r1, =A9_GLB_TIMER_BASE
	LDMDB	r0!, {r2-r3}
	STR	r2, [r1, #TIM64_CntLo]   @counter - lo word
	STR	r3, [r1, #TIM64_CntHi]   @counter - hi word

	LDMDB	r0!, {r2-r5}
	STR	r3, [r1, #TIM64_AutoInc] @Autoincrement register
	STR	r4, [r1, #TIM64_CmpLo]   @comparator - lo word
	STR	r5, [r1, #TIM64_CmpHi]   @comparator - hi word
	STR	r2, [r1, #TIM64_Ctl]     @restore the control last

slave_start_load_ctx:
	/* restore private timer  */
	LDR     r1, =A9_PRV_TIMER_BASE
	LDMDB	r0!, {r2-r3}
	STR	r2, [r1, #TIMER_Ld]   @timer load
	STR	r3, [r1, #TIMER_Ctl]  @timer control

	/**
	resume system mode registers
	enter system mode, no interrupts
	**/
	MOV     r2, #Mode_SYS | I_Bit | F_Bit
	MSR     cpsr_c, r2
	LDMDB   r0!, {r13, r14}

	/**
	resume abort mode registers
	enter abort mode, no interrupts
	**/
	MOV     r2, #Mode_ABT | I_Bit | F_Bit
	MSR     cpsr_c, r2
	LDMDB   r0!, {r1, r13, r14}
	MSR     spsr_c, r1

	/**
	resume undefine mode registers
	enter undefine mode, no interrupts
	**/
	MOV     r2, #Mode_UND | I_Bit | F_Bit
	MSR     cpsr_c, r2
	LDMDB   r0!, {r1, r13, r14}
	MSR     spsr_c, r1

	/**
	resume irq mode registers
	enter irq mode, no interrupts
	**/
	MOV     r2, #Mode_IRQ | I_Bit | F_Bit
	MSR     cpsr_c, r2
	LDMDB   r0!, {r1, r13, r14}
	MSR     spsr_c, r1

	/**
	resume fiq mode registers
	enter fiq mode, no interrupts
	**/
	MOV     r2, #Mode_FIQ | I_Bit | F_Bit
	MSR     cpsr_c, r2
	LDMDB   r0!, {r1, r8-r14}
	MSR     spsr_c, r1

	/**
	resume svc mode registers
	enter svc mode, no interrupts
	**/
	MOV     r2, #Mode_SVC | I_Bit | F_Bit
	MSR     cpsr_c, r2
	LDMDB   r0!, {r1, r13, r14}
	MSR     spsr_c, r1

	/* Restore CP15 register, need use r0 register, use R8 replace R0 to save Stack_pointer */
	MOV     r8,r0


	/** restore CP15 register **/
	LDMDB	r8!, {r0-r3}
	mcr	p15, 0, r0, c13, c0, 1	@ CONTEXTIDR
	mcr	p15, 0, r1, c13, c0, 2	@ TPIDRURW
	mcr	p15, 0, r2, c13, c0, 3	@ TPIDRURO
	mcr	p15, 0, r3, c13, c0, 4	@ TPIDRPRW

	LDMDB	r8!, {r4-r7}
	mcr	p15, 0, r4, c7, c4, 0	@ PAR
	mcr	p15, 0, r5, c10, c2, 0	@ PRRR
	mcr	p15, 0, r6, c10, c2, 1	@ NMRR
	mcr	p15, 0, r7, c12, c0, 0	@ VBAR

	LDMDB	r8!, {r0,r1}
	mcr	p15, 0, r0, c15, c0, 1	@ diag
	mcr	p15, 0, r1, c1, c0, 2	@ cpacr

	LDMDB	r8!, {r0, r4}
	mcr	p15, 2, r0, c0, c0, 0	@ csselr
	mcr	p15, 0, r4, c15, c0, 0	@ pctlr

	/* Invalid L1-Cache */
	mov     r3,#0x0
	mcr     p15, 2, r3, c0, c0, 0    @select L1 Data-cache
	mrc     p15, 1, r3, c0, c0, 0    @Read Current Cache Size Identification Register

	ldr     r1,=0x1ff
	and     r3, r1, r3, LSR #13      @r3 = number of sets in cache
	mov     r0,#0x0
way_loop:
	mov     r1, #0x0                 @r1 -> set counter
set_loop:
	mov     r2, r0, LSL #30
	orr     r2, r1, LSL #5           @r2->set/way cache-op format
	mcr     p15, 0, r2, c7, c6, 2    @Invalid Line descript by r2
	add     r1, r1, #1               @increment set counter

	cmp     r1, r3                   @check if last set is reached...
	ble     set_loop                 @if not continue set_loop
	add     r0,r0, #1                @else increment way counter

	cmp     r0,#4                    @check if last way is reached
	blt     way_loop                 @if not,continue way_loop

	/**
	now restore the critial P15 register
	restore critial P15 register before MMU Enabled
	save CTRL_Register in r0
	save Aux_Ctrl_register in r1
	TTBR0 in r2
	TTBR1 in r3
	TTBCR in r4
	DAC   in R5
	**/

	ADD	r10,r10,#0x4

	MRC	p15, 0, r6, c0, c0, 5	@ Read CPU MPIDR
	AND     r6, r6, #0x03           @ Mask off, leaving the CPU ID field
	CMP	r6, #0                  @IF (ID ==0)
	BNE	slave_restore_cp15

	LDMIA	r10, {r0-r5, r7-r9}

	B	mmu_restore

slave_restore_cp15:
	LDMIA   r10, {r0-r5}

	LDR	r9, =hisi_p2v(slave_mmu_enabled)

mmu_restore:

	MCR	p15, 0, r2, c2, c0, 0	@ TTBR0
	MCR	p15, 0, r3, c2, c0, 1	@ TTBR1
	MCR	p15, 0, r4, c2, c0, 2	@ TTBCR

	/**
	r0 store the Stored Control register value
	**/
	MCR    p15, 0, r0, c1, c0, 0
	NOP
	NOP
	NOP
	NOP
	mov	pc, r9

mmu_enalbed:
	str	r7, [r8]

slave_mmu_enabled:

	mcr	p15, 0, r1, c1, c0, 1	@ actlr
	mcr	p15, 0, r5, c3, c0, 0	@ domain access control reg

	/*r6 store the CPUID*/
	CMP	r6, #0                  @IF (ID ==0)
	BNE	goback_to_kernel

	/**
	we may back from suspend, securam is re-power on
	some data need to be reload.
	simply call hilpm_cp_securam_code
	**/
	bl      hilpm_cp_securam_code

#ifdef CONFIG_CPU_IDLE
	bl	hilpm_cp_cpuidle_code
#endif

goback_to_kernel:

	@resume current mode registers
	LDMFD	sp!, {r0-r12, r14}

	/*  go back to the call point */
	MOV	pc, lr
	NOP
	NOP
	NOP

SecuramBegin:

	/**
	STEP1: Protect the DDR Train Address and Traning Data into Syscontrol
	We need not do it, it will be config in fastboot
	**/

	/**
	config DDR enter self-refresh state
	**/
	LDR	r0, =REG_BASE_DDRC_CFG

	/**
	check SCBAKDATA4[bit 0]:
	0x1 means memory move successfully
	0x0 means memory move failed
	**/
	LDR	r4, =REG_BASE_SCTRL
	LDR	r2, [r4, #0x324]   @SCTRL SCBAKDATA4 register
	TST	r2, #0x1
	BEQ	DDR_Selfrefresh_start

	AND	r2, r2, #0xff000000
	LSR	r2, r2, #0x8

	/*block CS1_RANK1 refresh*/
	LDR	r1, =0x1172
	ORR r1, r2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_block_cmd_done_1:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_block_cmd_done_1

	/*block CS1_RANK0 refresh*/
	LDR	r1, =0x11B2
	ORR r1, r2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_block_cmd_done_2:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_block_cmd_done_2

	LDR	r2, [r4, #0x324]   @SCTRL SCBAKDATA4 register
	AND	r2, r2, #0x00ff0000

	/*block CS0_RANK1 refresh*/
	LDR	r1, =0x11D2
	ORR r1, r2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_block_cmd_done_3:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_block_cmd_done_3

	/*block CS0_RANK0 refresh*/
	LDR	r1, =0x11E2
	ORR r1, r2
	STR	r1, [r0, #0x24]
	LDR	r1, =0x1
	STR	r1, [r0, #0x28]
check_block_cmd_done_4:
	LDR	r1, [r0, #0x28]
	TST	r1, #0x1
	BNE	check_block_cmd_done_4

DDR_Selfrefresh_start:
	LDR r1,	=0x01
	STR r1,	[r0,#0x4]

	/* check DDR self-fresh status */
CheckDDREnterSF:
	LDR	r1, [r0, #0x0]
	TST	r1, #0x04
	BEQ	CheckDDREnterSF

	/**
	config DDR PHY enter CKE-Retention status
	fastboot code will do opposition relevent operations.
	**/
	LDR  r4,=REG_BASE_SCTRL
	LDR  r1,[r4,#0x20C]   @SCTRL SCPERCTRL3 register
	ORR  r1,r1,#0x3       @set sc_ddr_ret_en bit[1:0] to 0x3
	STR  r1,[r4,#0x20C]

	/**
	Set MDDRC's clock to DDRPHY's input clock
	fastboot code will do opposition relevent operations.
	**/
	LDR    r4,=REG_BASE_PMCTRL
	LDR    r1,=0x0
	STR    r1,[r4,#0xA8] @DDRCLKSEL

r_wait_mddrc_clk:
	LDR    r1,[r4,#0xA8]
	TST    r1,#0x2
	BNE    r_wait_mddrc_clk

	/*set ddr clk div*/		@360M
	LDR	r1, = 0x03
	STR	r1, [r4, #0x0AC]
r_waite_mddr_div:
	LDR	r1, [r4, #0x0AC]
	TST	r1, #0x20
	BEQ	r_waite_mddr_div

	/*ddr clk change to peri PLL*/
	MOV	r2, #0x0
	LDR	r1, =0x00
	STR	r1, [r4, #0x030]
r_wait_ddrcclk_sw:
	ADD	r2, r2, #0x1
	CMP	r2, #0x1000
	BEQ	r_wait_ddrcclk_ok
	LDR	r1, [r4, #0x30]
	CMP	r1, #0x00
	BNE	r_wait_ddrcclk_sw

r_wait_ddrcclk_ok:

	/*close GPU PLL*/
	LDR	r1, =0x00
	STR	r1, [r4, #0x028]

	/**
	Close LD03
	**/
	
	/*enable pmuspi*/
	/*pmuspi clk div 4*/
	LDR	r1, =REG_BASE_PCTRL
	LDR     r4, =0xFF0003
	STR	r4, [r1, #0x8]

	/*enable clk*/
	LDR	r1, =REG_BASE_SCTRL
	LDR	r4, =0x2
	STR	r4, [r1, #0x40]

	/*undo reset*/
	LDR	r4, =0x2
	STR	r4, [r1, #0x9C]

	/*close LDO3*/
	LDR	r1, =REG_BASE_PMUSPI
	LDR	r4, [r1, #0x8C]
	BIC	r4, #0x10
	STR	r4, [r1, #0x8C]

	/*disable pmuspi*/
	/*reset*/
	LDR	r1, =REG_BASE_SCTRL
	LDR	r4, =0x2
	STR	r4, [r1, #0x98]

	/*disable clk*/
	LDR	r4, =0x2
	STR	r4, [r1, #0x44]


	/**
	STEP2. Clear intr response mode status register
	Try clear intr response mode status first
	if the status is cleared, means there has no
	intr pending, go dpsleep, else do not configuration
	any dpsleep register and go back
	**/
	LDR   r4,=REG_BASE_SCTRL
	LDR   r1,=0x0
	STR   r1,[r4,#0xc]  @clear intr status register
	NOP
	NOP
	/* check if we are still in intr response status */
	/* 2012-2-14 do not care about wakeup intr*/
	@LDR   r1,[r4,#0xc]
	@TST   r1,#0x01
	@BNE   Back_from_WFI  @go directly to  Resume

	/* exit intr response mode */
	LDR r2,[r4,#8]
	BIC r2,#1
	STR r2,[r4,#8]

	/**
	STEP3 Protect EMMC/NAND Component IO, config WP to LOW CLOSE eMMC/NAND Component LDO
	NOW EMMC/NAND Driver do the protection operation, we do nothing here
	**/

	/**
	STEP4 config dpsleep register
	**/
	LDR   r4,=REG_BASE_SCTRL
	LDR   r1,[r4]
	LDR   r2,=0x01000007  @BIT24: Enable DpSleep Mode, BIT2|BIT1|BIT0 = 7
	BIC   r1,r1,r2        @clear first
	LDR   r2,=0x01000000  @BIT24: Enable DpSleep Mod=1, modctrl(BIT2|BIT1|BIT0) = 000 (Sleep)
	ORR   r1,r1,r2        @Enable DpSleep Mode,ModeCtrl=sleep
	STR   r1,[r4]

	/* STEP5 CHIP required, config L2CC for LOWPOWER */
	LDR   r4,=REG_BASE_L2CC
	LDR   r2,[r4,#0xF80]
	ORR   r1,r2,#0x1
	STR   r1,[r4,#0xF80]

	/**
	STEP6. CHIP required, configure SCU power status register
	       cause if we back from wfi, scu power status must be
	       restore back, we protect the data into r11
	**/
	LDR   r4,=REG_BASE_A9PER
	LDR   r11,[r4,#0x08]  @protect the SCU  power status to r11
	LDR   r1,=0x03030303  @all CPU to Power-off Mode
	STR   r1,[r4,#0x08]

	/**
	  STEP7. configure SCU Standby EN
	  CAUSTION: this configuration suggest to be set before the whole system start
	**/
	LDR   R4,=REG_BASE_A9PER
	LDR   r1,[r4]
	ORR   r1,r1,#0x20
	STR   r1,[r4]

	/**
	  STEP9  to enable ACP''s clock as a must for SCUIDLE
	**/
	LDR     R4, =REG_BASE_SCTRL
	MOV     R1, #0x10000000  @BIT28 for ACP clock enable control
	STR     R1, [R4,#0x30] @Register SCPEREN1


	/**
	 STEP10  clear ScuRAM Ready FLAG,please refer to OnChipROM Design
	 FIXME: the FLAG also including the optional information (Refer to OnChipROM design)
	**/
	LDR r4,=SECURAM_CODE_READY_FLAG
	LDR r1,=0x0
	STR r1,[r4,#0x0]

	/**
	 STEP11 Enter WFI
	 **/
	DSB
	WFI
	NOP
	NOP
	NOP
	NOP

Back_from_WFI:
	/**
	 We are draged back from an interrupt
	 Caustion: R6 is reserved to store the go-back PHY address

	STEP1: Restore the IO/LDO Configuration of EMMC/NAND Component
	WE DO Nothing here
	**/

	/* STEP2: Restore the Securam_CODE_READY FLAG. Refer to OnChipROM Design */
	LDR	r4,=SECURAM_CODE_READY_FLAG
	LDR	r1,[r4]
	LDR	r2,=0xFFFF
	BIC	r1,r1,r2
	LDR 	r2,=0xBEEF  @0xBEEF is the SECURAM_CODE_READY_FLAG
	ORR 	r1,r1,r2
	STR 	r1,[r4,#0x0]

	/**
	 STEP3: make Sure system in Normal state
	 if system in SLOW mode, configuration to NORMAL mode
	 FIXME: Coding here if needed
	 **/
	LDR	r2, =REG_BASE_SCTRL
	LDR	r1, [r2]
	AND	r4, r1, #0x4
	CMP 	r4, #0x4
	BEQ	cpunormal
	ORR	r1, r1, #0x4
	STR	r1, [r2]
normalwait:
	LDR	r1, [r2]
	AND	r1, r1, #0x78
	CMP	r1, #0x20
	BNE	normalwait
	
cpunormal:

	/**
	 STEP4: Restore DDR Configuration to Normal
	 Restore DDR PHY CLK, exit CKE-Retention mode
	 **/

	/** change DDR PHY CLK to output mode **/
	/**
	FIXME: Remove the comments when fastboot add the
	relevent operations.
	**/
	LDR    r4,=REG_BASE_PMCTRL
	LDR    r1,=0x1
	STR    r1,[r4,#0xA8] @DDRCLKSEL

	/** exit CKE retention mode **/
	LDR  r4,=REG_BASE_SCTRL
	LDR  r1,[r4,#0x20C]   @SCTRL SCPERCTRL3 register
	BIC  r1,r1,#0x3       @set sc_ddr_ret_en bit[1:0] to 0x0
	STR  r1,[r4,#0x20C]

	/**
	Open LD03
	**/
	
	/*enable pmuspi*/
	/*pmuspi clk div 4*/
	LDR	r1, =REG_BASE_PCTRL
	LDR     r4, =0xFF0003
	STR	r4, [r1, #0x8]

	/*enable clk*/
	LDR	r1, =REG_BASE_SCTRL
	LDR	r4, =0x2
	STR	r4, [r1, #0x40]

	/*undo reset*/
	LDR	r4, =0x2
	STR	r4, [r1, #0x9C]

	/*open LDO3*/
	LDR	r1, =REG_BASE_PMUSPI
	LDR	r4, [r1, #0x8C]
	ORR	r4, #0x10
	STR	r4, [r1, #0x8C]

	/*disable pmuspi*/
	/*reset*/
	/*LDR	r1, =REG_BASE_SCTRL*/
	/*LDR	r4, =0x2*/
	/*STR	r4, [r1, #0x98]*/

	/*disable clk*/
	/*LDR	r4, =0x2*/
	/*STR	r4, [r1, #0x44]*/

	/*about 100ms*/
	LDR	r4, =0x2625A00
ldo3delay:
	SUBS	r4, r4, #0x1
	BNE	ldo3delay

	/** Config DDR leave self-refresh mode **/
	LDR r0,=DDR_CTRLOR_BASE
	LDR r1,=0x00
	STR r1,[r0,#0x4]

	/** check DDR self-refresh status **/
CheckDDRLeaveSF:
	LDR    r1, [r0, #0x0]
	TST    r1, #0x04
	BNE    CheckDDRLeaveSF

	/** STEP5 restore SCU CPU power states, which restore in r11 before **/
	LDR   r4,=REG_BASE_A9PER
	STR   r11,[r4,#0x08]  @restore the SCU  power status from r11

	/** STEP6 go Back to DDR Address Store in R6 **/
	MOV    pc, r6
	NOP
	NOP

SecuramEnd:

	NOP
	.ltorg

.global hi_cpu_godpsleep_ddrbase
hi_cpu_godpsleep_ddrbase:
	.word   hi_cpu_godpsleep_ddrbase

.global hi_cpu_godpsleep_phybase
hi_cpu_godpsleep_phybase:
	.word   hi_cpu_godpsleep_phybase

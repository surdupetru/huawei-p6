#include <linux/linkage.h>
#include <mach/hipm.h>

/*
*-------------------------------------------------------------------------------
*   Function: hilpm_cp_securam_code
*
*   this function is the low level interface when deep sleep.
*
*/
ENTRY (hilpm_cp_cpuidle_code)
	/* save current mode r0~r12, r14, so we can use them */
	STMFD  sp!, {r0-r7}

	/* copy code to SECRAM memory  */
	LDR     r0, =(hisi_se_p2v(DISMMU_CODE_IN_SECURAM))
	ADR     r1, cpu_enteridle
	ADD     r2, r1, #SLAVE_CPU_ENTRY_CODE_LEN
cp_cpuidle_entry_Code:
	LDMIA   r1!, {r4-r7}
	STMIA   r0!, {r4-r7}
	CMP     r1, r2
	BLO     cp_cpuidle_entry_Code

	/** copy operation completed **/

	/** resume current mode registers **/
	LDMFD    sp!, {r0-r7}

	/** go back to the call point  **/
	MOV      pc, lr
	NOP
	NOP
	NOP

	.ltorg

cpu_enteridle:
	push	{r4-r10, r14}

	dsb

	/**
	config DDR enter self-refresh state
	**/
	LDR r0,	=0xFE3D4000
	LDR r1,	=0x01
	STR r1,	[r0,#0x4]

	/* check DDR self-fresh status */
CheckDDREnterSF:
	LDR	r1, [r0, #0x0]
	TST	r1, #0x04
	BEQ	CheckDDREnterSF

	dsb
	WFI
	NOP
	NOP
	NOP

	/** Config DDR leave self-refresh mode **/
	LDR r1,	=0x00
	STR r1,	[r0,#0x4]

	/** check DDR self-refresh status **/
CheckDDRLeaveSF:
	LDR    r1, [r0, #0x0]
	TST    r1, #0x04
	BNE    CheckDDRLeaveSF

	pop	{r4-r10, pc}

	nop
	nop
	nop
	nop


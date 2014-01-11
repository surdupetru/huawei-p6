#include <linux/linkage.h>
#include <mach/hipm.h>

/*
*-------------------------------------------------------------------------------
*   Function: hilpm_cp_securam_code
*
*   this function is the low level interface when deep sleep.
*
*/
ENTRY (hilpm_cp_securam_code)
	/* save current mode r0~r12, r14, so we can use them */
	STMFD  sp!, {r0-r7}

	/* copy code to SECRAM memory  */
	LDR     r0, =(hisi_se_p2v(SLAVE_CPU_ENTRY_CODE_ADDR))
	ADR     r1, __slave_entry
	ADD     r2, r1, #SLAVE_CPU_ENTRY_CODE_LEN
cp_slav_entry_Code:
	LDMIA   r1!, {r4-r7}
	STMIA   r0!, {r4-r7}
	CMP     r1, r2
	BLO     cp_slav_entry_Code
	/** copy operation completed **/

	/** resume current mode registers **/
	LDMFD    sp!, {r0-r7}

	/** go back to the call point  **/
	MOV      pc, lr
	NOP
	NOP
	NOP

	.ltorg

	/*
	__slave_entry is slave CPUs' init entry when power on.
	__slave_entry will judge each slave CPUs's init state(first startup or back from suspend)
	and lead to right entry point
	*/
__slave_entry:
	LDR    r8, hi_slave_mark_phybase
	MRC    p15, 0, r0, c0, c0, 5   @ Read CPU MPIDR
	AND    r0, r0, #0x03           @ Mask off, leaving the CPU ID field	
	LDR    r1, =SUSPEND_STORE_OFFSET_UNIT
	MUL    r2,r1,r0        @R2 store the offset,different of each CPU
	ADD    r8,r8,r2        @R8 store the Markaddress MARK!=0 means cpu booted before.

	@LDR    r3,[r8]
	@CMP    r3,#0
	@BEQ    JMP_ORG
	B    JMP_ORG

	/* jump to r3, r3 store the back addres of slave cpu */
	MOV    pc, r3
JMP_ORG:
	LDR    r0,hi_slave_orig_entry_address
	MOV    pc,r0

.global hi_slave_orig_entry_address
hi_slave_orig_entry_address:
	.word   hi_slave_orig_entry_address

.global hi_slave_mark_phybase
hi_slave_mark_phybase:
	.word   hi_slave_mark_phybase
	

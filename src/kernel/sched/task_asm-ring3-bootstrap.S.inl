/* MIT License
 *
 * Copyright (c) 2017 GrieferAtWork
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//GLOBAL_FUNCTION(ktask_ring3_bootstrap):
    /* NOTE: The task setup will have set up the stack:
     *     +0:  User-ESP0
     *     +4:  Physical address of the user's page directory (CR3)
     *     +8:  User-ESP
     *     +12: User-EIP
     * NOTE: With ESP0 already at the top of the stack,
     *       we don't need to re-arrange anything.
     */
    call _S(kcpu_setesp0)
    movI $(KSEG_USER_DATA|3), %eax /* |3: RPL: Request ring 3 */
    movI %eax, %ds
    movI %eax, %es 
    movI %eax, %fs 
    movI %eax, %gs 

    /* Load the given arguments into registers */
    movI (PS*0)(%esp), %eax /* ESP0 */
    movI (PS*1)(%esp), %ebx /* CR3 */
    movI (PS*2)(%esp), %ecx /* ESP */
    movI (PS*3)(%esp), %edx /* EIP */

    /* Switch to the virtual kernel stack and enable paging */
    movI %ebx, %cr3
    movI %eax, %esp

    /* NOTE: The |3 in these describe the current ring (which will be 3, the CPL in SS and CS) */
#ifdef POP_REGISTERS_FROM_ESP0
    // The '*_regs' version differs from the other, in that
    // is will pop EDX, ECX, EBX and EAX from the ESP0 stack
    // before building the layout of IRET
    // >> struct __packed basicregs { __u32 edx,ecx,ebx,eax; };
    // In addition, 'PS*2+12' Bytes of unused stack memory must be available.
    movI $(KSEG_USER_DATA|3),    (8*PS)(%esp)  // SS     (0x23)
    movI %ecx,                   (7*PS)(%esp)  // ESP    (bootstrap)
    movI $(KARCH_X86_EFLAGS_IF), (6*PS)(%esp)  // EFLAGS (IF)
    movI $(KSEG_USER_CODE|3),    (5*PS)(%esp)  // CS     (0x1B)
    movI %edx,                   (4*PS)(%esp)  // EIP    (bootstrap)
    popI %edx
    popI %ecx
    popI %ebx
    popI %eax
#else
    pushI $(KSEG_USER_DATA|3)      // SS     (0x23)
    pushI %ecx                     // ESP    (bootstrap)
    pushI $(KARCH_X86_EFLAGS_IF)   // EFLAGS (IF)
    pushI $(KSEG_USER_CODE|3)      // CS     (0x1B)
    pushI %edx                     // EIP    (bootstrap)
#if KCONFIG_SECUREUSERBOOTSTRAP
    movI $0,   %eax
    movI %eax, %ebx
    movI %eax, %ecx
    movI %eax, %edx
    movI %eax, %ebp
#endif
#endif
    iret


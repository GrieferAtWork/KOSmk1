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
#ifndef __KOS_KERNEL_CPU_H__
#define __KOS_KERNEL_CPU_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#ifndef __ASSEMBLY__
#include <kos/types.h>
#include <kos/errno.h>

__DECL_BEGIN

/* Max amount of CPUs that may be used. */
#define KCPU_MAXCOUNT   16

#define CPUID_INVALID  (KCPU_MAXCOUNT)
typedef __u8 cpuid_t;    /*< LAPIC/CPU ID number. */
typedef __u8 cpustate_t; /*< Set of 'CPUSTATE_*' */
#define CPUSTATE_NONE      0x00
#define CPUSTATE_AVAILABLE 0x01 /*< This CPU is available for use. */
#define CPUSTATE_STARTED   0x10 /*< Set by CPU init code (Used to confirm an intended startup) */
#define CPUSTATE_RUNNING   0x20 /*< Must be set by bootstrap code (Used to confirm a successful start sequence) */

struct kcpu2 {
 /* TODO: Merge with existing 'kcpu' structure. */
union __packed {
 cpuid_t    p_id;	           /*< Processor id (kcpu2_v vector index). */
 cpuid_t    p_lapic_id;	     /*< LAPIC id. */
};
 __u8       p_lapic_version; /*< LAPIC version (s.a.: 'APICVER_*'). */
 cpustate_t p_state;         /*< CPU State. */
 __u8       p_padding;       /*< Padding... */
#ifdef __x86__
 __u32      p_x86_features;  /*< Processor features (Set of 'X86_CPUFEATURE_*'). */
#endif
};

extern __size_t      kcpu2_c;                 /*< [const] Amount of available processors. */
extern struct kcpu2  kcpu2_v[KCPU_MAXCOUNT];  /*< [const] Global processor control vector. */
extern struct kcpu2 *kcpu2_boot;              /*< [1..1][const] Control entry for the boot processor. */

#define CPUINIT_IPI_ATTEMPTS             2    /*< Amount of times init should be attempted through IPI. */
#define CPUINIT_IPI_TIMEOUT              10   /*< Timeout when waiting for 'APIC_ICR_BUSY' to go away after IPI init. */
#define CPUINIT_ACKNOWLEDGE_SEND_TIMEOUT 1000 /*< Timeout in peer CPU during init to wait for startup intention acknowledgement. */
#define CPUINIT_ACKNOWLEDGE_RECV_TIMEOUT 100  /*< Timeout in host CPU during init to wait for startup confirmation. */

//////////////////////////////////////////////////////////////////////////
// Search for additional CPUs the system might offer
extern void kernel_initialize_cpu(void);

//////////////////////////////////////////////////////////////////////////
// Start a given cpu at the specified EIP address.
// WARNING: All other registers are either undefined arch-specific, or undefined
// WARNING: If the given CPU was already running, it will be reset.
// NOTE: The caller is responsible to pass a valid, available CPU id.
// @return: KE_OK:       Successfully started the given CPU.
// @return: KE_DEVICE:   The device didn't respond to the INIT command.
// @return: KE_PERM:     The CPU didn't accept the INIT request.
// @return: KE_TIMEDOUT: Code at the given START_EIP was either
//                       not executed, or failed to acknowledge
//                       'CPUSTATE_STARTED' with 'CPUSTATE_RUNNING'.
extern kerrno_t cpu_start(cpuid_t id, __u32 start_eip);
extern kerrno_t cpu_start_unlocked(cpuid_t id, __u32 start_eip);
#define CPU_START_EIPBITS  20

//////////////////////////////////////////////////////////////////////////
// Stop execution on the calling CPU (This function never returns).
extern __noreturn void cpu_stopme(void);

//////////////////////////////////////////////////////////////////////////
// Acknowledge a successful CPU startup.
extern void cpu_acknowledge(void);

//////////////////////////////////////////////////////////////////////////
// Returns the ID of a suspended CPU, or 'CPUID_INVALID' if none are
// @return: * :            ID of the first unused CPU.
// @return: CPUID_INVALID: No available CPU is unused.
extern __wunused cpuid_t cpu_unused(void);

//////////////////////////////////////////////////////////////////////////
// Returns the ID of the calling CPU
extern __wunused cpuid_t cpu_self(void);

__DECL_END
#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */


#endif /* !__KOS_KERNEL_CPU_H__ */

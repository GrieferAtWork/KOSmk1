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
#ifndef __KOS_KERNEL_MULTIBOOT_H__
#define __KOS_KERNEL_MULTIBOOT_H__ 1

#include <kos/config.h>

#ifdef __KERNEL__
/* multiboot.h - the header for Multiboot */
/* Copyright (C) 1999, 2001  Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

/* Macros. */

/* The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/* The flags for the Multiboot header. */
#ifdef __ELF__
# define MULTIBOOT_HEADER_FLAGS         0x00000003
#else
# define MULTIBOOT_HEADER_FLAGS         0x00010003
#endif

/* The magic number passed by a Multiboot-compliant boot loader. */
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

/* The size of our stack (16KB). */
#define STACK_SIZE                      0x4000

/* C symbol format. HAVE_ASM_USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym)                     _ ## sym
#else
# define EXT_C(sym)                     sym
#endif

#ifndef ASM
/* Do not include here in boot.S. */

/* Types. */

/* The Multiboot header. */
typedef struct multiboot_header
{
  unsigned long magic;
  unsigned long flags;
  unsigned long checksum;
  unsigned long header_addr;
  unsigned long load_addr;
  unsigned long load_end_addr;
  unsigned long bss_end_addr;
  unsigned long entry_addr;
} multiboot_header_t;

/* The symbol table for a.out. */
typedef struct aout_symbol_table
{
  unsigned long tabsize;
  unsigned long strsize;
  unsigned long addr;
  unsigned long reserved;
} aout_symbol_table_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table
{
  unsigned long num;
  unsigned long size;
  unsigned long addr;
  unsigned long shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info
{
#define MULTIBOOT_FLAG_HAVE_MEM       (1 << 0)
#define MULTIBOOT_FLAG_HAVE_BOOTDEV   (1 << 1)
#define MULTIBOOT_FLAG_HAVE_CMDLINE   (1 << 2)
#define MULTIBOOT_FLAG_HAVE_MODS      (1 << 3)
#define MULTIBOOT_FLAG_HAVE_MMAP      (1 << 6)
  unsigned long flags;
  unsigned long mem_lower;
  unsigned long mem_upper;
  unsigned long boot_device;
  unsigned long cmdline;
  unsigned long mods_count;
  unsigned long mods_addr;
  union
  {
    aout_symbol_table_t aout_sym;
    elf_section_header_table_t elf_sec;
  } u;
  unsigned long mmap_length;
  unsigned long mmap_addr;
} multiboot_info_t;

/* The module structure. */
typedef struct module
{
  unsigned long mod_start;
  unsigned long mod_end;
  unsigned long string;
  unsigned long reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map
{
  unsigned long size;
  unsigned long base_addr_low;
  unsigned long base_addr_high;
  unsigned long length_low;
  unsigned long length_high;
  unsigned long type;
} memory_map_t;

#endif /* ! ASM */
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_MULTIBOOT_H__ */

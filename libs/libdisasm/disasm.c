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
#ifndef __DISASM_C__
#define __DISASM_C__ 1

#include <kos/compiler.h>
#include <format-printer.h>
#include <disasm/disasm.h>
#include <assert.h>
#include <sys/types.h>

__DECL_BEGIN

#define printf(...) \
do if __unlikely((error = format_printf(printer,closure,__VA_ARGS__)) != 0) return error;\
while(0)
#define printl(s,l) \
do if __unlikely((error = (*printer)(s,l,closure)) != 0) return error;\
while(0)
#define print(s) printl(s,__compiler_STRINGSIZE(s))

#define INDENT "    "
static char const empty_prefix[] = "";

__public int
disasm_x86(void const *__restrict text, size_t text_size,
           pformatprinter printer, void *closure,
           uint32_t flags) {
 byte_t *iter,*end,*opstart,opcode; int temp,error;
 uint32_t newprefix,prefix;
 unsigned int address_width = 1;
 char const *line_prefix = empty_prefix;
 assert(printer);
 end = (iter = (byte_t *)text)+text_size;
 if (text_size >= 0x00000010) ++address_width;
 if (text_size >= 0x00000100) ++address_width;
 if (text_size >= 0x00001000) ++address_width;
 if (text_size >= 0x00010000) ++address_width;
 if (text_size >= 0x00100000) ++address_width;
 if (text_size >= 0x01000000) ++address_width;
 if (text_size >= 0x10000000) ++address_width;
#define PREFIX_NONE             0x00000000
#define PREFIX_LOCK             0x10000000         /*< LOCK prefix. */
#define PREFIX_REPN             0x10000001         /*< REPNE/REPNZ prefix. */
#define PREFIX_REP              0x10000002         /*< REP or REPE/REPZ prefix. */
#define PREFIX_OVERRIDE_CS      0x20000000         /*< CS segment override. */
#define PREFIX_OVERRIDE_SS      0x20000010         /*< SS segment override. */
#define PREFIX_OVERRIDE_DS      0x20000020         /*< DS segment override. */
#define PREFIX_OVERRIDE_ES      0x20000030         /*< ES segment override. */
#define PREFIX_OVERRIDE_FS      0x20000040         /*< FS segment override. */
#define PREFIX_OVERRIDE_GS      0x20000050         /*< GS segment override. */
#define PREFIX_BRANCH_NOT_TAKEN PREFIX_OVERRIDE_CS /*< Branch not taken. */
#define PREFIX_BRANCH_TAKEN     PREFIX_OVERRIDE_DS /*< Branch taken. */
#define PREFIX_OPSIZE           0x40000000         /*< Operand-size override prefix. */
#define PREFIX_ADDRSIZE         0x80000000         /*< Address-size override prefix. */
next:
 prefix = PREFIX_NONE;
 temp = 0;
 opstart = iter;
prefix_again:
 if (iter == end) goto done;
 switch (*iter) {
  /* Prefix group 1 */
  case 0xf0: newprefix = PREFIX_LOCK; break;
  case 0xf2: newprefix = PREFIX_REPN; break;
  case 0xf3: newprefix = PREFIX_REP; break;
  /* Prefix group 2 */
  case 0x2e: newprefix = PREFIX_OVERRIDE_CS; break;
  case 0x36: newprefix = PREFIX_OVERRIDE_SS; break;
  case 0x3e: newprefix = PREFIX_OVERRIDE_DS; break;
  case 0x26: newprefix = PREFIX_OVERRIDE_ES; break;
  case 0x64: newprefix = PREFIX_OVERRIDE_FS; break;
  case 0x65: newprefix = PREFIX_OVERRIDE_GS; break;
  /* Prefix group 3 */
  case 0x66: newprefix = PREFIX_OPSIZE; break;
  /* Prefix group 4 */
  case 0x67: newprefix = PREFIX_ADDRSIZE; break;
  default: goto end_prefix;
 }
 prefix |= newprefix,++iter;
 if (++temp != 4) goto prefix_again;
end_prefix:
#define ISPREFIX(x) ((prefix&(x))==(x))
 if (flags&DISASM_FLAG_ADDR) {
  printf("%.8Ix:+%.*Ix ",opstart,address_width,(size_t)(opstart-(byte_t *)text));
  line_prefix = INDENT;
 }
 if (ISPREFIX(PREFIX_LOCK)) {
  printf("%slock ",line_prefix);
  line_prefix = empty_prefix;
 }
 opcode = *iter++;
 if (opcode == 0x90) { /* nop */
  byte_t *nop_second = iter;
  while (iter != end && *iter == 0x90) ++iter;
  if (iter == nop_second) printf("%snop\n",line_prefix);
  else printf("%snop /* repeated %Iu times... */\n",
              line_prefix,(size_t)((iter-nop_second)+1));
  goto next;
 }
 if (opcode == 0x0f) {
  /* extended opcode */
  if (iter == end) goto done;
  opcode = *iter++;
  goto unknown; // TODO
 }

 switch (opcode) {
  // TODO
  default: break;
 }

unknown:
 printf(".byte %#.2I8x\n",opcode);
 goto next;
done:
 return 0;
}


#undef printl
#undef printf

__DECL_END

#endif /* !__DISASM_C__ */

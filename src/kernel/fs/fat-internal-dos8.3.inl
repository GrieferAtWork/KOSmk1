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
#ifndef __KOS_KERNEL_FS_FAT_INTERNAL_DOS8_3_C_INL__
#define __KOS_KERNEL_FS_FAT_INTERNAL_DOS8_3_C_INL__ 1

#include <kos/compiler.h>
#include <kos/kernel/fs/fat-internal.h>
#include <kos/kernel/debug.h>
#include <sys/types.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

__DECL_BEGIN

__local __constcall int kdos83_isvalch(char ch) {
 if (ch <= 31 || ch == 127) return 0;
 if (strchr("\"*+,/:;<=>?\\[]|.",ch)) return 0;
 return 1;
}


int
kdos83_makeshort(char const *__restrict name, size_t namesize,
                 int retry, __u8 *__restrict ntflags,
                 char result[KFATFILE_NAMEMAX+KFATFILE_EXTMAX]) {
 char const *extstart,*iter,*end; char *dst,ch;
 size_t basesize,extsize,matchsize;
 int retry_hex,retry_dig,has_mixed_case = 0;
 kassertobj(name);
 kassertobj(ntflags);
 kassertmem(result,(KFATFILE_NAMEMAX+KFATFILE_EXTMAX)*sizeof(char));
 assert(retry < KDOS83_RETRY_COUNT);
 *ntflags = KFATFILE_NFLAG_NONE;
 /* Strip leading+terminating dots & space. */
 while (namesize && (isspace(*name) || *name == '.')) ++name,--namesize;
 while (namesize && (isspace(name[namesize-1]))) --namesize;

 extstart = name+namesize;
 while (extstart != name && (extstart[-1] != '.')) --extstart;
 if (extstart == name) {
  extstart = name+namesize; /* No extension */
  extsize = 0;
  basesize = namesize;
 } else {
  basesize = (size_t)(extstart-name)-1;
  extsize = (namesize-basesize)-1;
 }
 memset(result,' ',(KFATFILE_NAMEMAX+KFATFILE_EXTMAX)*sizeof(char));

 /* Generate the extension */
 if (extsize) {
  dst = result+KFATFILE_NAMEMAX;
  end = (iter = extstart)+min(extsize,KFATFILE_EXTMAX);
  *ntflags |= KFATFILE_NFLAG_LOWEXT;
  while (iter != end) {
   ch = *iter++;
   if (islower(ch)) {
    if (!(*ntflags&KFATFILE_NFLAG_LOWEXT)) has_mixed_case = 1;
    ch = _toupper(ch);
   } else {
    if (*ntflags&KFATFILE_NFLAG_LOWEXT && iter != extstart) has_mixed_case = 1;
    *ntflags &= ~(KFATFILE_NFLAG_LOWEXT);
   }
   if __unlikely(!kdos83_isvalch(ch)) ch = '~';
   *dst++ = ch;
  }
 }

 if (basesize <= KFATFILE_NAMEMAX &&
     extsize <= KFATFILE_EXTMAX) {
  /* We can generate a (possibly mixed-case) 8.3-compatible filename */
  end = (iter = name)+basesize,dst = result;
  *ntflags |= KFATFILE_NFLAG_LOWBASE;
  while (iter != end) {
   ch = *iter++;
   if (islower(ch)) {
    if (!(*ntflags&KFATFILE_NFLAG_LOWBASE)) has_mixed_case = 1;
    ch = _toupper(ch);
   } else {
    if (*ntflags&KFATFILE_NFLAG_LOWBASE && iter != extstart) has_mixed_case = 1;
    *ntflags &= ~(KFATFILE_NFLAG_LOWBASE);
   }
   if __unlikely(!kdos83_isvalch(ch)) ch = '~';
   *dst++ = ch;
  }
  /* Fix 0xE5 --> 0x05 (srsly, dos?) */
  if (((__u8 *)result)[0] == 0xE5) ((__u8 *)result)[0] = 0x05;
  return has_mixed_case ? KDOS83_KIND_SHORT : KDOS83_KIND_CASE;
 }
 /* Must __MUST__ generate a long filename, also
  * taking the value of 'retry' into consideration.
  * Now for the hard part: The filename itself... */
 retry_hex = retry/9,retry_dig = (retry % 9);

 /* The first 2 short characters always match the
  * first 2 characters of the original base (in uppercase).
  * If no hex-range retries are needed, the first 6 match. */
 matchsize = retry_hex ? 2 : 6;
 if (matchsize > basesize) matchsize = basesize;
 end = (iter = name)+matchsize,dst = result;
 while (iter != end) {
  ch = toupper(*iter++);
  *dst++ = kdos83_isvalch(ch) ? ch : '~';
 }
 if (retry_hex) {
  /* Following the matching characters are 4 hex-chars
   * whenever more than 9 retry attempts have failed
   * >> This multiplies the amount of available names by 0xffff */
  *dst++ = tohex(1,(retry_hex & 0xf000) >> 12);
  *dst++ = tohex(1,(retry_hex & 0x0f00) >> 8);
  *dst++ = tohex(1,(retry_hex & 0x00f0) >> 4);
  *dst++ = tohex(1,(retry_hex & 0x000f));
 }
 assert(dst <= result+6);
 /* Following the shared name and the hex part is always a tilde '~' */
 *dst++ = '~';
 /* The last character then, is the non-hex digit (1..9) */
 *dst = '1'+retry_dig;

 /* Fix 0xE5 --> 0x05 (srsly, dos?) */
 if (((__u8 *)result)[0] == 0xE5) ((__u8 *)result)[0] = 0x05;

 /* And we're done! */
 return KDOS83_KIND_LONG;
}


__DECL_END

#endif /* !__KOS_KERNEL_FS_FAT_INTERNAL_DOS8_3_C_INL__ */

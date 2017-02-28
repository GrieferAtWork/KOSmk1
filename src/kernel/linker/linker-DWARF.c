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
#ifndef __KOS_KERNEL_LINKER_DWARF_C__
#define __KOS_KERNEL_LINKER_DWARF_C__ 1

#include <kos/config.h>
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/syslog.h>
#include <kos/errno.h>
#include <kos/kernel/fs/file.h>
#include <sys/types.h>
#include <stddef.h>
#include <string.h>
#include "linker-DWARF.h"

#if 0
/* These are not real includes, but are used as implementation help since
 * I couldn't find any good documentation on the actual file layout of
 * the format. */
#include "../../binutils/binutils-2.27/binutils/dwarf.h"
#include "../../binutils/binutils-2.27/binutils/dwarf.c"
/* DWARF: DWARF2_Internal_LineInfo */
#endif

__DECL_BEGIN

leb128_t
parse_sleb128(byte_t const **__restrict pdata,
              byte_t const *__restrict end) {
 byte_t const *iter = *pdata;
 byte_t b; unsigned int shift = 0;
 uleb128_t result = 0;
 assert(iter <= end);
 if (iter == end) return 0;
 do {
  b = *iter++;
  result |= ((uleb128_t)(b & 0x7f)) << shift;
  shift += 7;
  if (!(b&0x80) || shift >= sizeof(result)*8) break;
 } while (iter != end);
 if ((shift < 8*sizeof(result)) && (b&0x40))
      result |= -((uleb128_t)1 << shift);
 *pdata = iter;
 return (leb128_t)result;
}

uleb128_t
parse_uleb128(byte_t const **__restrict pdata,
              byte_t const *__restrict end) {
 byte_t const *iter = *pdata;
 byte_t b; unsigned int shift = 0;
 uleb128_t result = 0;
 assert(iter <= end);
 if (iter == end) return 0;
 do {
  b = *iter++;
  result |= ((uleb128_t)(b & 0x7f)) << shift;
  if (!(b&0x80) || ((shift += 7) >= sizeof(result)*8)) break;
 } while (iter != end);
 *pdata = iter;
 return result;
}


__crit kerrno_t
kshlib_dwarf_readlineinfo(DWARF2_Internal_LineInfo *ili,
                          struct kdwarf_section const *section,
                          struct kfile *__restrict file,
                          size_t *header_size) {
 kerrno_t error; size_t size; int dwarf_64;
#define DWARF_MIN_ILI_SIZE  (15) /* Minimum size of the DWARF LineInfo header. */
 if (section->ds_section_size < DWARF_MIN_ILI_SIZE) return KE_NOSPC;
 error = kfile_kernel_preadall(file,section->ds_section_offset,ili,DWARF_MIN_ILI_SIZE);
 if __unlikely(KE_ISERR(error)) return error;
 /* Fix 32-bit vs. 64-bit length. */
 if (ili->__li_length_32 == (__u32)-1) {
  /* 64-bit length */
  memmove(ili,(byte_t *)ili+4,DWARF_MIN_ILI_SIZE-4);
  size     = 12;
  dwarf_64 = 1;
 } else {
  memmove((byte_t *)ili+8,(byte_t *)ili+4,DWARF_MIN_ILI_SIZE-4);
  ili->li_length = (__u64)ili->__li_length_32;
  size     = 4;
  dwarf_64 = 0;
 }
 if (ili->li_length+size > section->ds_section_size) {
  /* Clamp sections too long. */
  ili->li_length = section->ds_section_size-size;
 }

 /* At this point we have either 3 or 11 bytes remaining.
  * >> The version number is already read. */
 if (ili->li_version != 2 &&
     ili->li_version != 3 &&
     ili->li_version != 4) {
  k_syslogf_prefixfile(KLOG_ERROR,file,"[DWARF] Unsupported version %I16u\n",ili->li_version);
  return KE_NOSYS;
 }
 *header_size = sizeof(DWARF2_Internal_LineInfo);
 /* 1 or 9 bytes remaining. */
 if (dwarf_64) {
  size = 13-1;
  if (ili->li_version >= 4) ++size;
  if (DWARF_MIN_ILI_SIZE+size > section->ds_section_size) return KE_NOSPC;
  /* Read the rest. */
  error = kfile_kernel_preadall(file,section->ds_section_offset+
                                DWARF_MIN_ILI_SIZE,ili,size);
  if __unlikely(KE_ISERR(error)) return error;
  if (ili->li_version < 4) {
fill_missing_ops_per_insn:
   /* li_max_ops_per_insn didn't exist yet. */
   memmove((&ili->li_max_ops_per_insn)+1,
           (&ili->li_max_ops_per_insn),sizeof(DWARF2_Internal_LineInfo)-
           offsetafter(DWARF2_Internal_LineInfo,li_max_ops_per_insn));
   ili->li_max_ops_per_insn = 1;
   --*header_size;
  }
 } else {
  *header_size -= 8;
  /* Expand 'li_prologue_length' */
  memmove((byte_t *)ili+offsetafter(DWARF2_Internal_LineInfo,li_prologue_length),
          (byte_t *)ili+offsetafter(DWARF2_Internal_LineInfo,__li_prologue_length_32),
          sizeof(DWARF2_Internal_LineInfo)-offsetafter(DWARF2_Internal_LineInfo,li_prologue_length));
  ili->li_prologue_length = (__u64)ili->__li_prologue_length_32;
  if (ili->li_version >= 4) {
   if (section->ds_section_size < sizeof(DWARF2_Internal_LineInfo)-8) return KE_NOSPC;
   /* Must re-read the last byte. */
   error = kfile_kernel_preadall(file,section->ds_section_offset+
                                 offsetof(DWARF2_Internal_LineInfo,li_opcode_base)-9,
                                 &ili->li_opcode_base,1);
   if __unlikely(KE_ISERR(error)) return error;
   ++size;
  } else {
   goto fill_missing_ops_per_insn;
  }
 }
 /* Fix invalid members. */
 if (!ili->li_line_range) ili->li_line_range = 1;

 /* Later code using li_length expects it to describe the chunk size post-header. */
 ili->li_length -= (*header_size-(dwarf_64 ? 12 : 4));

 return KE_OK;
}



//////////////////////////////////////////////////////////////////////////
// === ka2ldwarfchunk ===
__crit kerrno_t
ka2ldwarfchunk_init(struct ka2ldwarfchunk *__restrict self,
                    struct kfile *__restrict file,
                    struct kdwarf_section *__restrict section_avail) {
 kerrno_t error;
 size_t header_size;
 __u64 total_size;
 kassertobj(self);
 kassert_kfile(file);
 kassertobj(section_avail);
 error = kshlib_dwarf_readlineinfo(&self->dc_header,section_avail,
                                   file,&header_size);
 if __unlikely(KE_ISERR(error)) return error;
 assert(header_size <= section_avail->ds_section_size);
#if 0
 k_syslogf(KLOG_TRACE
          ,"li_length           = %d\n"
           "li_version          = %d\n"
           "li_prologue_length  = %d\n"
           "li_min_insn_length  = %d\n"
           "li_max_ops_per_insn = %d\n"
           "li_default_is_stmt  = %d\n"
           "li_line_base        = %d\n"
           "li_line_range       = %d\n"
           "li_opcode_base      = %d\n"
          ,(unsigned)self->dc_header.li_length
          ,(unsigned)self->dc_header.li_version
          ,(unsigned)self->dc_header.li_prologue_length
          ,(unsigned)self->dc_header.li_min_insn_length
          ,(unsigned)self->dc_header.li_max_ops_per_insn
          ,(unsigned)self->dc_header.li_default_is_stmt
          ,(int     )self->dc_header.li_line_base
          ,(unsigned)self->dc_header.li_line_range
          ,(unsigned)self->dc_header.li_opcode_base);
#endif
 /* Now that we're pretty sure its really a DWARF section, read the rest! */
 self->dc_size = (size_t)self->dc_header.li_length;
 if __unlikely(!self->dc_size) self->dc_data = NULL;
 else {
  if __unlikely(header_size == section_avail->ds_section_size) goto end;
  self->dc_data = (byte_t *)malloc(self->dc_size);
  if __unlikely(!self->dc_data) { error = KE_NOMEM; goto end; }
  error = kfile_kernel_preadall(file,section_avail->ds_section_offset+header_size,
                                self->dc_data,self->dc_size);
  if __unlikely(KE_ISERR(error)) goto err_secdata;
  /* Parse section data in 'self->dc_data|self->dc_size'. */
  error = ka2ldwarfchunk_parse(self,file);
  if __unlikely(KE_ISERR(error)) goto err_secdata;
 }
end:
 /* Always consume the section data. */
 total_size = (__u64)header_size+self->dc_header.li_length;
 assert(total_size <= section_avail->ds_section_size);
 section_avail->ds_section_offset += total_size;
 section_avail->ds_section_size   -= total_size;
 return error;
err_secdata: free(self->dc_data); goto end;
}

__crit kerrno_t
ka2ldwarfchunk_parse(struct ka2ldwarfchunk *__restrict self,
                     struct kfile *__restrict file) {
 byte_t *data_iter,*data_end;
 kassertobj(self);
 data_end = (data_iter = self->dc_data)+self->dc_size;
 self->dc_opcodec = self->dc_header.li_opcode_base;
 /* Parse the list of known opcodes. */
 if (self->dc_opcodec) --self->dc_opcodec;
 if (self->dc_opcodec) {
  self->dc_opcodec = min(self->dc_opcodec,(size_t)(data_end-data_iter));
  self->dc_opcodev = data_iter;
  data_iter += self->dc_opcodec;
 } else {
  self->dc_opcodev = NULL;
 }
 /* Parse the directory table. */
 self->dc_dirtabc = 0;
 self->dc_dirtabv = (char const *)data_iter;
 while (data_iter != data_end) {
  if (!*data_iter) { ++data_iter; break; }
  k_syslogf_prefixfile(KLOG_TRACE,file,"[DWARF] Dirtab entry %Iu: %q\n",
                       self->dc_dirtabc,data_iter);
  data_iter = (byte_t *)strnend((char const *)data_iter,
                               ((char const *)data_end-
                                (char const *)data_iter)-1)+1;
  ++self->dc_dirtabc;
 }
 /* Parse the file table. */
 self->dc_filtabc = 0;
 self->dc_filtabv = (char const *)data_iter;
 while (data_iter != data_end) {
  if (!*data_iter) { ++data_iter; break; }
  k_syslogf_prefixfile(KLOG_TRACE,file,"[DWARF] File entry %Iu: %q\n",
                       self->dc_filtabc,data_iter);
  data_iter = (byte_t *)strnend((char const *)data_iter,
                               ((char const *)data_end-
                                (char const *)data_iter)-1)+1;
  parse_uleb128((byte_t const **)&data_iter,data_end);
  parse_uleb128((byte_t const **)&data_iter,data_end);
  parse_uleb128((byte_t const **)&data_iter,data_end);
  ++self->dc_filtabc;
 }
 self->dc_filend = (char const *)data_iter;
 self->dc_codeend = (__byte_t const *)data_end;
 return KE_OK;
}

char const *
ka2ldwarfchunk_getdir(struct ka2ldwarfchunk const *__restrict self,
                      size_t index) {
 char const *result;
 kassertobj(self);
 if (index >= self->dc_dirtabc) return NULL;
 result = self->dc_dirtabv;
 while (index--) result = strend(result)+1;
 return result;
}

char const *
ka2ldwarfchunk_getfile(struct ka2ldwarfchunk const *__restrict self,
                       size_t index, struct kdwarffile *__restrict data) {
 char const *result;
 byte_t const *end,*iter;
 kassertobj(self);
 end = (byte_t const *)self->dc_filend;
 if (index >= self->dc_filtabc) {
  data->df_dir  = (uleb128_t)-1;
  data->df_time = (uleb128_t)-1;
  data->df_size = (uleb128_t)-1;
  return NULL;
 }
 iter = (byte_t const *)self->dc_filtabv;
 for (;;) {
  result = (char const *)iter;
  iter = (byte_t const *)(strnend((char *)iter,(size_t)((end-iter)-1)/sizeof(char))+1);
  data->df_dir  = parse_uleb128((byte_t const **)&iter,end);
  data->df_time = parse_uleb128((byte_t const **)&iter,end);
  data->df_size = parse_uleb128((byte_t const **)&iter,end);
  if (!index--) break;
 }
 return result;
}

__crit kerrno_t
ka2ldwarfchunk_exec(struct ka2ldwarfchunk *__restrict self,
                    ksymaddr_t symaddr, struct kfileandline *__restrict result) {
 byte_t const *iter,*end; byte_t opcode;
 size_t uladv; kerrno_t error;
 ssize_t adv; uintptr_t op_index;
 size_t    last_file,   file;
 size_t    last_column, column;
 size_t    last_line,   line;
 uintptr_t last_address,address;
 int new_address,has_column;
 size_t more_filetable_entries_c;
 char **more_filetable_entries_v;
 kassertobj(self);
 kassertobj(result);
#define RESET_STATE_MACHINE() \
  (address = 0,op_index = 0,file = 1,line = 0,\
   column = 0,has_column = 0/*,is_stmt = is_stmt,\
   basic_block = 0,end_sequence = 0,last_file_entry = 0*/)
 iter = self->dc_code,end = self->dc_codeend;
 more_filetable_entries_c = 0;
 more_filetable_entries_v = NULL;
 RESET_STATE_MACHINE();
 while (iter != end) {
  assert(iter < end);
  opcode = *iter++;
  /* Store the last state for later comparison. */
  last_file    = file;
  last_line    = line;
  last_column  = column;
  last_address = address;
  new_address  = 0;

  if (opcode >= self->dc_header.li_opcode_base) {
   opcode -= self->dc_header.li_opcode_base;
   uladv = opcode/self->dc_header.li_line_range;
   if (self->dc_header.li_max_ops_per_insn == 1) {
    uladv *= self->dc_header.li_min_insn_length;
    address += uladv;
   } else {
    address += ((op_index+uladv)/self->dc_header.li_max_ops_per_insn)*self->dc_header.li_min_insn_length;
    op_index = (op_index+uladv)%self->dc_header.li_max_ops_per_insn;
   }
   adv   = (ssize_t)(opcode % self->dc_header.li_line_range)+self->dc_header.li_line_base;
   line += adv;
  } else switch (opcode) {

   {
    size_t len;
    byte_t const *extended_iter;
   case DW_LNS_extended_op:
    len = (size_t)parse_uleb128(&iter,end);
    if (iter+len < iter || iter+len > end) len = (size_t)(end-iter);
    extended_iter = iter,iter += len;
    assert(iter <= end);
    if (len) {
     //byte_t const *extended_end = extended_iter+len;
     opcode = *extended_iter++;
     switch (opcode) {
      case DW_LNE_end_sequence:
       RESET_STATE_MACHINE();
       new_address = 1;
       break;

      case DW_LNE_set_address:
            if (len >= 8) address = (uintptr_t)*(__u64 *)extended_iter;
       else if (len >= 4) address = (uintptr_t)*(__u32 *)extended_iter;
       else if (len >= 2) address = (uintptr_t)*(__u16 *)extended_iter;
       else               address = (uintptr_t)*(__u8  *)extended_iter;
       new_address = 1;
       op_index    = 0;
       break;

      {
       char **new_morefile_v;
      case DW_LNE_define_file:
       new_morefile_v = (char **)realloc(more_filetable_entries_v,
                                        (more_filetable_entries_c+1)*
                                         sizeof(char *));
       if __unlikely(!new_morefile_v) { error = KE_NOMEM; goto done; }
       more_filetable_entries_v = new_morefile_v;
       new_morefile_v[more_filetable_entries_c++] = (char *)extended_iter;
       // extended_iter = (byte_t const *)strnend((char *)extended_iter,(size_t)(extended_end-extended_iter)-1)+1;
       // parse_uleb128(&extended_iter,extended_end);
       // parse_uleb128(&extended_iter,extended_end);
       // parse_uleb128(&extended_iter,extended_end);
      } break;

      case DW_LNE_set_discriminator:
       // Discriminator = parse_uleb128(&extended_iter,extended_end);
       break;
      default: break;
     }
    }
   } break;

   case DW_LNS_copy: /* ??? */ break;

   case DW_LNS_advance_pc:
    uladv = (size_t)parse_uleb128(&iter,end);
    if (self->dc_header.li_max_ops_per_insn == 1) {
     uladv   *= self->dc_header.li_min_insn_length;
     address += uladv;
    } else {
     address += ((op_index+uladv)/self->dc_header.li_max_ops_per_insn)*self->dc_header.li_min_insn_length;
     op_index = (op_index+uladv)%self->dc_header.li_max_ops_per_insn;
    }
    break;

   case DW_LNS_advance_line:
    line += (ssize_t)parse_sleb128(&iter,end);
    break;

   case DW_LNS_set_file:
    file = (size_t)parse_uleb128(&iter,end);
    break;

   case DW_LNS_set_column:
    column = (size_t)parse_uleb128(&iter,end);
    has_column = 1;
    break;

   case DW_LNS_negate_stmt:
    // is_stmt = !is_stmt;
    break;

   case DW_LNS_set_basic_block:
    // basic_block = 1;
    break;

   case DW_LNS_const_add_pc:
    uladv = ((255-self->dc_header.li_opcode_base)/self->dc_header.li_line_range);
    if (self->dc_header.li_max_ops_per_insn) {
     uladv   *= self->dc_header.li_min_insn_length;
     address += uladv;
    } else {
     address += ((op_index+uladv)/self->dc_header.li_max_ops_per_insn)*self->dc_header.li_min_insn_length;
     op_index = (op_index+uladv)%self->dc_header.li_max_ops_per_insn;
    }
    break;

   case DW_LNS_fixed_advance_pc:
    if (iter+sizeof(__u16) > end) { iter = end; break; }
    address += *(__u16 *)iter,iter += sizeof(__u16);
    op_index = 0;
    break;

   case DW_LNS_set_prologue_end:
    // prologue_end = true;
    break;

   case DW_LNS_set_epilogue_begin:
    // epilogue_begin = true;
    break;

   case DW_LNS_set_isa:
    /* isa = */parse_uleb128(&iter,end);
    break;

   default:
    if (--opcode < self->dc_opcodec) {
     int n = self->dc_opcodev[opcode];
     while (n--) parse_uleb128(&iter,end);
    }
    break;
  }

  /* Check if we are not covering the inquired address. */
  if (!new_address && last_address <= address &&
      last_address <= symaddr && address > symaddr) {
   struct kdwarffile fileinfo;
found_address:
   --last_file;
   if (last_file >= self->dc_filtabc) {
    last_file -= self->dc_filtabc;
    if (last_file < more_filetable_entries_c &&
        more_filetable_entries_v[last_file]) {
     char const *morefile_iter = more_filetable_entries_v[last_file] ;
     result->fal_file = morefile_iter;
     fileinfo.df_dir  = parse_uleb128((byte_t const **)&morefile_iter,end);
     fileinfo.df_time = parse_uleb128((byte_t const **)&morefile_iter,end);
     fileinfo.df_size = parse_uleb128((byte_t const **)&morefile_iter,end);
    } else goto default_lookup_file;
   } else {
default_lookup_file:
    result->fal_file = ka2ldwarfchunk_getfile(self,last_file,&fileinfo);
   }
   if (!fileinfo.df_dir) {
    result->fal_path  = "."; /*< Current directory. */
   } else {
    result->fal_path  = ka2ldwarfchunk_getdir(self,(size_t)fileinfo.df_dir-1);
   }
   result->fal_line   = (unsigned int)last_line;
   result->fal_column = (unsigned int)last_column;
   result->fal_flags  = KFILEANDLINE_FLAG_HASLINE;
   if (has_column) result->fal_flags = KFILEANDLINE_FLAG_HASCOL;
   error = KE_OK;
   goto done;
  }
 }
 /* Check for special case: end state is the address. */
 if (address == symaddr) {
  last_file = file;
  last_line = line;
  goto found_address;
 }
#undef RESET_STATE_MACHINE
 error = KE_NOENT;
done:
 free(more_filetable_entries_v);
 return error;
}




__crit kerrno_t
ka2ldwarf_doload_unlocked(struct ka2ldwarf *__restrict self,
                          struct kfile *__restrict file) {
 struct kdwarf_section secdata;
 size_t chunkc,chunka,new_chunka; kerrno_t error;
 struct ka2ldwarfchunk *chunkv,*new_chunkv;
 memcpy(&secdata,&self->d_section,sizeof(struct kdwarf_section));
 chunkc = chunka = 0,chunkv = NULL;
 /* Load as many sections as possible. */
 while (secdata.ds_section_size) {
  if (chunkc == chunka) {
   new_chunka = chunka ? chunka*2 : 2;
   new_chunkv = (struct ka2ldwarfchunk *)realloc(chunkv,new_chunka*
                                                 sizeof(struct ka2ldwarfchunk));
   if __unlikely(!new_chunkv) { error = KE_NOMEM; goto err; }
   chunkv = new_chunkv,chunka = new_chunka;
  }
  error = ka2ldwarfchunk_init(&chunkv[chunkc],file,&secdata);
  /* Handle initialization errors. */
  if __likely(KE_ISOK(error)) ++chunkc;
  else break;
 }
 if (chunka != chunkc) {
  /* Try to free up some unused buffer memory. */
  if (!chunkc) free(chunkv),chunkv = NULL;
  else {
   new_chunkv = (struct ka2ldwarfchunk *)realloc(chunkv,chunkc*
                                                 sizeof(struct ka2ldwarfchunk));
   if __likely(new_chunkv) chunkv = new_chunkv;
  }
 }
 self->d_chunkc = chunkc;
 self->d_chunkv = chunkv;
 return KE_OK;
err:
 while (chunkc--) {
  ka2ldwarfchunk_quit(&chunkv[chunkc]);
 }
 free(chunkv);
 return error;
}

__crit kerrno_t
ka2ldwarf_load_unlocked(struct ka2ldwarf *__restrict self,
                        struct kfile *__restrict file) {
 kerrno_t error;
 kassertobj(self);
 kassert_kfile(file);
 assert(kmutex_islocked(&self->d_lock));
 if (self->d_flags&A2L_DWARF_FLAG_LOAD) return KS_UNCHANGED; /* Already loaded. */
 error = ka2ldwarf_doload_unlocked(self,file);
 if __likely(KE_ISOK(error)) {
set_load_flag: self->d_flags |= A2L_DWARF_FLAG_LOAD;
 } else if (self->d_chunkc) {
  error = KE_OK; /*< Well... We managed to load something... */
  goto set_load_flag;
 }
 return error;
}




__crit void ka2ldwarf_delete(struct ka2ldwarf *__restrict self) {
 struct ka2ldwarfchunk *iter,*end;
 kassertobj(self);
 if (self->d_flags&A2L_DWARF_FLAG_LOAD) {
  /* Free lazily loaded data. */
  end = (iter = self->d_chunkv)+self->d_chunkc;
  for (; iter != end; ++iter) ka2ldwarfchunk_quit(iter);
  free(self->d_chunkv);
 }
 free(self);
}

__crit kerrno_t
ka2ldwarf_exec(struct ka2ldwarf *__restrict self,
                struct kshlib *__restrict lib, ksymaddr_t addr,
                struct kfileandline *__restrict result) {
 struct ka2ldwarfchunk *iter,*end;
 kerrno_t error;
 kassertobj(self);
 kassert_kshlib(lib);
 kassertobj(result);
 if __unlikely(KE_ISERR(error = kmutex_lock(&self->d_lock))) return error;
 /* Perform lazy initialization. */
 error = ka2ldwarf_load_unlocked(self,lib->sh_file);
 if __unlikely(KE_ISERR(error)) goto done;
 /* Iterate all chunks and see if they can translate the address. */
 end = (iter = self->d_chunkv)+self->d_chunkc;
 for (; iter != end; ++iter) {
  error = ka2ldwarfchunk_exec(iter,addr,result);
  if (KE_ISOK(error)) goto done;
 }
 error = KE_NOENT;
done:
 kmutex_unlock(&self->d_lock);
 return error;
}



__crit kerrno_t
kshlib_a2l_add_dwarf_debug_line(struct kshlib *__restrict self,
                                __u64 section_offset,
                                __u64 section_size) {
 struct ka2ldwarf *engine;
 struct kaddr2line *slot;
 engine = omalloc(struct ka2ldwarf);
 if __unlikely(!engine) return KE_NOMEM;
 /* Initialize the DWARF engine. */
 engine->d_section.ds_section_offset = section_offset;
 engine->d_section.ds_section_size   = section_size;
 engine->d_flags                     = A2L_DWARF_FLAG_NONE;
 engine->d_chunkc                    = 0;
 engine->d_chunkv                    = NULL;
 kmutex_init(&engine->d_lock);

 /* Register the engine. */
 slot = kaddr2linelist_alloc(&self->sh_addr2line);
 if __unlikely(!slot) goto err_engine;
 slot->a2l_kind  = KA2L_KIND_DWARF;
 slot->a2l_dwarf = engine;

 /* And we're done! */
 return KE_OK;
err_engine:
 free(engine);
 return KE_NOMEM;
}




__DECL_END

#endif /* !__KOS_KERNEL_LINKER_DWARF_C__ */

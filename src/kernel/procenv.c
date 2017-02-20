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
#ifndef __KOS_KERNEL_PROCENV_C__
#define __KOS_KERNEL_PROCENV_C__ 1

#include <kos/compiler.h>
#include <kos/config.h>
#include <kos/kernel/procenv.h>
#include <malloc.h>
#include <stddef.h>
#include <string.h>
#include <kos/kernel/proc.h>
#include <kos/kernel/util/string.h>
#include <ctype.h>
#include <kos/syslog.h>

__DECL_BEGIN

#define kenventry_delete free
__local __crit struct kenventry *
kenventry_new(char const *name, size_t name_size,
              char const *value, size_t value_size) {
 struct kenventry *result; char *iter;
 KTASK_CRIT_MARK
 result = (struct kenventry *)malloc(offsetof(struct kenventry,ee_name)+
                                    (name_size+value_size+2)*sizeof(char));
 if __unlikely(!result) return NULL;
 result->ee_namsize = name_size;
 iter = result->ee_name;
 memcpy(iter,name,name_size);
 iter += name_size;
 *iter++ = '\0';
 memcpy(iter,value,value_size);
 iter[value_size] = '\0';
 return result;
}

#define kenventry_copy(self) \
 (struct kenventry *)memdup(self,offsetof(struct kenventry,ee_name)+\
                            kenventry_memsz(self))


size_t
kenventry_hashof(char const *__restrict text,
                 size_t text_size) {
 char const *end = text+text_size;
 size_t result = 1;
 kassertmem(text,text_size);
 for (; text != end; ++text) 
  result = result*263+*text;
 return result;
}

__local void
kprocenv_quit_environ(struct kprocenv *__restrict self) {
 struct kenventry **iter,**end,*bucket,*next;
 end = (iter = self->pe_map)+KPROCENV_HASH_SIZE;
 for (; iter != end; ++iter) {
  bucket = *iter;
  while (bucket) {
   next = bucket->ee_next;
   kenventry_delete(bucket);
   bucket = next;
  }
 }
}
__local void
kprocenv_quit_args(struct kprocenv *__restrict self) {
 char **argv_iter,**argv_end;
 argv_end = (argv_iter = self->pe_argv)+self->pe_argc;
 for (; argv_iter != argv_end; ++argv_iter) free(*argv_iter);
 free(self->pe_argv);
}

__crit void
kprocenv_quit(struct kprocenv *__restrict self) {
 KTASK_CRIT_MARK
 kprocenv_quit_environ(self);
 kprocenv_quit_args(self);
}

__crit void
kprocenv_clear_c(struct kprocenv *__restrict self) {
 KTASK_CRIT_MARK
 kprocenv_quit(self);
 memset(&self->pe_memcur,0,
        sizeof(struct kprocenv)-
        offsetof(struct kprocenv,pe_memcur));
}
void
kprocenv_install_after_exec(struct kprocenv *__restrict self,
                            struct kprocenv *__restrict newenv,
                            int args_only) {
 kassert_kprocenv(self);
 kassert_kprocenv(newenv);
 assert(self != newenv);
 assert(self->pe_memmax == newenv->pe_memmax);
 if (!args_only) {
  kprocenv_quit_args(self);
  kprocenv_quit_environ(self);
  memcpy(self,newenv,sizeof(struct kprocenv));
 } else {
  char **argv_iter,**argv_end;
  size_t argv_size = self->pe_argc*sizeof(char *);
  argv_end = (argv_iter = self->pe_argv)+self->pe_argc;
  for (; argv_iter != argv_end; ++argv_iter) {
   argv_size += strlen(*argv_iter);
   free(*argv_iter);
  }
  free(self->pe_argv);
  assert(argv_size <= self->pe_memcur);
  self->pe_memcur -= argv_size;
  self->pe_memcur += newenv->pe_memcur;
  self->pe_argc = newenv->pe_argc;
  self->pe_argv = newenv->pe_argv;
 }
}



__crit kerrno_t
kprocenv_initcopy(struct kprocenv *__restrict self,
                  struct kprocenv const *__restrict right) {
 struct kenventry **iter,**end,*const *src;
 struct kenventry *entry_src,*dest,*next;
 char **arg_iter,**arg_end,**arg_src;
 KTASK_CRIT_MARK
 kassert_kprocenv(right);
 kassertobj(self);
 kobject_init(self,KOBJECT_MAGIC_PROCENV);
 self->pe_memmax = right->pe_memmax;
 self->pe_memcur = right->pe_memcur;
 if ((self->pe_argc = right->pe_argc) != 0) {
  self->pe_argv = (char **)malloc(self->pe_argc*sizeof(char *));
  if __unlikely(!self->pe_argv) return KE_NOMEM;
  arg_end = (arg_iter = self->pe_argv)+self->pe_argc;
  arg_src = right->pe_argv;
  for (; arg_iter != arg_end; ++arg_iter,++arg_src) {
   *arg_iter = strdup(*arg_src);
   if __unlikely(!*arg_iter) goto err_argiter;
  }
 } else {
  self->pe_argv = NULL;
 }
 end = (iter = self->pe_map)+KPROCENV_HASH_SIZE;
 src = right->pe_map;
 for (; iter != end; ++iter,++src) {
  if ((entry_src = *src) != NULL) {
   dest = kenventry_copy(entry_src);
   if __unlikely(!dest) goto err_iter;
   *iter = dest;
   while ((entry_src = entry_src->ee_next) != NULL) {
    dest->ee_next = kenventry_copy(entry_src);
    if __unlikely(!dest->ee_next) {
     next = *iter; 
     for (;;) {
      kenventry_delete(next);
      if (next == dest) break;
      next = next->ee_next;
     }
     goto err_iter;
    }
    dest = dest->ee_next;
   }
   assert(!dest->ee_next);
  } else {
   *iter = NULL;
  }
 }
 return KE_OK;
err_iter:
 while (iter-- != self->pe_map) {
  dest = *iter;
  while (dest) {
   next = dest->ee_next;
   kenventry_delete(dest);
   dest = next;
  }
 }
err_argiter:
 arg_end = (arg_iter = self->pe_argv)+self->pe_argc;
 for (;arg_iter != arg_end; ++arg_iter) free(*arg_iter);
 free(self->pe_argv);
 return KE_NOMEM;
}



__nomp char const *
kprocenv_getenv(struct kprocenv const *__restrict self,
                char const *__restrict name, size_t name_max) {
 size_t hash; struct kenventry *bucket;
 kassert_kprocenv(self);
 name_max = strnlen(name,name_max);
 hash = kenventry_hashof(name,name_max);
 bucket = self->pe_map[hash % KPROCENV_HASH_SIZE];
 while (bucket) {
  if (hash == bucket->ee_namhash &&
      name_max == bucket->ee_namsize &&
      memcmp(bucket->ee_name,name,name_max*sizeof(char)) == 0
      ) return bucket->ee_name+(name_max+1);
  bucket = bucket->ee_next;
 }
 return NULL;
}


__crit kerrno_t
kprocenv_setenv_c(struct kprocenv *__restrict self,
                  char const *__restrict name, size_t name_max,
                  char const *__restrict value, size_t value_max,
                  int override) {
 struct kenventry *bucket,**buckptr,*newentry;
 size_t hash,newmem;
 KTASK_CRIT_MARK
 kassert_kprocenv(self);
 name_max = strnlen(name,name_max);
 value_max = strnlen(value,value_max);
 hash = kenventry_hashof(name,name_max);
 bucket = *(buckptr = &self->pe_map[hash % KPROCENV_HASH_SIZE]);
 while (bucket) {
  assert(*buckptr == bucket);
  if (hash == bucket->ee_namhash &&
      name_max == bucket->ee_namsize &&
      memcmp(bucket->ee_name,name,name_max*sizeof(char)) == 0) {
   // Existing entry
   if (!override) return KE_EXISTS;
   newmem  = self->pe_memcur;
   newmem -= kenventry_memsz(bucket);
   newmem += (name_max+value_max+2)*sizeof(char);
   if __unlikely(newmem > self->pe_memmax) return KE_ACCES;
   newentry = kenventry_new(name,name_max,value,value_max);
   if __unlikely(!newentry) return KE_NOMEM;
   newentry->ee_namhash = hash;
   newentry->ee_next = bucket->ee_next;
   *buckptr = newentry;
   self->pe_memcur = newmem;
   kenventry_delete(bucket);
   return KS_FOUND;
  }
  bucket = *(buckptr = &bucket->ee_next);
 }
 // Allocate a new entry
 newmem  = self->pe_memcur;
 newmem += (name_max+value_max+2)*sizeof(char);
 if __unlikely(newmem > self->pe_memmax) return KE_ACCES;
 newentry = kenventry_new(name,name_max,value,value_max);
 if __unlikely(!newentry) return KE_NOMEM;
 self->pe_memcur = newmem;
 buckptr = &self->pe_map[hash % KPROCENV_HASH_SIZE];
 newentry->ee_namhash = hash;
 newentry->ee_next = *buckptr;
 *buckptr = newentry;
 return KE_OK;
}

__crit kerrno_t
kprocenv_delenv_c(struct kprocenv *__restrict self,
                  char const *__restrict name, size_t name_max) {
 struct kenventry *bucket,**buckptr;
 size_t hash;
 KTASK_CRIT_MARK
 kassert_kprocenv(self);
 name_max = strnlen(name,name_max);
 hash = kenventry_hashof(name,name_max);
 bucket = *(buckptr = &self->pe_map[hash % KPROCENV_HASH_SIZE]);
 while (bucket) {
  if (bucket->ee_namhash == hash &&
      bucket->ee_namsize == name_max &&
      memcmp(bucket->ee_name,name,name_max*sizeof(char)) == 0) {
   // Found it!
   *buckptr = bucket->ee_next;
   assert(self->pe_memcur >= kenventry_memsz(bucket));
   self->pe_memcur -= kenventry_memsz(bucket);
   kenventry_delete(bucket);
   return KE_OK;
  }
  bucket = *(buckptr = &bucket->ee_next);
 }
 return KE_NOENT;
}

__crit kerrno_t
kprocenv_putenv_c(struct kprocenv *__restrict self,
                  char const *__restrict text, size_t text_max) {
 char const *equals_sign;
 KTASK_CRIT_MARK
 if ((equals_sign = strnchr(text,text_max,'=')) == NULL) {
  // Delete an existing variable
  return kprocenv_delenv(self,text,text_max);
 }
 // Overwrite an existing variable/Set a new variable
 return kprocenv_setenv(self,text,(size_t)(equals_sign-text),text,(size_t)-1,1);
}

__crit kerrno_t
kprocenv_setargv_c(struct kprocenv *__restrict self, size_t max_argc,
                   char const __kernel *const __kernel *argv,
                   size_t const __kernel *max_arglenv) {
 char **new_argv,**arg_iter,**arg_end,*arg;
 char const *const *arg_src; size_t const *arglen_src;
 size_t vector_size,newmem,real_argc; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kprocenv(self);
 assertf(!self->pe_argc && !self->pe_argv,"Arguments have already been set");
 real_argc = 0;
 if (max_argc && (arg_src = argv) != NULL) {
  while (*arg_src++ && max_argc--) ++real_argc;
 }
 kassertmem(argv,real_argc*sizeof(char *));
 kassertmemnull(max_arglenv,real_argc*sizeof(size_t));
 vector_size = real_argc*sizeof(char *);
 assert(vector_size >= real_argc);
 newmem = self->pe_memcur+vector_size;
 if __unlikely(newmem > self->pe_memmax) return KE_ACCES;
 new_argv = (char **)malloc(vector_size);
 if __unlikely(!new_argv) return KE_NOMEM;
 arg_end = (arg_iter = new_argv)+real_argc,arg_src = argv;
 arglen_src = max_arglenv;
 for (; arg_iter != arg_end; ++arg_iter,++arg_src) {
  size_t arglen = (arglen_src ? strnlen(*arg_src,*arglen_src++) : strlen(*arg_src))*sizeof(char);
  newmem += arglen+sizeof(char);
  if __unlikely(newmem >= self->pe_memmax) { error = KE_ACCES; goto err_argiter; }
  arg = (char *)malloc(newmem+sizeof(char));
  if __unlikely(!arg) { error = KE_NOMEM; goto err_argiter; }
  memcpy(arg,*arg_src,arglen);
  arg[arglen] = '\0';
  *arg_iter = arg;
 }
 assert(newmem <= self->pe_memmax);
 self->pe_argv = new_argv;
 self->pe_argc = real_argc;
 self->pe_memcur = newmem;
 return KE_OK;
err_argiter:
 while (arg_iter-- != new_argv) free(*arg_iter);
 free(new_argv);
 return error;
}

__crit kerrno_t
kprocenv_setargv_cu(struct kprocenv *__restrict self, size_t max_argc,
                    char const __user *const __user *argv,
                    size_t const __user *max_arglenv) {
 char **new_argv,**temp_argv,*arg; size_t const __user *arglen_src;
 size_t arglen,newmem,curr_argc,curr_arga; kerrno_t error;
 KTASK_CRIT_MARK
 kassert_kprocenv(self);
 assertf(!self->pe_argc && !self->pe_argv,"Arguments have already been set");
 curr_arga = curr_argc = 0,new_argv = NULL;
 arglen_src = max_arglenv;
 newmem = self->pe_memcur;
 for (; max_argc; --max_argc,++curr_argc,++argv) {
  __user char const *user_arg;
  size_t user_maxlen;
  if (u_get(argv,user_arg) != 0) goto err_argiter_fault;
  if __unlikely(!user_arg) break; /*< Terminate on NULL-argument. */
  if (arglen_src) {
   if (u_get(arglen_src,user_maxlen) != 0) goto err_argiter_fault;
   ++arglen_src;
  } else user_maxlen = (size_t)-1;
  arglen = u_strnlen(user_arg,user_maxlen)*sizeof(char);
  newmem += arglen+sizeof(char);
  if __unlikely(newmem+sizeof(char *) >= self->pe_memmax) goto err_argiter_acces;
  arg = (char *)malloc(newmem+sizeof(char));
  if __unlikely(!arg) goto err_argiter_nomem;
  if __unlikely(u_getmem(user_arg,arg,arglen) != 0) { free(arg); goto err_argiter_fault; }
  arg[arglen] = '\0';
  //printf("ARG: %s\n",arg);
  if (curr_argc == curr_arga) {
   size_t max_more_entries,new_arga;
   // Must allocate more 
   assert(newmem <= self->pe_memmax);
   assert(curr_argc+max_argc >= max_argc);
   // Calculate max amount of additional entries allowed
   max_more_entries = (self->pe_memmax-newmem)/sizeof(char *);
   if (!max_more_entries) { free(arg); goto err_argiter_acces; }
   // Calculate max new buffer size
   new_arga = min(curr_arga ? curr_arga*2 : 2,curr_arga+max_more_entries);
   // Reduce buffer size by what the user specified as their max argument count
   new_arga = min(new_arga,curr_argc+max_argc);
   assertf(new_arga != 0,"%Iu",new_arga);
   assertf(new_arga > curr_arga,"%Iu %Iu",new_arga,curr_arga);
   temp_argv = (char **)realloc(new_argv,new_arga*sizeof(char *));
   if __unlikely(!temp_argv) { free(arg); goto err_argiter_nomem; }
   curr_arga = new_arga;
   new_argv = temp_argv;
  }
  assert(curr_arga > curr_argc);
  new_argv[curr_argc] = arg;
  newmem += sizeof(char *);
 }
 assert(newmem <= self->pe_memmax);
 if (curr_argc != curr_arga) {
  temp_argv = (char **)realloc(new_argv,curr_argc*sizeof(char *));
  if __likely(temp_argv) new_argv = temp_argv;
 }
 self->pe_argv = new_argv;
 self->pe_argc = curr_argc;
 self->pe_memcur = newmem;
 return KE_OK;
 if (0) { err_argiter_fault: error = KE_FAULT; }
 if (0) { err_argiter_acces: error = KE_ACCES; }
 if (0) { err_argiter_nomem: error = KE_NOMEM; }
/*err_argiter:*/
 while (curr_argc--) free(new_argv[curr_argc]);
 free(new_argv);
 return error;
}




#define env  (&kproc_kernel()->p_environ)

__local int
kernel_parse_arg(char const *__restrict arg,
                 size_t len, int warn_unknown) {
 size_t name_size,value_size; char const *value;
 kerrno_t error;
 while (len && isspace(arg[0])) ++arg,--len;
 while (len && isspace(arg[len-1])) --len;
 if (!len) return 0;
 k_syslogf(KLOG_TRACE,"[CMD] PARSE(%.*q) (%Iu characters)\n",(unsigned)len,arg,len);
 if ((value = (char const *)strnchr(arg,len,'=')) != NULL) {
  if (len && arg[0] == '-') --len,++arg;
  if (len && arg[0] == '-') --len,++arg;
  // Set environment variable
  name_size = (size_t)(value-arg);
  value_size = len-(size_t)(++value-arg);
 } else if (arg[0] == '-') {
  if (len >= 2 && arg[1] == '-') ++arg,--len;
  if (!--len) return 1; // This still counts as a valid argument
  name_size  = len,++arg;
  
  value      = "1";
  value_size = 1;
 } else {
  if (warn_unknown) {
   k_syslogf(KLOG_ERROR,"[CMD] Unrecognized argument format: %.*q\n",
        (unsigned)len,arg);
  }
  return 0;
 }
 k_syslogf(KLOG_INFO,"[CMD] setenv(%.*q,%.*q)\n",
          (unsigned)name_size,arg,
          (unsigned)value_size,value);
 error = kprocenv_setenv(env,arg,name_size,value,value_size,0);
 if (error == KE_EXISTS) {
  k_syslogf(KLOG_ERROR,"[CMD] Argument %.*q was already defined as '%#.*q=%#q'\n",
           (unsigned)len,arg,(unsigned)name_size,arg,
            kprocenv_getenv(env,arg,name_size));
 } else if (KE_ISERR(error)) {
  k_syslogf(KLOG_ERROR,"[CMD] Failed to set argument %.*q (%d)\n",
           (unsigned)len,arg,error);
 }
 return 1;
}

void kernel_initialize_cmdline(char const *cmd, size_t cmdlen) {
 int found_valid = 0;
 char const *iter,*end,*last = cmd;
 if (!cmdlen || !cmd) return;
 end = cmd+cmdlen;
 while ((iter = strnchr(last,(size_t)(end-last),' ')) != NULL) {
  if (kernel_parse_arg(last,(size_t)(iter-last),found_valid)) found_valid = 1;
  last = iter+1;
 }
 kernel_parse_arg(last,end-last,found_valid);
}


__DECL_END

#endif /* !__KOS_KERNEL_PROCENV_C__ */

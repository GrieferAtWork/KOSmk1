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
#ifndef __KOS_KERNEL_PROCENV_H__
#define __KOS_KERNEL_PROCENV_H__ 1

#include <kos/config.h>
#ifdef __KERNEL__
#include <kos/compiler.h>
#include <kos/types.h>
#include <kos/kernel/task.h>
#include <kos/kernel/object.h>
#ifndef __INTELLISENSE__
#include <malloc.h>
#endif

__DECL_BEGIN

//
// Process environment variables management
// NOTE: This sub-system is mainly used by windows, as in posix
//       management of environment variables is not actually
//       done by the kernel, but by the processes themselves.
//
// NOTE: Since the kernel is forced to have environment variables
//       as well, it fills its own with the arguments provided
//       on the grub command line.
//


#define KOBJECT_MAGIC_PROCENV  0x960CE74 /*< PROCENV. */
#define kassert_kprocenv(self) kassert_object(self,KOBJECT_MAGIC_PROCENV)

// Prefer using a static hash, as most processes
// usually have ~some~ variables set.
#define KPROCENV_HASH_SIZE 16


// Returns the hash of a given environment variable name.
extern __size_t kenventry_hashof(char const *text, __size_t text_size);

struct kenventry {
 struct kenventry *ee_next;    /*< [0..1][owned] Next environment entry who's name has the same hash. */
 __size_t          ee_namhash; /*< Hash of the name. */
 __size_t          ee_namsize; /*< Length of the name (only the name; excluding terminating \0). */
 char              ee_name[1]; /*< Name+'\0'+Value+'\0'. */
};
#define kenventry_value(self) ((self)->ee_name+((self)->ee_namsize+1))
#define kenventry_memsz(self) (((self)->ee_namsize+strlen(kenventry_value(self))+2)*sizeof(char))


struct kprocenv {
 KOBJECT_HEAD
 __size_t          pe_memmax;                  /*< Max amount of bytes allowed to be allocated for environment variables+arguments+argument vector. */
 __size_t          pe_memcur;                  /*< [<pe_memmax] The combined amount of characters in all variables and arguments (including \0 characters). */
 __size_t          pe_argc;                    /*< Amount of arguments passed to the process. */
 char            **pe_argv;                    /*< [1..1][owned][0..pa_argc][owned] Vector of process arguments. */
 struct kenventry *pe_map[KPROCENV_HASH_SIZE]; /*< [0..1][owned][KPROCENV_HASH_SIZE] Hash-map of set environment variables. */
};

#define KPROCENV_INIT_ROOT   {KOBJECT_INIT(KOBJECT_MAGIC_PROCENV) (__size_t)-1,0,0,NULL,{NULL,}}
#ifdef __INTELLISENSE__
extern __nonnull((1)) void kprocenv_initroot(struct kprocenv *self);
#else
#define kprocenv_initroot(self) \
 __xblock({ struct kprocenv *const __peself = (self);\
            kobject_initzero(__peself,KOBJECT_MAGIC_PROCENV);\
            __peself->pe_memmax = (__size_t)-1;\
            (void)0;\
 })
#endif

extern __crit __nonnull((1)) void kprocenv_quit(struct kprocenv *self);
extern __crit __nonnull((1)) void kprocenv_clear_c(struct kprocenv *self);
#define kprocenv_clear(self) KTASK_CRIT_V(kprocenv_clear_c(self))
extern __crit __nonnull((1)) void
kprocenv_install_after_exec(struct kprocenv *__restrict self,
                            struct kprocenv *__restrict newenv,
                            int args_only);

//////////////////////////////////////////////////////////////////////////
// Initialize 'self' as a copy of 'right'
extern __crit __wunused __nonnull((1,2)) kerrno_t
kprocenv_initcopy(struct kprocenv *self,
                  struct kprocenv const *right);


#define KPROCENV_FOREACH_BREAK    {__pe_map_iter=__pe_map_end;break;}

// KPROCENV_FOREACH(struct kprocenv const *self, char *&name, char *&value) { ... }
#define KPROCENV_FOREACH(self,name,value) \
 for (struct kenventry **__pe_map_iter = (self)->pe_map,\
                       **__pe_map_end = __pe_map_iter+KOBJECT_MAGIC_PROCENV,\
                        *__pe_bucket;\
      __pe_map_iter != __pe_map_end; ++__pe_map_iter)\
      for (__pe_bucket = *__pe_map_iter; __pe_bucket;\
           __pe_bucket = __pe_bucket->ee_next)\
           if (((name) = __pe_bucket->ee_name,\
               (value) = kenventry_value(__pe_bucket),\
                 0)); else


//////////////////////////////////////////////////////////////////////////
// Return the value of a given environment variable.
// NOTE: The caller is responsible to synchronize this call with set-operations.
// @return: * :   The value of the environment variable associated with the given name.
// @return: NULL: The given name doesn't describe a set environment variable.
extern __nomp __wunused __nonnull((1,2)) char const *
kprocenv_getenv(struct kprocenv const *__restrict self,
                char const *__restrict name, __size_t name_max);

//////////////////////////////////////////////////////////////////////////
// Set a given environment variable.
// @return: KE_OK:     The given variable was successfully set.
// @return: KS_FOUND:  'override' is non-ZERO and an old variable was overwritten.
// @return: KE_EXISTS: 'override' is ZERO and the given name already exists.
// @return: KE_ACCES:  Allocating a new entry for the given variable would exceed the allowed maximum.
// @return: KE_NOMEM:  Not enough memory to complete the operation.
extern __crit __wunused __nonnull((1,2,4)) kerrno_t
kprocenv_setenv_c(struct kprocenv *__restrict self,
                  char const *__restrict name, __size_t name_max,
                  char const *__restrict value, __size_t value_max,
                  int override);
#define kprocenv_setenv(self,name,name_max,value,value_max,override) \
 KTASK_CRIT(kprocenv_setenv_c(self,name,name_max,value,value_max,override))

//////////////////////////////////////////////////////////////////////////
// Delete a given environment variable.
// @return: KE_OK:    The given name was successfully deleted.
// @return: KE_NOENT: The given variable doesn't exist.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kprocenv_delenv_c(struct kprocenv *__restrict self,
                  char const *__restrict name, __size_t name_max);
#define kprocenv_delenv(self,name,name_max) \
 KTASK_CRIT(kprocenv_delenv_c(self,name,name_max))

//////////////////////////////////////////////////////////////////////////
// Perform putenv-style semantics.
// NOTE: The given text is _NOT_ inherited, or modified by the function.
// @return: KE_OK:    The variable described was set successfully.
// @return: KS_FOUND: The variable described was overwritten successfully.
// @return: KE_ACCES: Allocating a new entry for the given variable would exceed the allowed maximum.
// @return: KE_NOMEM: Not enough memory to complete the operation.
// @return: KE_NOENT: 'text' requested a delete operation, but the variable didn't exist.
extern __crit __wunused __nonnull((1,2)) kerrno_t
kprocenv_putenv_c(struct kprocenv *__restrict self,
                  char const *__restrict text, __size_t text_max);
#define kprocenv_putenv(self,text,text_max) KTASK_CRIT(kprocenv_putenv_c(self,text,text_max))


//////////////////////////////////////////////////////////////////////////
// Fill process arguments from the given arguments.
// WARNING: The caller must ensure that the argument vector was empty
// @param: arglenv: An optional argc-long vector of size_t, describing
//                  the maximal length to use for argument strings.
//                  When NULL, operate as if it was filled with (size_t)-1
// @return: KE_OK:    Arguments were successfully set.
// @return: KE_FAULT: [*_u] Invalid pointers were given.
// @return: KE_ACCES: The total amount of argument bytes exceeds what is allowed.
// @return: KE_NOMEM: Not enough memory to complete the operation.
extern __crit __nonnull((1)) kerrno_t
kprocenv_setargv_c(struct kprocenv *__restrict self, __size_t max_argc,
                   char const __kernel *const __kernel *argv,
                   __size_t const __kernel *max_arglenv);
extern __crit __nonnull((1)) kerrno_t
kprocenv_setargv_cu(struct kprocenv *__restrict self, __size_t max_argc,
                    char const __user *const __user *argv,
                    __size_t const __user *max_arglenv);
#define kprocenv_setargv(self,max_argc,argv,max_arglenv) \
 KTASK_CRIT(kprocenv_setargv_c(self,max_argc,argv,max_arglenv))
#define kprocenv_setargv_u(self,max_argc,argv,max_arglenv) \
 KTASK_CRIT(kprocenv_setargv_cu(self,max_argc,argv,max_arglenv))


__DECL_END
#endif /* __KERNEL__ */

#endif /* !__KOS_KERNEL_PROCENV_H__ */

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
#ifndef __KOS_KERNEL_PIPE_C__
#define __KOS_KERNEL_PIPE_C__ 1

#include <kos/config.h>
#include <kos/kernel/pipe.h>
#include <malloc.h>
#include <sys/types.h>

__DECL_BEGIN

void kpipe_destroy(struct kpipe *self) {
 kassert_object(self,KOBJECT_MAGIC_PIPE);
 kiobuf_quit(&self->p_iobuf);
 free(self);
}

__crit __ref struct kpipe *kpipe_new(size_t max_size) {
 __ref struct kpipe *result;
 KTASK_CRIT_MARK
 if __unlikely((result = omalloc(struct kpipe)) == NULL) return NULL;
 kobject_init(result,KOBJECT_MAGIC_PIPE);
 result->p_refcnt = 1;
 kiobuf_init_ex(&result->p_iobuf,max_size);
 return result;
}


__local __crit __ref struct kpipefile *
kpipefile_new(struct kfiletype *__restrict type,
              struct kpipe *__restrict pipe) {
 __ref struct kpipefile *result;
 KTASK_CRIT_MARK
 if __unlikely(KE_ISERR(kpipe_incref(pipe))) return NULL;
 if __unlikely((result = omalloc(struct kpipefile)) == NULL) goto err_pipe;
 kfile_init(&result->pr_file,type);
 result->pr_pipe = pipe; // Inherit reference
 return result;
err_pipe:
 kpipe_decref(pipe);
 return NULL;
}

__crit __ref struct kpipefile *
kpipefile_newreader(struct kpipe *__restrict pipe) {
 KTASK_CRIT_MARK
 return kpipefile_new(&kpipereader_type,pipe);
}
__crit __ref struct kpipefile *
kpipefile_newwriter(struct kpipe *__restrict pipe) {
 KTASK_CRIT_MARK
 return kpipefile_new(&kpipewriter_type,pipe);
}


static void
kpipefile_quit(struct kpipefile *__restrict self) {
 kpipe_decref(self->pr_pipe);
}
static kerrno_t
kpipefile_read(struct kpipefile *__restrict self,
               void *__restrict buf, size_t bufsize,
               size_t *__restrict rsize) {
 return kiobuf_read(&self->pr_pipe->p_iobuf,buf,bufsize,rsize,
                    KIO_BLOCKFIRST|KIO_NONE);
}
static kerrno_t
kpipefile_write(struct kpipefile *__restrict self,
                void const *__restrict buf, size_t bufsize,
                size_t *__restrict wsize) {
 return kiobuf_write(&self->pr_pipe->p_iobuf,buf,bufsize,wsize,
                     KIO_BLOCKFIRST|KIO_NONE);
}
static kerrno_t
kpipefile_seek(struct kpipefile *__restrict self, off_t off,
               int whence, pos_t *__restrict newpos) {
 kerrno_t error; size_t did_skip;
 if (whence != SEEK_CUR || off < 0) return KE_NOSYS;
 error = kiobuf_skip(&self->pr_pipe->p_iobuf,(size_t)off,&did_skip,
                     KIO_BLOCKNONE|KIO_NONE);
 if (__likely(KE_ISOK(error)) && newpos) *newpos = (pos_t)did_skip;
 return error;
}
static kerrno_t
kpipefile_flush(struct kpipefile *__restrict self) {
 return kiobuf_flush(&self->pr_pipe->p_iobuf);
}


struct kfiletype kpipereader_type = {
 .ft_size = sizeof(struct kpipefile),
 .ft_quit = (void(*)(struct kfile *__restrict))&kpipefile_quit,
 .ft_read = (kerrno_t(*)(struct kfile *__restrict,void *__restrict,size_t,size_t *__restrict))&kpipefile_read,
 .ft_seek = (kerrno_t(*)(struct kfile *__restrict,off_t,int,pos_t *__restrict))&kpipefile_seek,
};
struct kfiletype kpipewriter_type = {
 .ft_size  = sizeof(struct kpipefile),
 .ft_quit  = (void(*)(struct kfile *__restrict))&kpipefile_quit,
 .ft_write = (kerrno_t(*)(struct kfile *__restrict,void const *__restrict,size_t,size_t *__restrict))&kpipefile_write,
 .ft_flush = (kerrno_t(*)(struct kfile *__restrict))&kpipefile_flush,
};

__DECL_END

#endif /* !__KOS_KERNEL_PIPE_C__ */

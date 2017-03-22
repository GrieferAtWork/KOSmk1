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

#include "helper.h"
#include <stddef.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <format-printer.h>


static char *quote(char const *s) {
 struct stringprinter printer;
 if (stringprinter_init(&printer,0)) return NULL;
 if (format_quote(&stringprinter_print,&printer,s,(size_t)-1,0)) {
  stringprinter_quit(&printer);
  return NULL;
 }
 return stringprinter_pack(&printer,NULL);
}
static char *dequote(char const *s) {
 struct stringprinter printer;
 if (stringprinter_init(&printer,0)) return NULL;
 if (format_dequote(&stringprinter_print,&printer,s,(size_t)-1)) {
  stringprinter_quit(&printer);
  return NULL;
 }
 return stringprinter_pack(&printer,NULL);
}


static char const text[] = "\xAA\xAA" "Foobar\xAA\xAA";

TEST(quote) {
 char *dq,*q;

 q = quote(text);
 assertf(!strcmp(q,"\"\\xAA\\xAAFoobar\\xAA\\xAA\""),"%q",q);
 dq = dequote(q);
 assertf(!strcmp(dq,text),"%q",dq);
 free(dq);
 free(q);

 dq = dequote("Foo\\nBar");
 assertf(!strcmp(dq,"Foo\nBar"),"%q",dq);
 free(dq);
}

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
#ifndef __ECHO_C__
#define __ECHO_C__ 1

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
 char **iter,**end,*arg;
 int nolf = 0;
 int escape = 0;
 if (argc) --argc,++argv;

 end = (iter = argv)+argc;

 // Check for arguments
 while (iter != end && **iter == '-') {
  arg = (*iter)+1;
  do {
   if (*arg == 'n') { ++arg; nolf = 1; continue; }
   if (*arg == 'e') { ++arg; escape = 1; continue; }
  } while (0);
  if (arg == (*iter)+1) break; // Not really an argument
  if (*arg) {
   printf("Invalid argument: '%s'\n",*arg);
   return EXIT_FAILURE;
  }
 }
 if (iter != end) for (;;) {
  arg = *iter++;
  if (escape) {
   // TODO: Decode escape characters
  }
  printf("%s",arg); // This can be done better...
  if (iter == end) break;
  putchar(' ');
 }

 if (!nolf) putchar('\n');
 return EXIT_SUCCESS;
}

#endif /* !__ECHO_C__ */

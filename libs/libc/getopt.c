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
#ifndef __GETOPT_C__
#define __GETOPT_C__ 1

#include <kos/config.h>
#ifndef __KERNEL__
#include <kos/compiler.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

__DECL_BEGIN

/*
 ============
  Implementation in this file is derived from that of newlib-cygwin:
  The following disclaimer is taken from the original file /newlib/libc/stdlib/getopt.c:
 ============

  COPYRIGHT NOTICE AND DISCLAIMER:

  Copyright (C) 1997 Gregory Pietsch

  This file and the accompanying getopt.h header file are hereby placed in the 
  public domain without restrictions.  Just give the author credit, don't
  claim you wrote it or prevent anyone else from using it.

  Gregory Pietsch's current e-mail address:
  gpietsch@comcast.net
*/


__public char *optarg = 0;
__public int optind = 0;
__public int opterr = 1;
__public int optopt = '?';
static int optwhere = 0;

#define PERMUTE         0
#define RETURN_IN_ORDER 1
#define REQUIRE_ORDER   2

static void rev_order(char **argv, int num) {
 char *tmp; int i;
 for (i = 0; i < (num >> 1); ++i) {
  tmp = argv[i];
  argv[i] = argv[num-i-1];
  argv[num-i-1] = tmp;
 }
}

static void permute(char **argv, int len1, int len2) {
 rev_order(argv,len1);
 rev_order(argv,len1+len2);
 rev_order(argv,len2);
}
__local int is_option(char const *argv_element, int only) {
 return (argv_element == 0)
     || (argv_element[0] == '-')
     || (only && argv_element[0] == '+');
}

static int
getopt_internal(int argc, char *argv[], const char *shortopts,
                struct option const *longopts, int *longind, int only) {
 int has_arg = -1,ordering = PERMUTE;
 int permute_from = 0,num_nonopts = 0;
 int arg_next = 0,initial_colon = 0;
 int longopt_match = -1;
 char *possible_arg = NULL;
 char const *cp = NULL;
 if (!argc || !argv || (!shortopts && !longopts) ||
     optind >= argc || !argv[optind]) return EOF;
 if (!strcmp(argv[optind],"--")) { ++optind; return EOF; }
 if (!optind) optind = optwhere = 1; // First pass
 if (shortopts && (*shortopts == '-' || *shortopts == '+')) {
  ordering = (*shortopts == '-') ? RETURN_IN_ORDER : REQUIRE_ORDER;
  shortopts++;
 } else {
  ordering = getenv("POSIXLY_CORRECT") ? REQUIRE_ORDER : PERMUTE;
 }
 if (shortopts && shortopts[0] == ':') { ++shortopts; initial_colon = 1; }
 if (optwhere == 1) {
  switch (ordering) {
   default:
   case PERMUTE:
    permute_from = optind,num_nonopts = 0;
    while (!is_option(argv[optind],only)) ++optind,++num_nonopts;
    if (!argv[optind]) { optind = permute_from; return EOF; }
    else if (!strcmp(argv[optind],"--")) {
     permute(argv+permute_from,num_nonopts,1);
     optind = permute_from+1;
     return EOF;
    }
    break;
   case RETURN_IN_ORDER:
    if (!is_option(argv[optind],only)) {
     optarg = argv[optind++];
     return (optopt = 1);
    }
    break;
   case REQUIRE_ORDER:
    if (!is_option(argv[optind],only)) return EOF;
    break;
  }
 }
 if (!argv[optind]) return EOF;
 if (longopts &&
    (!memcmp(argv[optind],"--",2*sizeof(char)) ||
    (only && argv[optind][0] == '+')) && optwhere == 1) {
  int optindex; size_t match_chars;
  if (!memcmp(argv[optind],"--",2)) optwhere = 2;
  longopt_match = -1;
  possible_arg = strchr(argv[optind]+optwhere,'=');
  if (!possible_arg) {
   match_chars  = strlen(argv[optind]);
   possible_arg = argv[optind]+match_chars;
   match_chars  = match_chars-optwhere;
  } else {
   match_chars = (possible_arg-argv[optind])-optwhere;
  }
  for (optindex = 0; longopts[optindex].name; ++optindex) {
   if (!memcmp(argv[optind]+optwhere,longopts[optindex].name,match_chars)) {
    if (match_chars == strlen(longopts[optindex].name)) { longopt_match = optindex; break; }
    else {
     if (longopt_match < 0) longopt_match = optindex;
     else {
      if (opterr) {
       fputs(argv[0],stderr);
       fputs(": option `",stderr);
       fputs(argv[optind],stderr);
       fputs("' is ambiguous (could be `--",stderr);
       fputs(longopts[longopt_match].name,stderr);
       fputs("' or `--",stderr);
       fputs(longopts[optindex].name,stderr);
       fputs("')\n",stderr);
      }
      return (optopt = '?');
     }
    }
   }
  }
  if (longopt_match >= 0) {
   has_arg = longopts[longopt_match].has_arg;
  }
 }
 if (longopt_match < 0 && shortopts) {
  cp = strchr(shortopts,argv[optind][optwhere]);
  if (!cp) {
   if (opterr) {
    fputs(argv[0],stderr);
    fputs(": invalid option -- `-",stderr);
    fputc(argv[optind][optwhere],stderr);
    fputs("'\n",stderr);
   }
   optwhere++;
   if (argv[optind][optwhere] == '\0') optind++,optwhere = 1;
   return (optopt = '?');
  }
  has_arg = (cp[1] == ':') ? ((cp[2] == ':') ? OPTIONAL_ARG : REQUIRED_ARG) : NO_ARG;
  possible_arg = argv[optind]+optwhere+1;
  optopt = *cp;
 }
 arg_next = 0;
 switch (has_arg) {
  case OPTIONAL_ARG:
   if (*possible_arg == '=') ++possible_arg;
   optarg = (*possible_arg != '\0') ? possible_arg : NULL;
   optwhere = 1;
   break;
  case REQUIRED_ARG:
   if (*possible_arg == '=') ++possible_arg;
   if (*possible_arg != '\0') optarg = possible_arg,optwhere = 1;
   else if (optind+1 >= argc) {
    if (opterr) {
     fputs(argv[0],stderr);
     fputs(": argument required for option `-",stderr);
     if (longopt_match >= 0) {
      fputc('-',stderr);
      fputs(longopts[longopt_match].name,stderr);
      optopt = initial_colon ? ':' : '\?';
     } else {
      fputc(*cp,stderr);
      optopt = *cp;
     }
     fputs("'\n",stderr);
    }
    optind++;
    return initial_colon ? ':' : '\?';
   } else {
    optarg = argv[optind+1];
    arg_next = 1;
    optwhere = 1;
   }
   break;
  default:
  case NO_ARG:
   if (longopt_match < 0) {
    ++optwhere;
    if (argv[optind][optwhere] == '\0') optwhere = 1;
   } else optwhere = 1;
   optarg = 0;
   break;
 }
 if (ordering == PERMUTE && optwhere == 1 && num_nonopts != 0) {
  permute(argv+permute_from,num_nonopts,1+arg_next);
  optind = permute_from+1+arg_next;
 } else if (optwhere == 1) {
  optind = optind+1+arg_next;
 }
 if (longopt_match >= 0) {
  if (longind) *longind = longopt_match;
  if (longopts[longopt_match].flag) {
   *longopts[longopt_match].flag = longopts[longopt_match].val;
   return 0;
  } else {
   return longopts[longopt_match].val;
  }
 }
 return optopt;
}

__public int
getopt(int argc, char *argv[], const char *optstring) {
 return getopt_internal(argc,argv,optstring,NULL,NULL,0);
}

__public int
getopt_long(int argc, char *argv[], const char *shortopts,
            struct option const *longopts, int *longind) {
 return getopt_internal(argc,argv,shortopts,longopts,longind,0);
}
__public int
getopt_long_only(int argc, char *argv[], const char *shortopts,
                 struct option const *longopts, int *longind) {
 return getopt_internal(argc,argv,shortopts,longopts,longind,1);
}

__DECL_END
#endif /* !__KERNEL__ */

#endif /* !__GETOPT_C__ */

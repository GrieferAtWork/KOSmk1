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

#ifdef __INTELLISENSE__
#include "string.c"
#define STRN
#define STRI
__DECL_BEGIN
#endif

#ifdef STRI
#define CASTCH  STRICAST
#else
#define CASTCH 
#endif

#if defined(STRN) && defined(STRI)
__public int _strinwcmp(char const *__restrict str, size_t maxstr,
                        char const *__restrict wildpattern, size_t maxpattern)
#elif defined(STRN)
__public int _strnwcmp(char const *__restrict str, size_t maxstr,
                       char const *__restrict wildpattern, size_t maxpattern)
#elif defined(STRI)
__public int _striwcmp(char const *__restrict str, char const *__restrict wildpattern)
#else
__public int _strwcmp(char const *__restrict str, char const *__restrict wildpattern)
#endif
{
#ifdef STRN
 char const *string_end = str+maxstr;
 char const *pattern_end = wildpattern+maxpattern;
#define IF_STRN(x) x
#else
#define IF_STRN(x) 
#endif
 char card_post;
 for (;;) {
  assert(str != wildpattern);
  if (IF_STRN(str == string_end ||) !*str) {
   /* End of string (if the patter is empty, or only contains '*', we have a match) */
   while (IF_STRN(wildpattern != pattern_end &&) *wildpattern == '*') ++wildpattern;
   IF_STRN(if (wildpattern == pattern_end) return 0;)
   return -(int)CASTCH(*wildpattern);
  }
  if (IF_STRN(wildpattern == pattern_end ||)
      !*wildpattern) return (int)*str; /* Pattern end doesn't match */
  if (*wildpattern == '*') {
   /* Skip starts */
   do {
    ++wildpattern;
    IF_STRN(if (wildpattern == pattern_end) return 0;)
   } while (*wildpattern == '*');
   if ((card_post = *wildpattern++) == '\0')
    return 0; /* Pattern ends with '*' (matches everything) */
   if (card_post == '?') goto next; /* Match any --> already found */
#ifdef STRI
   card_post = CASTCH(card_post);
#endif
   for (;;) {
    char ch;
    IF_STRN(if (str == string_end) goto end_cardpost;)
    ch = *str++;
    if (CASTCH(ch) == card_post) {
     /* Recursively check if the rest of the string and pattern match */
#if defined(STRN) && defined(STRI)
     if (!_strinwcmp(str,(size_t)(string_end-str),wildpattern,(size_t)(pattern_end-wildpattern))) return 0;
#elif defined(STRN)
     if (!_strnwcmp(str,(size_t)(string_end-str),wildpattern,(size_t)(pattern_end-wildpattern))) return 0;
#elif defined(STRI)
     if (!_striwcmp(str,wildpattern)) return 0;
#else
     if (!_strwcmp(str,wildpattern)) return 0;
#endif
    } else if (!ch) {
     IF_STRN(end_cardpost:)
     return -(int)card_post; /* Wildcard suffix not found */
    }
   }
  }
  if (CASTCH(*wildpattern) == CASTCH(*str) ||
      *wildpattern == '?') {
next:
   ++str;
   ++wildpattern;
   continue; /* single character match */
  }
  break; /* mismatch */
 }
 return CASTCH(*str)-CASTCH(*wildpattern);
}

#undef IF_STRN
#undef CASTCH
#ifdef STRI
#undef STRI
#endif
#ifdef STRN
#undef STRN
#endif

#ifdef __INTELLISENSE__
__DECL_END
#endif


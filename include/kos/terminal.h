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
#ifndef __KOS_TERMINAL_H__
#define __KOS_TERMINAL_H__ 1

#include <kos/config.h>
#include <kos/compiler.h>

__DECL_BEGIN

// Terminal Color codes
#define TERM_COLOR_FG_BLACK         "0;30"
#define TERM_COLOR_FG_BLUE          "0;34"
#define TERM_COLOR_FG_GREEN         "0;32"
#define TERM_COLOR_FG_CYAN          "0;36"
#define TERM_COLOR_FG_RED           "0;31"
#define TERM_COLOR_FG_PURPLE        "0;35"
#define TERM_COLOR_FG_BROWN         "0;33"
#define TERM_COLOR_FG_LIGHT_GRAY    "0;37"
#define TERM_COLOR_FG_DARK_GRAY     "1;30"
#define TERM_COLOR_FG_LIGHT_BLUE    "1;34"
#define TERM_COLOR_FG_LIGHT_GREEN   "1;32"
#define TERM_COLOR_FG_LIGHT_CYAN    "1;36"
#define TERM_COLOR_FG_LIGHT_RED     "1;31"
#define TERM_COLOR_FG_LIGHT_PURPLE  "1;35"
#define TERM_COLOR_FG_YELLOW        "1;33"
#define TERM_COLOR_FG_WHITE         "1;37"

#define TERM_COLOR_BG_BLACK         "40"
#define TERM_COLOR_BG_BLUE          "44"
#define TERM_COLOR_BG_GREEN         "42"
#define TERM_COLOR_BG_CYAN          "46"
#define TERM_COLOR_BG_RED           "41"
#define TERM_COLOR_BG_PURPLE        "45"
#define TERM_COLOR_BG_BROWN         "43"
#define TERM_COLOR_BG_LIGHT_GRAY    "47"

#define TERM_COLOR(fg)     "\e[" fg "m"
#define TERM_COLOR2(bg,fg) "\e[" bg ";" fg "m"
#define TERM_COLOR3(bg)    "\e[" bg "m"
#define TERM_COLOR_RESET   TERM_COLOR(TERM_COLOR_FG_LIGHT_GRAY)
#define TERM_COLOR_RESETBG TERM_COLOR3(TERM_COLOR_BG_BLACK)

__DECL_END

#endif /* !__KOS_TERMINAL_H__ */

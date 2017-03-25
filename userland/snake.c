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
#ifndef __SNAKE_C__
#define __SNAKE_C__ 1


#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>
#include <proc.h>
#include <kos\syslog.h>

#define IN   STDIN_FILENO
#define OUT  STDOUT_FILENO

#define print(s) write(OUT,s,sizeof(s)-sizeof(char))

#define CELL_NONE   0
#define CELL_SNAKE  1
#define CELL_CHERRY 2
static char const *cell_kind[] = {
 "\033[0m",   /* reset */
 "\033[107m", /* white */
 "\033[101m", /* red */
};

static int cursor_x = 0;
static int cursor_y = 0;
static int screen_x = 80;
static int screen_y = 25;
static char const *last_cell = NULL;
static void setcursor(int x, int y) {
 if (cursor_x != x || cursor_y != y) {
  dprintf(OUT,"\033[%d;%dH",x+1,y+1);
  cursor_x = x;
  cursor_y = y;
 }
}
static void cell_set(int x, int y, int kind) {
 char const *newcell = cell_kind[kind];
 if (newcell != last_cell) {
  last_cell = newcell;
  dprintf(OUT,"%s",newcell);
 }
 setcursor(x,y);
 dprintf(OUT," \033[D");
}

struct cell { int x,y; };
struct snake {
 size_t       cellc;
 struct cell *cellv;
 int          dir;
};
#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

static struct snake my_snake = {0,NULL,DIR_NONE};
static void extend(size_t n) {
 struct cell *newcellv,*iter,*end;
 size_t newcellc = my_snake.cellc+n;
 newcellv = (struct cell *)realloc(my_snake.cellv,
                                   newcellc*sizeof(struct cell));
 my_snake.cellv = newcellv;
 newcellv += my_snake.cellc;
 my_snake.cellc = newcellc;
 end = (iter = newcellv)+n;
 --newcellv;
 for (; iter != end; ++iter) *iter = *newcellv;
}

static void drawhead(void) {
 cell_set(my_snake.cellv[0].x,
          my_snake.cellv[0].y,
          CELL_SNAKE);
}
static void drawtail(void) {
 cell_set(my_snake.cellv[my_snake.cellc-1].x,
          my_snake.cellv[my_snake.cellc-1].y,
          CELL_NONE);
}
static void domove(void) {
 struct cell *head = my_snake.cellv;
 memmove(head+1,head,(my_snake.cellc-1)*sizeof(struct cell));
 switch (my_snake.dir) {
  case DIR_UP:    if (--head->y < 0) head->y = screen_y-1; break;
  case DIR_DOWN:  if (++head->y >= screen_y) head->y = 0; break;
  case DIR_LEFT:  if (--head->x < 0) head->x = screen_x-1; break;
  case DIR_RIGHT: if (++head->x >= screen_x) head->x = 0; break;
  default: break;
 }
}
static void move(void) {
 if (my_snake.dir == DIR_NONE) return;
 drawtail();
 domove();
 drawhead();
}

static int mover_thread = -1;
static void *mover_threadmain(void *closure) {
 struct timespec tmo = {0,10000000l};
 for (;;) {
  task_sleep(task_self(),&tmo);
  move();
 }
 return closure;
}

static int process_input(void) {
#define handle(str) \
   ((!memcmp(iter,str,__COMPILER_STRINGSIZE(str)) ?\
     (iter += __COMPILER_STRINGSIZE(str),1) : 0))
 char buf[16]; ssize_t s;
 char *iter,*end;
 s = read(IN,buf,sizeof(buf));
 if (s <= 0) return 0;
 for (end = (iter = buf)+s; iter != end;) {
       if (handle("\033[D")) { my_snake.dir = DIR_LEFT; }
  else if (handle("\033[C")) { my_snake.dir = DIR_RIGHT; }
  else if (handle("\033[A")) { my_snake.dir = DIR_UP; }
  else if (handle("\033[B")) { my_snake.dir = DIR_DOWN; }
  else if (handle("k")) extend(20);
  else if (handle("\3")) return 0;
  else ++iter;
 }
#undef handle
 return 1;
}




static struct termios oldios;
static void init(void) {
 struct termios newios;
 char answer[64]; ssize_t rsize;
 tcgetattr(STDIN_FILENO,&oldios);
 memcpy(&newios,&oldios,sizeof(struct termios));
 newios.c_lflag &= ~(ICANON|ECHO);
 tcsetattr(STDIN_FILENO,TCSAFLUSH,&newios);
 print("\033[25l"
       "\033[s"
       "\033[4096;4096H"
       "\033[n");
 rsize = read(IN,answer,sizeof(answer));
 if (rsize >= 0) {
  answer[rsize] = '\0';
  sscanf(answer,"\033[%d;%dR",&screen_y,&screen_x);
  ++screen_x;
  ++screen_y;
 }
 print("\033[1;1H");
 my_snake.cellc = 1;
 my_snake.cellv = (struct cell *)calloc(my_snake.cellc,
                                        sizeof(struct cell));
 mover_thread = task_newthread(&mover_threadmain,NULL,TASK_NEWTHREAD_DEFAULT);
}

static void fini(void) {
 task_terminate(mover_thread,NULL);
 close(mover_thread);
 free(my_snake.cellv);
 print("\033[0m"
       "\033[25h"
       "\033[u");
 tcsetattr(STDIN_FILENO,TCSAFLUSH,&oldios);
}

int main(int argc, char *argv[]) {
 setenv("why","all work no play makes jack a dull boy",1);
 init();
 while (process_input());
 fini();
 return EXIT_SUCCESS;
}


#endif /* !__SNAKE_C__ */

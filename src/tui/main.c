#include "tui/main.h"
#include "board.h"
#include "game.h"
#include "screen.h"
#include <curses.h>
#include <stdio.h>
#include <string.h>

int run_pseudographical_mode(void) {
  //Initialising ncurses
  initscr();
  noecho();
  cbreak();
  timeout(0);

  //Check if colors are supported
  if (!has_colors()) {
    endwin();
    printf("Your terminal does not support color\n");
    return 0;
  }

  Screen* screen = screen_new();
  WINDOW* menu = newwin(20, 30, screen->y_max / 2 - 10, screen->x_max / 2 - 15);
  box(menu, 0, 0);
  mvwprintw(menu, 2, 3, "Number of Players:");
  mvwprintw(menu, 4, 3, "Number of Penguins:");
  refresh();
  wrefresh(menu);
  keypad(menu, true);
  int key;
  while ((key = getch()) != 27) {
    if (key == KEY_RESIZE) {
      wclear(menu);
      getmaxyx(stdscr, screen->y_max, screen->x_max);
      mvwin(menu, screen->y_max / 2 - 10, screen->x_max / 2 - 15);
      mvwprintw(menu, 3, 3, "Input a number:");
      box(menu, 0, 0);
      wrefresh(menu);
    }
  }

  getchar();
  endwin();
  return 0;
}
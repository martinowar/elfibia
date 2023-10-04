#include "elfibia.h"

#include <curses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>

#define MENU_FIRST_ROW 1
#define MENU_FIRST_COLUMN 1

#define CONTENT_FIRST_ROW 1
#define CONTENT_FIRST_COLUMN 50

typedef struct
{
    int menu_items_count;
    char **menu_strings;
    WINDOW *wnd_menu;
    ITEM **menu_items;
    MENU *main_menu;
    WINDOW *wnd_content;
} efb_visual_context;

static void destroy_menu(efb_visual_context *visual_ctx);
static void create_menu(efb_visual_context *visual_ctx);
static void redraw_view(efb_visual_context *visual_ctx);

static void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length) / 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y, x, "%s", string);
	wattroff(win, color);
	refresh();
}

static void init_view(efb_visual_context *visual_ctx, char **menu_strings, const int menu_items_count)
{
    visual_ctx->menu_strings = menu_strings;
    visual_ctx->menu_items_count = menu_items_count;
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);

    visual_ctx->wnd_menu = NULL;
    visual_ctx->menu_items = NULL;
}

static void destroy_menu(efb_visual_context *visual_ctx)
{
    if (visual_ctx->wnd_menu != NULL)
    {
        unpost_menu(visual_ctx->main_menu);
        free_menu(visual_ctx->main_menu);
        for (int idx = 0; idx < visual_ctx->menu_items_count; ++idx)
        {
            free_item(visual_ctx->menu_items[idx]);
        }

        delwin(visual_ctx->wnd_menu);
        free(visual_ctx->menu_items);
        visual_ctx->wnd_menu = NULL;
        visual_ctx->menu_items = NULL;
    }
}

static void create_menu(efb_visual_context *visual_ctx)
{
    int idx;

    visual_ctx->menu_items = (ITEM **)calloc(visual_ctx->menu_items_count, sizeof(ITEM *));
    for (idx = 0; idx < visual_ctx->menu_items_count; ++idx)
    {
        char * str = "NULL";
        if (visual_ctx->menu_strings[idx] != NULL)
        {
            str = visual_ctx->menu_strings[idx];
        }

        visual_ctx->menu_items[idx] = new_item(str, str);
    }

    visual_ctx->menu_items[idx] = new_item(NULL, NULL);

    visual_ctx->main_menu = new_menu((ITEM **)visual_ctx->menu_items);

    visual_ctx->wnd_menu = newwin(LINES - 5, 45, MENU_FIRST_ROW, MENU_FIRST_COLUMN);

    keypad(visual_ctx->wnd_menu, TRUE);

    set_menu_win(visual_ctx->main_menu, visual_ctx->wnd_menu);

    // TODO check if the X and Y values are correct
    // TODO Do not use magic numbers
    set_menu_sub(visual_ctx->main_menu, derwin(visual_ctx->wnd_menu, LINES - 5 - 4, 38, 3, 1));
    set_menu_format(visual_ctx->main_menu, LINES - 5 - 4, 1);

    set_menu_mark(visual_ctx->main_menu, " > ");

    box(visual_ctx->wnd_menu, 0, 0);
    print_in_middle(visual_ctx->wnd_menu, 1, 0, 45, "Sections", COLOR_PAIR(1));
    mvwaddch(visual_ctx->wnd_menu, 2, 0, ACS_LTEE);
    mvwhline(visual_ctx->wnd_menu, 2, 1, ACS_HLINE, 43);
    mvwaddch(visual_ctx->wnd_menu, 2, 44, ACS_RTEE);

    post_menu(visual_ctx->main_menu);
    // TODO: check why the menu is not drawn if the next lines are uncommented
//    wrefresh(visual_ctx->wnd_menu);
//    refresh();
}


static void print_menu_item_content(efb_visual_context *visual_ctx, const int item_idx)
{
    wclear(visual_ctx->wnd_content);
    // TODO Draw the box in a different function
    box(visual_ctx->wnd_content, 0, 0);
    wattron(visual_ctx->wnd_content, COLOR_PAIR(1));
    mvwprintw(visual_ctx->wnd_content, 1, 1, "%s", get_menu_item_content(item_idx));
    wattroff(visual_ctx->wnd_content, COLOR_PAIR(1));
    wrefresh(visual_ctx->wnd_content);
}

static void create_content_view(efb_visual_context *visual_ctx)
{
    visual_ctx->wnd_content = newwin(LINES - 5, 45, CONTENT_FIRST_ROW, CONTENT_FIRST_COLUMN);

    box(visual_ctx->wnd_content, 0, 0);

//    print_menu_item_content(visual_ctx, item_index(current_item(visual_ctx->main_menu)));
}

static void redraw_view(efb_visual_context *visual_ctx)
{
    destroy_menu(visual_ctx);
    create_menu(visual_ctx);
    create_content_view(visual_ctx);

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(LINES - 1, 0, "Navigation: UP / DOWN / PgUp / PgDown / Home / End; Exit: q)");
    attroff(COLOR_PAIR(2));

    refresh();
    // TODO check why the menu is shown only if wrefresh() is called after refresh()
    wrefresh(visual_ctx->wnd_menu);
    wrefresh(visual_ctx->wnd_content);
}

void efb_draw_view(char **menu_strings, const int menu_items_count)
{
    efb_visual_context efb_visual_ctx;
    init_view(&efb_visual_ctx, menu_strings, menu_items_count);
    redraw_view(&efb_visual_ctx);

    int ch_key;

    // TODO which wnd should handle the keys?
    // TODO check if wgetch calls wrefresh()
//    while((ch_key = wgetch(efb_visual_ctx.wnd_menu)) != 'q')
    while((ch_key = wgetch(stdscr)) != 'q')
    {
        switch(ch_key)
        {
            case KEY_DOWN:
                menu_driver(efb_visual_ctx.main_menu, REQ_DOWN_ITEM);
                print_menu_item_content(&efb_visual_ctx, item_index(current_item(efb_visual_ctx.main_menu)));
                break;
            case KEY_UP:
                menu_driver(efb_visual_ctx.main_menu, REQ_UP_ITEM);
                print_menu_item_content(&efb_visual_ctx, item_index(current_item(efb_visual_ctx.main_menu)));
                break;
            case KEY_NPAGE:
                menu_driver(efb_visual_ctx.main_menu, REQ_SCR_DPAGE);
                print_menu_item_content(&efb_visual_ctx, item_index(current_item(efb_visual_ctx.main_menu)));
                break;
            case KEY_PPAGE:
                menu_driver(efb_visual_ctx.main_menu, REQ_SCR_UPAGE);
                print_menu_item_content(&efb_visual_ctx, item_index(current_item(efb_visual_ctx.main_menu)));
                break;
            case KEY_HOME:
                menu_driver(efb_visual_ctx.main_menu, REQ_FIRST_ITEM);
                print_menu_item_content(&efb_visual_ctx, item_index(current_item(efb_visual_ctx.main_menu)));
                break;
            case KEY_END:
                menu_driver(efb_visual_ctx.main_menu, REQ_LAST_ITEM);
                print_menu_item_content(&efb_visual_ctx, item_index(current_item(efb_visual_ctx.main_menu)));
                break;
            case KEY_RESIZE:
                redraw_view(&efb_visual_ctx);
                break;
		}

        wrefresh(efb_visual_ctx.wnd_menu);
        refresh();
	}

	destroy_menu(&efb_visual_ctx);

	endwin();
}

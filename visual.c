#include "elfibia.h"

#include <curses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>

#define STATUS_LINE_HEIGHT 1

#define MENU_WIDTH 45
#define MENU_FIRST_ROW 0
#define MENU_FIRST_COLUMN 0

#define FIRST_MENU_INDEX 0

#define CONTENT_FIRST_ROW 1
#define CONTENT_FIRST_COLUMN (MENU_WIDTH + 3)
#define CONTENT_HEIGHT (LINES - 3)

typedef struct
{
    int menu_items_count;
    int content_top_row;
    int content_row_count;
    char **menu_strings;
    WINDOW *wnd_menu;
    ITEM **menu_items;
    MENU *main_menu;
    WINDOW *wnd_content_box;
    WINDOW *wnd_content;
} efb_visual_context;

static void init_view(efb_visual_context *visual_ctx, char **menu_strings, const int menu_items_count)
{
    visual_ctx->menu_strings = menu_strings;
    visual_ctx->menu_items_count = menu_items_count;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_MAGENTA);
    init_pair(2, COLOR_WHITE, COLOR_RED);

    visual_ctx->wnd_menu = NULL;
    visual_ctx->menu_items = NULL;
    visual_ctx->wnd_content_box = NULL;
    visual_ctx->wnd_content = NULL;
}

static void destroy_menu(efb_visual_context *visual_ctx)
{
    if (visual_ctx->wnd_menu != NULL)
    {
        for (int idx = 0; idx < visual_ctx->menu_items_count; idx++)
        {
            free_item(visual_ctx->menu_items[idx]);
        }

        unpost_menu(visual_ctx->main_menu);
        free_menu(visual_ctx->main_menu);

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
    for (idx = 0; idx < visual_ctx->menu_items_count; idx++)
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
    visual_ctx->wnd_menu = newwin(LINES - STATUS_LINE_HEIGHT, MENU_WIDTH, MENU_FIRST_ROW, MENU_FIRST_COLUMN);
    set_menu_win(visual_ctx->main_menu, visual_ctx->wnd_menu);

    // TODO Do not use magic numbers
    set_menu_sub(visual_ctx->main_menu, derwin(visual_ctx->wnd_menu, LINES - STATUS_LINE_HEIGHT - 1, 38, MENU_FIRST_ROW + 1, MENU_FIRST_COLUMN + 1));
    set_menu_format(visual_ctx->main_menu, LINES - STATUS_LINE_HEIGHT - 1, 1);
    set_menu_mark(visual_ctx->main_menu, " > ");
    box(visual_ctx->wnd_menu, 0, 0);
    post_menu(visual_ctx->main_menu);
}

static void redraw_content_view(efb_visual_context *visual_ctx)
{
    if (visual_ctx->wnd_content_box != NULL)
    {
        wclear(visual_ctx->wnd_content_box);
        wrefresh(visual_ctx->wnd_content_box);
        delwin(visual_ctx->wnd_content_box);
    }

    visual_ctx->wnd_content_box = newwin(LINES - STATUS_LINE_HEIGHT, COLS - MENU_WIDTH - 3, MENU_FIRST_ROW, CONTENT_FIRST_COLUMN);
    box(visual_ctx->wnd_content_box, 0, 0);
    wrefresh(visual_ctx->wnd_content_box);

    if (visual_ctx->wnd_content != NULL)
    {
        static int counter = 0;
        wattrset(visual_ctx->wnd_content, COLOR_PAIR(1));
// TODO remove this debug line
        mvprintw(0, COLS - 40, "(%d) (menu count: %d)", counter++, visual_ctx->menu_items_count);

        wnoutrefresh(stdscr);
        pnoutrefresh(visual_ctx->wnd_content, visual_ctx->content_top_row, 0, CONTENT_FIRST_ROW, CONTENT_FIRST_COLUMN + 1, LINES - STATUS_LINE_HEIGHT - 2, COLS - 2);
        doupdate();
    }
}

static void display_menu_item_content(efb_visual_context *visual_ctx, const int item_idx)
{
    char *ptr_content = efb_get_menu_item_content(item_idx);
    char *ptr_tmp_content = ptr_content;

    visual_ctx->content_row_count = 0;

    while (*ptr_tmp_content != '\0')
    {
        while ((*ptr_tmp_content != '\n') && (*ptr_tmp_content != '\0'))
        {
            ptr_tmp_content++;
        }

        if (*ptr_tmp_content == '\n')
        {
            visual_ctx->content_row_count++;
            ptr_tmp_content++;
        }
    }

    visual_ctx->content_row_count++;

    if (visual_ctx->wnd_content != NULL)
    {
        wclear(visual_ctx->wnd_content);
        wrefresh(visual_ctx->wnd_content);
        delwin(visual_ctx->wnd_content);
        visual_ctx->content_top_row = 0;
    }

    visual_ctx->wnd_content = newpad(visual_ctx->content_row_count, COLS - MENU_WIDTH);
    wattrset(visual_ctx->wnd_content, COLOR_PAIR(1));
    wbkgd(visual_ctx->wnd_content, (chtype) (' ' | COLOR_PAIR(2)));

    int row_idx = 0;
    while (*ptr_content != '\0')
    {
        wmove(visual_ctx->wnd_content, row_idx, 0);

        while ((*ptr_content != '\n') && (*ptr_content != '\0'))
        {
            waddch(visual_ctx->wnd_content, *ptr_content & 0xff);
            ptr_content++;
        }

        if (*ptr_content == '\n')
        {
            row_idx++;
            ptr_content++;
        }
    }

    redraw_content_view(visual_ctx);
}

static void redraw_view(efb_visual_context *visual_ctx)
{
    destroy_menu(visual_ctx);
    create_menu(visual_ctx);

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(LINES - 1, 0, "Menu: KeyUp / KeyDown / PgUp / PgDown / Home / End; Content: k (UP) / j (DOWN); Exit: q");
    attroff(COLOR_PAIR(2));

    refresh();
    wrefresh(visual_ctx->wnd_menu);

    redraw_content_view(visual_ctx);
}

static void process_key_press(efb_visual_context *visual_ctx, const int pressed_key)
{
    menu_driver(visual_ctx->main_menu, pressed_key);
    display_menu_item_content(visual_ctx, item_index(current_item(visual_ctx->main_menu)));
    wrefresh(visual_ctx->wnd_menu);
}

void efb_draw_view(char **menu_strings, const int menu_items_count)
{
    efb_visual_context efb_visual_ctx;
    init_view(&efb_visual_ctx, menu_strings, menu_items_count);
    display_menu_item_content(&efb_visual_ctx, FIRST_MENU_INDEX);
    redraw_view(&efb_visual_ctx);

    int ch_key;

    while((ch_key = wgetch(stdscr)) != 'q')
    {
        switch(ch_key)
        {
            case 'k': // scroll the menu item content
                if (efb_visual_ctx.content_top_row > 0)
                {
                    efb_visual_ctx.content_top_row--;
                }
                redraw_content_view(&efb_visual_ctx);
                break;
            case 'j': // scroll the menu item content
                if (efb_visual_ctx.content_top_row < (efb_visual_ctx.content_row_count - LINES + STATUS_LINE_HEIGHT + 1))
                {
                    efb_visual_ctx.content_top_row++;
                }
                redraw_content_view(&efb_visual_ctx);
                break;
            case KEY_DOWN:
                process_key_press(&efb_visual_ctx, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                process_key_press(&efb_visual_ctx, REQ_UP_ITEM);
                break;
            case KEY_NPAGE:
                process_key_press(&efb_visual_ctx, REQ_SCR_DPAGE);
                break;
            case KEY_PPAGE:
                process_key_press(&efb_visual_ctx, REQ_SCR_UPAGE);
                break;
            case KEY_HOME:
                process_key_press(&efb_visual_ctx, REQ_FIRST_ITEM);
                break;
            case KEY_END:
                process_key_press(&efb_visual_ctx, REQ_LAST_ITEM);
                break;
            case KEY_RESIZE:
                // TODO Should I keep the current menu element selected?
                redraw_view(&efb_visual_ctx);
                break;
		}

        refresh();
	}

    destroy_menu(&efb_visual_ctx);

	if (efb_visual_ctx.wnd_content != NULL)
    {
        delwin(efb_visual_ctx.wnd_content);
    }

	endwin();
}

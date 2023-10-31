#include "elfibia.h"

#include <curses.h>
#include <menu.h>
#include <stdlib.h>
#include <string.h>

#define STATUS_LINE_HEIGHT 1

#define MENU_WIDTH 45
#define MENU_HEIGHT (LINES - STATUS_LINE_HEIGHT)
#define MENU_FIRST_ROW 0
#define MENU_FIRST_COLUMN 0

#define FIRST_MENU_INDEX 0

#define CONTENT_BOX_FIRST_ROW (MENU_FIRST_ROW)
#define CONTENT_BOX_FIRST_COLUMN (MENU_WIDTH)
#define CONTENT_BOX_WIDTH (COLS - CONTENT_BOX_FIRST_COLUMN)
#define CONTENT_BOX_HEIGHT (LINES - CONTENT_BOX_FIRST_ROW - STATUS_LINE_HEIGHT)

#define CONTENT_INDENT 1
#define CONTENT_FIRST_COLUMN (CONTENT_BOX_FIRST_COLUMN + CONTENT_INDENT)
#define CONTENT_FIRST_ROW (CONTENT_BOX_FIRST_ROW + CONTENT_INDENT)
#define CONTENT_WIDTH (CONTENT_BOX_WIDTH - 2 * CONTENT_INDENT)
#define CONTENT_HEIGHT (CONTENT_BOX_HEIGHT - 2 * CONTENT_INDENT)

typedef struct
{
    int menu_items_count;
    int content_top_row;
    int content_row_count;
    item_data *it_data;
    WINDOW *wnd_menu;
    ITEM **menu_items;
    MENU *main_menu;
    WINDOW *wnd_content_box;
    WINDOW *wnd_content;
} efb_draw_context;

static void init_view(efb_draw_context *draw_ctx, item_data *it_data, const int menu_items_count)
{
    draw_ctx->it_data = it_data;
    draw_ctx->menu_items_count = menu_items_count;
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_GREEN);
    init_pair(2, COLOR_WHITE, COLOR_RED);

    draw_ctx->wnd_menu = NULL;
    draw_ctx->menu_items = NULL;
    draw_ctx->wnd_content_box = NULL;
    draw_ctx->wnd_content = NULL;
}

static void destroy_menu(efb_draw_context *draw_ctx)
{
    if (draw_ctx->wnd_menu != NULL)
    {
        for (int idx = 0; idx < draw_ctx->menu_items_count; idx++)
        {
            free_item(draw_ctx->menu_items[idx]);
        }

        unpost_menu(draw_ctx->main_menu);
        free_menu(draw_ctx->main_menu);

        delwin(draw_ctx->wnd_menu);
        free(draw_ctx->menu_items);
        draw_ctx->wnd_menu = NULL;
        draw_ctx->menu_items = NULL;
    }
}

static void create_menu(efb_draw_context *draw_ctx)
{
    int idx;

    draw_ctx->menu_items = (ITEM **)calloc(draw_ctx->menu_items_count + 1, sizeof(ITEM *));
    for (idx = 0; idx < draw_ctx->menu_items_count; idx++)
    {
        char * str_name = "NULL";
        char * str_descr = "NULL";
        if (draw_ctx->it_data[idx].item_name != NULL)
        {
            str_name = draw_ctx->it_data[idx].item_name;
        }

        if (draw_ctx->it_data[idx].item_descr != NULL)
        {
            str_descr = draw_ctx->it_data[idx].item_descr;
        }

        draw_ctx->menu_items[idx] = new_item(str_name, str_descr);
    }

    draw_ctx->menu_items[idx] = NULL;

    draw_ctx->main_menu = new_menu((ITEM **)draw_ctx->menu_items);
    draw_ctx->wnd_menu = newwin(MENU_HEIGHT, MENU_WIDTH, MENU_FIRST_ROW, MENU_FIRST_COLUMN);
    set_menu_win(draw_ctx->main_menu, draw_ctx->wnd_menu);

    // TODO Do not use magic numbers
    set_menu_sub(draw_ctx->main_menu, derwin(draw_ctx->wnd_menu, MENU_HEIGHT - 1, MENU_WIDTH - 2, MENU_FIRST_ROW + 1, MENU_FIRST_COLUMN + 1));
    set_menu_format(draw_ctx->main_menu, MENU_HEIGHT - 2, 1);
    set_menu_mark(draw_ctx->main_menu, " > ");
    box(draw_ctx->wnd_menu, 0, 0);
    post_menu(draw_ctx->main_menu);
}

static void redraw_content_view(efb_draw_context *draw_ctx)
{
    if (draw_ctx->wnd_content_box != NULL)
    {
        wclear(draw_ctx->wnd_content_box);
        wrefresh(draw_ctx->wnd_content_box);
        delwin(draw_ctx->wnd_content_box);
    }

    draw_ctx->wnd_content_box = newwin(CONTENT_BOX_HEIGHT, CONTENT_BOX_WIDTH, CONTENT_BOX_FIRST_ROW, CONTENT_BOX_FIRST_COLUMN);
    box(draw_ctx->wnd_content_box, 0, 0);
    wrefresh(draw_ctx->wnd_content_box);

    if (draw_ctx->wnd_content != NULL)
    {
        wattrset(draw_ctx->wnd_content, COLOR_PAIR(1));
        wnoutrefresh(stdscr);
        pnoutrefresh(draw_ctx->wnd_content, draw_ctx->content_top_row, 0, CONTENT_FIRST_ROW, CONTENT_FIRST_COLUMN, CONTENT_FIRST_ROW + CONTENT_HEIGHT - 1, CONTENT_FIRST_COLUMN + CONTENT_WIDTH - 1);
        doupdate();
    }
}

static void display_menu_item_content(efb_draw_context *draw_ctx, const int item_idx)
{
    char *ptr_content = efb_get_menu_item_content(item_idx);
    char *ptr_tmp_content = ptr_content;

    draw_ctx->content_row_count = 0;

    while (*ptr_tmp_content != '\0')
    {
        while ((*ptr_tmp_content != '\n') && (*ptr_tmp_content != '\0'))
        {
            ptr_tmp_content++;
        }

        if (*ptr_tmp_content == '\n')
        {
            draw_ctx->content_row_count++;
            ptr_tmp_content++;
        }
    }

    draw_ctx->content_row_count++;

    if (draw_ctx->wnd_content != NULL)
    {
        wclear(draw_ctx->wnd_content);
        wrefresh(draw_ctx->wnd_content);
        delwin(draw_ctx->wnd_content);
        draw_ctx->content_top_row = 0;
    }

    draw_ctx->wnd_content = newpad(draw_ctx->content_row_count, COLS - MENU_WIDTH);
    wattrset(draw_ctx->wnd_content, COLOR_PAIR(1));
    wbkgd(draw_ctx->wnd_content, (chtype) (' ' | COLOR_PAIR(1)));

    int row_idx = 0;
    while (*ptr_content != '\0')
    {
        wmove(draw_ctx->wnd_content, row_idx, 0);

        while ((*ptr_content != '\n') && (*ptr_content != '\0'))
        {
            waddch(draw_ctx->wnd_content, *ptr_content & 0xff);
            ptr_content++;
        }

        if (*ptr_content == '\n')
        {
            row_idx++;
            ptr_content++;
        }
    }

    redraw_content_view(draw_ctx);
}

static void redraw_view(efb_draw_context *draw_ctx)
{
    destroy_menu(draw_ctx);
    create_menu(draw_ctx);

    clear();
    attron(COLOR_PAIR(2));
    mvprintw(LINES - 1, 0, "Menu: KeyUp / KeyDown / PgUp / PgDown / Home / End; Content: k (UP) / j (DOWN); Exit: q");
    attroff(COLOR_PAIR(2));

    refresh();
    wrefresh(draw_ctx->wnd_menu);

    redraw_content_view(draw_ctx);
}

static void process_key_press(efb_draw_context *draw_ctx, const int pressed_key)
{
    menu_driver(draw_ctx->main_menu, pressed_key);
    display_menu_item_content(draw_ctx, item_index(current_item(draw_ctx->main_menu)));
    wrefresh(draw_ctx->wnd_menu);
}

void efb_draw_view(item_data *it_data, const int menu_items_count)
{
    efb_draw_context efb_draw_ctx;
    init_view(&efb_draw_ctx, it_data, menu_items_count);
    display_menu_item_content(&efb_draw_ctx, FIRST_MENU_INDEX);
    redraw_view(&efb_draw_ctx);

    int ch_key;

    while((ch_key = wgetch(stdscr)) != 'q')
    {
        switch(ch_key)
        {
            case 'k': // scroll the menu item content
                if (efb_draw_ctx.content_top_row > 0)
                {
                    efb_draw_ctx.content_top_row--;
                }
                redraw_content_view(&efb_draw_ctx);
                break;
            case 'j': // scroll the menu item content
                if (efb_draw_ctx.content_top_row < (efb_draw_ctx.content_row_count - LINES + STATUS_LINE_HEIGHT + 1))
                {
                    efb_draw_ctx.content_top_row++;
                }
                redraw_content_view(&efb_draw_ctx);
                break;
            case KEY_DOWN:
                process_key_press(&efb_draw_ctx, REQ_DOWN_ITEM);
                break;
            case KEY_UP:
                process_key_press(&efb_draw_ctx, REQ_UP_ITEM);
                break;
            case KEY_NPAGE:
                process_key_press(&efb_draw_ctx, REQ_SCR_DPAGE);
                break;
            case KEY_PPAGE:
                process_key_press(&efb_draw_ctx, REQ_SCR_UPAGE);
                break;
            case KEY_HOME:
                process_key_press(&efb_draw_ctx, REQ_FIRST_ITEM);
                break;
            case KEY_END:
                process_key_press(&efb_draw_ctx, REQ_LAST_ITEM);
                break;
            case KEY_RESIZE:
                // TODO Should I keep the current menu element selected?
                redraw_view(&efb_draw_ctx);
                break;
		}

        refresh();
	}

    destroy_menu(&efb_draw_ctx);

	if (efb_draw_ctx.wnd_content != NULL)
    {
        delwin(efb_draw_ctx.wnd_content);
    }

	endwin();
}

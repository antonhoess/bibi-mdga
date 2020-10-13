//XXX KI coden?

#include <cairo.h>
#include <canberra.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#define FIELD_DIM       30
#define AREA_DIM        11
#define SAVED_MOVES_CNT 10000


typedef struct point_s {
  int x;
  int y;
} point_t;

typedef struct color_s {
  float r;
  float g;
  float b;
} color_t;

typedef enum {
  PLAYER_COLOR_BLUE,
  PLAYER_COLOR_RED,
  PLAYER_COLOR_YELLOW,
  PLAYER_COLOR_GREEN,
  PLAYER_COLOR_CNT,
  PLAYER_COLOR_NONE,
} player_colors_t;

typedef enum {
  FIELD_TYPE_PARKING,
  FIELD_TYPE_START,
  FIELD_TYPE_NORMAL,
  FIELD_TYPE_GOAL,
  FIELD_TYPE_CNT,
} field_type_t;

typedef enum {
  MODE_INIT,
  MODE_REC,
  MODE_GAME,
  MODE_CNT,
} basic_mode_t;

typedef enum {
  MODE_REC_NONE,
  MODE_REC_RECORD,
  MODE_REC_REPLAY,
  MODE_REC_CNT,
} rec_mode_t;

typedef enum {
  MODE_GAME_NONE,
  MODE_GAME_INIT,
  MODE_GAME_CHOOSE_PLAYER,
  MODE_GAME_PLAY,
  MODE_GAME_FINISHED,
  MODE_GAME_CNT,
} game_mode_t;

typedef enum {
  MODE_GAME_PLAY_NONE,
  MODE_GAME_PLAY_ROLL,
  MODE_GAME_PLAY_MOVE,
  MODE_GAME_PLAY_CHANGE_PLAYER,
  MODE_GAME_PLAY_CNT,
} play_game_mode_t;

typedef enum {
  MODE_TYPE_BASIC,
  MODE_TYPE_REC,
  MODE_TYPE_GAME,
  MODE_TYPE_GAME_PLAY,
  MODE_TYPE_CNT,
} mode_type_t;

typedef struct field_s {
  int x;
  int y;
  player_colors_t p;
  field_type_t t;
} field_t;

typedef struct figure_s {
  player_colors_t p;
  int i; // Field index
  field_type_t t; // Combined with index
} figure_t;

typedef struct player_s {
  figure_t f[4];
  int playing;
} player_t;

basic_mode_t mode = MODE_INIT;
rec_mode_t rec_mode = MODE_REC_NONE;
game_mode_t game_mode = MODE_GAME_NONE;
play_game_mode_t play_game_mode = MODE_GAME_PLAY_NONE;

field_t fields[(AREA_DIM - 1) * 4];
field_t fields_parking[PLAYER_COLOR_CNT][4];
field_t fields_goal[PLAYER_COLOR_CNT][4];
color_t color[PLAYER_COLOR_CNT];
player_t player[PLAYER_COLOR_CNT];
ca_context *cba_ctx_sound = NULL;
int cp = 0; // Current player
int number = 0;
int start_roll_cnt = 0;
int force_start_cnt = -1;
int figure_movable[4];

int rmode = 0; // record / replay: 0 = off, 1 = active // Can only set active bevor starting the game and cannot be set active after setting it off durgin the game
int saved_moves[SAVED_MOVES_CNT]; // Moves with following types (0 = empty, -4..-1 = figure number to move, 1..4 = roll number)
int saved_moves_cnt = 0;

static void
do_drawing(cairo_t *);

static void
refresh();


GtkWidget *window = NULL;
GtkWidget *darea = NULL;
point_t ws; // Window-size


static mode_type_t
get_mode_type()
{
  // Backwards through the hierarchy
  // ../.. level 2
  if(play_game_mode != MODE_GAME_PLAY_NONE)
    return MODE_TYPE_GAME_PLAY;
  // ../.. level 1
  else if(game_mode != MODE_GAME_NONE)
    return MODE_TYPE_GAME;
  // ../.. level 1
  else if(rec_mode != MODE_REC_NONE)
    return MODE_TYPE_REC;
  // ../.. level 0
  else
    return MODE_TYPE_BASIC;

  return -1;
}


static int
get_mode(mode_type_t t)
{
  switch(t) {
    case MODE_TYPE_GAME_PLAY:
      return play_game_mode;
      break;

    case MODE_TYPE_GAME:
      return game_mode;
      break;

    case MODE_TYPE_REC:
      return rec_mode;
      break;

    case MODE_TYPE_BASIC:
      return mode;
      break;

    case MODE_TYPE_CNT:
      break;
  }

  return -1;
}


static char *
get_mode_type_str(mode_type_t t)
{
  char *str = NULL;

  switch(t) {
    case MODE_TYPE_BASIC:
      str = "MODE_TYPE_BASIC";
      break;

    case MODE_TYPE_REC:
      str = "MODE_TYPE_REC";

      break;
    case MODE_TYPE_GAME:
      str = "MODE_TYPE_GAME";
      break;

    case MODE_TYPE_GAME_PLAY:
      str = "MODE_TYPE_GAME_PLAY";
      break;

    case MODE_TYPE_CNT:
      str = "";
  }

  return str;
}


static char *
get_mode_str(mode_type_t t, int m)
{
  char *str = NULL;

  switch(t) {
    case MODE_TYPE_BASIC:
      switch((basic_mode_t)m) {
        case MODE_INIT:
          str = "MODE_INIT";
          break;

        case MODE_REC:
          str = "MODE_REC";
          break;

        case MODE_GAME:
          str = "MODE_GAME";
          break;

        case MODE_CNT:
          str = "";
          break;
      }
      break;

    case MODE_TYPE_REC:
      switch((rec_mode_t)m) {
        case MODE_REC_NONE:
          str = "MODE_REC_NONE";
          break;

        case MODE_REC_RECORD:
          str = "MODE_REC_RECORD";
          break;

        case MODE_REC_REPLAY:
          str = "MODE_REC_REPLAY";
          break;

        case MODE_REC_CNT:
          str = "";
          break;
      }
      break;

    case MODE_TYPE_GAME:
      switch((game_mode_t)m) {
        case MODE_GAME_NONE:
          str = "MODE_GAME_NONE";
          break;

        case MODE_GAME_INIT:
          str = "MODE_GAME_INIT";
          break;

        case MODE_GAME_CHOOSE_PLAYER:
          str = "MODE_GAME_CHOOSE_PLAYER";
          break;

        case MODE_GAME_PLAY:
          str = "MODE_GAME_PLAY";
          break;

        case MODE_GAME_FINISHED:
          str = "MODE_GAME_FINISHED";
          break;

        case MODE_GAME_CNT:
          str = "";
          break;
      }
      break;

    case MODE_TYPE_GAME_PLAY:
      switch((play_game_mode_t)m) {
        case MODE_GAME_PLAY_NONE:
          str = "MODE_GAME_PLAY_NONE";
          break;

        case MODE_GAME_PLAY_ROLL:
          str = "MODE_GAME_PLAY_ROLL";
          break;

        case MODE_GAME_PLAY_MOVE:
          str = "MODE_GAME_PLAY_MOVE";
          break;

        case MODE_GAME_PLAY_CHANGE_PLAYER:
          str = "MODE_GAME_PLAY_CHANGE_PLAYER";
          break;

        case MODE_GAME_PLAY_CNT:
          str = "";
          break;
      }
      break;

    case MODE_TYPE_CNT:
      str = "";
      break;
  }

  return str;
}


// In this function we apply all implicit (!) mode-switch rules
static int
set_mode(mode_type_t t, int m)
{
  //XXX Einbauen, so dass er rekursiz resettet in alle zweite hoch - aber wohl nur wenn die modi komplizierter werden und sich die Zweige in mehr als zwei Ebenen verzweigen
  int told = 0, tnew = 0;
  int mold = 0, mnew = 0;

  told = get_mode_type();
  mold = get_mode(told);
  tnew = t;
  mnew = m;

  // Backwards through the hierarchy
  // ../.. level 2
  if(t == MODE_TYPE_GAME_PLAY) {
    play_game_mode = m;
    t = MODE_TYPE_GAME;
    m = MODE_GAME_PLAY;
  }

  // .. level 1
  if(t == MODE_TYPE_GAME) {
    // Set
    game_mode = m;
    t = MODE_TYPE_BASIC;
    m = MODE_GAME;
    // Reset
    rec_mode = MODE_REC_NONE;
  }

  if(t == MODE_TYPE_REC) {
    // Set
    rec_mode = m;
    t = MODE_TYPE_BASIC;
    m = MODE_REC;
    // Reset
    game_mode = MODE_GAME_NONE;
  }

  // .. level 0
  if(t == MODE_TYPE_BASIC) {
    // Set
    mode = m;
  }

  if(told != tnew || mold != mnew) {
    printf("Mode changes: (%-20s | %-25s)\n", get_mode_type_str(told), get_mode_str(told, mold));
    printf("(%d|%d)->(%d|%d)  (%-20s | %-25s)\n\n", told, mold, tnew, mnew, get_mode_type_str(tnew), get_mode_str(tnew, mnew));
  }

  return 0;
}


static void
create_field(field_t *f, int x, int y, player_colors_t p, field_type_t t)
{
  if(f != NULL) {
    f->x = x;
    f->y = y;
    f->p = p;
    f->t = t;
  }
}


static void
init_colors()
{
  color[PLAYER_COLOR_BLUE] = (color_t){0.0, 0.0, 1.0};
  color[PLAYER_COLOR_RED] = (color_t){1.0, 0.0, 0.0};
  color[PLAYER_COLOR_YELLOW] = (color_t){1.0, 1.0, 0.0};
  color[PLAYER_COLOR_GREEN] = (color_t){0.0, 1.0, 0.0};
}


static void
create_figures()
{
  int i = 0;
  int p = 0; // Player
  int f = 0; // Field-index
  int x = 0, y = 0;
  int hw = (AREA_DIM - 1) / 2; // Half width

  for(p = 0; p < PLAYER_COLOR_CNT; p++) {
    // Parking
    switch(p) {
      case 0:
        x = 0, y = 0;
        break;

      case 1:
        x = AREA_DIM - 2, y = 0;
        break;

      case 2:
        x = AREA_DIM - 2, y = AREA_DIM - 2;
        break;

      case 3:
        x = 0, y = AREA_DIM - 2;
        break;

      default:
        break;
    }

    for(i = 0; i < 4; i++) {
      create_field(&fields_parking[p][i], x + i % 2, y + i / 2, p, FIELD_TYPE_PARKING);
    }

    // Goal
    for(i = 0; i < 4; i++) {
      if(p == 0)
        create_field(&fields_goal[p][i], 1 + i, hw, p, FIELD_TYPE_GOAL);
      else if(p == 1)
        create_field(&fields_goal[p][i], hw, 1 + i, p, FIELD_TYPE_GOAL);
      else if(p == 2)
        create_field(&fields_goal[p][i], AREA_DIM - 2 - i, (AREA_DIM - 1) / 2, p, FIELD_TYPE_GOAL);
      else //if(p == 3)
        create_field(&fields_goal[p][i], hw, AREA_DIM - 2 - i, p, FIELD_TYPE_GOAL);
    }
  }

  // Normal
  f = 0;
  // .. player 1's block
  create_field(&fields[f++], 0, hw - 1, 0, FIELD_TYPE_START); // Start

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], 1 + i, hw - 1, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw - 1, hw - 2 - i, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  create_field(&fields[f++], hw, 0, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  // .. player 2's block
  create_field(&fields[f++], hw + 1, 0, 1, FIELD_TYPE_START); // Start

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw + 1, 1 + i, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw + 2 + i, hw - 1, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  create_field(&fields[f++], hw * 2, hw, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  // .. player 3's block
  create_field(&fields[f++], hw * 2, hw + 1, 2, FIELD_TYPE_START); // Start

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw * 2 - 1 - i, hw + 1, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw + 1, hw + 2 + i, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  create_field(&fields[f++], hw, hw * 2, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  // .. player 4's block
  create_field(&fields[f++], hw - 1, hw * 2, 3, FIELD_TYPE_START); // Start

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw - 1, hw * 2 -1 -i, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  for(i = 0; i < hw - 1; i++)
    create_field(&fields[f++], hw - 2 - i, hw + 1, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);

  create_field(&fields[f++], 0, hw, PLAYER_COLOR_NONE, FIELD_TYPE_NORMAL);
}


static gboolean
on_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  cairo_t *cr = NULL;
  cr = gdk_cairo_create(gtk_widget_get_window(widget));
  cairo_translate(cr, 0.5, 0.5);
  do_drawing(cr);
  cairo_translate(cr, -0.5, -0.5);
  cairo_destroy(cr);

  return FALSE;
}


static void
reset_figure_movable()
{
  int i = 0;

  for(i = 0; i < 4; i++)
    figure_movable[i] = 0;
}


static void
init_for_new_game()
{
  int i = 0, p = 0;

  for(p = 0; p < 4; p++) {
    for(i = 0; i < 4; i++) {
      player[p].f[i].p = p;
      player[p].f[i].i = i;
      player[p].f[i].t = FIELD_TYPE_PARKING;
    }
  }

  reset_figure_movable();

  set_mode(MODE_TYPE_GAME, MODE_GAME_CHOOSE_PLAYER);
}


static void
draw_field(cairo_t *cr, field_t *f)
{
  float dm = FIELD_DIM * 0.8;

  if(f->t == FIELD_TYPE_GOAL)
    dm = FIELD_DIM * 0.6;

  cairo_arc(cr, f->x * FIELD_DIM + FIELD_DIM / 2, f->y * FIELD_DIM + FIELD_DIM / 2, dm / 2, 0, 2 * M_PI);
  if(f->t == FIELD_TYPE_NORMAL)
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  else
    cairo_set_source_rgb(cr, color[f->p].r, color[f->p].g, color[f->p].b);

  cairo_fill_preserve(cr);

  if(f->t == FIELD_TYPE_NORMAL)
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  else
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);

  cairo_stroke(cr);
}


static void
draw_figure(cairo_t *cr, figure_t *f, int figure_number)
{
  field_t *fl = NULL;
  float dm = FIELD_DIM * 0.8;
  char figure_number_str[10];

  if(f->t == FIELD_TYPE_PARKING)
    fl = &fields_parking[f->p][f->i];
  else if(f->t == FIELD_TYPE_NORMAL)
    fl = &fields[f->i];
  else //if(f->t == FIELD_TYPE_GOAL)
    fl = &fields_goal[f->p][f->i];

  cairo_save(cr);
  cairo_set_line_width(cr, 3.0);

  cairo_new_path(cr);
  cairo_arc(cr, fl->x * FIELD_DIM + FIELD_DIM / 2, fl->y * FIELD_DIM + FIELD_DIM / 2, dm / 2, 0, 2 * M_PI);

  if(f->p == cp && figure_movable[figure_number])
    cairo_set_source_rgb(cr, color[f->p].r / 2.0, color[f->p].g / 2.0, color[f->p].b / 2.0);
  else
    cairo_set_source_rgb(cr, color[f->p].r, color[f->p].g, color[f->p].b);

  cairo_fill_preserve(cr);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_stroke(cr);

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, FIELD_DIM * 0.5);
  cairo_move_to(cr, fl->x * FIELD_DIM + FIELD_DIM * 0.30, fl->y * FIELD_DIM + FIELD_DIM * 0.70);
  sprintf(figure_number_str, "%d", figure_number + 1);
  cairo_show_text(cr, figure_number_str);

  cairo_restore(cr);
}


static void
draw_cube(cairo_t *cr, int number)
{
  float d = FIELD_DIM; // Dim
  float r = 5.0;  // Radius
  float x = (AREA_DIM * FIELD_DIM) / 2.0;
  float y = (AREA_DIM * FIELD_DIM) / 2.0;
  float hs = d / 2.0 - r; // Half side length
  float re = d / 10.0; // Diameter eyes


  if(!(play_game_mode == MODE_GAME_PLAY_ROLL || play_game_mode == MODE_GAME_PLAY_MOVE || play_game_mode == MODE_GAME_PLAY_CHANGE_PLAYER))
    return;

  cairo_save(cr);

  cairo_set_line_width(cr, 2.0);

  // The cube block itself
  cairo_new_path(cr);
  cairo_arc(cr, x + hs, y + hs, r, M_PI * 0.0, M_PI * 0.5);
  cairo_arc(cr, x - hs, y + hs, r, M_PI * 0.5, M_PI * 1.0);
  cairo_arc(cr, x - hs, y - hs, r, M_PI * 1.0, M_PI * 1.5);
  cairo_arc(cr, x + hs, y - hs, r, M_PI * 1.5, M_PI * 0.0);
  cairo_close_path(cr);

  cairo_set_source_rgb(cr, color[cp].r, color[cp].g, color[cp].b);
  cairo_fill_preserve(cr);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_stroke(cr);

  // Eyes
  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

  if(number <= 0) {
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, FIELD_DIM * 1.0);
    cairo_move_to(cr, x - FIELD_DIM * 0.25, y + FIELD_DIM * 0.35);
    cairo_show_text(cr, "?");
  } else {
    if(number == 1 || number == 3 || number == 5) {
      cairo_arc(cr, x, y, re, 0, M_PI * 2.0);
      cairo_fill(cr);
    }

    if(number == 2 || number == 3 || number == 4 || number == 5 || number == 6) {
      cairo_arc(cr, x + re * 2, y - re * 2, re, 0, M_PI * 2.0);
      cairo_arc(cr, x - re * 2, y + re * 2, re, 0, M_PI * 2.0);
      cairo_fill(cr);
    }

    if(number == 4 || number == 5 || number == 6) {
      cairo_arc(cr, x - re * 2, y - re * 2, re, 0, M_PI * 2.0);
      cairo_arc(cr, x + re * 2, y + re * 2, re, 0, M_PI * 2.0);
      cairo_fill(cr);
    }

    if(number == 6) {
      cairo_arc(cr, x - re * 2, y, re, 0, M_PI * 2.0);
      cairo_arc(cr, x + re * 2, y, re, 0, M_PI * 2.0);
      cairo_fill(cr);
    }
  }

  cairo_restore(cr);
}


static void
print_centered_text(GtkWidget *window, cairo_t *cr, char *text, double font_size)
{
  int w = 0, h = 0;
  int pw = 0, ph = 0;
  PangoLayout *layout = NULL;
  PangoFontDescription *desc = NULL;

  cairo_save(cr);

  layout = pango_cairo_create_layout(cr);
  pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

  pango_layout_set_text(layout, text, -1);
  desc = pango_font_description_new();
  pango_font_description_set_absolute_size(desc, font_size * PANGO_SCALE);
  pango_font_description_set_family(desc, "Sans");
  pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);

  pango_layout_get_pixel_size(layout, &pw, &ph);
  gtk_window_get_size(GTK_WINDOW(window), &w, &h);

  cairo_move_to(cr, (w - pw) / 2, (h - ph) / 2);

//  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  pango_cairo_update_layout(cr, layout);
  pango_cairo_show_layout(cr, layout);

  g_object_unref(layout);

  cairo_restore(cr);
}


static void
do_drawing(cairo_t *cr)
{
  int i = 0, p = 0;
  int hw = (AREA_DIM - 1) / 2; // Half width

  cairo_set_line_width(cr, 1.0);

  // Background
  cairo_set_source_rgb(cr, 0.2, 0.2, 1.0);
  cairo_rectangle(cr, 0, 0, ws.x, ws.y);
  cairo_fill(cr);

  // Draw fields
  // .. parking
  for(p = 0; p < PLAYER_COLOR_CNT; p++) {
    for(i = 0; i < 4; i++) {
      draw_field(cr, &fields_parking[p][i]);
    }
  }

  // .. normal (incl. start)
  for(i = 0; i < (AREA_DIM - 1) * 4; i++) {
    draw_field(cr, &fields[i]);
  }

  // .. goal
  for(p = 0; p < PLAYER_COLOR_CNT; p++) {
    for(i = 0; i < 4; i++) {
      draw_field(cr, &fields_goal[p][i]);
    }
  }

  // Draw figures
  for(p = 0; p < PLAYER_COLOR_CNT; p++) {
    if(!player[p].playing)
      continue;

    for(i = 0; i < 4; i++) {
      draw_figure(cr, &player[p].f[i], i);
    }
  }

  // Background whitening
  if(game_mode == MODE_GAME_CHOOSE_PLAYER || game_mode == MODE_GAME_FINISHED) {
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.5);
    cairo_rectangle(cr, 0, 0, ws.x, ws.y);
    cairo_fill(cr);
  }

  // Draw figures
  if(game_mode == MODE_GAME_CHOOSE_PLAYER) {
    for(p = 0; p < PLAYER_COLOR_CNT; p++) {
      if(!player[p].playing)
        continue;

      for(i = 0; i < 4; i++) {
        draw_figure(cr, &player[p].f[i], i);
      }
    }
  }

  // Draw cube
  draw_cube(cr, number);

  // Draw text for some cases
  if(game_mode == MODE_GAME_CHOOSE_PLAYER) {
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    print_centered_text(window, cr, "Spieler auswählen\n(ein-/ausschalten)\ndurch Drücken von 1..4:", FIELD_DIM * 0.5);
  } else if(game_mode == MODE_GAME_FINISHED) {
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    print_centered_text(window, cr, "Spiel beendet!\n\n Gratulation!", FIELD_DIM * 1.0);
  }

  // Record / replay info
  if(rmode == 1) {
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.2);
    print_centered_text(window, cr, "RECORD", FIELD_DIM * 2.0);
  } else if(rmode == 2) {
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.2);
    print_centered_text(window, cr, "REPLAY", FIELD_DIM * 2.0);
  }
}


static void
rotate_player()
{
  int i = 0;

  for(i = 0; i < 4; i++) {
    cp = (cp + 1) % 4;
    if(player[cp].playing)
      break;
 }

  number = 0;
  set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_ROLL);
}


static int
check_start_occupied()
{
  int i = 0;
  int start_occupied = 0;
  figure_t *f = NULL;

  for(i = 0; i < 4; i++) {
    f = &player[cp].f[i];

    if(f->t == FIELD_TYPE_NORMAL && f->i == cp * (AREA_DIM - 1)) {
      start_occupied = 1;
      break;
    }
  }

  return start_occupied;
}


static int
get_num_figures_parking()
{
  int i = 0;
  int cnt = 0;

  for(i = 0; i < 4; i++) {
    if(player[cp].f[i].t == FIELD_TYPE_PARKING)
      cnt++;
  }

  return cnt;
}


static int
force_start() // Roll three time
{
  int i = 0, f = 0;
  int parking_pos_cnt = 0;
  int figure_match = 0;

  for(i = 0; i < 4; i++) {
    if(player[cp].f[i].t == FIELD_TYPE_PARKING)
      parking_pos_cnt++;
  }

  if(parking_pos_cnt == 4) {
    return 1;
  } else {
    for(i = 3; i >= parking_pos_cnt; i--) {
      figure_match = 0;
      for(f = 0; f < 4; f++) {
        if(player[cp].f[f].t == FIELD_TYPE_GOAL && player[cp].f[f].i == i) {
          figure_match = 1;
          break;
        }
      }

      if(!figure_match)
        return 0;
    }
  }

  return 1;
}


static int
check_field_occupied(int fi, int *p, int *f, int pi) // Field index, i = player-index to ignore
{
  int _f = 0, _p = 0;
  int occ = 0;

  for(_p = 0; _p < 4; _p++) {
    if(!player[_p].playing || (pi >= 0 && pi == _p))
      continue;

    for(_f = 0; _f < 4; _f++) {
      if(player[_p].f[_f].t == FIELD_TYPE_NORMAL && player[_p].f[_f].i == fi) {
        if(p != NULL)
          *p = _p;

        if(f != NULL)
          *f = _f;

        if(_p == cp) {
          return 1;
        } else {
          return 2;
        }
      }
    }
  }

  return 0;
}

static int
check_player_won()
{
  int i = 0;
  int won = 1;

  for(i = 0; i < 4; i++) {
    if(player[cp].f[i].t != FIELD_TYPE_GOAL) {
      won =  0;
      break;
    }
  }

 return won;
}


static int
get_figure_index(int p, int fi)
{
  int i = 0;

  for(i = 0; i < 4; i++) {
    if(player[p].f[i].i == fi)
      return i;
  }

  return -1;
}


static int
get_goal_fields_free()
{
  int i = 0;
  int res = 4;
  figure_t *f = NULL;

  for(i = 0; i < 4; i++) {
    f = &player[cp].f[i];
    if(f->t == FIELD_TYPE_GOAL && f->i < res)
     res = f->i;
  }

  return res;
}


static int
check_figure_movable()
{
  int i = 0, k = 0;
  int fc = 0;
  int _cp = (cp == 0 ? 4 : cp);
  int num_figures_parking = get_num_figures_parking();
  int fi = 0;
  int fi_ori = 0;
  int f = 0, p = 0;
  figure_t *fig = NULL;
  int tmp = 0;
  int ret = 0;
  int gff = get_goal_fields_free();

  reset_figure_movable();

  // Start case- we return here, because we have to force this case
  if(num_figures_parking > 0) {
    if(num_figures_parking == 4) {
      if(number == 6) {
        for(i = 0; i < 4; i++) {
          figure_movable[i] = 1;
          ret = 1;
        }
      }
    } else { // 0 < num_figures_parking < 4
      if(check_start_occupied()) {
        fi = cp * (AREA_DIM - 1);

        for(i = 0; i < 4 - num_figures_parking; i++) {
          if(check_field_occupied(fi, &p, &f, -1) == 1) {
            fi_ori = fi;
            fi = (player[p].f[f].i + number) % ((AREA_DIM - 1) * 4);
          } else {
            figure_movable[get_figure_index(cp, fi_ori)] = 1;
            ret = 1;
            break;
          }
        }
      } else { // Start field not occupied
        if(number == 6) {
          for(k = 0; k < 4; k++) {
            if(player[cp].f[k].t == FIELD_TYPE_PARKING) {
              figure_movable[k] = 1;
              ret = 1;
            }
          }
          ret = 1;
        }
      }
    }
  }

  // No force cases
  if(ret == 0) {
    // In-Goal case
    for(i = 0; i < 4; i++) {
      fig = &player[cp].f[i];

      if(fig->t == FIELD_TYPE_GOAL) {
        if(fig->i + number < 4) {
          tmp = 1;

          for(k = 0; k < 4; k++) {
            if(k != i && player[cp].f[k].t == FIELD_TYPE_GOAL && player[cp].f[k].i > fig->i && player[cp].f[k].i <= fig->i + number) {
              tmp = 0;
              break;
            }
          }

          if(tmp == 1) {
            figure_movable[i] = 1;
            ret = 1;
          }
        }
      }
    }

    // Normal case
    for(i = 0; i < 4; i++) {
      int n2n = 0;
      fig = &player[cp].f[i];
      tmp = (AREA_DIM - 1) * _cp;

      // Check if the possible new field is within the one round
      if(fig->t == FIELD_TYPE_NORMAL) {
        if(cp == 0) {
          if(fig->i < tmp && fig->i + number < tmp)
             n2n = 1;
        } else {
          if(!(fig->i < tmp && fig->i + number >= tmp))
            n2n = 1;
        }

        if(n2n) {
          fi = (fig->i + number) % ((AREA_DIM - 1) * 4); // Possible new position

          if(check_field_occupied(fi, &p, &f, -1) != 1) {
            figure_movable[i] = 1;
            ret = 1;
          }
        } else if((fig->i >= tmp && fig->i + number > tmp + gff) || (fig->i < tmp && fig->i + number <= tmp + gff)) { // From a normal field to a goal field
          fi = (fig->i + number) % ((AREA_DIM - 1) * 4); // Possible new position

          if(fi - (AREA_DIM - 1) * cp < gff) {
            figure_movable[i] = 1;
            ret = 1;
          }
        }
      }
    }
  }

  return ret;
}


static void
roll()
{
  int i = 0, p = 0;
  figure_t *f = NULL;
  int moved = 0;

  number = (rand() % 6) + 1;

  if(rmode == 1)
    saved_moves[saved_moves_cnt++] = number;
  else if(rmode == 2)
    number = saved_moves[saved_moves_cnt++];

//XXX

/*
cp = 1;
number = 6;

player[0].f[0].t = FIELD_TYPE_NORMAL;
player[0].f[0].i = 16;
player[0].f[1].t = FIELD_TYPE_NORMAL;
player[0].f[1].i = 17;
player[0].f[1].t = FIELD_TYPE_NORMAL;
player[0].f[1].i = 10;

player[1].f[0].t = FIELD_TYPE_NORMAL;
player[1].f[0].i = 22;
player[1].f[0].t = FIELD_TYPE_NORMAL;
player[1].f[0].i = 27;
player[1].f[0].t = FIELD_TYPE_NORMAL;
player[1].f[0].i = 14;
*/

/*
cp = 0;
number = 6;

player[0].f[0].t = FIELD_TYPE_NORMAL;
player[0].f[0].i = 3;
*/

  // Roll three times if there's no other option
  if(force_start_cnt == -1 && force_start())
    force_start_cnt = 3;

  set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_MOVE);

  if(number == 6) {
    force_start_cnt = -1;
  } else {
    if(force_start_cnt > 0) {
      force_start_cnt--;
      if(force_start_cnt == 0) {
        force_start_cnt = -1;
        set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_CHANGE_PLAYER);
      } else {
        set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_ROLL);
      }
    }
  }

  if(play_game_mode == MODE_GAME_PLAY_MOVE && check_figure_movable() == 0) {
    if(number == 6)
      set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_ROLL);
    else
      set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_CHANGE_PLAYER);
  }
}


static void
move(int figure_index)
{
  figure_t *fig = &player[cp].f[figure_index];
  int _cp = cp;
  int p = 0, f = 0;

  if(_cp == 0)
    _cp = 4;

  reset_figure_movable();

  // PARKING -> NORMAL
  if(fig->t == FIELD_TYPE_PARKING) {
    fig->t = FIELD_TYPE_NORMAL;
    fig->i = (AREA_DIM - 1) * cp;
    ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_MoveParkingNormal.ogg", NULL);
  } else if(fig->t == FIELD_TYPE_NORMAL) {
    // NORMAL ->
    if(fig->i < (AREA_DIM - 1) * _cp && fig->i + number >= (AREA_DIM - 1) * _cp) { // -> GOAL
      fig->i = (fig->i + number) % ((AREA_DIM - 1) * _cp);
      fig->t = FIELD_TYPE_GOAL;
      ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_MoveNormalGoal.ogg", NULL);
    } else { // -> NORMAL
      fig->i = (fig->i + number) % ((AREA_DIM - 1) * 4);
      ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_Move.ogg", NULL);
    }
  } else if(fig->t == FIELD_TYPE_GOAL) {
    // GOAL ->
    fig->i += number;
    ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_Move.ogg", NULL);
  }

  if(fig->t == FIELD_TYPE_NORMAL) {
  ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_KickEnemy.ogg", NULL);
    if(check_field_occupied(fig->i, &p, &f, cp) == 2) {
      player[p].f[f].i = get_figure_index(p, fig->i);
      player[p].f[f].t = FIELD_TYPE_PARKING;
    }
  }

  if(check_player_won()) {
    set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_FINISHED);
    ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_GameFinished.ogg", NULL);
  } else {
   if(number == 6)
      set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_ROLL);
    else
      rotate_player();
  }
}


static void
refresh()
{
  gtk_widget_queue_draw(darea);
}


static gboolean
clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
/*
  if (event->button == 1) {
    roll();
    refresh();
  } else if (event->button == 3) {
    rotate_player();
    refresh();
  }
*/
  return TRUE;
}


static void
on_window_show (GtkWidget *widget, gpointer user_data)
{
  ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_Splash.ogg", NULL);
}


static void
set_window_size()
{
  ws.x = AREA_DIM * FIELD_DIM;
  ws.y = AREA_DIM * FIELD_DIM;

  GdkGeometry hints;
  hints.min_width = ws.x;
  hints.max_width = ws.x;
  hints.min_height = ws.y;
  hints.max_height = ws.y;

  gtk_window_set_geometry_hints(GTK_WINDOW(window), window, &hints, (GdkWindowHints)(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE));
}


static int
save_moves_to_file()
{
  FILE *fp = NULL;

  if(!(fp = fopen("moves.bin", "wb"))) {
    return -1;
  } else {
    fwrite(saved_moves, sizeof(saved_moves), 1, fp);
    fclose(fp);
  }

  return 0;
}


static int
read_moves_from_file()
{
  FILE *fp = NULL;

  if(!(fp = fopen("moves.bin", "r"))) {
    return -1;
  } else {
    fread(saved_moves, sizeof(saved_moves), 1, fp);
    fclose(fp);
  }

  return 0;
}


gboolean
on_key_press (GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  GdkModifierType modifiers;

  modifiers = gtk_accelerator_get_default_mod_mask();

  int num = -1;
  int i = 0;
  int num_set = 0;

  switch (event->keyval) {
    case GDK_r:
      if(game_mode == MODE_GAME_CHOOSE_PLAYER) {
        rmode = (rmode + 1) % 3;
        saved_moves_cnt = 0;
        refresh();
      }
      break;

    case GDK_s:
      if(game_mode != MODE_GAME_CHOOSE_PLAYER) {
        if(rmode == 1) {
          save_moves_to_file();
          printf("save_moves_to_file()\n");
        }
      } else {
        if(rmode == 2) {
          read_moves_from_file();
          printf("read_moves_from_file()\n");
        }
      }
      break;

    case GDK_KEY_1:
    case GDK_KEY_2:
    case GDK_KEY_3:
    case GDK_KEY_4:
      num = event->keyval - GDK_KEY_1;
      if(game_mode == MODE_GAME_CHOOSE_PLAYER) {
        player[num].playing = !player[num].playing;
        refresh();
      } else if(play_game_mode == MODE_GAME_PLAY_MOVE) {
        if(figure_movable[num]) {
/*
          if(rmode == 1)
            saved_moves[saved_moves_cnt++] = -num;
          else
            num = -saved_moves[saved_moves_cnt++];
*/

          move(num);
          refresh();
        }
      }
      break;

    case GDK_KEY_5:
    case GDK_KEY_6:
    case GDK_KEY_7:
    case GDK_KEY_8:
      num = event->keyval - GDK_KEY_5;
      for(i = 0; i < 4; i++)
        printf("p %d (i = %d, t = %d)\n", num + 1, player[num].f[i].i, player[num].f[i].t);
      break;

    case GDK_KEY_Return:
      if(game_mode == MODE_GAME_CHOOSE_PLAYER) {
        for(i = 0; i < 4; i++) {
          if(player[i].playing)
            num_set++;
        }

        if(num_set >= 2) {
          for(i = 0; i < 4; i++) {
            if(player[i].playing) {
              cp = i;
              break;
            }
          }
          number = -1;
          set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_ROLL);
          refresh();
          ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_GameStart.ogg", NULL);
        }
      } else if(play_game_mode == MODE_GAME_PLAY_ROLL) {
        roll();
        ca_context_play (cba_ctx_sound, 0, CA_PROP_MEDIA_FILENAME, "SFX_Roll.ogg", NULL);
        refresh();
      } else if(play_game_mode == MODE_GAME_PLAY_CHANGE_PLAYER) {
        number = 0;
        rotate_player();
        set_mode(MODE_TYPE_GAME_PLAY, MODE_GAME_PLAY_ROLL);
        refresh();
      } else if(game_mode == MODE_GAME_FINISHED) {
        init_for_new_game();
        set_mode(MODE_TYPE_GAME, MODE_GAME_CHOOSE_PLAYER);
        refresh();
      }

      break;
  }

  return FALSE;
}


int
main(int argc, char *argv[])
{
  srand(time(NULL));

  ca_context_create (&cba_ctx_sound);

  init_colors();
  create_figures();
  init_for_new_game();

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  darea = gtk_drawing_area_new();
  gtk_container_add(GTK_CONTAINER(window), darea);

  gtk_widget_add_events(darea, GDK_BUTTON_PRESS_MASK);

  g_signal_connect(darea,  "expose-event",       G_CALLBACK(on_expose_event), NULL);
  g_signal_connect(darea,  "button-press-event", G_CALLBACK(clicked), NULL);
  g_signal_connect(window, "show",               G_CALLBACK(on_window_show), NULL);
  g_signal_connect(window, "key-press-event",    G_CALLBACK(on_key_press), NULL);
  g_signal_connect(window, "destroy",            G_CALLBACK(gtk_main_quit), NULL);


  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  set_window_size();
  gtk_window_set_title(GTK_WINDOW(window), "MÄDN für meine zwei liebsten Bibis");

  init_for_new_game();

  gtk_widget_show_all(window);

  gtk_main();

  return 0;
}

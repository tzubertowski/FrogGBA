/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"
#include <pspiofilemgr.h>

#define GPSP_CONFIG_FILENAME  "froggba.cfg"
#define GPSP_CONFIG_NUM       (24 + 16) // options + game pad config + overlay options + aspect ratio + compatibility mode + button mapping
#define GPSP_GAME_CONFIG_NUM  (7 + 16)

#define COLOR_BG            COLOR15( 8, 15, 12)  // Soft mint green background
#define COLOR_ROM_INFO      COLOR15(18, 28, 22)  // Darker mint for ROM info
#define COLOR_ACTIVE_ITEM   COLOR15(31, 31, 29)  // Soft cream white for active items
#define COLOR_INACTIVE_ITEM COLOR15(12, 25, 18)  // Medium mint for inactive items

#define COLOR_HELP_TEXT     COLOR15(20, 28, 22)  // Readable mint for help text
#define COLOR_INACTIVE_DIR  COLOR15(15, 26, 20)  // Pastel mint for directories
#define COLOR_SCROLL_BAR    COLOR15(10, 18, 14)  // Darker mint for scroll bar

#define COLOR_BATT_NORMAL   COLOR_HELP_TEXT
#define COLOR_BATT_LOW      COLOR15(28, 22, 8)   // Pastel amber for low battery
#define COLOR_BATT_CHARG    COLOR15(16, 28, 18)  // Brighter mint for charging

#define FILE_LIST_ROWS      (20)
#define FILE_LIST_POS_X     (18)
#define FILE_LIST_POS_Y     (16)
#define DIR_LIST_POS_X      (360)
#define PAGE_SCROLL_NUM     (10)

#define MENU_LIST_POS_X     (18)

#define SCREEN_IMAGE_POS_X  (228)
#define SCREEN_IMAGE_POS_Y  (44)

#define BATT_STATUS_POS_X   (PSP_SCREEN_WIDTH - (FONTWIDTH * 14))  // 396
#define TIME_STATUS_POS_X   (BATT_STATUS_POS_X - (FONTWIDTH * 22)) // 264
#define DIR_NAME_LENGTH     ((TIME_STATUS_POS_X / FONTWIDTH) - 2)  // 42

// scroll bar
#define SBAR_X1  (2)
#define SBAR_X2  (12)
#define SBAR_Y1  (16)
#define SBAR_Y2  (255)

#define SBAR_T   (SBAR_Y1 + 2)
#define SBAR_B   (SBAR_Y2 - 2)
#define SBAR_H   (SBAR_B - SBAR_T)
#define SBAR_X1I (SBAR_X1 + 2)
#define SBAR_X2I (SBAR_X2 - 2)
#define SBAR_Y1I (SBAR_H * scroll_value[0] / num[0] + SBAR_T)
#define SBAR_Y2I (SBAR_H * (scroll_value[0] + FILE_LIST_ROWS) / num[0] + SBAR_T)


typedef enum
{
  NUMBER_SELECTION_OPTION = 0x01,
  STRING_SELECTION_OPTION = 0x02,
  SUBMENU_OPTION          = 0x04,
  ACTION_OPTION           = 0x08
} MENU_OPTION_TYPE_ENUM;

struct _MenuType
{
  void (*init_function)(void);
  void (*passive_function)(void);
  struct _MenuOptionType *options;
  u32 num_options;
};

struct _MenuOptionType
{
  void (*action_function)(void);
  void (*passive_function)(void);
  struct _MenuType *sub_menu;
  const char *display_string;
  void *options;
  u32 *current_option;
  u32 num_options;
  u32 help_string;
  u32 line_number;
  MENU_OPTION_TYPE_ENUM option_type;
};

typedef struct _MenuOptionType MenuOptionType;
typedef struct _MenuType MenuType;

#define MAKE_MENU(name, init_function, passive_function)                      \
  MenuType name##_menu =                                                      \
  {                                                                           \
    init_function,                                                            \
    passive_function,                                                         \
    name##_options,                                                           \
    sizeof(name##_options) / sizeof(MenuOptionType)                           \
  }                                                                           \

#define GAMEPAD_CONFIG_OPTION(display_string, number)                         \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  display_string,                                                             \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + gamepad_config_line_to_button[number],                 \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  MSG_PAD_MENU_HELP_0,                                                        \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define ANALOG_CONFIG_OPTION(display_string, number)                          \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  display_string,                                                             \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + number + 12,                                           \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  MSG_PAD_MENU_HELP_0,                                                        \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define CHEAT_OPTION(number)                                                  \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  cheat_format_str[number],                                                   \
  enable_disable_options,                                                     \
  &(cheats[number].cheat_active),                                             \
  2,                                                                          \
  MSG_CHEAT_MENU_HELP_0,                                                      \
  (number) % 10,                                                              \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define SAVESTATE_OPTION(number)                                              \
{                                                                             \
  menu_select_savestate,                                                      \
  NULL,                                                                       \
  NULL,                                                                       \
  savestate_timestamps[number],                                               \
  NULL,                                                                       \
  &savestate_action,                                                          \
  2,                                                                          \
  MSG_STATE_MENU_HELP_0,                                                      \
  number,                                                                     \
  NUMBER_SELECTION_OPTION | ACTION_OPTION                                     \
}                                                                             \

#define ACTION_OPTION(action_function, passive_function, display_string, help_string, line_number) \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  0,                                                                          \
  help_string,                                                                \
  line_number,                                                                \
  ACTION_OPTION                                                               \
}                                                                             \

#define SUBMENU_OPTION(sub_menu, display_string, help_string, line_number)    \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sub_menu,                                                                   \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sizeof(sub_menu) / sizeof(MenuOptionType),                                  \
  help_string,                                                                \
  line_number,                                                                \
  SUBMENU_OPTION                                                              \
}                                                                             \

#define ACTION_SUBMENU_OPTION(sub_menu, action_function, display_string, help_string, line_number) \
{                                                                             \
  action_function,                                                            \
  NULL,                                                                       \
  sub_menu,                                                                   \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sizeof(sub_menu) / sizeof(MenuOptionType),                                  \
  help_string,                                                                \
  line_number,                                                                \
  SUBMENU_OPTION | ACTION_OPTION                                              \
}                                                                             \

#define SELECTION_OPTION(passive_function, display_string, options, option_ptr, num_options, help_string, line_number, type) \
{                                                                             \
  NULL,                                                                       \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type                                                                        \
}                                                                             \

#define ACTION_SELECTION_OPTION(action_function, passive_function, display_string, options, option_ptr, num_options, help_string, line_number, type) \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type | ACTION_OPTION                                                        \
}                                                                             \


#define STRING_SELECTION_OPTION(passive_function, display_string, options, option_ptr, num_options, help_string, line_number) \
  SELECTION_OPTION(passive_function, display_string, options, option_ptr, num_options, help_string, line_number, STRING_SELECTION_OPTION)

#define NUMERIC_SELECTION_OPTION(passive_function, display_string, option_ptr, num_options, help_string, line_number) \
  SELECTION_OPTION(passive_function, display_string, NULL, option_ptr, num_options, help_string, line_number, NUMBER_SELECTION_OPTION)

#define STRING_SELECTION_ACTION_OPTION(action_function, passive_function, display_string, options, option_ptr, num_options, help_string, line_number) \
  ACTION_SELECTION_OPTION(action_function, passive_function, display_string,  options, option_ptr, num_options, help_string, line_number, STRING_SELECTION_OPTION)

#define NUMERIC_SELECTION_ACTION_OPTION(action_function, passive_function, display_string, option_ptr, num_options, help_string, line_number) \
  ACTION_SELECTION_OPTION(action_function, passive_function, display_string,  NULL, option_ptr, num_options, help_string, line_number, NUMBER_SELECTION_OPTION)

#define NUMERIC_SELECTION_ACTION_HIDE_OPTION(action_function, passive_function, display_string, option_ptr, num_options, help_string, line_number) \
  ACTION_SELECTION_OPTION(action_function, passive_function, display_string, NULL, option_ptr, num_options, help_string, line_number, NUMBER_SELECTION_OPTION)


char dir_roms[MAX_PATH];
char dir_save[MAX_PATH];
char dir_state[MAX_PATH];
char dir_cfg[MAX_PATH];
char dir_snap[MAX_PATH];
char dir_cheat[MAX_PATH];//cheat
char dir_overlay[MAX_PATH];//overlay

u32 menu_cheat_page = 0;

// Overlay variables
#define MAX_OVERLAYS 10
char overlay_names[MAX_OVERLAYS][64] = {
  "None", "None", "None", "None", "None", 
  "None", "None", "None", "None", "None"
};
const char *overlay_name_options[MAX_OVERLAYS] = {
  overlay_names[0], overlay_names[1], overlay_names[2], overlay_names[3], overlay_names[4],
  overlay_names[5], overlay_names[6], overlay_names[7], overlay_names[8], overlay_names[9]
};
u32 num_overlays = 1; // Start with 1 for "None"
static u32 overlays_scanned = 0; // Flag to track if we've already scanned

// Recent games tracking system
#define MAX_RECENT_GAMES 5
static char recent_games[MAX_RECENT_GAMES][MAX_PATH];
static int num_recent_games = 0;
static const char *recent_games_display[MAX_RECENT_GAMES];

// Global yes/no options - needed for overlay menu
static const char *global_yes_no_options[2];

// Global menu structures for overlay - must be here to avoid scope issues  
MenuOptionType overlay_options_global[5] = {
  {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, STRING_SELECTION_OPTION},
  {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, STRING_SELECTION_OPTION}, 
  {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, NUMBER_SELECTION_OPTION},
  {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, NUMBER_SELECTION_OPTION},
  {NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, SUBMENU_OPTION}
};

MenuType overlay_menu_global = {
  NULL, // init_function - will be set later
  NULL, // passive_function  
  overlay_options_global,
  5
};

// Initialize overlays at emulator boot - called from main.c
void init_overlays_at_boot(void)
{
  u32 i;
  SceUID dir;
  SceIoDirent dirent;
  char *ext;
  
  /*FILE *debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    fprintf(debug_log, "DEBUG: init_overlays_at_boot() called\n");
    fprintf(debug_log, "DEBUG: dir_overlay = '%s'\n", dir_overlay);
    fflush(debug_log);
    fclose(debug_log);
  }*/
  
  // Start with just "None"
  num_overlays = 1;
  
  // Try to open the overlays directory
  dir = sceIoDopen(dir_overlay);
  
  /*debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    fprintf(debug_log, "DEBUG: sceIoDopen returned %d\n", dir);
    fflush(debug_log);
    fclose(debug_log);
  }*/
  
  if (dir >= 0)
  {
    // Scan for PNG files in the overlay directory
    while (sceIoDread(dir, &dirent) > 0 && num_overlays < MAX_OVERLAYS)
    {
      // Skip directories and hidden files
      if (!(dirent.d_stat.st_mode & FIO_SO_IFREG) || dirent.d_name[0] == '.')
        continue;
        
      // Check if it's an OVL file (or PNG for backwards compatibility)
      ext = strrchr(dirent.d_name, '.');
      if (ext && (strcasecmp(ext, ".ovl") == 0 || strcasecmp(ext, ".png") == 0))
      {
        /*debug_log = fopen("froggba_debug.log", "a");
        if (debug_log) {
          fprintf(debug_log, "DEBUG: Found overlay: %s\n", dirent.d_name);
          fflush(debug_log);
          fclose(debug_log);
        }*/
        
        // Copy filename and remove extension for display
        strncpy(overlay_names[num_overlays], dirent.d_name, sizeof(overlay_names[num_overlays]) - 1);
        overlay_names[num_overlays][sizeof(overlay_names[num_overlays]) - 1] = '\0';
        
        // Remove .ovl or .png extension from display name
        char *dot = strrchr(overlay_names[num_overlays], '.');
        if (dot) *dot = '\0';
        
        // Pointer is already set up safely at compile time
        num_overlays++;
      }
    }
    
    sceIoDclose(dir);
  }
  
  /*debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    fprintf(debug_log, "DEBUG: init_overlays_at_boot complete, found %u overlays\n", num_overlays);
    fflush(debug_log);
    fclose(debug_log);
  }*/
  
  // Mark as scanned
  overlays_scanned = 1;
  
  // Ensure option_overlay_selected is within bounds
  if (option_overlay_selected >= num_overlays)
  {
    option_overlay_selected = 0;
  }
  
  // Update the menu structure with the actual number of overlays found
  overlay_options_global[0].num_options = num_overlays;
}

// Forward declaration
static void overlay_changed(void);

// Empty function - overlays already loaded at boot
void scan_overlay_files(void)
{
  // Do nothing - overlays are cached from boot
}

// Called when overlay selection changes
static void overlay_changed(void)
{
  extern int overlay_needs_update;
  
  /*FILE *debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    fprintf(debug_log, "overlay_changed: selected=%d, num_overlays=%d, name='%s'\n", 
            option_overlay_selected, num_overlays, 
            option_overlay_selected < num_overlays ? overlay_names[option_overlay_selected] : "INVALID");
    fclose(debug_log);
  }*/
  
  if (option_overlay_selected < num_overlays) {
    extern int overlay_needs_update;
    load_overlay(overlay_names[option_overlay_selected]);
    overlay_needs_update = 1;
  }
}

// Called when overlay enabled/disabled changes
static void overlay_enabled_changed(void)
{
  extern int overlay_needs_update;
  extern void load_overlay(const char *filename);
  extern char overlay_names[][64];
  
  /*FILE *debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    fprintf(debug_log, "overlay_enabled_changed: enabled=%d, selected=%d\n", 
            option_overlay_enabled, option_overlay_selected);
    fclose(debug_log);
  }*/
  
  if (option_overlay_enabled && option_overlay_selected > 0) {
    // Load the selected overlay
    load_overlay(overlay_names[option_overlay_selected]);
    overlay_needs_update = 1;
    
    // FORCE TEST: Apply overlay immediately to test if rendering works
    extern void apply_overlay_borders(void);
    apply_overlay_borders();
  } else {
    // Clear overlay when disabled and force screen refresh
    extern void clear_overlay(void);
    extern void force_screen_refresh(void);
    clear_overlay();
    force_screen_refresh(); // Clear any remaining overlay pixels
  }
}

// Called when X/Y offset changes - need to reapply overlay and regenerate display list
static void overlay_offset_changed(void)
{
  extern void set_gba_resolution(void);
  extern void force_screen_refresh(void);
  extern int overlay_needs_update;
  
  // Force complete screen refresh
  force_screen_refresh();
  
  // Mark overlay for re-rendering
  overlay_needs_update = 1;
  
  // Regenerate display list with new offset - THIS IS CRITICAL
  set_gba_resolution();
  
  // DEBUG: Log when offset changes
  /*FILE *debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    extern u32 option_overlay_offset_x, option_overlay_offset_y;
    fprintf(debug_log, "overlay_offset_changed: new offset X=%d Y=%d, set overlay_needs_update=1, called set_gba_resolution\n", 
            option_overlay_offset_x, option_overlay_offset_y);
    fclose(debug_log);
  }*/
}

// This function is no longer needed - arrays are initialized at compile time

// Initialize overlay menu structure - to be called when we have MSG available
static void init_overlay_menu_late(void) {
    /*FILE *debug_log = fopen("froggba_debug.log", "a");
    if (debug_log) {
        fprintf(debug_log, "init_overlay_menu_late: Setting up overlay menu callbacks\n");
        fclose(debug_log);
    }*/
    // Set up the overlay menu options with proper MSG references
    overlay_options_global[0].display_string = MSG[MSG_OVERLAY_MENU_0];
    overlay_options_global[0].options = (void*)overlay_name_options;
    overlay_options_global[0].current_option = &option_overlay_selected;
    overlay_options_global[0].num_options = MAX_OVERLAYS;
    overlay_options_global[0].help_string = MSG_OVERLAY_MENU_HELP_0;
    overlay_options_global[0].line_number = 0;
    overlay_options_global[0].passive_function = overlay_changed;
    
    overlay_options_global[1].display_string = MSG[MSG_OVERLAY_MENU_1];
    overlay_options_global[1].current_option = &option_overlay_enabled;
    overlay_options_global[1].num_options = 2;
    overlay_options_global[1].help_string = MSG_OVERLAY_MENU_HELP_1;
    overlay_options_global[1].line_number = 1;
    overlay_options_global[1].passive_function = overlay_enabled_changed;
    
    overlay_options_global[2].display_string = MSG[MSG_OVERLAY_MENU_2];
    overlay_options_global[2].current_option = &option_overlay_offset_x;
    overlay_options_global[2].num_options = 241; // 0-240
    overlay_options_global[2].help_string = MSG_OVERLAY_MENU_HELP_2;
    overlay_options_global[2].line_number = 2;
    overlay_options_global[2].passive_function = overlay_offset_changed;
    
    overlay_options_global[3].display_string = MSG[MSG_OVERLAY_MENU_3];
    overlay_options_global[3].current_option = &option_overlay_offset_y;
    overlay_options_global[3].num_options = 113; // 0-112
    overlay_options_global[3].help_string = MSG_OVERLAY_MENU_HELP_3;
    overlay_options_global[3].line_number = 3;
    overlay_options_global[3].passive_function = overlay_offset_changed;
    
    overlay_options_global[4].display_string = MSG[MSG_OVERLAY_MENU_4];
    overlay_options_global[4].help_string = MSG_OVERLAY_MENU_HELP_4;
    overlay_options_global[4].line_number = 4;
    
    // No init function needed - we scan at startup
    overlay_menu_global.init_function = NULL;
}

// Recent games tracking functions
void load_recent_games(void) {
  char recent_games_file[MAX_PATH];
  sprintf(recent_games_file, "%sfroggba_recent.txt", dir_cfg);
  
  // Initialize display array
  for (int i = 0; i < MAX_RECENT_GAMES; i++) {
    recent_games[i][0] = '\0';
    recent_games_display[i] = NULL;
  }
  num_recent_games = 0;
  
  FILE *file = fopen(recent_games_file, "r");
  if (file) {
    char line[MAX_PATH];
    while (fgets(line, sizeof(line), file) && num_recent_games < MAX_RECENT_GAMES) {
      // Remove newline
      int len = strlen(line);
      if (len > 0 && line[len-1] == '\n') {
        line[len-1] = '\0';
      }
      
      // Copy to recent games array
      strcpy(recent_games[num_recent_games], line);
      recent_games_display[num_recent_games] = recent_games[num_recent_games];
      num_recent_games++;
    }
    fclose(file);
  }
}

static void save_recent_games(void) {
  char recent_games_file[MAX_PATH];
  sprintf(recent_games_file, "%sfroggba_recent.txt", dir_cfg);
  
  FILE *file = fopen(recent_games_file, "w");
  if (file) {
    for (int i = 0; i < num_recent_games; i++) {
      fprintf(file, "%s\n", recent_games[i]);
    }
    fclose(file);
  }
}

static void add_recent_game(const char *game_path) {
  if (!game_path || strlen(game_path) == 0) return;
  
  // Check if game already exists in recent list
  int existing_index = -1;
  for (int i = 0; i < num_recent_games; i++) {
    if (strcmp(recent_games[i], game_path) == 0) {
      existing_index = i;
      break;
    }
  }
  
  if (existing_index >= 0) {
    // Move existing game to top
    char temp[MAX_PATH];
    strcpy(temp, recent_games[existing_index]);
    
    // Shift games down
    for (int i = existing_index; i > 0; i--) {
      strcpy(recent_games[i], recent_games[i-1]);
    }
    
    // Put game at top
    strcpy(recent_games[0], temp);
  } else {
    // Add new game at top
    // Shift existing games down
    for (int i = (num_recent_games < MAX_RECENT_GAMES ? num_recent_games : MAX_RECENT_GAMES - 1); i > 0; i--) {
      strcpy(recent_games[i], recent_games[i-1]);
    }
    
    // Add new game at top
    strcpy(recent_games[0], game_path);
    
    // Update count
    if (num_recent_games < MAX_RECENT_GAMES) {
      num_recent_games++;
    }
  }
  
  // Update display pointers
  for (int i = 0; i < num_recent_games; i++) {
    recent_games_display[i] = recent_games[i];
  }
  
  save_recent_games();
}

// Explicit declaration to ensure visibility
extern u32 option_optimization_level;

const char *optimization_level_options[] =
{
  "Maximum Accuracy", "Safe Optimizations", "Moderate Performance", "Maximum Performance"
};

u32 ALIGN_DATA gamepad_config_line_to_button[] =
{
 8,
 6,
 7,
 9,
 1,
 2,
 3,
 0,
 4,
 5,
 11,
 10,
};

u32 savestate_slot = 0;


void _flush_cache(void);

static int sort_function(const void *dest_str_ptr, const void *src_str_ptr);

static s32 save_game_config_file(void);

static void update_status_string(char *time_str, char *batt_str, u16 *color_batt);
static void update_status_string_gbk(char *time_str, char *batt_str, u16 *color_batt);
static void get_timestamp_string(char *buffer, u16 msg_id, ScePspDateTime *msg_time, int day_of_week);

static void get_savestate_info(char *filename, u16 *snapshot, char *timestamp);
static void get_savestate_filename(u32 slot, char *name_buffer);

static void get_snapshot_filename(char *name, const char *ext);
static void save_bmp(const char *path, u16 *screen_image);


void _flush_cache(void)
{
  invalidate_all_cache();
}


static int sort_function(const void *dest_str_ptr, const void *src_str_ptr)
{
  char *dest_str = *((char **)dest_str_ptr);
  char *src_str  = *((char **)src_str_ptr);

  if (src_str[0] == '.')
    return 1;

  if (dest_str[0] == '.')
    return -1;

  if ((isalpha((int)src_str[0]) != 0) || (isalpha((int)dest_str[0]) != 0))
    return strcasecmp(dest_str, src_str);

  return strcmp(dest_str, src_str);
}

s32 load_file(const char **wildcards, char *result, char *default_dir_name)
{
  char current_dir_name[MAX_PATH];
  char current_dir_short[81];
  u32  current_dir_length;

  SceUID current_dir;
  SceIoDirent current_file;

  u32 total_file_names_allocated;
  u32 total_dir_names_allocated;
  char **file_list;
  char **dir_list;
  char *ext_pos;

  #define FILE_LIST (0)
  #define DIR_LIST  (1)

  u32 column;
  u32 num[2];
  u32 selection[2];
  u32 scroll_value[2];
  u32 in_scroll[2];

  u32 current_file_number, current_dir_number;
  u16 current_line_color;

  u32 i;
  s32 return_value = 1;
  s32 repeat;

  GUI_ACTION_TYPE gui_action;

  char time_str[40];
  char batt_str[40];
  u16 color_batt_life = COLOR_BATT_NORMAL;
  u32 counter = 0;

  auto void filelist_term(void);
  auto void malloc_error(void);


  void filelist_term(void)
  {
    for (i = 0; i < num[FILE_LIST]; i++)
    {
      free(file_list[i]);
    }

    free(file_list);

    for (i = 0; i < num[DIR_LIST]; i++)
    {
      free(dir_list[i]);
    }

    free(dir_list);
  }

  void malloc_error(void)
  {
    clear_screen(COLOR32_BLACK);
    error_msg(MSG[MSG_ERR_MALLOC], CONFIRMATION_QUIT);
    quit();
  }

  #define CHECK_MEM_ALLOCATE(mem_block)                                       \
  {                                                                           \
    if (mem_block == NULL)                                                    \
      malloc_error();                                                         \
  }                                                                           \


  if (default_dir_name != NULL)
  {
    chdir(default_dir_name);
  }

  while (return_value == 1)
  {
    column = FILE_LIST;

    selection[FILE_LIST]    = 0;
    scroll_value[FILE_LIST] = 0;
    in_scroll[FILE_LIST]    = 0;

    selection[DIR_LIST]     = 0;
    scroll_value[DIR_LIST]  = 0;
    in_scroll[DIR_LIST]     = 0;

    memset(&current_file, 0, sizeof(current_file));

    total_file_names_allocated = 32;
    total_dir_names_allocated  = 32;

    file_list = (char **)safe_malloc(sizeof(char *) * total_file_names_allocated);
    dir_list  = (char **)safe_malloc(sizeof(char *) * total_dir_names_allocated);

    memset(file_list, 0, sizeof(char *) * total_file_names_allocated);
    memset(dir_list,  0, sizeof(char *) * total_dir_names_allocated);

    num[FILE_LIST] = 0;
    num[DIR_LIST]  = 0;

    // Add recent games to the file list first (with "★ " prefix)
    // Only show recent games when browsing for ROM files (based on file extension)
    int is_rom_browsing = 0;
    if (wildcards && wildcards[0]) {
      // Check if we're looking for ROM extensions
      for (int w = 0; wildcards[w]; w++) {
        if (strstr(wildcards[w], ".gba") || strstr(wildcards[w], ".zip") || 
            strstr(wildcards[w], ".GBA") || strstr(wildcards[w], ".ZIP")) {
          is_rom_browsing = 1;
          break;
        }
      }
    }
    
    if (num_recent_games > 0 && is_rom_browsing) {
      // Add separator
      file_list[num[FILE_LIST]] = (char *)safe_malloc(strlen("--- Recent Games ---") + 1);
      sprintf(file_list[num[FILE_LIST]], "--- Recent Games ---");
      num[FILE_LIST]++;
      
      // Add recent games
      for (i = 0; i < num_recent_games && i < 5; i++) {
        // Extract just the filename from the full path for display
        char *filename = strrchr(recent_games[i], '/');
        if (filename) {
          filename++; // Skip the '/'
        } else {
          filename = recent_games[i];
        }
        
        // Allocate space for "> " prefix + filename
        file_list[num[FILE_LIST]] = (char *)safe_malloc(strlen(filename) + 3);
        sprintf(file_list[num[FILE_LIST]], "> %s", filename);
        num[FILE_LIST]++;
        
        // Reallocate if needed
        if (num[FILE_LIST] == total_file_names_allocated) {
          file_list = (char **)realloc(file_list, sizeof(char *) * (total_file_names_allocated << 1));
          CHECK_MEM_ALLOCATE(file_list);
          memset(file_list + total_file_names_allocated, 0, sizeof(char *) * total_file_names_allocated);
          total_file_names_allocated <<= 1;
        }
      }
      
      // Add separator after recent games
      file_list[num[FILE_LIST]] = (char *)safe_malloc(strlen("--- All Games ---") + 1);
      sprintf(file_list[num[FILE_LIST]], "--- All Games ---");
      num[FILE_LIST]++;
    }

    getcwd(current_dir_name, MAX_PATH);
    strcat(current_dir_name, "/");

    if (strlen(strstr(current_dir_name, ":/")) != 2)
    {
      dir_list[num[DIR_LIST]] = (char *)safe_malloc(strlen("..") + 1);

      sprintf(dir_list[num[DIR_LIST]], "%s", "..");
      num[DIR_LIST]++;
    }

    scePowerLock(0);
    current_dir = sceIoDopen(current_dir_name);

    while (sceIoDread(current_dir, &current_file) > 0)
    {
      if (current_file.d_name[0] == '.')
        continue;

      if (FIO_S_ISDIR(current_file.d_stat.st_mode) != 0)
      {
        dir_list[num[DIR_LIST]] = (char *)safe_malloc(strlen(current_file.d_name) + 1);

        sprintf(dir_list[num[DIR_LIST]], "%s", current_file.d_name);
        num[DIR_LIST]++;
      }
      else
      {
        if ((ext_pos = strrchr(current_file.d_name, '.')) != NULL)
        {
          for (i = 0; wildcards[i] != NULL; i++)
          {
            if (strcasecmp(ext_pos, wildcards[i]) == 0)
            {
              file_list[num[FILE_LIST]] = (char *)safe_malloc(strlen(current_file.d_name) + 1);

              sprintf(file_list[num[FILE_LIST]], "%s", current_file.d_name);
              num[FILE_LIST]++;
              break;
            }
          }
        }
      }

      if (num[FILE_LIST] == total_file_names_allocated)
      {
        file_list = (char **)realloc(file_list, sizeof(char *) * (total_file_names_allocated << 1));
        CHECK_MEM_ALLOCATE(file_list);
        memset(file_list + total_file_names_allocated, 0, sizeof(char *) * total_file_names_allocated);

        total_file_names_allocated <<= 1;
      }

      if (num[DIR_LIST] == total_dir_names_allocated)
      {
        dir_list = (char **)realloc(dir_list, sizeof(char *) * (total_dir_names_allocated << 1));
        CHECK_MEM_ALLOCATE(dir_list);
        memset(dir_list + total_dir_names_allocated, 0, sizeof(char *) * total_dir_names_allocated);

        total_dir_names_allocated <<= 1;
      }

    } /* end of while */

    sceIoDclose(current_dir);
    scePowerUnlock(0);

    // Only sort the normal files, not recent games
    if (num_recent_games > 0 && is_rom_browsing) {
      // Sort only the files after the recent games section
      int recent_section_size = num_recent_games + 2; // +2 for the two separator lines
      if (num[FILE_LIST] > recent_section_size) {
        qsort((void *)(file_list + recent_section_size), 
              num[FILE_LIST] - recent_section_size, 
              sizeof(char *), sort_function);
      }
    } else {
      qsort((void *)file_list, num[FILE_LIST], sizeof(char *), sort_function);
    }
    qsort((void *)dir_list,  num[DIR_LIST],  sizeof(char *), sort_function);


    current_dir_length = strlen(current_dir_name);

    if (current_dir_length > DIR_NAME_LENGTH)
    {
      memcpy(current_dir_short, "...", 3);
      memcpy(current_dir_short + 3, current_dir_name + (current_dir_length - (DIR_NAME_LENGTH - 3)), DIR_NAME_LENGTH - 3);
      current_dir_short[DIR_NAME_LENGTH] = 0;
    }
    else
    {
      memcpy(current_dir_short, current_dir_name, current_dir_length + 1);
    }


    repeat = 1;

    if (num[FILE_LIST] == 0)
    {
      column = DIR_LIST;
    }

    while (repeat)
    {
      clear_screen(COLOR15_TO_32(COLOR_BG));
		print_string(current_dir_short, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);
      if ((counter % 30) == 0)
	  {
	    update_status_string(time_str, batt_str, &color_batt_life);
	  }
      counter++;
	  print_string(time_str, TIME_STATUS_POS_X, 2, COLOR_HELP_TEXT, BG_NO_FILL);
 
	  print_string(batt_str, BATT_STATUS_POS_X, 2, color_batt_life, BG_NO_FILL);

	  print_string(MSG[MSG_BROWSER_HELP], 30, 258, COLOR_HELP_TEXT, BG_NO_FILL);

      // Show mod credit instead of ROM Buffer
      print_string("FrogGBA - TempGBA mod by Prosty", 270, 258, COLOR_HELP_TEXT, BG_NO_FILL);

      // PSP controller - hold
      if (get_pad_input(PSP_CTRL_HOLD) != 0)
		{
		print_string(FONT_KEY_ICON_GBK, 6, 258, COLOR15_YELLOW, BG_NO_FILL);
		}
      // draw scroll bar
      if (num[FILE_LIST] > FILE_LIST_ROWS)
      {
        draw_box_line(SBAR_X1,  SBAR_Y1,  SBAR_X2,  SBAR_Y2,  COLOR_SCROLL_BAR);
        draw_box_fill(SBAR_X1I, SBAR_Y1I, SBAR_X2I, SBAR_Y2I, COLOR_SCROLL_BAR);
      }

      for (i = 0; i < FILE_LIST_ROWS; i++)
      {
        current_file_number = i + scroll_value[FILE_LIST];

        if (current_file_number < num[FILE_LIST])
        {
          if ((current_file_number == selection[FILE_LIST]) && (column == FILE_LIST))
            current_line_color = COLOR_ACTIVE_ITEM;
          else
            current_line_color = COLOR_INACTIVE_ITEM;

          print_string(file_list[current_file_number], FILE_LIST_POS_X, FILE_LIST_POS_Y + (i * FONTHEIGHT), current_line_color, BG_NO_FILL);
        }
      }

      for (i = 0; i < FILE_LIST_ROWS; i++)
      {
        current_dir_number = i + scroll_value[DIR_LIST];

        if (current_dir_number < num[DIR_LIST])
        {
          if ((current_dir_number == selection[DIR_LIST]) && (column == DIR_LIST))
            current_line_color = COLOR_ACTIVE_ITEM;
          else
            current_line_color = COLOR_INACTIVE_DIR;

          print_string(dir_list[current_dir_number], DIR_LIST_POS_X, FILE_LIST_POS_Y + (i * FONTHEIGHT), current_line_color, COLOR_BG);
        }
      }

      if (num[DIR_LIST] > FILE_LIST_ROWS)
      {
        if (scroll_value[DIR_LIST] != 0)
          print_string(FONT_CURSOR_UP_FILL, PSP_SCREEN_WIDTH - (FONTWIDTH * 2), FILE_LIST_POS_Y, COLOR_SCROLL_BAR, COLOR_BG);

        if (num[DIR_LIST] > (scroll_value[DIR_LIST] + FILE_LIST_ROWS))
          print_string(FONT_CURSOR_DOWN_FILL, PSP_SCREEN_WIDTH - (FONTWIDTH * 2), FILE_LIST_POS_Y + ((FILE_LIST_ROWS - 1) * FONTHEIGHT), COLOR_SCROLL_BAR, COLOR_BG);
      }

      __draw_volume_status(1);
      flip_screen(1);


      gui_action = get_gui_input();

      switch (gui_action)
      {
        case CURSOR_DOWN:
          if (selection[column] < (num[column] - 1))
          {
            selection[column]++;

            if (in_scroll[column] == (FILE_LIST_ROWS - 1))
              scroll_value[column]++;
            else
              in_scroll[column]++;
          }
          break;

        case CURSOR_RTRIGGER:
          if (num[column] > PAGE_SCROLL_NUM)
          {
            if (selection[column] < (num[column] - PAGE_SCROLL_NUM))
            {
              selection[column] += PAGE_SCROLL_NUM;

              if (in_scroll[column] >= (FILE_LIST_ROWS - PAGE_SCROLL_NUM))
              {
                scroll_value[column] += PAGE_SCROLL_NUM;

                if (scroll_value[column] > (num[column] - FILE_LIST_ROWS))
                {
                  scroll_value[column] = num[column] - FILE_LIST_ROWS;
                  in_scroll[column] = selection[column] - scroll_value[column];
                }
              }
              else
              {
                in_scroll[column] += PAGE_SCROLL_NUM;
              }
            }
            else
            {
              selection[column] = num[column] - 1;
              in_scroll[column] += PAGE_SCROLL_NUM;

              if (in_scroll[column] >= (FILE_LIST_ROWS - 1))
              {
                if (num[column] > (FILE_LIST_ROWS - 1))
                {
                  in_scroll[column] = FILE_LIST_ROWS - 1;
                  scroll_value[column] = num[column] - FILE_LIST_ROWS;
                }
                else
                {
                  in_scroll[column] = num[column] - 1;
                }
              }
            }
          }
          else
          {
            selection[column] = num[column] - 1;
            in_scroll[column] = num[column] - 1;
          }
          break;

        case CURSOR_UP:
          if (selection[column] != 0)
          {
            selection[column]--;

            if (in_scroll[column] == 0)
              scroll_value[column]--;
            else
              in_scroll[column]--;
          }
          break;

        case CURSOR_LTRIGGER:
          if (selection[column] >= PAGE_SCROLL_NUM)
          {
            selection[column] -= PAGE_SCROLL_NUM;

            if (in_scroll[column] < PAGE_SCROLL_NUM)
            {
              if (scroll_value[column] >= PAGE_SCROLL_NUM)
              {
                scroll_value[column] -= PAGE_SCROLL_NUM;
              }
              else
              {
                scroll_value[column] = 0;
                in_scroll[column] = selection[column];
              }
            }
            else
            {
              in_scroll[column] -= PAGE_SCROLL_NUM;
            }
          }
          else
          {
            selection[column] = 0;
            in_scroll[column] = 0;
            scroll_value[column] = 0;
          }
          break;

        case CURSOR_RIGHT:
          if (column == FILE_LIST)
          {
            if (num[DIR_LIST] != 0)
              column = DIR_LIST;
          }
          break;

        case CURSOR_LEFT:
          if (column == DIR_LIST)
          {
            if (num[FILE_LIST] != 0)
              column = FILE_LIST;
          }
          break;

        case CURSOR_SELECT:
          if (column == DIR_LIST)
          {
            repeat = 0;
            chdir(dir_list[selection[DIR_LIST]]);
          }
          else
          {
            if (num[FILE_LIST] != 0)
            {
              char *selected_file = file_list[selection[FILE_LIST]];
              
              // Check if it's a separator line
              if (strncmp(selected_file, "---", 3) == 0) {
                // Don't do anything for separators
                break;
              }
              
              // Check if it's a recent game (starts with "> ")
              if (strncmp(selected_file, "> ", 2) == 0) {
                // Get the actual game index (minus the header and star prefix)
                int recent_index = selection[FILE_LIST] - 1; // Subtract the "Recent Games" header
                if (recent_index >= 0 && recent_index < num_recent_games) {
                  // Use the full path from recent_games array
                  strcpy(result, recent_games[recent_index]);
                  repeat = 0;
                  return_value = 0;
                }
              } else {
                // Normal file selection
                repeat = 0;
                return_value = 0;
                strcpy(result, selected_file);
              }
            }
          }
          break;

        case CURSOR_BACK:
          // ROOT
          if (strlen(strstr(current_dir_name, ":/")) == 2)
            break;

          repeat = 0;
          chdir("..");
          break;

        case CURSOR_EXIT:
          return_value = -1;
          repeat = 0;
          break;

        case CURSOR_DEFAULT:
          break;

        case CURSOR_NONE:
          break;
      }

    } /* end while (repeat) */

    filelist_term();

  } /* end while (return_value == 1) */

  return return_value;
}


static void get_savestate_info(char *filename, u16 *snapshot, char *timestamp)
{
  SceUID savestate_file;
  char savestate_path[MAX_PATH];

  sprintf(savestate_path, "%s%s", dir_state, filename);

  scePowerLock(0);

  FILE_OPEN(savestate_file, savestate_path, READ);

  if (FILE_CHECK_VALID(savestate_file))
  {
    u64 savestate_tick_utc;
    u64 savestate_tick_local;

    ScePspDateTime savestate_time = { 0 };

    if (snapshot != NULL)
      FILE_READ(savestate_file, snapshot, GBA_SCREEN_SIZE);
    else
      FILE_SEEK(savestate_file, GBA_SCREEN_SIZE, SEEK_SET);

    FILE_READ_VARIABLE(savestate_file, savestate_tick_utc);

    FILE_CLOSE(savestate_file);

    sceRtcConvertUtcToLocalTime(&savestate_tick_utc, &savestate_tick_local);
    sceRtcSetTick(&savestate_time, &savestate_tick_local);
    int day_of_week = sceRtcGetDayOfWeek(savestate_time.year, savestate_time.month, savestate_time.day);

    get_timestamp_string(timestamp, MSG_STATE_MENU_DATE_FMT_0, &savestate_time, day_of_week);
  }
  else
  {
    if (snapshot != NULL)
    {
      memset(snapshot, 0, GBA_SCREEN_SIZE);
		print_string_ext(MSG[MSG_STATE_MENU_STATE_NONE], X_POS_CENTER, 74, COLOR15_WHITE, BG_NO_FILL, snapshot, GBA_SCREEN_WIDTH);
    }

    sprintf(timestamp, "%s", MSG[(date_format == 0) ? MSG_STATE_MENU_DATE_NONE_0 : MSG_STATE_MENU_DATE_NONE_1]);
  }

  scePowerUnlock(0);
}

static void get_savestate_filename(u32 slot, char *name_buffer)
{
  char savestate_ext[16];

  sprintf(savestate_ext, "_%d.svs", (int)slot);
  change_ext(gamepak_filename, name_buffer, savestate_ext);
}


void action_loadstate(void)
{
  char savestate_filename[MAX_FILE];

  // Free overlay memory to ensure enough RAM for load operation
  pause_overlay_for_saveload();
  
  get_savestate_filename(savestate_slot, savestate_filename);
  load_state(savestate_filename);
  
  // Restore overlay after load completes
  resume_overlay_after_saveload();
}

void action_savestate(void)
{
  char savestate_filename[MAX_FILE];
  u16 *current_screen;

  current_screen = copy_screen();

  // Free overlay memory to ensure enough RAM for save operation
  pause_overlay_for_saveload();
  
  get_savestate_filename(savestate_slot, savestate_filename);
  save_state(savestate_filename, current_screen);

  free(current_screen);
  
  // Restore overlay after save completes
  resume_overlay_after_saveload();
}


u32 menu(void)
{
  // Use the same debug log as HOME button
  /*FILE *debug_log = fopen("froggba_debug.log", "a");
  if (debug_log) {
    fprintf(debug_log, "DEBUG: menu() function entry point\n");
    fprintf(debug_log, "DEBUG: overlay_menu_global address: %p\n", &overlay_menu_global);
    fprintf(debug_log, "DEBUG: overlay_options_global address: %p\n", overlay_options_global);
    fprintf(debug_log, "DEBUG: overlay_name_options address: %p\n", overlay_name_options);
    fflush(debug_log);
    fclose(debug_log);
  }*/

	int id_language;
  u32 i;

  u32 repeat = 1;
  u32 return_value = 0;

  u32 first_load = 0;

  GUI_ACTION_TYPE gui_action;
  SceCtrlData ctrl_data;

  char game_title[MAX_FILE];
  //char backup_id[16];
  u16 *screen_image_ptr = NULL;
  u16 *current_screen   = NULL;
  u16 *savestate_screen = NULL;

  u32 savestate_action = 0;
  static char savestate_timestamps[10][40];  // moved to static to prevent stack overflow

  char time_str[40];
  char batt_str[40];
  u16 color_batt_life = COLOR_BATT_NORMAL;
  u32 counter = 0;

  char filename_buffer[MAX_PATH];

  char line_buffer[80];

  static char cheat_format_str[MAX_CHEATS][25*4];// gpsp kai 41*4 - moved to static to prevent stack overflow

  MenuType *current_menu;
  MenuOptionType *current_option;
  MenuOptionType *display_option;

  u32 menu_init_flag = 0;

  u32 current_option_num = 0;
  u32 menu_main_option_num = 0;


  const char *yes_no_options[] =
  {
    MSG[MSG_NO],
    MSG[MSG_YES]
  };
  
  // Only do this initialization once
  static int menu_initialized = 0;
  if (!menu_initialized) {
    
    // Set up global yes/no options first
    global_yes_no_options[0] = MSG[MSG_NO];
    global_yes_no_options[1] = MSG[MSG_YES];
    
    // Set up overlay menu with MSG and global yes_no_options now available
    init_overlay_menu_late();
    overlay_options_global[1].options = (void*)global_yes_no_options;
    
    menu_initialized = 1;
  }

  const char *enable_disable_options[] =
  {
    MSG[MSG_DISABLED],
    MSG[MSG_ENABLED]
  };

  const char *on_off_options[] =
  {
    MSG[MSG_OFF],
    MSG[MSG_ON]
  };

  const char *scale_options[] =
  {
    MSG[MSG_SCN_SCALED_NONE],
    MSG[MSG_SCN_SCALED_X15_GU],
    MSG[MSG_SCN_SCALED_X15_SW],
    MSG[MSG_SCN_SCALED_USER]
  };

  const char *frameskip_options[] =
  {
    MSG[MSG_AUTO],
    MSG[MSG_MANUAL],
    MSG[MSG_OFF]
  };

  const char *color_correction_options[] =
  {
    MSG[MSG_OFF],
    "GPSP",
    "Retro"
  };

  const char *button_mapping_options[] =
  {
    "X/O",
    "O/X"
  };

  const char *stack_optimize_options[] =
  {
    MSG[MSG_OFF],
    MSG[MSG_AUTO]
  };

  const char *update_backup_options[] =
  {
    MSG[MSG_EXITONLY],
    MSG[MSG_AUTO]
  };


  const char *sound_volume_options[] =
  {
    "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"
  };

  const char *clock_speed_options[] =
  {
    "222MHz", "266MHz", "300MHz", "333MHz"
  };

  const char *image_format_options[] =
  {
    "PNG", "BMP"
  };

  const char *language_options[] =
  {
    MSG[MSG_LANG_JAPANESE], MSG[MSG_LANG_ENGLISH]
  };

  const char *aspect_ratio_options[] =
  {
    "Core Provided (3:2)", "Zoom (Fill Screen)", "Stretch (Full PSP)"
  };

  // Since we can't add messages to the message system, use direct string pointers cast as message indices
  // This is a hack but avoids breaking the message system alignment
  const char aspect_ratio_label[] = "Aspect Ratio: %s";
  const char aspect_ratio_help[] = "Left/Right: Select aspect ratio";

  // Dynamic overlay options will be updated by scan_overlay_files()
  // overlay_name_options is now a global variable

  const char *optimization_level_options_local[] =
  {
    "Maximum Accuracy", "Safe Optimizations", "Moderate Performance", "Maximum Performance"
  };

  const char *gamepad_config_buttons[] =
  {
    "Up", 
    "Down",
    "Left",
    "Right",
    "A",
    "B",
    "L",
    "R",
    "START",
    "SELECT",
    "menu",
    "fast forward",
    "load state",
    "save state",
    "RAPID FIRE A",
    "RAPID FIRE B",
    "RAPID FIRE L",
    "RAPID FIRE R",
    "show fps",
    "none"
  };

  auto void choose_menu(MenuType *new_menu);

  auto void menu_init(void);
  auto void menu_term(void);
  auto void menu_exit(void);
  auto void menu_quit(void);
  auto void menu_reset(void);
  auto void menu_suspend(void);

  auto void menu_screen_capture(void);

  auto void menu_change_state(void);
  auto void menu_select_savestate(void);
  auto void menu_save_state(void);
  auto void menu_load_state(void);
  auto void menu_load_state_file(void);

  auto void menu_default(void);
  auto void menu_load_cheat_file(void);
  auto void submenu_cheats_misc(void);

  auto void menu_load_file(void);

  auto void submenu_emulator(void);
  auto void submenu_gamepad(void);
  auto void submenu_analog(void);
  auto void submenu_savestate(void);
  auto void submenu_main(void);
  auto void reload_cheats_page(void);

  auto void draw_analog_pad_range(void);
  auto void load_savestate_timestamps(void);

  auto void gamepak_file_none(void);
  auto void gamepak_file_reopen(void);

  void menu_init(void)
  {
    menu_init_flag = 1;
    scan_overlay_files();
  }

  void menu_term(void)
  {
    screen_image_ptr = NULL;

    if (savestate_screen != NULL)
    {
      free(savestate_screen);
      savestate_screen = NULL;
    }

    if (current_screen != NULL)
    {
      free(current_screen);
      current_screen = NULL;
    }
  }

  void menu_exit(void)
  {
    if (!first_load)
      repeat = 0;
  }

  void menu_quit(void)
  {
    menu_term();
    quit();
  }

  void menu_suspend(void)
  {
    save_game_config_file();

    if (!first_load)
      update_backup_immediately();

    scePowerTick(0);
    scePowerRequestSuspend();
  }

  void menu_load_file(void)
  {
    const char *file_ext[] = { ".zip", ".gba", ".bin", ".agb", ".gbz", NULL };

    save_game_config_file();

    if (!first_load)
      update_backup_immediately();

    if (load_file(file_ext, filename_buffer, dir_roms) == 0)
    {
      if (load_gamepak(filename_buffer) < 0)
      {
        clear_screen(COLOR32_BLACK);
        error_msg(MSG[MSG_ERR_LOAD_GAMEPACK], CONFIRMATION_CONT);

        gamepak_file_none();

        menu_init();
        choose_menu(current_menu);
        counter = 0;

        return;
      }

      // Track recently played game with full path
      char full_game_path[MAX_PATH];
      if (filename_buffer[0] == '/' || strstr(filename_buffer, ":/")) {
        // Already a full path
        strcpy(full_game_path, filename_buffer);
      } else {
        // Construct full path
        sprintf(full_game_path, "%s%s", dir_roms, filename_buffer);
      }
      add_recent_game(full_game_path);

      reset_gba();
      reg[CHANGED_PC_STATUS] = 1;

      return_value = 1;
      repeat = 0;
    }
    else
    {
      menu_init();
      choose_menu(current_menu);
      counter = 0;
    }
  }

  void menu_reset(void)
  {
    if (!first_load)
    {
      reset_gba();
      reg[CHANGED_PC_STATUS] = 1;

      return_value = 1;
      repeat = 0;
    }
  }

  void menu_screen_capture(void)
  {
    if (!first_load)
    {
      scePowerLock(0);
      set_cpu_clock(PSP_CLOCK_333);

      if (option_screen_capture_format != 0)
      {
        get_snapshot_filename(filename_buffer, "bmp");
        save_bmp(filename_buffer, current_screen);
      }
      else
      {
        get_snapshot_filename(filename_buffer, "png");
        save_png(filename_buffer, current_screen);
      }

      set_cpu_clock(PSP_CLOCK_222);
      scePowerUnlock(0);
    }
  }

  void menu_change_state(void)
  {
    get_savestate_filename(savestate_slot, filename_buffer);
    get_savestate_info(filename_buffer, savestate_screen, line_buffer);
    sprintf(savestate_timestamps[savestate_slot], "%d: %s", (int)savestate_slot, line_buffer);

    screen_image_ptr = savestate_screen;
  }

  void menu_save_state(void)
  {
    if (!first_load)
    {
      get_savestate_filename(savestate_slot, filename_buffer);
      save_state(filename_buffer, current_screen);

      get_savestate_info(filename_buffer, savestate_screen, line_buffer);
      sprintf(savestate_timestamps[savestate_slot], "%d: %s", (int)savestate_slot, line_buffer);
      
      // Return to game after saving state
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_load_state(void)
  {
    if (!first_load)
    {
      get_savestate_filename(savestate_slot, filename_buffer);
      load_state(filename_buffer);

      return_value = 1;
      repeat = 0;
    }
  }

  void menu_select_savestate(void)
  {
    if (savestate_action != 0)
      menu_save_state();
    else
      menu_load_state();
  }

  void menu_load_state_file(void)
  {
    const char *file_ext[] = { ".svs", NULL };

    if ((load_file(file_ext, filename_buffer, dir_state) == 0) && !first_load)
    {
      load_state(filename_buffer);
      return_value = 1;
      repeat = 0;
    }
    else
    {
      menu_init();
      choose_menu(current_menu);
      counter = 0;
    }
  }

  void menu_default(void)
  {
	option_screen_scale = SCALED_X15_GU;
	option_screen_mag = 170;
	option_screen_filter = FILTER_BILINEAR;
	option_compatibility_mode = 0; // Default to fast mode
	option_button_mapping = 0; // Default to X/O mapping
	psp_fps_debug = 0;
	option_frameskip_type = FRAMESKIP_AUTO;
	option_frameskip_value = 9;
	option_clock_speed = PSP_CLOCK_333;
	option_sound_volume = 10;
	option_stack_optimize = 1;
	option_boot_mode = 0;
	option_update_backup = 1;		//auto
	option_screen_capture_format = 0;
	option_enable_analog = 0;
	option_analog_sensitivity = 4;
	
	// Get system language setting
	int id_language;
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &id_language);
	// Map PSP system language to our supported languages
	// 0 = Japanese, 1 = English, others default to English
	if (id_language == 0)
		option_language = 0;  // Japanese
	else
		option_language = 1;  // English (default for all other languages)

  }

  void menu_load_cheat_file(void)
  {
    const char *file_ext[] = { ".cht", NULL };
    char load_filename[MAX_FILE];

    if(load_file(file_ext, load_filename, dir_cheat) != -1)
    {

	  u32 i,j;
      for(j = 0; j < MAX_CHEATS; j++)
      {
        cheats[j].cheat_active = 0;
        cheats[j].cheat_name[0] = '\0';
      }

      add_cheats(load_filename);
      for(i = 0; i < MAX_CHEATS; i++)
      {

        if(i >= num_cheats)
        {
          sprintf(cheat_format_str[i], MSG[MSG_CHEAT_MENU_NON_LOAD], i);
        }
        else
        {
          sprintf(cheat_format_str[i], MSG[MSG_CHEAT_MENU_0], i, cheats[i].cheat_name);
        }
      }
	  //menu_cheat_page = 0;
      choose_menu(current_menu);
    }
    else
    {
      choose_menu(current_menu);
    }
  }

  // scan_overlay_files is now defined globally above

  #define DRAW_TITLE(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_GBA_ICON, MSG[title]);                  \
   print_string(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  #define DRAW_TITLE_PSP(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_PSP_ICON, MSG[title]);                  \
   print_string(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  #define DRAW_TITLE_SAVESTATE(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_MSC_ICON, MSG[title]);                  \
   print_string(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \


  #define DRAW_TITLE_GBK(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_GBA_ICON_GBK, MSG[title]);                  \
   print_string_gbk(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  #define DRAW_TITLE_PSP_GBK(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_PSP_ICON_GBK, MSG[title]);                  \
   print_string_gbk(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  #define DRAW_TITLE_SAVESTATE_GBK(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_MSC_ICON_GBK, MSG[title]);                  \
   print_string_gbk(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  #define DRAW_TITLE_OPT_GBK(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_OPT_ICON_GBK, MSG[title]);                  \
   print_string_gbk(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  #define DRAW_TITLE_PAD_GBK(title)                                                   \
   sprintf(line_buffer, "%s %s", FONT_PAD_ICON_GBK, MSG[title]);                  \
   print_string_gbk(line_buffer, 6, 2, COLOR_HELP_TEXT, BG_NO_FILL);              \

  void submenu_emulator(void)
  {
    DRAW_TITLE_OPT_GBK(MSG_OPTION_MENU_TITLE);
  }


  void submenu_cheats_misc(void)
  {
    DRAW_TITLE_PSP_GBK(MSG_CHEAT_MENU_TITLE);
  }

  void submenu_gamepad(void)
  {
    DRAW_TITLE_PAD_GBK(MSG_PAD_MENU_TITLE);
    
  }

  void submenu_analog(void)
  {
    DRAW_TITLE_PAD_GBK(MSG_A_PAD_MENU_TITLE);

    
    draw_analog_pad_range();
  }

  void submenu_savestate(void)
  {
    DRAW_TITLE_SAVESTATE_GBK(MSG_STATE_MENU_TITLE);

    if (menu_init_flag != 0)
    {
      savestate_action = 0;
      menu_change_state();

      current_option_num = savestate_slot;
      current_option = current_menu->options + current_option_num;

      menu_init_flag = 0;
    }

    if (current_option_num < 10)
    {
      if (savestate_slot != current_option_num)
      {
        savestate_slot = current_option_num;
        menu_change_state();
      }
	print_string(MSG[savestate_action ? MSG_SAVE : MSG_LOAD], MENU_LIST_POS_X + ((strlen(savestate_timestamps[0]) + 1) * FONTWIDTH), (current_option_num * FONTHEIGHT) + 28, COLOR_ACTIVE_ITEM, BG_NO_FILL);
    }
  }

  void submenu_main(void)
  {
    DRAW_TITLE_GBK(MSG_MAIN_MENU_TITLE);

    if (menu_init_flag != 0)
    {
      screen_image_ptr = current_screen;

      current_option_num = menu_main_option_num;
      current_option = current_menu->options + current_option_num;

      menu_init_flag = 0;
    }

    if (menu_main_option_num != current_option_num)
      menu_main_option_num = current_option_num;
  }

  void draw_analog_pad_range(void)
  {
    char print_buffer[40];
    u32 lx, ly;
    u32 analog_sensitivity, inv_analog_sensitivity;

    #define PAD_RANGE (255 >> 1)
    #define BASE_X (SCREEN_IMAGE_POS_X + ((GBA_SCREEN_WIDTH  - PAD_RANGE) >> 1))
    #define BASE_Y (SCREEN_IMAGE_POS_Y + ((GBA_SCREEN_HEIGHT - PAD_RANGE) >> 1))

    sceCtrlPeekBufferPositive(&ctrl_data, 1);
    lx = ctrl_data.Lx;
    ly = ctrl_data.Ly;

    analog_sensitivity = 20 + (option_analog_sensitivity * 10);
    inv_analog_sensitivity = 255 - analog_sensitivity;

    draw_box_alpha(SCREEN_IMAGE_POS_X, SCREEN_IMAGE_POS_Y, SCREEN_IMAGE_POS_X + GBA_SCREEN_WIDTH - 1, SCREEN_IMAGE_POS_Y + GBA_SCREEN_HEIGHT - 1, 0xBF000000);

    sprintf(print_buffer, "Lx:%3d Ly:%3d", (int)lx, (int)ly);
    print_string(print_buffer, SCREEN_IMAGE_POS_X + 6, SCREEN_IMAGE_POS_Y + 2, COLOR15_WHITE, BG_NO_FILL);

    if (lx < analog_sensitivity)
      print_string(FONT_CURSOR_LEFT, BASE_X - (FONTWIDTH << 1), BASE_Y + ((PAD_RANGE - FONTHEIGHT) >> 1), COLOR15_WHITE, BG_NO_FILL);

    if (lx > inv_analog_sensitivity)
      print_string(FONT_CURSOR_RIGHT, BASE_X + PAD_RANGE, BASE_Y + ((PAD_RANGE - FONTHEIGHT) >> 1), COLOR15_WHITE, BG_NO_FILL);

    if (ly < analog_sensitivity)
      print_string(FONT_CURSOR_UP, BASE_X + (PAD_RANGE >> 1) - FONTWIDTH, BASE_Y - FONTHEIGHT, COLOR15_WHITE, BG_NO_FILL);

    if (ly > inv_analog_sensitivity)
      print_string(FONT_CURSOR_DOWN, BASE_X + (PAD_RANGE >> 1) - FONTWIDTH, BASE_Y + PAD_RANGE, COLOR15_WHITE, BG_NO_FILL);

    lx >>= 1;
    ly >>= 1;
    analog_sensitivity >>= 1;
    inv_analog_sensitivity >>= 1;

    draw_box_line(BASE_X, BASE_Y, BASE_X + PAD_RANGE, BASE_Y + PAD_RANGE, COLOR15_WHITE);

    // dead zone
    draw_box_alpha(BASE_X + analog_sensitivity, BASE_Y + analog_sensitivity, BASE_X + inv_analog_sensitivity, BASE_Y + inv_analog_sensitivity, 0x5F000000 | 255);
    draw_box_line(BASE_X + analog_sensitivity, BASE_Y + analog_sensitivity, BASE_X + inv_analog_sensitivity, BASE_Y + inv_analog_sensitivity, COLOR15_RED);

    // pointer
    draw_box_line(BASE_X + lx - 2, BASE_Y + ly - 2, BASE_X + lx + 2, BASE_Y + ly + 2, COLOR15_WHITE);
    draw_hline(BASE_X + lx - 5, BASE_X + lx + 5, BASE_Y + ly, COLOR15_WHITE);
    draw_vline(BASE_X + lx, BASE_Y + ly - 5, BASE_Y + ly + 5, COLOR15_WHITE);
  }

  void load_savestate_timestamps(void)
  {
    for (i = 0; i < 10; i++)
    {
      get_savestate_filename(i, filename_buffer);
      get_savestate_info(filename_buffer, NULL, line_buffer);
      sprintf(savestate_timestamps[i], "%lu: %s", i, line_buffer);
    }
  }

  void gamepak_file_none(void)
  {
    gamepak_filename[0] = 0;
    game_title[0] = 0;

    first_load = 1;

    memset(current_screen, 0x00, GBA_SCREEN_SIZE);
	print_string_ext(MSG[MSG_NON_LOAD_GAME], X_POS_CENTER, 74, COLOR15_WHITE, BG_NO_FILL, current_screen, GBA_SCREEN_WIDTH);
  }

  void gamepak_file_reopen(void)
  {
    for (i = 0; i < 5; i++)
    {
      FILE_OPEN(gamepak_file_large, gamepak_filename_raw, READ);

      if (FILE_CHECK_VALID(gamepak_file_large))
        return;

      sceKernelDelayThread(500 * 1000);
    }

    clear_screen(COLOR32_BLACK);
    error_msg(MSG[MSG_ERR_OPEN_GAMEPACK], CONFIRMATION_QUIT);
    quit();
  }



  // Marker for help information, don't go past this mark (except \n)------*
  MenuOptionType emulator_options[] =
  {
    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_0], scale_options, &option_screen_scale, 4, MSG_OPTION_MENU_HELP_0, 0),

    NUMERIC_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_1], &option_screen_mag, 201, MSG_OPTION_MENU_HELP_1, 1),

    {NULL, NULL, NULL, "Aspect Ratio: %s", (void*)aspect_ratio_options, &option_aspect_ratio, 3, 0, 2, STRING_SELECTION_OPTION},

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_2], on_off_options, &option_screen_filter, 2, MSG_OPTION_MENU_HELP_2, 3),

    {NULL, NULL, NULL, "Compatibility Mode: %s", (void*)on_off_options, &option_compatibility_mode, 2, MSG_OPTION_MENU_HELP_7, 5, STRING_SELECTION_OPTION},

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_SHOW_FPS], on_off_options, &psp_fps_debug, 2, MSG_OPTION_MENU_HELP_SHOW_FPS, 6),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_COLOR_CORRECTION], color_correction_options, &option_color_correction, 3, MSG_OPTION_MENU_HELP_COLOR_CORRECTION, 7),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_BUTTON_MAPPING], button_mapping_options, &option_button_mapping, 2, MSG_OPTION_MENU_HELP_BUTTON_MAPPING, 8),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_3], frameskip_options, &option_frameskip_type, 3, MSG_OPTION_MENU_HELP_3, 9),

    NUMERIC_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_4], &option_frameskip_value, 10, MSG_OPTION_MENU_HELP_4, 10),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_5], clock_speed_options, &option_clock_speed, 4, MSG_OPTION_MENU_HELP_5, 11), 

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_6], sound_volume_options, &option_sound_volume, 11, MSG_OPTION_MENU_HELP_6, 12),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_7], stack_optimize_options, &option_stack_optimize, 2, MSG_OPTION_MENU_HELP_7, 13),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_8], yes_no_options, &option_boot_mode, 2, MSG_OPTION_MENU_HELP_8, 14),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_9], update_backup_options, &option_update_backup, 2, MSG_OPTION_MENU_HELP_9, 15), 

    STRING_SELECTION_OPTION(NULL, MSG[MSG_OPTION_MENU_10], language_options, &option_language, 2, MSG_OPTION_MENU_HELP_10, 16),

    ACTION_OPTION(NULL, NULL, MSG[MSG_OPTION_MENU_DEFAULT], MSG_OPTION_MENU_HELP_DEFAULT, 17),

    ACTION_SUBMENU_OPTION(NULL, NULL, MSG[MSG_OPTION_MENU_11], MSG_OPTION_MENU_HELP_11, 18)
  };

  MAKE_MENU(emulator, NULL, NULL);

  // Overlay menu is now initialized globally to avoid scope issues
  // The overlay_options_global and overlay_menu_global are used directly from main menu

  MenuOptionType cheats_misc_options[] =
  {
    CHEAT_OPTION((10 * menu_cheat_page) + 0),
    CHEAT_OPTION((10 * menu_cheat_page) + 1),
    CHEAT_OPTION((10 * menu_cheat_page) + 2),
    CHEAT_OPTION((10 * menu_cheat_page) + 3),
    CHEAT_OPTION((10 * menu_cheat_page) + 4),
    CHEAT_OPTION((10 * menu_cheat_page) + 5),
    CHEAT_OPTION((10 * menu_cheat_page) + 6),
    CHEAT_OPTION((10 * menu_cheat_page) + 7),
    CHEAT_OPTION((10 * menu_cheat_page) + 8),
    CHEAT_OPTION((10 * menu_cheat_page) + 9),

    NUMERIC_SELECTION_OPTION(reload_cheats_page, MSG[MSG_CHEAT_MENU_3], &menu_cheat_page, MAX_CHEATS_PAGE, MSG_CHEAT_MENU_HELP_3, 11),
    ACTION_OPTION(NULL, NULL, MSG[MSG_CHEAT_MENU_1], MSG_CHEAT_MENU_HELP_1, 13),

    SUBMENU_OPTION(NULL, MSG[MSG_CHEAT_MENU_2], MSG_CHEAT_MENU_HELP_2, 15)
  };

  MAKE_MENU(cheats_misc, NULL, NULL);

  MenuOptionType savestate_options[] =
  {
    SAVESTATE_OPTION(0),
    SAVESTATE_OPTION(1),
    SAVESTATE_OPTION(2),
    SAVESTATE_OPTION(3),
    SAVESTATE_OPTION(4),
    SAVESTATE_OPTION(5),
    SAVESTATE_OPTION(6),
    SAVESTATE_OPTION(7),
    SAVESTATE_OPTION(8),
    SAVESTATE_OPTION(9),

    ACTION_OPTION(NULL, NULL, MSG[MSG_STATE_MENU_1], MSG_STATE_MENU_HELP_1, 11),

    ACTION_SUBMENU_OPTION(NULL, NULL, MSG[MSG_STATE_MENU_2], MSG_STATE_MENU_HELP_2, 13)
  };

  MAKE_MENU(savestate, NULL, NULL);

  MenuOptionType gamepad_config_options[] =
  {
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_0], 0),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_1], 1),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_2], 2),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_3], 3),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_4], 4),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_5], 5),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_6], 6),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_7], 7),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_8], 8),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_9], 9),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_10], 10),
    GAMEPAD_CONFIG_OPTION(MSG[MSG_PAD_MENU_11], 11),

    ACTION_SUBMENU_OPTION(NULL, NULL, MSG[MSG_PAD_MENU_12], MSG_PAD_MENU_HELP_1, 13)
  };

  MAKE_MENU(gamepad_config, NULL, NULL);

  MenuOptionType analog_config_options[] =
  {
    ANALOG_CONFIG_OPTION(MSG[MSG_A_PAD_MENU_0], 0),
    ANALOG_CONFIG_OPTION(MSG[MSG_A_PAD_MENU_1], 1),
    ANALOG_CONFIG_OPTION(MSG[MSG_A_PAD_MENU_2], 2),
    ANALOG_CONFIG_OPTION(MSG[MSG_A_PAD_MENU_3], 3),

    STRING_SELECTION_OPTION(NULL, MSG[MSG_A_PAD_MENU_4], yes_no_options, &option_enable_analog, 2, MSG_A_PAD_MENU_HELP_0, 5),
    NUMERIC_SELECTION_OPTION(NULL, MSG[MSG_A_PAD_MENU_5], &option_analog_sensitivity, 10, MSG_A_PAD_MENU_HELP_1, 6),

    ACTION_SUBMENU_OPTION(NULL, NULL, MSG[MSG_A_PAD_MENU_6], MSG_A_PAD_MENU_HELP_2, 8)
  };

  MAKE_MENU(analog_config, NULL, NULL);

  MenuOptionType main_options[] =
  {
    NUMERIC_SELECTION_ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_0], &savestate_slot, 10, MSG_MAIN_MENU_HELP_0, 0),

    NUMERIC_SELECTION_ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_1], &savestate_slot, 10, MSG_MAIN_MENU_HELP_1, 1),

    ACTION_SUBMENU_OPTION(&savestate_menu, NULL, MSG[MSG_MAIN_MENU_2], MSG_MAIN_MENU_HELP_2, 2),

    STRING_SELECTION_ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_3], image_format_options, &option_screen_capture_format, 2, MSG_MAIN_MENU_HELP_3, 4),

    SUBMENU_OPTION(&emulator_menu, MSG[MSG_MAIN_MENU_4], MSG_MAIN_MENU_HELP_4, 6), 

    SUBMENU_OPTION(&gamepad_config_menu, MSG[MSG_MAIN_MENU_5], MSG_MAIN_MENU_HELP_5, 7),

    SUBMENU_OPTION(&analog_config_menu, MSG[MSG_MAIN_MENU_6], MSG_MAIN_MENU_HELP_6, 8),

    SUBMENU_OPTION(&cheats_misc_menu, MSG[MSG_MAIN_MENU_CHEAT], MSG_MAIN_MENU_HELP_CHEAT, 10),

    SUBMENU_OPTION(&overlay_menu_global, MSG[MSG_MAIN_MENU_OVERLAY], MSG_MAIN_MENU_HELP_OVERLAY, 11),

    ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_7], MSG_MAIN_MENU_HELP_7, 13),

    ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_8], MSG_MAIN_MENU_HELP_8, 15),

    ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_9], MSG_MAIN_MENU_HELP_9, 16),

    ACTION_OPTION(NULL, NULL, MSG[MSG_MAIN_MENU_11], MSG_MAIN_MENU_HELP_11, 18)
  };


  MAKE_MENU(main, NULL, NULL);


  void choose_menu(MenuType *new_menu)
  {
    printf("DEBUG: choose_menu called with menu: %p\n", new_menu);
    
    if (new_menu == NULL) {
      printf("DEBUG: new_menu was NULL, using main_menu\n");
      new_menu = &main_menu;
    }

    printf("DEBUG: Setting current_menu to %p\n", new_menu);
    current_menu = new_menu;
    
    printf("DEBUG: new_menu->options = %p\n", new_menu->options);
    printf("DEBUG: Setting current_option to %p\n", new_menu->options);
    current_option = new_menu->options;
    
    printf("DEBUG: Setting current_option_num to 0\n");
    current_option_num = 0;
    
    printf("DEBUG: choose_menu completed successfully\n");
  }

  void reload_cheats_page()
  {
    for(i = 0; i<10; i++)
    {
      cheats_misc_options[i].display_string = cheat_format_str[(10 * menu_cheat_page) + i];
      cheats_misc_options[i].current_option = &(cheats[(10 * menu_cheat_page) + i].cheat_active);
    }
  }


  sound_pause = 1;

  current_screen = copy_screen();
  savestate_screen = (u16 *)safe_malloc(GBA_SCREEN_SIZE);

  if (gamepak_filename[0] == 0)
    gamepak_file_none();
  else
    change_ext(gamepak_filename, game_title, "");

  screen_image_ptr = current_screen;

  load_savestate_timestamps();

  if (FILE_CHECK_VALID(gamepak_file_large))
  {
    FILE_CLOSE(gamepak_file_large);
    gamepak_file_large = -2;
  }


  for(i = 0; i < MAX_CHEATS; i++)
  {
    if(i >= num_cheats)
    {
      sprintf(cheat_format_str[i], MSG[MSG_CHEAT_MENU_NON_LOAD], i);
    }
    else
    {
      sprintf(cheat_format_str[i], MSG[MSG_CHEAT_MENU_0], i, cheats[i].cheat_name);
    }
  }
  //menu_cheat_page = 0;
  reload_cheats_page();

  video_resolution_large();
  set_cpu_clock(PSP_CLOCK_222);
  
  choose_menu(&main_menu);
  

  while (repeat)
  {
    clear_screen(COLOR15_TO_32(COLOR_BG));

    if ((counter % 30) == 0)
	{
	  update_status_string(time_str, batt_str, &color_batt_life);
	}
    counter++;
	print_string(time_str, TIME_STATUS_POS_X, 2, COLOR_HELP_TEXT, BG_NO_FILL);

	print_string(batt_str, BATT_STATUS_POS_X, 2, color_batt_life, BG_NO_FILL);

    print_string(game_title, 228, 28, COLOR_INACTIVE_ITEM, BG_NO_FILL);
    blit_to_screen(screen_image_ptr, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, SCREEN_IMAGE_POS_X, SCREEN_IMAGE_POS_Y);

    //print_string(backup_id, 228, 208, COLOR_INACTIVE_ITEM, BG_NO_FILL);

    if (current_menu && current_menu->init_function != NULL)
    {
      current_menu->init_function();
    }

    if (current_menu && current_menu->options)
    {
      display_option = current_menu->options;

      for (i = 0; i < current_menu->num_options; i++, display_option++)
      {
      if (display_option->option_type & NUMBER_SELECTION_OPTION)
      {
        sprintf(line_buffer, display_option->display_string, *(display_option->current_option));
      }
      else
      {
        if (display_option->option_type & STRING_SELECTION_OPTION)
          sprintf(line_buffer, display_option->display_string, ((u32 *)display_option->options)[*(display_option->current_option)]);
        else
          strcpy(line_buffer, display_option->display_string);
      }
/*file charset*/
		print_string(line_buffer, MENU_LIST_POS_X, (display_option->line_number * FONTHEIGHT) + 28, (display_option == current_option) ? COLOR_ACTIVE_ITEM : COLOR_INACTIVE_ITEM, BG_NO_FILL);
      }
    }

	print_string(MSG[current_option->help_string], 30, 258, COLOR_HELP_TEXT, BG_NO_FILL);
    print_string("FrogGBA - TempGBA mod by Prosty", 270, 258, COLOR_HELP_TEXT, BG_NO_FILL);

    // PSP controller - hold
    if (get_pad_input(PSP_CTRL_HOLD) != 0)
      print_string(FONT_KEY_ICON, 6, 258, COLOR15_YELLOW, BG_NO_FILL);

    __draw_volume_status(1);
    flip_screen(1);


    gui_action = get_gui_input();

    switch (gui_action)
    {
      case CURSOR_DOWN:
        current_option_num = (current_option_num + 1) % current_menu->num_options;

        current_option = current_menu->options + current_option_num;
        break;

      case CURSOR_UP:
        if (current_option_num != 0)
          current_option_num--;
        else
          current_option_num = current_menu->num_options - 1;

        current_option = current_menu->options + current_option_num;
        break;

      case CURSOR_RIGHT:
        if ((current_option->option_type & (NUMBER_SELECTION_OPTION | STRING_SELECTION_OPTION)) != 0)
        {
          *(current_option->current_option) = (*current_option->current_option + 1) % current_option->num_options;

          if (current_option->passive_function != NULL)
            current_option->passive_function();
        }
        break;

      case CURSOR_LEFT:
        if ((current_option->option_type & (NUMBER_SELECTION_OPTION | STRING_SELECTION_OPTION)) != 0)
        {
          u32 current_option_val = *(current_option->current_option);

          if (current_option_val != 0)
            current_option_val--;
          else
            current_option_val = current_option->num_options - 1;

          *(current_option->current_option) = current_option_val;

          if (current_option->passive_function != NULL)
            current_option->passive_function();
        }
        break;

      case CURSOR_RTRIGGER:
        if (current_menu == &main_menu)
        {
          menu_init();
          choose_menu(&savestate_menu);
        }
        break;

      case CURSOR_LTRIGGER:
        if (current_menu == &main_menu)
          menu_load_file();
        if (current_menu == &cheats_misc_menu)
          menu_load_cheat_file();
        break;

      case CURSOR_DEFAULT:
	  {
        /*if (current_menu == &emulator_menu)
		{	
			option_screen_scale = SCALED_X15_GU;
			option_screen_mag = 170;
			option_screen_filter = FILTER_BILINEAR;
			psp_fps_debug = 0;
			option_frameskip_type = FRAMESKIP_AUTO;
			option_frameskip_value = 9;
			option_clock_speed = PSP_CLOCK_333;
			option_sound_volume = 10;
			option_stack_optimize = 1;
			option_boot_mode = 0;
			option_update_backup = 0;
			option_screen_capture_format = 0;
			option_enable_analog = 0;
			option_analog_sensitivity = 4;
		}*/
	}
        break;

      case CURSOR_EXIT:
        if (current_menu == &main_menu)
        {
          menu_exit();
        }
        else
        {
          menu_init();
          choose_menu(&main_menu);
        }
        break;

      case CURSOR_SELECT:
        switch (current_option->option_type & (ACTION_OPTION | SUBMENU_OPTION))
        {
          case (ACTION_OPTION | SUBMENU_OPTION):
            if (current_option->action_function != NULL)
              current_option->action_function();
            choose_menu(current_option->sub_menu);
            break;

          case ACTION_OPTION:
            if (current_option->action_function != NULL)
              current_option->action_function();
            else
            {
              // Handle menu actions inline to avoid corrupted function pointers
              switch (current_option->line_number)
              {
                case 0:  // Load State
                  action_loadstate();
                  repeat = 0;  // Return to game after load
                  break;
                case 1:  // Save State  
                  action_savestate();
                  repeat = 0;  // Return to game after save
                  break;
                case 13: // "Load Game"
                  menu_load_file();
                  break;
                case 15: // "Reset Game"
                  menu_reset();
                  break;
                case 16: // "Return to Game"
                  repeat = 0;
                  break;
                case 18: // "Quit"
                  menu_quit();
                  break;
                default:
                  // For other actions, just continue for now
                  break;
              }
            }
            break;

          case SUBMENU_OPTION:
            choose_menu(current_option->sub_menu);
            break;

          default:
            break;
        }
        break;

      case CURSOR_BACK:
      case CURSOR_NONE:
        break;
    }

  } /* end while */


  scePowerLock(0);

  if (gamepak_file_large == -2)
    gamepak_file_reopen();

  while (get_pad_input(0x0001FFFF) != 0);

  menu_term();

  set_sound_volume();
  set_cpu_clock(option_clock_speed);

  sceDisplayWaitVblankStart();
  video_resolution_small();

  sound_pause = 0;
  //menu_cheat_page = 0;//

  scePowerUnlock(0);


  return return_value;
}


/*-----------------------------------------------------------------------------
  Status bar
-----------------------------------------------------------------------------*/

static void update_status_string(char *time_str, char *batt_str, u16 *color_batt)
{
  ScePspDateTime current_time = { 0 };

  u32 i = 0;
  int batt_life_per;
  int batt_life_time;

  static const char batt_icon[4][4] =
  {
    FONT_BATTERY0 "\0", // empty
    FONT_BATTERY1 "\0",
    FONT_BATTERY2 "\0",
    FONT_BATTERY3 "\0", // full
  };

  sceRtcGetCurrentClockLocalTime(&current_time);
  int day_of_week = sceRtcGetDayOfWeek(current_time.year, current_time.month, current_time.day);

  get_timestamp_string(time_str, MSG_MENU_DATE_FMT_0, &current_time, day_of_week);


  batt_life_per = scePowerGetBatteryLifePercent();

  if (batt_life_per < 0)
  {
    sprintf(batt_str, "%3s --%%", batt_icon[0]);
  }
  else
  {
    if (batt_life_per > 66)      i = 3;
    else if (batt_life_per > 33) i = 2;
    else if (batt_life_per >  9) i = 1;
    else                         i = 0;

    sprintf(batt_str, "%3s%3d%%", batt_icon[i], batt_life_per);
  }

  if (scePowerIsPowerOnline() == 1)
  {
    sprintf(batt_str, "%s%s", batt_str, MSG[MSG_CHARGE]);
  }
  else
  {
    batt_life_time = scePowerGetBatteryLifeTime();

    if (batt_life_time < 0)
      sprintf(batt_str, "%s%s", batt_str, "[--:--]");
    else
      sprintf(batt_str, "%s[%2d:%02d]", batt_str, (batt_life_time / 60) % 100, batt_life_time % 60);
  }

  if (scePowerIsBatteryCharging() == 1)
  {
    *color_batt = COLOR_BATT_CHARG;
  }
  else
  {
    if (scePowerIsLowBattery() == 1)
      *color_batt = COLOR_BATT_LOW;
    else
      *color_batt = COLOR_BATT_NORMAL;
  }
}

static void update_status_string_gbk(char *time_str, char *batt_str, u16 *color_batt)
{
  ScePspDateTime current_time = { 0 };

  u32 i = 0;
  int batt_life_per;
  int batt_life_time;

  static const char batt_icon[4][4] =
  {
    FONT_BATTERY0_GBK "\0", // empty
    FONT_BATTERY1_GBK "\0",
    FONT_BATTERY2_GBK "\0",
    FONT_BATTERY3_GBK "\0", // full
  };

  sceRtcGetCurrentClockLocalTime(&current_time);
  int day_of_week = sceRtcGetDayOfWeek(current_time.year, current_time.month, current_time.day);

  get_timestamp_string(time_str, MSG_MENU_DATE_FMT_0, &current_time, day_of_week);


  batt_life_per = scePowerGetBatteryLifePercent();

  if (batt_life_per < 0)
  {
    sprintf(batt_str, "%3s --%%", batt_icon[0]);
  }
  else
  {
    if (batt_life_per > 66)      i = 3;
    else if (batt_life_per > 33) i = 2;
    else if (batt_life_per >  9) i = 1;
    else                         i = 0;

    sprintf(batt_str, "%3s%3d%%", batt_icon[i], batt_life_per);
  }

  if (scePowerIsPowerOnline() == 1)
  {
    sprintf(batt_str, "%s%s", batt_str, MSG[MSG_CHARGE]);
  }
  else
  {
    batt_life_time = scePowerGetBatteryLifeTime();

    if (batt_life_time < 0)
      sprintf(batt_str, "%s%s", batt_str, "[--:--]");
    else
      sprintf(batt_str, "%s[%2d:%02d]", batt_str, (batt_life_time / 60) % 100, batt_life_time % 60);
  }

  if (scePowerIsBatteryCharging() == 1)
  {
    *color_batt = COLOR_BATT_CHARG;
  }
  else
  {
    if (scePowerIsLowBattery() == 1)
      *color_batt = COLOR_BATT_LOW;
    else
      *color_batt = COLOR_BATT_NORMAL;
  }
}


static void get_timestamp_string(char *buffer, u16 msg_id, ScePspDateTime *msg_time, int day_of_week)
{
  const char *week_str[] =
  {
    MSG[MSG_DAYW_0], MSG[MSG_DAYW_1], MSG[MSG_DAYW_2], MSG[MSG_DAYW_3], MSG[MSG_DAYW_4], MSG[MSG_DAYW_5], MSG[MSG_DAYW_6], ""
  };

  switch (date_format)
  {
    case 0: // DATE_FORMAT_YYYYMMDD
      sprintf(buffer, MSG[msg_id + 0], msg_time->year, msg_time->month, msg_time->day, week_str[day_of_week], msg_time->hour, msg_time->minute, msg_time->second, (msg_time->microsecond / 1000));
      break;
    case 1: // DATE_FORMAT_MMDDYYYY
      sprintf(buffer, MSG[msg_id + 1], msg_time->month, msg_time->day, msg_time->year, week_str[day_of_week], msg_time->hour, msg_time->minute, msg_time->second, (msg_time->microsecond / 1000));
      break;
    case 2: // DATE_FORMAT_DDMMYYYY
      sprintf(buffer, MSG[msg_id + 1], msg_time->day, msg_time->month, msg_time->year, week_str[day_of_week], msg_time->hour, msg_time->minute, msg_time->second, (msg_time->microsecond / 1000));
      break;
  }
}


/*-----------------------------------------------------------------------------
  Save Config Files
-----------------------------------------------------------------------------*/

static s32 save_game_config_file(void)
{
  SceUID game_config_file;
  char game_config_path[MAX_PATH];
  char game_config_filename[MAX_FILE];
  s32 return_value = -1;

  if (gamepak_filename[0] == 0)
    return return_value;

  change_ext(gamepak_filename, game_config_filename, ".cfg");
  sprintf(game_config_path, "%s%s", dir_cfg, game_config_filename);

  scePowerLock(0);

  FILE_OPEN(game_config_file, game_config_path, WRITE);

  if (FILE_CHECK_VALID(game_config_file))
  {
    u32 i;
    u32 file_options[GPSP_GAME_CONFIG_NUM];

    file_options[0]  = option_screen_scale;
    file_options[1]  = option_screen_mag;
    file_options[2]  = option_screen_filter;
    file_options[3] = option_frameskip_type;
    file_options[4] = option_frameskip_value;
    file_options[5] = option_clock_speed;
    file_options[6]  = option_sound_volume;

    for (i = 0; i < 16; i++)
    {
      file_options[7 + i] = gamepad_config_map[i];
    }

    FILE_WRITE_ARRAY(game_config_file, file_options);
    FILE_CLOSE(game_config_file);

    return_value = 0;
  }

  scePowerUnlock(0);

  return return_value;
}

s32 save_config_file(void)
{
  SceUID config_file;
  char config_path[MAX_PATH];

  s32 ret_value = -1;

  save_game_config_file();

  sprintf(config_path, "%s%s", main_path, GPSP_CONFIG_FILENAME);

  scePowerLock(0);

  FILE_OPEN(config_file, config_path, WRITE);

  if (FILE_CHECK_VALID(config_file))
  {
    u32 i;
    u32 file_options[GPSP_CONFIG_NUM];

    file_options[0]  = option_screen_scale;
    file_options[1]  = option_screen_mag;
    file_options[2]  = option_screen_filter;
    file_options[3] = psp_fps_debug;
    file_options[4] = option_frameskip_type;
    file_options[5] = option_frameskip_value;
    file_options[6] = option_clock_speed;
    file_options[7]  = option_sound_volume;
    file_options[8]  = option_stack_optimize;
    file_options[9]  = option_boot_mode;
    file_options[10]  = option_update_backup;
    file_options[11]  = option_screen_capture_format;
    file_options[12]  = option_enable_analog;
    file_options[13]  = option_analog_sensitivity;
    file_options[14]  = option_language;
    file_options[15]  = option_color_correction;
    file_options[16]  = option_overlay_enabled;
    file_options[17]  = option_overlay_selected;
    file_options[18]  = option_overlay_offset_x;
    file_options[19]  = option_overlay_offset_y;
    file_options[20]  = option_aspect_ratio;
    file_options[21]  = option_compatibility_mode;
    file_options[22]  = option_button_mapping;

    for (i = 0; i < 16; i++)
    {
      file_options[23 + i] = gamepad_config_map[i];
    }

    FILE_WRITE_ARRAY(config_file, file_options);
    FILE_CLOSE(config_file);

    ret_value = 0;
  }

  scePowerUnlock(0);

  return ret_value;
}


/*-----------------------------------------------------------------------------
  Load Config Files
-----------------------------------------------------------------------------*/

s32 load_game_config_file(void)
{
  SceUID game_config_file;
  char game_config_filename[MAX_FILE];
  char game_config_path[MAX_PATH];

  change_ext(gamepak_filename, game_config_filename, ".cfg");
  sprintf(game_config_path, "%s%s", dir_cfg, game_config_filename);

  FILE_OPEN(game_config_file, game_config_path, READ);

  if (FILE_CHECK_VALID(game_config_file))
  {
    u32 file_size = file_length(game_config_path);

    // Sanity check: File size must be the right size
    if (file_size == (GPSP_GAME_CONFIG_NUM * 4))
    {
      u32 i;
      u32 file_options[file_size / 4];
      s32 menu_button = -1;

      FILE_READ_ARRAY(game_config_file, file_options);

      option_screen_scale   = file_options[0] % 4;
      option_screen_mag     = file_options[1] % 201;
      option_screen_filter  = file_options[2] % 2;
      option_frameskip_type  = file_options[3] % 3;
      option_frameskip_value = file_options[4];
      option_clock_speed     = file_options[5] % 4;
      option_sound_volume   = file_options[6] % 11;

      for (i = 0; i < 16; i++)
      {
        gamepad_config_map[i] = file_options[7 + i] % (BUTTON_ID_NONE + 1);

        if (gamepad_config_map[i] == BUTTON_ID_MENU)
          menu_button = i;
      }

      if ((enable_home_menu == 0) && (menu_button == -1))
        gamepad_config_map[0] = BUTTON_ID_MENU;

      if (option_frameskip_value > 9)
        option_frameskip_value = 9;

	  u32 j;
      for(j = 0; j < MAX_CHEATS; j++)
      {
        cheats[j].cheat_active = 0;
        cheats[j].cheat_name[0] = '\0';
      }

      FILE_CLOSE(game_config_file);

      return 0;
    }
  }

  option_frameskip_type = FRAMESKIP_AUTO;
  option_frameskip_value = 9;
  option_clock_speed = PSP_CLOCK_333;

  return -1;
}

s32 load_config_file(void)
{
  SceUID config_file;
  char config_path[MAX_PATH];

  sprintf(config_path, "%s%s", main_path, GPSP_CONFIG_FILENAME);

  FILE_OPEN(config_file, config_path, READ);

  if (FILE_CHECK_VALID(config_file))
  {
    u32 file_size = file_length(config_path);

    // Sanity check: File size must be the right size
    if (file_size == (GPSP_CONFIG_NUM * 4))
    {
      u32 i;
      u32 file_options[file_size / 4];
      s32 menu_button = -1;

      FILE_READ_ARRAY(config_file, file_options);

      option_screen_scale   = file_options[0] % 4;
      option_screen_mag     = file_options[1] % 201;
      option_screen_filter  = file_options[2] % 2;
      psp_fps_debug       = file_options[3] % 2;
      option_frameskip_type  = file_options[4] % 3;
      option_frameskip_value = file_options[5];
      option_clock_speed     = file_options[6] % 4;
      option_sound_volume   = file_options[7] % 11;
      option_stack_optimize = file_options[8] % 2;
      option_boot_mode      = file_options[9] % 2;
      option_update_backup  = file_options[10] % 2;
      option_screen_capture_format = file_options[11] % 2;
      option_enable_analog  = file_options[12] % 2;
      option_analog_sensitivity = file_options[13] % 10;
      option_language = file_options[14] % 2;  // Only Japanese (0) and English (1)
      option_color_correction = file_options[15] % 3;  // 0 = Off, 1 = GPSP, 2 = Retro
      option_overlay_enabled = file_options[16] % 2;  // 0 = Off, 1 = On
      option_overlay_selected = file_options[17] % 10;  // 0-9 overlay selection
      option_overlay_offset_x = file_options[18] % 241;  // 0-240 X offset
      option_overlay_offset_y = file_options[19] % 113;  // 0-112 Y offset
      option_aspect_ratio = file_options[20] % 3;  // 0-2 aspect ratio
      option_compatibility_mode = file_options[21] % 2;  // 0 = Fast, 1 = Accurate
      option_button_mapping = file_options[22] % 2;  // 0 = X/O, 1 = O/X
      
      // Update memory timing when loading config
      set_compatibility_mode(option_compatibility_mode);

      for (i = 0; i < 16; i++)
      {
        gamepad_config_map[i] = file_options[23 + i] % (BUTTON_ID_NONE + 1);

        if (gamepad_config_map[i] == BUTTON_ID_MENU)
          menu_button = i;
      }

      if ((enable_home_menu == 0) && (menu_button == -1))
        gamepad_config_map[0] = BUTTON_ID_MENU;

      FILE_CLOSE(config_file);
    }

    return 0;
  }

  option_screen_scale = SCALED_X15_GU;
  option_screen_mag = 170;
  option_screen_filter = FILTER_BILINEAR;
  psp_fps_debug = 0;
  option_sound_volume = 10;
  option_stack_optimize = 1;
  option_boot_mode = 0;
  option_update_backup = 1;		//auto
  option_screen_capture_format = 0;
  option_enable_analog = 0;
  option_analog_sensitivity = 4;
  option_language = 1;  // Default to English
  option_color_correction = 0;  // Default to Off
  option_button_mapping = 0;  // Default to X/O mapping

  return -1;
}


s32 load_dir_cfg(char *file_name)
{
  char current_line[256];
  char current_variable[256];
  char current_value[256];

  const char item_roms[]  = "rom_directory";
  const char item_save[]  = "save_directory";
  const char item_state[] = "save_state_directory";
  const char item_cfg[]   = "game_config_directory";
  const char item_snap[]  = "snapshot_directory";
  const char item_cheat[]  = "cheat_directory";
  const char item_overlay[] = "overlay_directory";

  FILE *dir_config;
  SceUID check_dir = -1;

  char str_buf[256];
  u32 str_line = 7;

  auto void add_launch_directory(void);
  auto void set_directory(char *dir_name, const char *item_name);
  auto void check_directory(char *dir_name, const char *item_name);

  void add_launch_directory(void)
  {
    if (strchr(current_value, ':') == NULL)
    {
      strcpy(str_buf, current_value);
      sprintf(current_value, "%s%s", main_path, str_buf);
    }

    if (current_value[strlen(current_value) - 1] != '/')
    {
      strcat(current_value, "/");
    }
  }

  void set_directory(char *dir_name, const char *item_name)
  {
    if (strcasecmp(current_variable, item_name) == 0)
    {
      if ((check_dir = sceIoDopen(current_value)) >= 0)
      {
        strcpy(dir_name, current_value);
        sceIoDclose(check_dir);
      }
      else
      {
        sprintf(str_buf, MSG[MSG_ERR_SET_DIR_0], current_variable);
	    print_string(str_buf, 7, str_line, COLOR15_WHITE, COLOR15_BLACK);
        str_line += FONTHEIGHT;

        strcpy(dir_name, main_path);
        
        // Special case: overlay directory should have /overlays/ suffix
        if (strcasecmp(item_name, "overlay_directory") == 0)
        {
          strcat(dir_name, "overlays/");
        }
      }
    }
  }

  void check_directory(char *dir_name, const char *item_name)
  {
    if (dir_name[0] == 0)
    {
      sprintf(str_buf, MSG[MSG_ERR_SET_DIR_1], item_name);
	  print_string(str_buf, 7, str_line, COLOR15_WHITE, COLOR15_BLACK);
      str_line += FONTHEIGHT;

      strcpy(dir_name, main_path);
      
      // Special case: overlay directory should have /overlays/ suffix
      if (strcasecmp(item_name, "overlay_directory") == 0)
      {
        strcat(dir_name, "overlays/");
      }
    }
  }

  dir_roms[0]  = 0;
  dir_save[0]  = 0;
  dir_state[0] = 0;
  dir_cfg[0]   = 0;
  dir_snap[0]  = 0;
  dir_cheat[0]  = 0;
  dir_overlay[0] = 0;

  dir_config = fopen(file_name, "r");

  if (dir_config != NULL)
  {
    while (fgets(current_line, 256, dir_config))
    {
      if (parse_config_line(current_line, current_variable, current_value) != -1)
      {
        add_launch_directory();

        set_directory(dir_roms,  item_roms);
        set_directory(dir_save,  item_save);
        set_directory(dir_state, item_state);
        set_directory(dir_cfg,   item_cfg);
        set_directory(dir_snap,  item_snap);
        set_directory(dir_cheat, item_cheat);
        set_directory(dir_overlay, item_overlay);
      }
    }

    fclose(dir_config);

    check_directory(dir_roms,  item_roms);
    check_directory(dir_save,  item_save);
    check_directory(dir_state, item_state);
    check_directory(dir_cfg,   item_cfg);
    check_directory(dir_snap,  item_snap);
    check_directory(dir_cheat, item_cheat);
    
    // Always ensure overlay directory is set properly
    if (dir_overlay[0] == 0) {
      strcpy(dir_overlay, main_path);
      strcat(dir_overlay, "overlays/");
    }
    
    check_directory(dir_overlay, item_overlay);

    if (str_line > 7)
    {
      sprintf(str_buf, MSG[MSG_ERR_SET_DIR_2], main_path);
      sprintf(str_buf, "%s\n\n%s", str_buf, MSG[MSG_ERR_CONT]);

      str_line += FONTHEIGHT;
	  print_string(str_buf, 7, str_line, COLOR15_WHITE, COLOR15_BLACK);
      error_msg("", CONFIRMATION_NONE);
    }

    return 0;
  }

  // set launch directory
  strcpy(dir_roms,  main_path);
  strcpy(dir_save,  main_path);
  strcpy(dir_state, main_path);
  strcpy(dir_cfg,   main_path);
  strcpy(dir_snap,  main_path);
  strcpy(dir_cheat, main_path);
  strcpy(dir_overlay, main_path);
  strcat(dir_overlay, "overlays/");
  
  // Ensure overlay directory exists or fallback gracefully
  if (dir_overlay[0] == 0) {
    strcpy(dir_overlay, main_path);
    strcat(dir_overlay, "overlays/");
  }
  
  return -1;
}


/*-----------------------------------------------------------------------------
  Screen Capture
-----------------------------------------------------------------------------*/

static void get_snapshot_filename(char *name, const char *ext)
{
  char filename[MAX_FILE];
  char timestamp[80];

  ScePspDateTime current_time = { 0 };

  change_ext(gamepak_filename, filename, "_");

  sceRtcGetCurrentClockLocalTime(&current_time);
  get_timestamp_string(timestamp, MSG_SS_DATE_FMT_0, &current_time, 7);

  sprintf(name, "%s%s%s.%s", dir_snap, filename, timestamp, ext);
}

static void save_bmp(const char *path, u16 *screen_image)
{
  const u8 ALIGN_DATA header[] =
  {
     'B',  'M', 0x36, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00,  240, 0x00, 0x00, 0x00,  160, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  SceUID ss;

  u32 x, y;
  u16 color;
  u16 *src_ptr;
  u8  *bmp_data, *dest_ptr;

  bmp_data = (u8 *)malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * 3);

  if (bmp_data == NULL)
  {
    clear_screen(COLOR32_BLACK);
    error_msg(MSG[MSG_ERR_MALLOC], CONFIRMATION_CONT);
    return;
  }

  dest_ptr = bmp_data;

  for (y = 0; y < GBA_SCREEN_HEIGHT; y++)
  {
    src_ptr = &screen_image[(GBA_SCREEN_HEIGHT - y - 1) * GBA_SCREEN_WIDTH];

    for (x = 0; x < GBA_SCREEN_WIDTH; x++)
    {
      color = src_ptr[x];

      *dest_ptr++ = (u8)COL15_GET_B8(color);
      *dest_ptr++ = (u8)COL15_GET_G8(color);
      *dest_ptr++ = (u8)COL15_GET_R8(color);
    }
  }

  FILE_OPEN(ss, path, WRITE);

  if (FILE_CHECK_VALID(ss))
  {
    FILE_WRITE_VARIABLE(ss, header);
    FILE_WRITE(ss, bmp_data, GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * 3);
    FILE_CLOSE(ss);
  }

  free(bmp_data);
}


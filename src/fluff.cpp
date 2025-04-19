// Fluff  -- A fast, light utility for files 
// See About_text below for copyright and distribution license

#define APP_VER "1.1.6" // Last update 2025-04-07

/* Version 1.1.6 updated 2025-04-07 by Michael Losh
 * Fix to reset scroll to first file when revisiting directories
 * (otherwise, current scroll position may not allow any files to be seen
 *  after directory changes)
 * Moved config file location to ~/.config/fluff.conf
 * 

 * Version 1.1.5 updated 2025-03-29 by Michael Losh
 * Up, Back, FWD directory location buttons added to toolbar, reworked directory history management
 * 

 * Version 1.1.4 updated 2025-03-25 by Michael Losh
 * Don't look for TCE location in /opt/.tce_dir 
 * Default to /opt for loc2 button if /etc/sysconfig/tcedir TCE symlink is not present
 * Don't warn for selected files/folders if another process moves/deletes them

 * Version 1.1.3 updated 2025-03-22 by Michael Losh
 * fixes for improper file list item height (wrong item selected from pointer click)

 * Version 1.1.2 updated 2025-03-02 by Michael Losh
 * better child window resizing (WIP!)
 * icon in about and other message boxes

 * Version 1.1.1 updated 2025-03-02 by Michael Losh
 * fix for redrawing with new window title

 * Version 1.1.0 updated 2025-03-02 by Michael Losh
 * Updates to build cleanly in FLTK 1.4.x

 * Version 1.0.9 updated 2022-02-28 by Rich
 * Added "-Wimplicit-fallthrough" pragmas to silence warnings.
 * Reversed the following changes:
 * Changed cmd_list_p->data(n, (void*)i); to cmd_list_p->data(n, (void*)&assoc[i]);
 * Changed hint_list_p->data(n, (void*)fh); to hint_list_p->data(n, (void*)&file_hint[fh]);
 * Defined i and fh as intptr_t to eliminate cast to pointer from integer of different size
 *   warning when compiling 64 bit version.
 * Added -std=c++03 to compile command.
 * 
 * Version 1.0.8 updated 2020-02-01 by Rich
 * Changed #include <Fl/Fl_Hold_Browser.H to #include <FL/Fl_Hold_Browser.H
 * Changed {0xFF, 0xD8, 0x00, 0x00} to {'\xFF', '\xD8', '\x00', '\x00'} in
 * struct filetype_hint FiletypeHint. Compiler was generating int instead of byte.
 * I think the next 2 were typos.
 * They caused "cast to pointer from integer of different size" compiler warnings.
 * Changed cmd_list_p->data(n, (void*)i); to cmd_list_p->data(n, (void*)&assoc[i]);
 * Changed hint_list_p->data(n, (void*)fh); to hint_list_p->data(n, (void*)&file_hint[fh]);
 * Changed define FIRST_CONFIGURED_FILEHINT 11 to define FIRST_CONFIGURED_FILEHINT 12
 * because XPM FiletypeHint would not display.
 */
 
// DEFINE TO 1 TO SEE PROGRESS SLOWLY
#if 0
#define DELAY_FOR_TESTING  Fl::wait(1.0)
#else
#define DELAY_FOR_TESTING
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b))?(a):(b)
#endif

#define DEFAULT_DIR_PERMS    (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)

#define strclr(str) (str[0] = '\0')

#define MAXFILETYPE 100
#define MAXFTNAMELEN 64   // max length of filetype's descriptive name
#define MAXFILEHINT 200 
#define MAXASSOC 100        // application associations

#define MAXFNAMELEN 252     // filename
#define MAXFPATHLEN 748     // total path
#define MAXFFULLPATHLEN 1000    // total path plus name

#define MAXNINAME 80         // name of user or user group 
//#define MAXONAME 80         // user ("owner") name

#define MAXPATLEN 16           // file ident. matching pattern

#define MAXALBLLEN 32          // Assoc. app. "Action" label 
#define MAXCMDSPECLEN 128      // Assoc. app. command specification

#define MAXPROGOPERLEN 32
#define MAXPROGUNITLEN 32
#define MAXPROGLBLLEN  80

#define USING_SELECTIONS 1      // Source items are specified by selections in file list

/* ------------------------------------------------------- */

const char About_text[] = 
"Fluff File Manager version %s\n"
"copyright 2010 - 2025 by Michael A. Losh and contributors\n"
"\n"
"Fluff is free software: you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation, version 3.\n"
"\n"
"Fluff is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"See http://www.gnu.org/licenses/ for more details.";

/*-----------------------------------------------------------------*/

#include <stdio.h>
#include <cstdlib>
#include <stdint.h>		// intptr_t
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Multi_Browser.H>
#include <FL/Fl_Pack.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/fl_show_colormap.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_PNG_Image.H>
//#include <FL/names.h> 


// Simple types, structs ====================================
enum {DELETE_SELECTED, DELETE_DIR};
enum {HINTMETHOD_STRING, HINTMETHOD_BYTES, HINTMETHOD_EXTENSION};
struct filetype_hint {
    int filetype;
    int hintmethod;
    int offset;
    char pat[MAXPATLEN];
    int patlen;
};

struct association {
    int filetype;
    char action_label[MAXALBLLEN];
    char cmd_spec[MAXCMDSPECLEN];
};

struct name_id_item {
    struct name_id_item* next_p;
    int id;
    char name[MAXNINAME];
};

// Global Enumerations ========================================
// File type codes

// Executable specification type codes
enum {SPECTYPE_RUN = 1, SPECTYPE_OPEN = 2};

// Menu items
enum {  MI_NEW_FILE = 0, MI_NEW_FOLDER, MI_NEW_SYMLINK, 
        MI_RUN, MI_OPEN, MI_OPENWITH, MI_SELECTALL,
        MI_RENAME, MI_COPY, MI_COPYTO, MI_MOVE,
        MI_TRASH, MI_RESTORE, MI_DELETE, MI_PROPERTIES,
        MI_SELECT_ALL, MI_PASTE, MI_OPENTERM, 
        MI_OPTIONS, MI_CANCEL, MI_FILETYPES,
        MI_ABOUT, MI_HELP,
        MI_UP_DIR, MI_BK_DIR, MI_FW_DIR,
        MI_QUIT
};

// file copy or move operations
typedef enum {COPY_OPER, MOVE_OPER} fileop_enum;

// Main window modes
enum {MAINWND_MODE_VIEWING = 0, MAINWND_MODE_RENAMING};

// Batch copy/move constants
enum {BATCH_ASK_EACH = 0, BATCH_REPLACE_ALL, BATCH_SKIP_ALL, };
enum {FILE_ANSWER_EACH = 0, FILE_ANSWER_PROCEED, FILE_ANSWER_SKIP, FILE_ANSWER_UNKNOWN};

// Last click locations
enum {LASTCLICK_NONE, LASTCLICK_IN_TREE, LASTCLICK_IN_LIST};

// Sorting order specifications
enum {  SORTSPEC_NAME_ASC = 0, SORTSPEC_NAME_DESC, 
        SORTSPEC_TYPE_ASC,     SORTSPEC_TYPE_DESC,
        SORTSPEC_SIZE_ASC,     SORTSPEC_SIZE_DESC,
        SORTSPEC_DATE_ASC,     SORTSPEC_DATE_DESC
     };
     
// Progress indicator measurement types
enum { PROG_BY_FILE = 0, PROG_BY_BYTE};

// Classes ========================================

// The class Fl_DND_Box is based on an article "Initiating Drag and Drop by Example" 
// by "alvin" on the FLTK.org website, which carries a general copyright
// "1998-2008 by Bill Spitzak and others"
class Fl_DND_Box : public Fl_Box
{
    public:
        static void callback_deferred(void *v);
        Fl_DND_Box(int X, int Y, int W, int H, const char *L = 0);
        virtual ~Fl_DND_Box();
        int event();
        const char* event_text();
        int event_length();
        int handle(int e);

    protected:
        // The event which caused Fl_DND_Box to execute its callback
        int evt;
        char *evt_txt;
        int evt_len;
};


class file_item
{
    public:
        char path[MAXFPATHLEN];
        char name[MAXFNAMELEN];
        char fullpath[MAXFFULLPATHLEN];
        char typestr[80];
        char sizestr[16];
        char permstr[16];
        char ownstr[MAXNINAME + MAXNINAME + 8];
        char datestr[64];
        struct stat64 status;
        struct stat64 lstatus;        
        int filetype;
        int level;
        int expanded;
        int children;
        int subdirs;
        file_item* parent_fi_p;
        file_item* child_fi_p;
        file_item* next_fi_p;
        int is_reg(void);
        int is_dir(void);
        int is_link(void);
        int is_chardev(void);
        int is_blockdev(void);
        int is_fifo(void);
        int is_socket(void);
        file_item();
        file_item(char* thepath, char* thename);
        char* get_fullpath(void);
        const char* extension(void);
        int guess_filetype(void);
        int fetch_status(void);
};

// forward declaration
void switch_directory(file_item* new_dir_fi_p, bool is_fwd = true); 
void btnbar_go_bk_cb(void);
void btnbar_go_fw_cb(void);
void btnbar_go_up_cb(void);


class Dir_Tree_Browser : public Fl_Select_Browser {
    protected:
    int handle(int e);
    
    public:
    int dragx;
    int dragy;
    int drag_targ_itemnum;
    int list_item_height;
    Dir_Tree_Browser(int x, int y, int w, int h);
    int item_number_under_mouse(int mouse_y);
    file_item* file_item_under_mouse(int mouse_y);
    void* item_first(void) const;
    int visible_lines(void);
    int top_visible_line(void); 
    void populate(void);
};

class File_Detail_List_Browser : public Fl_Multi_Browser {
    protected:
    void draw(void); 

    public:
    int handle(int e);
    int dragx;
    int dragy;
    int dragging;
    int partitioning;
	int item_number_under_mouse(int mouse_y);
	int item_y_coordinate(int line); // relative to widget coordinate y
    File_Detail_List_Browser(int x, int y, int w, int h);
    int num_selected(void);
    long long size_selected(void);
    void populate(file_item* dir_fi_p);
};

class File_Props_Window : public Fl_Window {
protected:
    int handle(int e) ;

public:
    int                 changed;
    file_item*          fi_p;
    Fl_Pack*            window_pack_p;
    Fl_Pack*            perm_pack_p;
    Fl_Box*             perm_lbl_p;
    Fl_Pack*            ownperm_pack_p;
    Fl_Box*             ownperm_lbl_p;
    Fl_Check_Button*    ownperm_r_chk_p;
    Fl_Check_Button*    ownperm_w_chk_p;
    Fl_Check_Button*    ownperm_x_chk_p;
    Fl_Pack*            grpperm_pack_p;
    Fl_Box*             grpperm_lbl_p;
    Fl_Check_Button*    grpperm_r_chk_p;
    Fl_Check_Button*    grpperm_w_chk_p;
    Fl_Check_Button*    grpperm_x_chk_p;
    Fl_Pack*            othperm_pack_p;
    Fl_Box*             othperm_lbl_p;
    Fl_Check_Button*    othperm_r_chk_p;
    Fl_Check_Button*    othperm_w_chk_p;
    Fl_Check_Button*    othperm_x_chk_p;
    Fl_Box*             filetype_lbl_p;
    Fl_Box*             filetype_dsp_p;
    Fl_Button*          filetype_btn_p;
    Fl_Box*             linkpath_lbl_p;
    Fl_Box*             linkpath_dsp_p;
    //Fl_Button*          visitlink_btn_p;
    Fl_Box*             filename_lbl_p;
    Fl_Input*           filename_inp_p;
    Fl_Box*             filepath_lbl_p;
    Fl_Box*             filepath_dsp_p;
    Fl_Box*             filesize_lbl_p;
    Fl_Box*             filesize_dsp_p;
    Fl_Box*             fileowner_lbl_p;
    Fl_Input*           fileowner_inp_p;
    Fl_Box*             mdate_lbl_p; 
    Fl_Box*             mdate_dsp_p; 
    Fl_Box*             cdate_lbl_p; 
    Fl_Box*             cdate_dsp_p; 
    Fl_Box*             adate_lbl_p; 
    Fl_Box*             adate_dsp_p; 
    Fl_Pack*            button_pack_p;
    Fl_Button*          close_btn_p;
    Fl_Button*          revert_btn_p;
    Fl_Button*          apply_btn_p;
    Fl_Box*             dir_content_dsp_p;
    File_Props_Window (int x, int y, int w, int h, file_item* fileitem_p);
    ~File_Props_Window ();
    void setup(void);
    void show_file(file_item* fileitem_p);
    void update_buttons(void); 
    static void close_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void revert_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void apply_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void name_inp_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void fileowner_inp_cb(Fl_Widget* thewidget_p, void* theparent_p);

    static void perm_chk_cb(Fl_Widget* thewidget_p, void* theparent_p);
    //static void visitlink_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void filetype_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
};

#define FILETYPE_LBL_CTRL      1
#define TREAT_GENERIC_CHK_CTRL 2 
#define CMD_LIST_CTRL          3
#define OPEN_WITH_BTN_CTRL     4
#define MORE_BTN_CTRL          5
#define CANCEL_BTN_CTRL        6

class Open_With_Window : public Fl_Window {
public:
    int                 changed;
    int                 effective_filetype;
    
    Fl_Box*             filetype_lbl_p;
    Fl_Check_Button*    treat_generic_chk_p;
    Fl_Hold_Browser*    cmd_list_p;
    Fl_Button*          open_with_btn_p;
    Fl_Button*          manage_assoc_btn_p;
    Fl_Button*          cancel_btn_p;
    
                        Open_With_Window(int x, int y, int w, int h);
                        ~Open_With_Window();
    void                setup(void);
    void                update();
};

#define LIST_MAW_CTRL       1
#define RAISE_BTN_MAW_CTRL  2
#define LOWER_BTN_MAW_CTRL  3
#define ALABEL_INP_MAW_CTRL 4
#define CMD_INP_MAW_CTRL    5
#define ADD_BTN_MAW_CTRL    6
#define UNDO_BTN_MAW_CTRL   7
#define REMOV_BTN_MAW_CTRL  8
#define CLOSE_BTN_MAW_CTRL  9

class Manage_Associations_Window : public Fl_Window {
protected:

public:
    int                 filetype;
    int                 sel_assoc;
    association*        assoc_array;    // ptr to caller's copy
    association         assoc[MAXASSOC];// local copy
    int*                assoc_count_ptr;// ptr to caller's copy
    int                 assocs;         // local copy
    char                filetype_name_str[MAXFTNAMELEN];
    Fl_Box*             filetype_lbl_p;
    Fl_Hold_Browser*    cmd_list_p;
    Fl_Button*          raise_precedence_btn_p;   
    Fl_Button*          lower_precedence_btn_p;
    Fl_Box*             precedence_lbl_p;
    Fl_Input*           action_label_inp_p;
    Fl_Input*           cmd_inp_p;
    Fl_Button*          addnew_btn_p;
    Fl_Button*          undo_changes_btn_p;
    Fl_Button*          remove_btn_p;
    Fl_Button*          close_btn_p;
    
                        Manage_Associations_Window(int x, int y, int w, int h);
                        ~Manage_Associations_Window();
    void                enable_editing(void);
    void                disable_editing(void);
    void                show(int thefiletype, 
                             char* the_filetype_name_str, 
                             int*  the_assoc_count_ptr, 
                             association* the_assoc_array);
    void                hide(void);
    void                setup(void);
    void                update();
    void                swap_assoc_order(int sel, int other);
    static void         list_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         inp_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         raise_prec_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         lower_prec_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         remove_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         addnew_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         undo_changes_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    static void         close_btn_cb(Fl_Widget* thewidget_p, void* theparent_p);
    
};

const char HinttypeStr_str[] = "Header string";
const char HinttypeData_str[] = "Header bytes";
const char HinttypeExt_str[] = "Filename extension";

class Manage_Filetypes_Window : public Fl_Window {
public:
    char                filetype_name_str[MAXFILETYPE][MAXFTNAMELEN];
    int                 filetypes;
    char                new_filetype_name_str[MAXFTNAMELEN];
    filetype_hint       new_hint;
    int                 new_info_ready;
    filetype_hint       file_hint[MAXFILEHINT];
    int                 file_hints;
    association         assoc[MAXASSOC];
    int                 assocs;

    int                 sel_hint;
    int                 skip_pat_in_update;
    
    Fl_Box*             hint_lbl_p;
    Fl_Hold_Browser*    hint_list_p;

    Fl_Group*           editable_grp_p;
    Fl_Box*             filetype_lbl_p;
    Fl_Input*           filetype_name_inp_p;
    Fl_Button*          assoc_btn_p;
    Fl_Button*          addnew_type_btn_p;
    Fl_Box*             filehint_lbl_p;
    Fl_Check_Button*    embed_str_type_chk_p;
    Fl_Check_Button*    embed_data_type_chk_p;
    Fl_Check_Button*    extension_type_chk_p;
    Fl_Input*           hint_match_inp_p;
    Fl_Int_Input*       hint_offset_inp_p;

    Fl_Button*          addnew_hint_btn_p;
    Fl_Button*          remove_hint_btn_p;
    Fl_Button*          undo_changes_btn_p;
    Fl_Button*          close_btn_p;
    
                        Manage_Filetypes_Window(int x, int y, int w, int h);
                        ~Manage_Filetypes_Window();
    void                present(int filetype = -1);
    void                withdraw(void);

    void                setup(void);
    int                 hint_count_for_filetype(int filetype); 
    int                 scroll_to_hints_for_filetype(int filetype);
    int                 scroll_to_hint(int hint);
    void                show_hint(void);
    void                update(void);
};

//class Fluff_Window : public Fl_Window {
class Fluff_Window : public Fl_Double_Window {
    protected:
    int handle(int e) ;

    public:
    int mode;
    char path[MAXFFULLPATHLEN];
    char wintitle[1024];
    int progress_by_bytes;
    char prog_oper_str[MAXPROGOPERLEN];
    char prog_unit_str[MAXPROGUNITLEN];
    char prog_label_str[MAXPROGLBLLEN];
    int  prog_max_amount;
    float prog_divider;
    
    Fl_Pack* btnbarpack_p;
    //Fl_Button* quit_btn_p;
    Fl_Button* run_btn_p;
    Fl_Button* open_btn_p;
    Fl_Button* props_btn_p;
    Fl_Check_Button* showall_chk_p;
    Fl_Check_Button* sudo_chk_p;
    Fl_Button* copy_btn_p;
    Fl_Button* paste_btn_p;
    Fl_Button* trash_btn_p;
    Fl_Button* delete_btn_p;
    Fl_Button* loc1_btn_p;
    Fl_Button* loc2_btn_p;

    Fl_Button* go_up_btn_p;  // go up a folder level
    Fl_Button* go_bk_btn_p;  // go back to previous location
    Fl_Button* go_fw_btn_p;  // go forward to location you were at before

    Fl_Box*    path_lbl_p;
    Fl_Menu_Button* pathhist_menu_p;
    Fl_Button* about_btn_p;
    Fl_Button* help_btn_p;
    
    Fl_Pack* browspack_p;

    Fl_Group* treegroup_p;
    Dir_Tree_Browser * tree_p;
    Fl_Group* status_grp_p;
    Fl_Box*   mainwnd_status_lbl_p;

    Fl_Progress*   progress_p;
    Fl_Group* listgroup_p;
    File_Detail_List_Browser* list_p;
    Fl_Box*   files_info_lbl_p;
    file_item* seldir_fi_p;
    //file_item* seldir2_fi_p;
    file_item* sel_fi_p;                    // selected file in list browser
    file_item* targdir_fi_p;
    file_item* rename_fi_p;
    File_Props_Window* fprops_p;
    Fl_Input* rename_inp_p;
    int adjusting;
    int last_click_browser;
    int showingall;
    int sortspec;
    int rename_x;
    int rename_y;
    int dragx;
    int dragy;
    Fluff_Window();
    ~Fluff_Window();
    void set_titles(void);
    void do_menu(void);
    void setup(void);     
    void enable_renaming(file_item* fi_p, int x, int y);
    void end_renaming(void);
    void commit_renaming(void);
    void update_btnbar(void);
    void adjust_pathhist_menu_size(void); 
    void init_progress(int by_bytes, const char* unit_str, const char* oper_str, int max);
    void update_progress(int amount);
    void complete_progress(void);

};

// Global Values, Arrays, Constants ==================================

enum {  FT_DIR = 0, FT_CHARDEV, FT_BLOCKDEV, FT_FIFO, FT_SOCKET,
        FT_TEXT, FT_EXEC, FT_SHELLSCRIPT, FT_TCZ_EXT, FT_LST,
        FT_IMAGE_JPG, FT_IMAGE_PNG, FT_IMAGE_GIF,  
        FT_IMAGE_BMP, FT_IMAGE_XPM
};


char FiletypeName_str[MAXFILETYPE][MAXFTNAMELEN] = {
    "dir",
    "char dev",
    "block dev",
    "fifo",
    "socket",
    "file", 
    "executable",
    "shell script",
    "tcz",
    "list",
    "jpg image", 
    "png image", 
    "gif image",
    "bmp image",
    "xpm image",
    ""
};

#define FIRST_CONFIGURED_FILETYPE   FT_IMAGE_XPM + 1
int Filetypes = FIRST_CONFIGURED_FILETYPE;

struct filetype_hint FiletypeHint[MAXFILEHINT] = {
    // filetype,     hint_method,    offset, pattern, length
    {FT_TEXT,        HINTMETHOD_STRING,   0, "",     0},
    {FT_EXEC,        HINTMETHOD_STRING,   1, "ELF",  3},
    {FT_SHELLSCRIPT, HINTMETHOD_STRING,   0, "#!/",  3},
    {FT_TCZ_EXT,     HINTMETHOD_EXTENSION,0, ".tcz", 4},
    {FT_LST,         HINTMETHOD_EXTENSION,0, ".lst", 4},
    {FT_IMAGE_JPG,   HINTMETHOD_BYTES,    0, {'\xFF', '\xD8', '\x00', '\x00'}, 2 },
    {FT_IMAGE_JPG,   HINTMETHOD_STRING,   6, "JFIF", 4 },
    {FT_IMAGE_JPG,   HINTMETHOD_STRING,   6, "Exif", 4 },
    {FT_IMAGE_PNG,   HINTMETHOD_STRING,   1, "PNG",  3 },
    {FT_IMAGE_GIF,   HINTMETHOD_STRING,   0, "GIF",  3 },
    {FT_IMAGE_BMP,   HINTMETHOD_STRING,   0, "BM",   2 },
    {FT_IMAGE_XPM,   HINTMETHOD_STRING,   3, "XPM",  3 },
    {-1,             0,                   0, "",     0 }
};
int HintCountPerFiletype[MAXFILEHINT] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
#define FIRST_CONFIGURED_FILEHINT 12
int FiletypeHints = FIRST_CONFIGURED_FILEHINT;

int AssocCountPerFiletype[MAXASSOC] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
struct association Association[MAXASSOC] = {
    //type       //action label //run cmd                
    {FT_TEXT,       "View",     "xterm -e vi -R %s &"   },
    {FT_TEXT,       "Edit",     "xterm -e vi %s &"            },

    {FT_DIR,        "Explore",  "fluff %s &"         },  // does not really launch separately
    {FT_DIR,        "Archive",  "tar -czf archive.tar.gz %s &"},

    {FT_EXEC,       "Run",      "./%s &"                },

    {FT_SHELLSCRIPT,"Run",      "xterm -e ./%s &"       },          
    {FT_SHELLSCRIPT,"Edit",     "editor %s &"           },

    {FT_TCZ_EXT,    "Run",      "tce-run %s &"          },

    {FT_LST,        "Edit",     "editor %s &"            },

    {-1, "", ""}
};
int Associations = 9;

static void MainMenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item main_right_click_menu[20] = {
    {"&About",              FL_F + 10,      MainMenuCB, (void*)MI_ABOUT},
    {"&Help",               FL_F + 1,       MainMenuCB, (void*)MI_HELP,FL_MENU_DIVIDER},
    {"&Filetypes",          'f',            MainMenuCB, (void*)MI_FILETYPES,FL_MENU_DIVIDER},
    {"&Up to parent dir.",   FL_CTRL + 'u', MainMenuCB, (void*)MI_UP_DIR},
    {"&Back in dir. hist",   FL_CTRL + 'b', MainMenuCB, (void*)MI_BK_DIR},
    {"&Fwd. in dir. hist",   FL_CTRL + 'f', MainMenuCB, (void*)MI_FW_DIR,FL_MENU_DIVIDER},
    {"&Filetypes",          'f',            MainMenuCB, (void*)MI_FILETYPES,FL_MENU_DIVIDER},
    {"&Quit",               FL_CTRL + 'q',  MainMenuCB, (void*)MI_QUIT},
    {0}
};

static void DirTreeMenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item dirtree_right_click_menu[20] = {
    {"Open &terminal here",     FL_ALT+'t', DirTreeMenuCB, (void*)MI_OPENTERM},
    {"Create director&y",       'y', DirTreeMenuCB, (void*)MI_NEW_FOLDER,FL_MENU_DIVIDER},
    {"&Paste to here",           FL_CTRL+'v', DirTreeMenuCB, (void*)MI_PASTE, FL_MENU_INACTIVE},
    {"&Copy selected to here",  'c', DirTreeMenuCB, (void*)MI_COPYTO, FL_MENU_INACTIVE},
    {"&Move selected to here",  'm', DirTreeMenuCB, (void*)MI_MOVE, FL_MENU_INACTIVE},
    {"&Rename",                 FL_F + 2,    DirTreeMenuCB, (void*)MI_RENAME,FL_MENU_DIVIDER},
    {"In&sert in trash",        FL_Insert, DirTreeMenuCB, (void*)MI_TRASH},
    {"&Delete",                 FL_Delete,   DirTreeMenuCB, (void*)MI_DELETE,FL_MENU_DIVIDER},
    {"Propert&ies",             FL_F + 7, DirTreeMenuCB, (void*)MI_PROPERTIES,FL_MENU_DIVIDER},
    {"&About",                  FL_F + 10, DirTreeMenuCB, (void*)MI_ABOUT},
    {"&Help",                   FL_F + 1, DirTreeMenuCB, (void*)MI_HELP,FL_MENU_DIVIDER},
    {"&Up to parent dir.",      FL_CTRL + 'u', DirTreeMenuCB, (void*)MI_UP_DIR},
    {"&Back in dir. hist",      FL_CTRL + 'b', DirTreeMenuCB, (void*)MI_BK_DIR},
    {"&Fwd. in dir. hist",      FL_CTRL + 'f', DirTreeMenuCB, (void*)MI_FW_DIR,FL_MENU_DIVIDER},
    {"&Quit",                   FL_CTRL + 'q', DirTreeMenuCB, (void*)MI_QUIT},
    {0}
};

static void FileMenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item file_right_click_menu[20] = {
    {"---",                 FL_F+3, FileMenuCB, (void*)MI_RUN},
    {"---",                 FL_F+4, FileMenuCB, (void*)MI_OPEN},
    {"Open &with...",       FL_F+8, FileMenuCB, (void*)MI_OPENWITH},
    {"Select &all",         FL_CTRL+'a', FileMenuCB, (void*)MI_SELECTALL},
    {"&Copy",               FL_CTRL+'c', FileMenuCB, (void*)MI_COPY},
    {"&Paste",              FL_CTRL+'v', FileMenuCB, (void*)MI_PASTE},
    {"&Rename",             FL_F + 2,    FileMenuCB, (void*)MI_RENAME,FL_MENU_DIVIDER},
    {"In&sert in trash",    FL_Insert, FileMenuCB, (void*)MI_TRASH},
    {"Re&store from trash", 's', FileMenuCB, (void*)MI_RESTORE},
    {"&Delete",             FL_Delete,   FileMenuCB, (void*)MI_DELETE,FL_MENU_DIVIDER},
    {"Open &terminal here", FL_ALT+'t', FileMenuCB, (void*)MI_OPENTERM},
    {"Propert&ies",         FL_F + 7, FileMenuCB, (void*)MI_PROPERTIES,FL_MENU_DIVIDER},
    {"&Up to parent dir.",  FL_CTRL + 'u', FileMenuCB, (void*)MI_UP_DIR},
    {"&Back in dir. hist",  FL_CTRL + 'b', FileMenuCB, (void*)MI_BK_DIR},
    {"&Fwd. in dir. hist",  FL_CTRL + 'f', FileMenuCB, (void*)MI_FW_DIR,FL_MENU_DIVIDER},
    {"&Help",               FL_F + 10, FileMenuCB, (void*)MI_HELP},
    {"Cance&l",             'l', FileMenuCB, (void*)MI_CANCEL},
    {0}
};

#define PASTE_DIRTREE_MENU_ITEM_INDEX   2
#define PASTE_FILELIST_MENU_ITEM_INDEX   5
#define COPY_TO_MENU_ITEM_INDEX 3
#define MOVE_TO_MENU_ITEM_INDEX 4
#define INSERT_IN_TRASH_ITEM_INDEX 7
#define RESTORE_FROM_TRASH_ITEM_INDEX 8

// File list column widths in pixels
//                name, type, size, date, perms, own 
int ColWidths[] = {150,  42,   64,   84,  74,  0};

// File type/hint list    ftype, htype, pattern, offset 
int FileTypeColWidths[] = {112,  140,   100,      0,    0};

struct name_id_item * GroupList_p = NULL;
struct name_id_item * OwnerList_p = NULL;

#define MAX_WATCH 1024
int DirWatch_fd;            // file descriptor for inotify
int DirWatch_wd[MAX_WATCH]; // watch descriptors
file_item* Watched_fi_p[MAX_WATCH]; // watch descriptors
int DirWatches = 0;
#define WATCHBUFLEN 1024
char WatchEventBuf[1024];

Fl_Input* Rename_inp_p = NULL;

int BatchReplaceMode = BATCH_ASK_EACH;  // how to handle batch copy/move ops

char* MarkedFileItems_sz_p = NULL;
Fluff_Window* MainWnd_p = NULL;
Open_With_Window* OpenWithWnd_p = NULL;
Manage_Associations_Window* ManageAssocWnd_p = NULL;
Manage_Filetypes_Window* ManageFiletypesWnd_p = NULL;

int UseSudo = 0;
int Running = 1;
file_item* Root_fi_p = NULL;
file_item* Loc1_fi_p = NULL;
file_item* Loc2_fi_p = NULL;

#define VISITED_DIR_MAX 16
file_item* VisitedDir[VISITED_DIR_MAX];
int VisitedDirs = 0;
unsigned int VisitedDirNewest = 0;
unsigned int VisitedDirCur = 0;

int OperationInProgress = 0;
double TimeToResync_rsec = 0.0;


// Color Management
#define FLUFF_NORMTEXT_COLOR  (Fl_Color)16
#define FLUFF_HIDETEXT_COLOR  (Fl_Color)17
#define FLUFF_LISTBG_COLOR    (Fl_Color)18
#define FLUFF_LISTBG2_COLOR   (Fl_Color)19
char TextColor_str[8]  = "@C16";    
char HideColor_str[8]  = "@C17";
char BgColor_str[8]    = "@B18";    
char Bg2Color_str[8]   = "@B19";
char BtnColor_str[8]   = "@B49";   

int FollowLinks = 0;
double GuiEnterEventTime_rsec = 0.0;
double FileSyncRequestTime_rsec = 0.0;

// Forward references ========================================
int get_child_file_items(file_item* fi_p);
int build_tree_for_branch(char* path, file_item* root_fi_p, char* targpath, file_item** targ_fi_pp);
void prune_single_file_item(file_item* fi_p);
void prune_children(file_item* fi_p);
void prune_file_item(file_item* fi_p);
void remove_dir_watches(void);
void remove_watch(file_item* dir_fi_p);
void setup_dir_watch(file_item* dir_fi_p);
void filelist_change_cb(void);
int conditionally_rename(file_item* fi_p, const char* newname);
void dirtree_dnd_cb(Fl_Widget *o, void *v);
void filelist_dnd_cb(Fl_Widget *o, void *v);
static void enable_menu_item(Fl_Menu_Item* menu_p, int index, int enable = 1);
void mark_file_items(File_Detail_List_Browser* mb_p);
void btnbar_quit_cb(void);
void btnbar_action1_cb(void);
void btnbar_action2_cb(void);
void btnbar_props_cb(void);
void btnbar_showall_cb(void);
void btnbar_sudo_cb(void);
void btnbar_copy_cb(void);
void btnbar_paste_cb(void);
void btnbar_trash_cb(void);
void btnbar_delete_cb(void);
void btnbar_help_cb(void);
char* sel_files_str(void);
void perform_file_copy_or_move(fileop_enum oper, file_item* dir_fi_p, int using_sel = 0);
void perform_file_delete(int what);
void relabel_menu_item(Fl_Menu_Item* menu_p, int index, const char* newlabel);
void sync_gui_with_filesystem(int try_to_reselect = 0);
void begin_renaming(void);
int column_in_filelist(int x);
int new_item_sorts_before_existing_item(file_item* new_fi_p, file_item* fi_p, int sortspec);
void analyze_and_disp_status(void);
void bytecount_size_str(char* sizestr, int bytes);
int handle_global_keys(int e);
void perform_select_all(void);
int int_value_from_input_field(Fl_Input* inp_p, int* value, const char* warning_str = NULL);
int marked_file_items(const char* urls);

// Functions ==================================================================
double systemtime_as_real(void)
{
    static struct timeval now;
    static double now_rsec = 0.0;
    if (gettimeofday(&now, NULL)) {
        perror("gettimeofday");
        return 0.0l;
    }
    now_rsec = ((double)now.tv_sec) + (((double)now.tv_usec) / 1000000.0);
    return now_rsec;
}

char* strnzcpy(char* dstr, const char* sstr, size_t buflen) {
    int n = buflen - 1;
    if (n < 0) n = 0;
    char* ret = strncpy(dstr, sstr, n);
    dstr[n] = '\0';
    return ret;
} 
void wait_cursor(void)
{
    fl_cursor(FL_CURSOR_WAIT, FL_BLACK, FL_WHITE);
    OperationInProgress = 1;
    Fl::check();
}

void normal_cursor(void)
{
    fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
    OperationInProgress = 0;
    Fl::wait(0.01);
}

char* get_cmd_spec_for_filetype(int filetype, int spectype)
{
    int i;
    int found = 0;
    for (i = 0; i < Associations; i++) {
        if (Association[i].filetype == filetype) {
            found++;
            if (found == spectype) {
                return Association[i].cmd_spec;
            }
        }
    }
    return NULL;
}

char* get_action_label_for_filetype(int filetype, int spectype) 
{
    int i;
    int found = 0;
    for (i = 0; i < Associations; i++) {
        if (Association[i].filetype == filetype) {
            found++;
            if (found == spectype) {
                return Association[i].action_label;
            }
        }
    }
    return NULL;    
}

void scan_users_and_groups(void) {
    FILE * grpf_p = fopen("/etc/group", "r");
    struct name_id_item * prev_p = NULL;
    struct name_id_item * new_item_p;
    struct group* group_ent_p = NULL;
    if (grpf_p) {
        do {
            group_ent_p = fgetgrent(grpf_p);
            if (group_ent_p) {
                new_item_p= new name_id_item;
                new_item_p->next_p = NULL;
                strnzcpy(new_item_p->name, group_ent_p->gr_name, MAXNINAME);
                new_item_p->id = group_ent_p->gr_gid;
                if (prev_p) {
                    prev_p->next_p = new_item_p;
                }
                else {
                    GroupList_p = new_item_p;
                }
                //printf("Group %d is '%s'\n", new_item_p->id, new_item_p->name); fflush(0);
                prev_p = new_item_p;
            }
        } while (group_ent_p && !feof(grpf_p));
        fclose(grpf_p);
    }
    
    FILE * passwdf_p = fopen("/etc/passwd", "r");
    struct passwd* owner_ent_p = NULL;
    prev_p = NULL;
    if (passwdf_p) {
        do {
            owner_ent_p = fgetpwent(passwdf_p);
            if (owner_ent_p) {
                new_item_p = new name_id_item;
                new_item_p->next_p = NULL;
                strnzcpy(new_item_p->name, owner_ent_p->pw_name, MAXNINAME);                
                new_item_p->id = owner_ent_p->pw_uid;
                if (prev_p) {
                    prev_p->next_p = new_item_p;
                }
                else {
                    OwnerList_p = new_item_p;
                }
                //printf("Owner %d is '%s'\n", new_item_p->id, new_item_p->name); fflush(0);
                prev_p = new_item_p;
            }
        } while (owner_ent_p && !feof(passwdf_p));
        fclose(passwdf_p);
    }
}

const char* name_str_from_id(name_id_item* listroot_p, int id)
{
    struct name_id_item * item_p = listroot_p;
    while (item_p) {
        if (item_p->id == id) {
            //printf("Found! Id %d is '%s'\n", item_p->id, item_p->name); fflush(0);
            return (const char*)item_p->name;
        }
        //printf("    Id %d is '%s'\n", item_p->id, item_p->name); fflush(0);
        item_p = item_p->next_p;
    }

    // no name found, maybe an ID from mounted file system
    // so format static string using ID number and return its addr.
    static char name_str[MAXNINAME];
    sprintf(name_str, "%d", id);
    return name_str;
}

// Initial size and location hard-coded, 
// but later size and loc recorded in fluff.conf for recall
int MainXPos =  15;
int MainYPos =  40;
int MainWide = 624;
int MainHigh = 500;
int DirTreeWide = MainWide * 21 / 100;

//char StartPath_str[MAXFPATHLEN]    = "/";
char TrashbinPath_str[MAXFPATHLEN] = "/home/tc/.trash";
char Loc1Label_str[MAXALBLLEN]     = "Home";
char Loc1Path_str[MAXFPATHLEN]     = "/home/tc";
char Loc2Label_str[MAXALBLLEN]     = "TCE";
char Loc2Path_str[MAXFPATHLEN]     = "";

int ConfirmDelete = 0;
int ShowAllFiles = 0;
int AllowUseSudo = 1;
int SortOrder = SORTSPEC_NAME_ASC;

int ConfigSaved = 0;

void save_configuration(void)
{
    if (ConfigSaved) {
        return;
    }
    char configfilename[MAXFFULLPATHLEN];
    char* env_home_p = getenv("HOME");
    sprintf(configfilename, "%s/.config/fluff.conf", env_home_p);
    FILE* cfgf = NULL;
    
    cfgf = fopen(configfilename, "w");
    if (cfgf) {
        fprintf(cfgf, "# Fluff file manager configuration file\n");
        time_t now = time(NULL);
        fprintf(cfgf, "# Auto-written %s\n", ctime(&now));
        fprintf(cfgf, "# NOTE: if you hand-edit, please take care to spell\n");
        fprintf(cfgf, "#       and structure configuration statements carefully!\n");
        fprintf(cfgf, "\n");
        fflush(0);
            //printf("save_configuration(): MainWnd_p is 0x%08X, label %s, x is %d\n", 
            //        MainWnd_p, MainWnd_p->label(), MainWnd_p->x()); fflush(0);
        
        fprintf(cfgf, "# Main Window placement and size\n");
        fprintf(cfgf, "MainXPos = %d\n", MainWnd_p->x());     
        fprintf(cfgf, "MainYPos = %d\n", MainWnd_p->y());     
        fprintf(cfgf, "MainWide = %d\n", MainWnd_p->w());
        fprintf(cfgf, "MainHigh = %d\n", MainWnd_p->h());
        fprintf(cfgf, "\n");
        fflush(0);

        fprintf(cfgf, "# File List Window Column Widths, in pixels\n"); 
        fprintf(cfgf, "DirTreeWide = %d\n", DirTreeWide);
        fprintf(cfgf, "FileNameColWidth = %d\n", ColWidths[0]);
        fprintf(cfgf, "FileTypeColWidth = %d\n", ColWidths[1]);
        fprintf(cfgf, "FileSizeColWidth = %d\n", ColWidths[2]);
        fprintf(cfgf, "FileDateColWidth = %d\n", ColWidths[3]);
        fprintf(cfgf, "FilePermColWidth = %d\n", ColWidths[4]);
        fprintf(cfgf, "\n");
        fflush(0);

        fprintf(cfgf, "# Paths\n");
//        fprintf(cfgf, "StartPath = %s\n", StartPath_str);
        fprintf(cfgf, "TrashbinPath = %s\n", TrashbinPath_str);
        fprintf(cfgf, "Loc1Path = %s\n", Loc1Path_str);
        fprintf(cfgf, "Loc1Label = %s\n", Loc1Label_str);
        fprintf(cfgf, "Loc2Path = %s\n", Loc2Path_str);
        fprintf(cfgf, "Loc2Label = %s\n", Loc2Label_str);
        
        fprintf(cfgf, "\n");
        fflush(0);

        fprintf(cfgf, "# Options\n");
        fprintf(cfgf, "ConfirmDelete = %d\n", ConfirmDelete);
        fprintf(cfgf, "ShowAllFiles = %d\n", MainWnd_p->showingall);
        fprintf(cfgf, "AllowUseSudo = %d\n", AllowUseSudo);
        fprintf(cfgf, "FollowLinks = %d\n", FollowLinks);

        fprintf(cfgf, "SortOrder = %d\n", MainWnd_p->sortspec);
        fprintf(cfgf, "# Sorting: 0=name+asc, 1=name+desc, 2=type+asc, 3=type+desc\n");
        fprintf(cfgf, "#          4=size+asc, 5=size+desc, 6=date+asc, 7=date+desc\n");
        fprintf(cfgf, "\n");
        fflush(0);
        
        fprintf(cfgf, "Filetypes:\n");
        fprintf(cfgf, "# The following are built-in\n");
        fprintf(cfgf, "# executable, string, ELF, 1\n");
        fprintf(cfgf, "# shell script, string, #!/, 0\n");
        fprintf(cfgf, "# tcz, extension, .tcz, 0\n");
        fprintf(cfgf, "# jpg image, bytes, FF D8, 0\n");
        fprintf(cfgf, "# jpg image, string, JFIF, 6\n");
        fprintf(cfgf, "# jpg image, string, Exif, 6\n");
        fprintf(cfgf, "# png image, string, PNG, 1\n");
        fprintf(cfgf, "# gif image, string, GIF, 0\n");
        fprintf(cfgf, "# bmp image, string, BM, 0\n");
        fprintf(cfgf, "# xpm image, string, XPM, 3\n");

        fprintf(cfgf, "# User Defined...\n");
        fprintf(cfgf, "# (type label), (hint method), (pattern), (offset into content)\n");
        fflush(0);
        int ft = 0; // type FT_TEXT
        char hint_str[1024] = {0};
        for (ft = FIRST_CONFIGURED_FILETYPE; ft < Filetypes; ft++) {
            int hint = 0;
            char num[4];
            for (hint = 0; hint < FiletypeHints; hint++) {
                //printf("-- Hint pattern '%s', filetype '%s'\n", FiletypeHint[hint].pat, FiletypeName_str[FiletypeHint[hint].filetype]); fflush(0);
                if (FiletypeHint[hint].filetype == ft) {
                    strnzcpy(hint_str, FiletypeName_str[ft], 1024);
                    strcat(hint_str, ", ");
                    if (FiletypeHint[hint].hintmethod == HINTMETHOD_BYTES) {
                        strcat(hint_str, "bytes,");
                        for (int b = 0; FiletypeHint[hint].pat[b] != 0; b++) {
                            sprintf(num, " %02X", (unsigned char)FiletypeHint[hint].pat[b]);
                            strcat(hint_str, num);
                        }
                        strcat(hint_str, ", ");
                        sprintf(num, "%d", FiletypeHint[hint].offset);
                        strcat(hint_str, num);
                    }
                    else if (FiletypeHint[hint].hintmethod == HINTMETHOD_STRING) {
                        strcat(hint_str, "string, ");
                        strcat(hint_str, FiletypeHint[hint].pat);
                        strcat(hint_str, ", ");
                        sprintf(num, "%d", FiletypeHint[hint].offset);
                        strcat(hint_str, num);
                    }
                    else if (FiletypeHint[hint].hintmethod == HINTMETHOD_EXTENSION) {
                        strcat(hint_str, "extension, ");
                        strcat(hint_str, FiletypeHint[hint].pat);
                        strcat(hint_str, "");
                    }                
                    strcat(hint_str, "\n");
                    fprintf(cfgf, "%s", hint_str);
                } //end if
            } //end for (hint ...
            
        } //end for (ft ...
        
        fprintf(cfgf, "\n");
        fflush(0);
        
        fprintf(cfgf, "# Filetype Application Associations\n");
        fprintf(cfgf, "Associations:\n");
        fprintf(cfgf, "# (type label), (action label), (cmd spec)\n");
        int assoc = 0;

        for(assoc = 0 ; assoc < Associations; assoc++) {
            fprintf(cfgf, "%s, %s, %s\n", 
                    FiletypeName_str[Association[assoc].filetype], 
                    Association[assoc].action_label, 
                    Association[assoc].cmd_spec);
        }
        
		fclose(cfgf);
		ConfigSaved = 1;
    }
    else {
		fl_alert("Could not save Fluff configuration\n to %s", configfilename);		
	}
}

int new_assoc_num_for_filetype (int filetype)
{
    int assoc = 0;
    int skip = AssocCountPerFiletype[filetype];
    for (assoc = 0; assoc < Associations; assoc++) {
        if (Association[assoc].filetype == filetype) {
            if (skip > 0) {
                 skip--;
             }
             else {
                 return assoc;
             }
        }
    }
    
    if (assoc >= MAXASSOC) assoc = MAXASSOC - 1; // reuse last space if not enough
    return assoc;
}

int filetype_index_by_name(const char* name)
{
    int filetype;
    for (filetype = 0; filetype < Filetypes; filetype++) {
        if (!strcmp(FiletypeName_str[filetype], name)) {
            return filetype;
        }
    }
    return Filetypes;
}

void consider_filetype(char* confline) {
    char buf[256];
    char typelabel[256];
    int  hint;
    int  offset;
    int  filetype;
    char* p = confline;
    char* b = typelabel;
    
    if (confline[0] == '#') {
        //printf("Skipping conf. line %s", confline); fflush(0);
        return;
    }
    if (confline[strlen(confline) - 1] == '\n') {
        confline[strlen(confline) - 1] = '\0';  // trim off newline
    }
    
    while (*p && *p != ',') {
        *b++ = *p++;        
    }
    *b = '\0';
    if (!*p) return;
    //printf("File type named is '%s'...\n", typelabel); fflush(0);
    
    filetype = filetype_index_by_name(typelabel);
    if (filetype == Filetypes) {
        // It's a new one
        if (filetype < MAXFILETYPE - 1) {
            Filetypes++;
        }
        strnzcpy(FiletypeName_str[filetype], typelabel, MAXFTNAMELEN);
    }
    
    hint = FiletypeHints; // new_hint_num_for_filetype(filetype);
    FiletypeHint[hint].filetype = filetype;
    //printf("... File type hint num %d...\n", hint); fflush(0);
    
    if (*p) p++;    // skip comma
    while (*p == ' ' || *p == '\t') p++; // skip whitespace
    b = buf;
    while (*p && *p != ',') {
        *b++ = *p++;
        
    }
    *b = '\0';    
    //printf"... ... File type hint method '%s'...\n", buf); fflush(0);
    if (!strcmp(buf, "bytes")) {
        FiletypeHint[hint].hintmethod = HINTMETHOD_BYTES;
        
        if (*p) {
            p++;    // skip comma
        }
        else {
            printf("ERROR: filetype hint pattern not provided!\n");
            FiletypeHint[hint].filetype = -1;
            return;
        }
        while (*p == ' ' || *p == '\t') p++; // skip whitespace
        int c = 0;
        int n;
        bzero(FiletypeHint[hint].pat, 16);
        FiletypeHint[hint].patlen = 0;
        while (*p && *p != ',') {
            b = buf;
            while (*p && *p != ' ' && *p != ',') {
                *b++ = *p++;
            }
            *b = '\0';
            if (1 == sscanf(buf, "%x", &n)) {
                FiletypeHint[hint].pat[c] = (unsigned char)n;
                c++;
                FiletypeHint[hint].patlen++;
                while (*p == ' ' || *p == '\t') p++; // skip whitespace
            }
        }
        //printf"... ... ... byte pattern has %d bytes starting with 0x%02X...\n", c, (unsigned char)FiletypeHint[hint].pat[0]); fflush(0);
    }
    else {
        if (!strcmp(buf, "string")) {
            FiletypeHint[hint].hintmethod = HINTMETHOD_STRING;
        }
        else if (!strcmp(buf, "extension")) {
            FiletypeHint[hint].hintmethod = HINTMETHOD_EXTENSION;
        } else {
            printf("ERROR: filetype hint method '%s' not understood!\n", p);
            FiletypeHint[hint].filetype = -1;
        }

        if (*p) {
            p++;    // skip comma
        }
        else {
            printf("ERROR: filetype hint pattern not provided!\n");
            FiletypeHint[hint].filetype = -1;
            return;
        }
        while (*p == ' ' || *p == '\t') p++; // skip whitespace
        b = buf;
        while (*p && *p != ',') {
            *b++ = *p++;
        }
        *b = '\0';
        //printf"... ... ... File type hint pattern '%s'...\n", buf); fflush(0);
        strnzcpy(FiletypeHint[hint].pat, buf, MAXPATLEN);
        FiletypeHint[hint].patlen = strlen(FiletypeHint[hint].pat);
    }
        
    if (*p) {
        p++;    // skip comma
    }
    else if (FiletypeHint[hint].hintmethod != HINTMETHOD_EXTENSION) {
        printf("ERROR: filetype hint pattern offset not provided!\n");
        FiletypeHint[hint].filetype = -1;
        return;
    }
    //printf"... ... ... (scanning from '%s')\n", p); fflush(0);
    while (*p == ' ' || *p == '\t') p++; // skip whitespace
    b = buf;
    while (*p && *p != ',' && *p != ' ') {
        *b++ = *p++;
    }
    *b = '\0';
    if (1 == sscanf(buf, "%i", &offset)) {
       FiletypeHint[hint].offset = offset;
       //printf"... ... ... ... File type hint pattern offset %d...\n", offset); fflush(0);
    } 
    else if (FiletypeHint[hint].hintmethod != HINTMETHOD_EXTENSION) { 
        //printf"ERROR: filetype hint pattern offset not provided!\n"); fflush(0);
        FiletypeHint[hint].filetype = -1;
        return;
    }    

    HintCountPerFiletype[filetype]++;
    FiletypeHints++;
}

void consider_association(char* confline) {
    char buf[256];
    char typelabel[256];
    int  filetype;
    
    char* p = confline;
    char* b = typelabel;
    
    if (confline[0] == '#') {
        //printf("Skipping conf. line %s", confline); fflush(0);
        return;
    }
    if (confline[strlen(confline) - 1] == '\n') {
        confline[strlen(confline) - 1] = '\0';  // trim off newline
    }
    
    while (*p && *p != ',') {
        *b++ = *p++;        
    }
    *b = '\0';
    if (!*p) return;
    //printf("File type '%s'...\n", typelabel); fflush(0);
    
    // See if this type is already defined
    filetype = filetype_index_by_name(typelabel);
    if (filetype >= Filetypes) {
        //printf"ERROR: file type '%s' is not known!\n", typelabel); fflush(0);
        return;
    }

    int assoc = new_assoc_num_for_filetype(filetype);

    Association[assoc].filetype = filetype;
    //printf("... Assoc num %d...\n", assoc); fflush(0);
    
    if (*p) p++;    // skip comma
    while (*p == ' ' || *p == '\t') p++; // skip whitespace
    b = buf;
    while (*p && *p != ',' && (b - buf) < 32) {
        *b++ = *p++;        
    }
    *b = '\0';    
    //printf("... ... Action label is '%s'...\n", buf); fflush(0);
    strnzcpy(Association[assoc].action_label, buf, MAXALBLLEN);
    
    if (*p) p++;    // skip comma
    while (*p == ' ' || *p == '\t') p++; // skip whitespace
    b = buf;
    while (*p && *p != ',' && (b - buf) < 128) {
        *b++ = *p++;        
    }
    *b = '\0';    
    //printf("... ... ... Command spec is '%s'...\n", buf); fflush(0);
    strnzcpy(Association[assoc].cmd_spec, buf, MAXCMDSPECLEN);
       
    AssocCountPerFiletype[filetype]++;
    Associations++;
    
}
void load_configuration(void)
{
    char configfilename[MAXFNAMELEN];
    char confline[256];
    char strbuf[MAXFPATHLEN];
    char* sp = strbuf;
    char* env_home_p = getenv("HOME");
    sprintf(configfilename, "%s/.config/fluff.conf", env_home_p);
    FILE* cfgf = NULL;
    int n;
    
    bzero(HintCountPerFiletype, sizeof(HintCountPerFiletype));
    
    cfgf = fopen(configfilename, "r");
    if (cfgf) {
        while (fgets(confline, 255, cfgf)) {
            if (confline[0] == '#' || confline[0] == '\n') {
                continue;   // skip lines marked with # as comments
            }           
            else if (1 == sscanf(confline, "MainXPos = %d\n", &n)) {
                MainXPos = n;  continue;
            }
            else if (1 == sscanf(confline, "MainYPos = %d\n", &n)) {
                MainYPos = n;  continue;
            }
            else if (1 == sscanf(confline, "MainWide = %d\n", &n)) {
                MainWide = n;  continue;
            }
            else if (1 == sscanf(confline, "MainHigh = %d\n", &n)) {
                MainHigh = n;  continue;
            }
            else if (1 == sscanf(confline, "DirTreeWide = %d\n", &n)) {
                DirTreeWide = n;  continue;
            }
//            else if (1 == sscanf(confline, "StartPath = %s\n", sp)) {
//                strnzcpy(StartPath_str, sp, 256);
//                continue;
//            }
            else if (1 == sscanf(confline, "TrashbinPath = %s\n", sp)) {
                strnzcpy(TrashbinPath_str, sp, MAXFPATHLEN);  continue;
            }
            else if (1 == sscanf(confline, "Loc1Path = %s\n", sp)) {
                strnzcpy(Loc1Path_str, sp, MAXFPATHLEN);  continue;
            }
            else if (1 == sscanf(confline, "Loc1Label = %s\n", sp)) {
                strnzcpy(Loc1Label_str, sp, MAXALBLLEN);  continue;
            }
            else if (1 == sscanf(confline, "Loc2Path = %s\n", sp)) {
                strnzcpy(Loc2Path_str, sp, MAXFPATHLEN);  continue;
            }
            else if (1 == sscanf(confline, "Loc2Label = %s\n", sp)) {
                strnzcpy(Loc2Label_str, sp, MAXALBLLEN);  continue;
            }
            else if (1 == sscanf(confline, "ConfirmDelete = %d\n", &n)) {
                ConfirmDelete = n;  continue;
            }
            else if (1 == sscanf(confline, "ShowAllFiles = %d\n", &n)) {
                ShowAllFiles = n;  continue;
            }
            else if (1 == sscanf(confline, "AllowUseSudo = %d\n", &n)) {
                AllowUseSudo = n;  continue;
            }
            else if (1 == sscanf(confline, "SortOrder = %d\n", &n)) {
                SortOrder = n;  continue;
            }
            else if (1 == sscanf(confline, "FollowLinks = %d\n", &n)) {
                FollowLinks = n;  continue;
            }
            else if (1 == sscanf(confline, "FileNameColWidth = %d\n", &n)) {
                ColWidths[0] = n;  continue;
            }
            else if (1 == sscanf(confline, "FileTypeColWidth = %d\n", &n)) {
                ColWidths[1] = n;  continue;
            }
            else if (1 == sscanf(confline, "FileSizeColWidth = %d\n", &n)) {
                ColWidths[2] = n;  continue;
            }
            else if (1 == sscanf(confline, "FileDateColWidth = %d\n", &n)) {
                ColWidths[3] = n;  continue;
            }
            else if (1 == sscanf(confline, "FilePermColWidth = %d\n", &n)) {
                ColWidths[4] = n;  continue;
            }
            else if (!strncmp(confline, "Filetypes:", strlen("Filetypes:"))) {
                //printf("Configuring file types now...\n"); fflush(0);
                
                while (fgets(confline, 255, cfgf) && !strstr(confline, "Associations:")) {
                    consider_filetype(confline);
                }
                
                if (!strncmp(confline, "Associations:", strlen("Associations:"))) {
                    //printf("Configuring file associations now...\n"); fflush(0);
                    Associations = 0;   // Wipe out pre-config associations
                
                    while (fgets(confline, 255, cfgf)) {
                        consider_association(confline);
                    }
                }
            }
            else {
                printf("Don't know how to process '%s'...\n", confline); fflush(0);
            }
        } // while there is more conf. file            
    } // end if (cfgf)
}

// Class Implementations ================================================
int file_item::is_reg(void)     { return S_ISREG(status.st_mode);}
int file_item::is_dir(void)     { return S_ISDIR(status.st_mode);}
int file_item::is_link(void)    { return S_ISLNK(lstatus.st_mode);}
int file_item::is_chardev(void) { return S_ISCHR(status.st_mode);}
int file_item::is_blockdev(void){ return S_ISBLK(status.st_mode);}
int file_item::is_fifo(void)    { return S_ISFIFO(status.st_mode);}
int file_item::is_socket(void)  { return S_ISSOCK(status.st_mode);}

file_item::file_item() :
    filetype(FT_TEXT), 
    level(0), expanded(0), children(-1), subdirs(0), 
    parent_fi_p(NULL), child_fi_p(NULL),
    next_fi_p(NULL) 
{
    strclr(path);
    strclr(name);
    strclr(fullpath);
    strclr(typestr);
    strclr(sizestr);
    strclr(permstr);
    strclr(ownstr);
    strclr(datestr);
    memset(&status, 0, sizeof(struct stat64));
};

const char* file_item::extension(void)
{
    if (!*name) return NULL;
    
    char* c = name + strlen(name) - 1;
    int l = 1;
    while (*c && c > name && *c != '.') {
        c--;
        l++;
    }
    if (c > name) {
        return (const char*)c;
    }
    else {
        return NULL;
    }
}

char* file_item::get_fullpath(void) {
    sprintf(fullpath, "%s%s", path, name);
    return fullpath;
}

int file_item::guess_filetype(void) {
    filetype = FT_TEXT;
    
    if (is_dir()) {
        filetype = FT_DIR;
    }
    else if (is_chardev()) {
        filetype = FT_CHARDEV;
    }
    else if (is_blockdev()) {
        filetype = FT_BLOCKDEV;
    }
    else if (is_fifo()) {
        filetype = FT_FIFO;
    }
    else if (is_socket()) {
        filetype = FT_SOCKET;
    }
    else if (is_reg()) {
        //printf"Checking type of regular file '%s'...", name);
        filetype = FT_TEXT;
        int imfd = open(fullpath, O_RDONLY);
        if (-1 == imfd) {
            return filetype;
        }
        
        char imbyte[32];
        if (-1 == read(imfd, imbyte, 32)) {
            close(imfd);
            return filetype;
        }
        //printf"There are %d filetype hints.\n", FiletypeHints);
        for (int hint = 1; hint < FiletypeHints; hint++) {
            struct filetype_hint* fth_p = & FiletypeHint[hint];
            int patlen = strlen(fth_p->pat);
            if (fth_p->hintmethod == HINTMETHOD_BYTES || fth_p->hintmethod == HINTMETHOD_STRING ) { 
                //printf"checking file for pattern '%s' at offset %d...\n",
                //        fth_p->pat, fth_p->offset); fflush(0);
                if(!strncmp(imbyte+(fth_p->offset), fth_p->pat, patlen)) {
                    filetype = fth_p->filetype;
                    //printf"  yes, it's a '%s'\n", FiletypeName_str[filetype]); fflush(0);
                    break;
                }
            }
            else if (fth_p->hintmethod == HINTMETHOD_EXTENSION) {
                //printf"checking extension '%s' for pattern '%s'...", name + strlen(name) + 1 - patlen, fth_p->pat);
                if (   fth_p->hintmethod == HINTMETHOD_EXTENSION 
                     && !strncmp(name + strlen(name) - patlen, fth_p->pat, patlen)) 
                {
                    filetype = fth_p->filetype;
                    //printf"  yes, it's a '%s'\n", FiletypeName_str[filetype]);
                    break;
                }
            }
        }
        close(imfd);        
        //printf"  generic file\n");
    }

    return filetype;
}

void bytecount_size_str(char* sizestr, long long bytes)
{
    if (bytes < (1536)) {
        sprintf(sizestr, "%lld bytes", bytes);
    }
    else if (bytes < (1024 * 1024)) {  
        sprintf(sizestr, "%1.2f KB", (float)bytes / 1024.0); // really, KiB!
    }
    else if (bytes < (1024 * 1024 * 1024)) {
        sprintf(sizestr, "%1.2f MB", 
                (float)bytes / (1024.0*1024.0)); // really, MiB!
    }
    else {
        sprintf(sizestr, "%1.2f GB", 
                (float)bytes / (1024.0*1024.0*1024.0)); // really, GiB!
    }
    
}

int file_item::fetch_status(void) {
    if (!*fullpath) {
        get_fullpath();
    }
    int ret = lstat64(fullpath, &lstatus);
    if (ret) {
#ifdef DIAG
        printf("Error attempting lstat64 of '%s'\n", fullpath);
        perror("lstat64");
#endif        
        return 0;
    }
    
    ret = stat64(fullpath, &status);
    if (ret) {
#ifdef DIAG
        printf("Error attempting stat64 of '%s'\n", fullpath);
        perror("stat64");
#endif
        
        return 0;
    }

    // Update display strings
    guess_filetype();
    sprintf(typestr, "%4s%s", 
            is_link() ? "-->" : "",
            FiletypeName_str[filetype]);
    
    bytecount_size_str(sizestr, (long long)status.st_size);
    
    struct tm mod_tm;
    memcpy(&mod_tm, localtime(&status.st_mtime), sizeof(mod_tm));
    sprintf(datestr, "%04d-%02d-%02d %02d:%02d",
            mod_tm.tm_year+1900, mod_tm.tm_mon+1, mod_tm.tm_mday,
            mod_tm.tm_hour, mod_tm.tm_min);

    //strclr(permstr);
    if (is_dir()) {
        permstr[0] = 'd';
    }
    else if (is_link()) {
        permstr[0] = 'l';
    }
    else {
        permstr[0] = '-';
    }
    permstr[1] = (status.st_mode & S_IRUSR) ? 'r' : '-';
    permstr[2] = (status.st_mode & S_IWUSR) ? 'w' : '-';
    permstr[3] = (status.st_mode & S_IXUSR) ? 'x' : '-';

    permstr[4] = (status.st_mode & S_IRGRP) ? 'r' : '-';
    permstr[5] = (status.st_mode & S_IWGRP) ? 'w' : '-';
    permstr[6] = (status.st_mode & S_IXGRP) ? 'x' : '-';

    permstr[7] = (status.st_mode & S_IROTH) ? 'r' : '-';
    permstr[8] = (status.st_mode & S_IWOTH) ? 'w' : '-';
    permstr[9] = (status.st_mode & S_IXOTH) ? 'x' : '-';
    permstr[10] = '\0';
    sprintf(ownstr, "%s:%s", 
            name_str_from_id(OwnerList_p, status.st_uid),
            name_str_from_id(GroupList_p, status.st_gid));
    
    return 1;
}

file_item::file_item(char* thepath, char* thename) :
    filetype(FT_TEXT), 
    level(0), expanded(0), children(-1), subdirs(0), 
    parent_fi_p(NULL), child_fi_p(NULL),
    next_fi_p(NULL)
{
    strnzcpy(path, thepath, MAXFPATHLEN);
    strnzcpy(name, thename, MAXFNAMELEN);
    strclr(fullpath);
    strclr(typestr);
    strclr(sizestr);
    strclr(permstr);
    strclr(ownstr);
    strclr(datestr);
    memset(&lstatus, 0, sizeof(struct stat64));
    memset(&status,  0, sizeof(struct stat64));
    //printf(" ( constructing fi for %s%s)...", thepath, thename);
    fetch_status();
};

void Fl_DND_Box::callback_deferred(void *v)
{
    Fl_DND_Box *w = (Fl_DND_Box*)v;

    w->do_callback();
}

Fl_DND_Box::Fl_DND_Box(int X, int Y, int W, int H, const char *L)
        : Fl_Box(X,Y,W,H,L), evt(FL_NO_EVENT), evt_txt(0), evt_len(0)
{
    labeltype(FL_NO_LABEL);
    box(FL_NO_BOX);
    clear_visible_focus();
}

Fl_DND_Box::~Fl_DND_Box()
{
    delete [] evt_txt;
}

int Fl_DND_Box::event()
{
    return evt;
}

const char* Fl_DND_Box::event_text()
{
    return evt_txt;
}

int Fl_DND_Box::event_length()
{
    return evt_len;
}


int Fl_DND_Box::handle(int e)
{
    //printf("Fl_DND_Box event %d\n", e); fflush(0);
    switch(e)
    {

        /* Receiving Drag and Drop */
        case FL_DND_ENTER:
        case FL_DND_RELEASE:
        case FL_DND_LEAVE:
            evt = e;
            return 1;
            
        case FL_DND_DRAG:
            evt = e;
            //printf("DND Drag event... mouse y =%d\n", Fl::event_y());
            do_callback();
            return 1;

        case FL_PASTE:
            evt = e;

            // make a copy of the DND payload
            evt_len = Fl::event_length() + 8;

            delete [] evt_txt;

            evt_txt = new char[evt_len];
            strnzcpy(evt_txt, Fl::event_text(), evt_len);

            // If there is a callback registered, call it.
            // The callback must access Fl::event_text() to
            // get the string or file path(s) that was dropped.
            // Note that do_callback() is not called directly.
            // Instead it will be executed by the FLTK main-loop
            // once we have finished handling the DND event.
            // This allows caller to popup a window or change widget focus.
            if(callback() && ((when() & FL_WHEN_RELEASE) || (when() & FL_WHEN_CHANGED)))
                Fl::add_timeout(0.0, Fl_DND_Box::callback_deferred, (void*)this);
            return 1;
    }

    return Fl_Box::handle(e);
}

Dir_Tree_Browser::Dir_Tree_Browser(int x, int y, int w, int h) :
    Fl_Select_Browser(x, y, w, h), drag_targ_itemnum(0), list_item_height(16)  {
    };
    
int Dir_Tree_Browser::item_number_under_mouse(int mouse_y) {
    int target_y = vposition() + mouse_y;
    return 1 + (target_y / list_item_height);
}

int Dir_Tree_Browser::visible_lines(void) {
    void* bi_p = item_first(); // browser item ptr.
    int line_h = item_height(bi_p);
    int x, y, w, h;
    bbox(x, y, w, h);
    return h/line_h;
}

int Dir_Tree_Browser::top_visible_line(void) {
    void* bi_p = item_first(); // browser item ptr.
    int line_h = item_height(bi_p);
    int x, y, w, h;
    bbox(x, y, w, h);
    return 1 + (vposition()/line_h);
}

file_item* Dir_Tree_Browser::file_item_under_mouse(int mouse_y) {
    int item = item_number_under_mouse(mouse_y);
    if (item > 0) {
        return (file_item*)data(item);
    }
    else {
        return NULL;
    }
}
void* Dir_Tree_Browser::item_first(void) const {
    return Fl_Select_Browser::item_first();
};

int Dir_Tree_Browser::handle(int e)
{
    int ret = 0;
    //printf("Dir_Tree_Browser event %d, last click %d\n", e, MainWnd_p->last_click_browser); fflush(0);

    switch ( e ) {
        case FL_KEYDOWN:
            return handle_global_keys(e);
            break;
            
        default:
            ret = Fl_Select_Browser::handle(e);
            break;

    }
    return(ret);
}

void Dir_Tree_Browser::populate(void) 
{
    file_item* fi_p_stack[16];
    int si = 0;
    int targetline = 0;
    fi_p_stack[++si] = Root_fi_p;
    char dispname_str[1024];
    static int need_item_height = 1;

    clear();
    //printf("Populating tree window\n"); fflush(0);
    while (si) {
        sprintf(dispname_str, "%s@.", 
                (fi_p_stack[si]->name[0] == '.') 
                    ? HideColor_str : TextColor_str
                );
        if (si > 1 && fi_p_stack[si]->subdirs > 1) {
            if (fi_p_stack[si]->expanded) {
                strcat(dispname_str, "-");
            }
            else {
                strcat(dispname_str, "+");
            }
        }
        else {
            strcat(dispname_str, " ");
        }
        int i;
        //printf("item's nesting level is %d\n",fi_p_stack[si]->level); 
        for (i = 0; i < fi_p_stack[si]->level; i++) {
            strcat(dispname_str, "    ");
        }
        strcat(dispname_str, fi_p_stack[si]->name);
        
        if (fi_p_stack[si]->is_dir() || fi_p_stack[si] == Root_fi_p) {
            strcat(dispname_str, "/");
            if (    fi_p_stack[si]->expanded
                ||  fi_p_stack[si] == MainWnd_p->seldir_fi_p) {
                setup_dir_watch(fi_p_stack[si]);
            }
        }
 
        //printf("Name is %s\n", fi_p_stack[si]->name);
        if (    strcmp(fi_p_stack[si]->name, ".") 
            &&  strcmp(fi_p_stack[si]->name, "..")
            &&  fi_p_stack[si]->is_dir()
            &&  (MainWnd_p->showingall || (fi_p_stack[si]->name[0] != '.'))
           ) 
        {
            //printf("Adding item string '%s'\n", dispname_str); fflush(0);
            add(dispname_str, (void*)fi_p_stack[si]);
            if (    MainWnd_p->seldir_fi_p 
                &&  (MainWnd_p->seldir_fi_p == fi_p_stack[si])) 
            {
                select(size());
                targetline = size();
            }
            if (need_item_height) {
                void* bi_p = item_first(); // browser item ptr.
                if (bi_p)  list_item_height = item_height(bi_p);
                need_item_height = 0;
            }
        }
        
        //printf("    Child node is 0x%08X, sibling is 0x%08X\n", fi_p_stack[si]->child_fi_p, fi_p_stack[si]->next_fi_p);
        int found = 0;
        int unnesting = 0;
        while (si && !found) {
            //printf("Looking for next item to display, si is %d\n", si);
            if (!unnesting && fi_p_stack[si]->child_fi_p && fi_p_stack[si]->expanded) {
                //printf("Found child item '%s'\n", fi_p_stack[si]->child_fi_p->name);
                fi_p_stack[si+1] = fi_p_stack[si]->child_fi_p;
                si++;
                found = 1;
            }
            else if (fi_p_stack[si]->next_fi_p) {
                //printf("Found sibling item '%s'\n", fi_p_stack[si]->next_fi_p->name);
                fi_p_stack[si] = fi_p_stack[si]->next_fi_p;
                found = 1;
            }
            else {
                si--;
                unnesting = 1;
            }
            //getchar();
        }
    }

    // Now perhaps scroll the browser list to better show the wanted item
    int vis_lines = visible_lines();
    //printf("There are %d total lines, %d visible lines, and line %d is now top\n", 
    //       size(), vis_lines, firstline);
    int newtopline = targetline - (vis_lines/4);
    if (newtopline < 1)  newtopline = 1;
    // Maybe make this an optional behavior
    topline(newtopline);
    //printf("Want to position line %d to be visible, setting top to %d\n",
//            targetline, newtopline);
}


File_Detail_List_Browser::File_Detail_List_Browser(int x, int y, int w, int h) :
    Fl_Multi_Browser(x, y, w, h), dragging(0), partitioning(0) //, list_item_height(16) 
{
}

int File_Detail_List_Browser::num_selected(void) {
    int num = 0;
    int i;
    for (i = 1; i <= size(); i++) {
        if (selected(i)) {
            num++;
        }
    }
    return num;
}

long long File_Detail_List_Browser::size_selected(void) {
    long long totsize = 0;
    int i;
    file_item* fi_p;
    //printf("size_selected(): %d items in list\n", size()); fflush(0);
    for (i = 1; i <= size(); i++) {
        //printf("item %d, total items %d\n", i, size()); fflush(0);
        if (selected(i)) {
            fi_p = (file_item*)data(i);
            if (fi_p) {
                totsize += fi_p->status.st_size;
            }
        }
    }
    return totsize;
}

void File_Detail_List_Browser::populate(file_item* dir_fi_p) 
{
    //~ printf("File_Detail_List_Browser::populate()   ------------------ \n");
    //~ clear();
    //~ add("(dummy)", NULL);    
    //~ printf("Text size %d, line spacing %d, item height %d\n", 
			//~ textsize(), linespacing(), item_height(item_first()));  fflush(0);
    char fi_str[1024];
    int rc = chdir(dir_fi_p->fullpath);
    if (rc == -1) {
        perror(dir_fi_p->fullpath);
    }
    else {
        //printf("Changed working directory to '%s'\n", dir_fi_p->fullpath);
    }
    file_item* file_p = NULL; // dir_fi_p->child_fi_p;
    //Fl_Multi_Browser* p = MainWnd_p->list_p;
    clear();

    get_child_file_items(dir_fi_p);
    file_p = dir_fi_p->child_fi_p;
    Fl_Browser* br_p = new Fl_Browser(0,0,1,1);
    br_p->clear();
    br_p->hide();
    while (file_p) {
        //printf("Sorting: trying to add/insert %s\n", file_p->name);
        if (!br_p->size()) {
            br_p->add(file_p->name, (void*)file_p);
            //printf("      Sorting: added '%s' in first line\n", file_p->name);
        }
        else {
            int s = br_p->size();
            for (int i = 1; i <= s; i++) {
                file_item* list_fi_p = (file_item*)br_p->data(i);
                //printf("   Sorting: at line %d, existing item is %s\n", i, br_p->text(i));
                if (    new_item_sorts_before_existing_item(
                            file_p, list_fi_p, MainWnd_p->sortspec)
                    && strcmp(file_p->name, list_fi_p->name) ) {
                    br_p->insert(i, file_p->name, (void*)file_p);
                    //printf("      Sorting: inserted '%s'before '%s', ino1 %d, ino2 %d\n", 
                    //        file_p->name, list_fi_p->name, file_p->status.st_ino, list_fi_p->status.st_ino);
                    break;
                } 
            }
            if (s == br_p->size()) {
                // same item count, so item not inserted yet... add to end
                br_p->add(file_p->name, (void*)file_p);
                //printf("      Sorting: added '%s' at end\n", file_p->name);
            }
        }
        file_p = file_p->next_fi_p;
    }
  /*printf("OK, here is the sorted file list...\n");
    for (int i = 1; i <= br_p->size(); i++) {
        file_item* fp = (file_item*)br_p->data(i);
        printf("%30s, %20s, %14s, %20s\n", 
                fp->name, fp->typestr, fp->sizestr, fp->datestr);
    } 
*/    
    //printf("Filling the dir list with %d children of '%s'\n", 
    //        dir_fi_p->children, dir_fi_p->fullpath);
    

    int line = 1;
    const char* bg_str = "";
    sprintf(fi_str, "@b@c%s%s@.filename\t"
                    "@b@c%s%s@.type\t"
                    "@b@c%s%s@.size\t"
                    "@b@c%s%s@.date\t"
                    "@b@c%s%s@.perm.\t"
                    "@b@c%s%s@.owner", 
                    BtnColor_str, TextColor_str, 
                    BtnColor_str, TextColor_str, 
                    BtnColor_str, TextColor_str, 
                    BtnColor_str, TextColor_str, 
                    BtnColor_str, TextColor_str, 
                    BtnColor_str, TextColor_str 
                    );
                    
    add(fi_str, (void*)0);
    int i = 1;
    while(i <= br_p->size()) {
        file_p = (file_item*)br_p->data(i++);
        if (    MainWnd_p->showingall 
            ||  file_p->name[0] != '.'
            ||  !strcmp("..", file_p->name) ) 
        {
            bg_str = (line % 2 == 0) ? BgColor_str : Bg2Color_str;
            //printf("HideColor_str is %s\n", HideColor_str); fflush(0);
  
            sprintf(fi_str, "%s%s%s@.%s%s\t%s@.%s\t%s@.%s\t%s@.%s\t%s@.%s\t%s@.%s", 
                    file_p->is_dir() ? "@b" : "",
                    (file_p->name[0] == '.') ? HideColor_str : TextColor_str,
//                    (file_p->name[0] == '.') ? "@C156" : TextColor_str,
                    bg_str, 
                    file_p->name,
                    file_p->is_dir() ? " /" : "",
                     
                    bg_str, file_p->typestr, 
                    bg_str, file_p->sizestr,
                    bg_str, file_p->datestr, 
                    bg_str, file_p->permstr, 
                    bg_str, file_p->ownstr
                    );
            add(fi_str, (void*)file_p); //printf("browser: added line '%s'\n", fi_str); fflush(0);
            line++;
        }
    }
    MainWnd_p->update_btnbar();
    delete br_p;    
}

#define abs(x) (((x) < 0) ? (-1*(x)):(x))

void try_assoc_app(char* cmd_spec, char* name_str)
{
    if (!cmd_spec || !*cmd_spec || !name_str || !*name_str)  return;
    char* cmdstr     = (char*)malloc(strlen(cmd_spec) + strlen(name_str) + 32);
    char* fullcmdstr = (char*)malloc(strlen(cmd_spec) + strlen(name_str) + 32);
    if (cmdstr && fullcmdstr) {
        sprintf(cmdstr, cmd_spec, name_str);
        sprintf(fullcmdstr, "%s%s", UseSudo ? "sudo " : "", cmdstr);
        //printf("trying cmd: %s\n", fullcmdstr); fflush(0);
        int err = system(fullcmdstr);
        if (err) {
            int cmdlen = strlen(fullcmdstr);
            fl_alert("Fluff received error code %d from command%s\n"
                     "   %s", err, 
                     (cmdlen < 60) ? ":" : "", 
                     (cmdlen < 60) ? fullcmdstr : "");
        }
    }
    else {
        perror("memory allocation");
    }
    free(fullcmdstr);
    free(cmdstr);
}

void OWW__control_cb(Fl_Widget* thewidget_p, long ctrl_id)
{
    Open_With_Window* p = OpenWithWnd_p;
    switch(ctrl_id) {
        case TREAT_GENERIC_CHK_CTRL:
            p->update();
            break;
        case OPEN_WITH_BTN_CTRL: {
            association* assoc_p = (association*)p->cmd_list_p->data(p->cmd_list_p->value());
            if (!assoc_p) return;
            
            //printf("User selected command '%s' as the open-with association\n", 
            //       assoc_p->cmd_spec);
            
            p->hide();
            char name_str[256];
            sprintf(name_str, "\"%s\"", MainWnd_p->sel_fi_p->name);
            try_assoc_app(assoc_p->cmd_spec, name_str);
            }
            break;
        case MORE_BTN_CTRL: {
            ManageAssocWnd_p->show( p->effective_filetype, 
                                    FiletypeName_str[p->effective_filetype],
                                    &Associations, Association);
            }
            break;
        case CANCEL_BTN_CTRL:
            p->hide();
            break;
        default:
            break;
    }
}

Open_With_Window::Open_With_Window(int x, int y, int w, int h)
    :   Fl_Window(x, y, w, h, "Open With...")
{
    filetype_lbl_p = NULL; 
    cmd_list_p = NULL; 
    open_with_btn_p = NULL; 
    manage_assoc_btn_p = NULL;
    cancel_btn_p = NULL;
}

Open_With_Window::~Open_With_Window()
{
}


void Open_With_Window::setup(void)
{
    int x = 20, y = 12;
    int h = 24, w = Open_With_Window::w() - (2 * x);
    int bw = (w - 2*x)/3, vs = h + 8;
    begin();
    filetype_lbl_p = new Fl_Box(x, y, w, h); 
    y += vs; 
    treat_generic_chk_p = new Fl_Check_Button(x, y, 3*bw, h, "Open with option for generic file"); 
    treat_generic_chk_p->callback((Fl_Callback1*)OWW__control_cb, TREAT_GENERIC_CHK_CTRL);
    y += vs;
    cmd_list_p = new Fl_Hold_Browser( x, y, w, 6 * h);
    y += (6 * h) + h; 
    open_with_btn_p = new Fl_Button(x, y, bw, h, "Open");
    open_with_btn_p->callback((Fl_Callback1*)OWW__control_cb, OPEN_WITH_BTN_CTRL);
    manage_assoc_btn_p = new Fl_Button(x + bw + x + bw + x, y, bw, h, "More...");
    manage_assoc_btn_p->callback((Fl_Callback1*)OWW__control_cb, MORE_BTN_CTRL);
    manage_assoc_btn_p->tooltip("Add/manage file associations"); 
    cancel_btn_p = new Fl_Button(x + bw + x, y, bw, h, "Close"); 
    cancel_btn_p->callback((Fl_Callback1*)OWW__control_cb, CANCEL_BTN_CTRL);
    end();
}

void Open_With_Window::update(void)
{
    char buf[512];
    //printf("Selected item is at addr 0x%08X\n", MainWnd_p->sel_fi_p);
    if (MainWnd_p->sel_fi_p) {
        if (treat_generic_chk_p->value() == 1) {
            effective_filetype = FT_TEXT;
            sprintf(buf, "Associated program for generic file");
        }
        else {
            effective_filetype = MainWnd_p->sel_fi_p->filetype;
            sprintf(buf, "Associated program for file type %s", 
                    FiletypeName_str[effective_filetype]);
        }
        filetype_lbl_p->copy_label(buf);
        int i;
        cmd_list_p->clear();
        //printf("Looking for associations for filetype %d\n", MainWnd_p->sel_fi_p->filetype); fflush(0);
        for (i = 0; i < Associations; i++) {
            if (Association[i].filetype == effective_filetype) {
                sprintf(buf, "%s:  %s", Association[i].action_label, Association[i].cmd_spec);
                cmd_list_p->add(buf);
                cmd_list_p->data(cmd_list_p->size(), (void*)&Association[i]);
                //printf("Added assoc: %s\n", Association[i].cmd_spec); fflush(0);
            }
        }
        if (cmd_list_p->size()) {
            //printf("Selecting first assoc\n"); fflush(0);
            cmd_list_p->value(1);
        }
    }
    //printf("Done with 'open with' box update\n"); fflush(0);
}


Manage_Associations_Window::Manage_Associations_Window(int x, int y, int w, int h)
    :   Fl_Window(x, y, w, h, "Manage Associations")
{
    assoc_count_ptr = NULL;
    sel_assoc = -1;
    filetype_lbl_p = NULL; 
    assocs = 0;
    cmd_list_p = NULL; 
    raise_precedence_btn_p = NULL; 
    lower_precedence_btn_p = NULL;
    precedence_lbl_p = NULL;
    action_label_inp_p = NULL;
    cmd_inp_p = NULL;
    undo_changes_btn_p = NULL;
    addnew_btn_p = NULL;
    close_btn_p = NULL;
    remove_btn_p = NULL;
}

Manage_Associations_Window::~Manage_Associations_Window()
{
}

void Manage_Associations_Window::enable_editing(void)
{
    action_label_inp_p->activate();
    cmd_inp_p->activate();
}

void Manage_Associations_Window::disable_editing(void)
{
    action_label_inp_p->value("");
    cmd_inp_p->value("");
    action_label_inp_p->deactivate();
    cmd_inp_p->deactivate();
}

void Manage_Associations_Window::show(int thefiletype, 
                                 char* the_filetype_name_str, 
                                 int*  the_assoc_count_ptr, 
                                 association* the_assoc_array)
{
    filetype = thefiletype;
    strnzcpy(filetype_name_str, the_filetype_name_str, MAXFTNAMELEN);

    assoc_count_ptr = the_assoc_count_ptr;  // copy ptr
    assocs = *assoc_count_ptr;              // local copy of count
    assoc_array = the_assoc_array;      // copy pointer to caller's copy
    memcpy(assoc, assoc_array, sizeof(Association)); // make local copy
    sel_assoc = -1;
    update();
    Fl_Window::show();    
}

void Manage_Associations_Window::hide()
{
     // update caller's copies of the association array and count
    memcpy(assoc_array, assoc, sizeof(Association));  
    *assoc_count_ptr = assocs;  
    MainWnd_p->update_btnbar();
    Fl_Window::hide();
}

void swap_associations(association* a_p, association* b_p) {
    association assoc_buf;
    
    //printf("Swap assoc entry... a is '%s', b is '%s', %d associations\n",
    //        a_p->cmd_spec, b_p->cmd_spec, p->assocs);
    // copy a's info
    assoc_buf.filetype = a_p->filetype;
    strnzcpy(assoc_buf.action_label, a_p->action_label, MAXALBLLEN);
    strnzcpy(assoc_buf.cmd_spec, a_p->cmd_spec, MAXCMDSPECLEN);
    
    // move b's info to a's slot
    a_p->filetype = b_p->filetype;
    strnzcpy(a_p->action_label, b_p->action_label, MAXALBLLEN);
    strnzcpy(a_p->cmd_spec, b_p->cmd_spec, MAXCMDSPECLEN);
    
    // copy a's info into b's slot
    b_p->filetype = assoc_buf.filetype;
    strnzcpy(b_p->action_label, assoc_buf.action_label, MAXALBLLEN);
    strnzcpy(b_p->cmd_spec, assoc_buf.cmd_spec, MAXCMDSPECLEN);
    //printf("Swap assoc exit... a is '%s', b is '%s', %d associations\n",
    //        a_p->cmd_spec, b_p->cmd_spec, p->assocs); fflush(0);
    
}

#define LIST_MAW_CTRL       1
#define RAISE_BTN_MAW_CTRL  2
#define LOWER_BTN_MAW_CTRL  3
#define ALABEL_INP_MAW_CTRL 4
#define CMD_INP_MAW_CTRL    5
#define ADD_BTN_MAW_CTRL    6
#define UNDO_BTN_MAW_CTRL   7
#define REMOVE_BTN_MAW_CTRL 8
#define CLOSE_BTN_MAW_CTRL  9

void MAW__control_cb(Fl_Widget* thewidget_p, long ctrl_id)
{
    Manage_Associations_Window* p = ManageAssocWnd_p;
    Fl_Hold_Browser* list_p = p->cmd_list_p;
    int line = list_p->value();
    long sel  = (long)list_p->data(line); // selected association
    long other; 
    int lines = list_p->size();

    switch(ctrl_id) {
    case LIST_MAW_CTRL: {
        p->sel_assoc = sel;
        p->action_label_inp_p->value(p->assoc[sel].action_label);
        p->cmd_inp_p->value(p->assoc[sel].cmd_spec);
        p->enable_editing();
        }
        break;
        
    case RAISE_BTN_MAW_CTRL: {
        if (line > 1) {
            other = (long)list_p->data(line - 1);
            p->swap_assoc_order(sel, other);
        }    
        }
        break;

    case LOWER_BTN_MAW_CTRL: {
        if (line < lines) {
            other = (long)list_p->data(line + 1);
            p->swap_assoc_order(sel, other);
        }    
        }
        break;
        
    case ALABEL_INP_MAW_CTRL: 
    case CMD_INP_MAW_CTRL:   {
        strnzcpy( p->assoc[sel].action_label, p->action_label_inp_p->value(), MAXALBLLEN);
        strnzcpy( p->assoc[sel].cmd_spec, p->cmd_inp_p->value(), MAXCMDSPECLEN);
        p->update();
        }
        break;

    case ADD_BTN_MAW_CTRL: {
        //printf("Adding association #%d\n", assocs); fflush(0);
        int newassoc = p->assocs;
        p->assoc[newassoc].filetype = p->filetype;
        strnzcpy(p->assoc[newassoc].action_label, "Action", MAXALBLLEN);
        strnzcpy(p->assoc[newassoc].cmd_spec, "progname %s &", MAXCMDSPECLEN);
        p->sel_assoc = p->assocs;
        (p->assocs)++;
        p->update();
        lines = p->cmd_list_p->size();
        p->cmd_list_p->value(lines);    // select last item, the new one
        MAW__control_cb(thewidget_p, LIST_MAW_CTRL);
        }
        break;

    case UNDO_BTN_MAW_CTRL: {        
        p->assocs = *(p->assoc_count_ptr);                    // local copy of count
        memcpy(p->assoc, p->assoc_array, sizeof(Association)); // make local copy
        p->sel_assoc = -1;
        p->update();
        }
        break;

    case REMOVE_BTN_MAW_CTRL: {
        association* last_assoc_p = &p->assoc[p->assocs - 1];
        association* a_p = &p->assoc[sel];
        association* b_p = a_p + 1;
        while (b_p <= last_assoc_p) {
            swap_associations(a_p, b_p);
            a_p = b_p;
            b_p++;
        }
        p->assocs--;
        p->sel_assoc = -1;
        ManageAssocWnd_p->update();
        }
        break;

    case CLOSE_BTN_MAW_CTRL: {
        p->hide();
        OpenWithWnd_p->update();
        }
        break;
    }
}

void Manage_Associations_Window::setup(void)
{
    int x = 20, y = 12;
    int h = 24, w = 300, bw = 40, fw = w + bw + 2*h, vs = h + 8;
    begin();
    filetype_lbl_p = new Fl_Box(x, y, fw, h);
    y += vs; 
    cmd_list_p = new Fl_Hold_Browser( x, y, w, 6 * h);
    cmd_list_p->callback((Fl_Callback1*)MAW__control_cb, LIST_MAW_CTRL);
    raise_precedence_btn_p = new Fl_Button(x + w + 8, y, bw, h, "@+38>");
    raise_precedence_btn_p->callback((Fl_Callback1*)MAW__control_cb, RAISE_BTN_MAW_CTRL);
    y += vs; 
    precedence_lbl_p = new Fl_Box(x + w + 4, y - 8, 100, bw, "Precedence"); ;
    y += vs; 
    lower_precedence_btn_p = new Fl_Button(x + w + 8, y, bw, h, "@+32>");
    lower_precedence_btn_p->callback((Fl_Callback1*)MAW__control_cb, LOWER_BTN_MAW_CTRL);
    y += vs; 
    addnew_btn_p = new Fl_Button(x + w + 8, y, 2*bw, h, "Add new");
    addnew_btn_p->callback((Fl_Callback1*)MAW__control_cb, ADD_BTN_MAW_CTRL);
    y += vs; 
    remove_btn_p = new Fl_Button(x + w + 8, y, 2*bw, h, "Delete"); 
    remove_btn_p->callback((Fl_Callback1*)MAW__control_cb, REMOVE_BTN_MAW_CTRL);
    y = 6*vs + 8;
    action_label_inp_p = new Fl_Input(x + 2*bw, y, w - 5* bw, h, "Action label");
    action_label_inp_p->callback((Fl_Callback1*)MAW__control_cb, ALABEL_INP_MAW_CTRL);
    action_label_inp_p->when(FL_WHEN_CHANGED);
    y += vs; 
    cmd_inp_p = new Fl_Input(x + 2*bw, y, w - 2* bw, h, "Command line");
    cmd_inp_p->callback((Fl_Callback1*)MAW__control_cb, CMD_INP_MAW_CTRL);
    cmd_inp_p->when(FL_WHEN_CHANGED);
    y += vs; 
    int tx = x + w - 5*bw - h;
    undo_changes_btn_p = new Fl_Button(tx, y, 3*bw, h, "Undo changes");
    undo_changes_btn_p->callback((Fl_Callback1*)MAW__control_cb, UNDO_BTN_MAW_CTRL);
    tx += 3*bw + h;    
    close_btn_p = new Fl_Button(tx, y, 2*bw, h, "Close"); 
    close_btn_p->callback((Fl_Callback1*)MAW__control_cb, CLOSE_BTN_MAW_CTRL);
    end();
}

void Manage_Associations_Window::update(void)
{
    char buf[512];
    int sel_line = 0;
    cmd_list_p->clear();
    sprintf(buf, "Associated applications for filetype %s", filetype_name_str);
    filetype_lbl_p->copy_label(buf);
    int n;
    intptr_t i;
    //printf("Looking for associations for filetype %d\n", MainWnd_p->sel_fi_p->filetype); fflush(0);
    for (i = 0, n = 1; i < assocs; i++) {
        if (assoc[i].filetype == filetype) {
            sprintf(buf, "%d. %s:  %s", n, assoc[i].action_label, 
                    assoc[i].cmd_spec);
            cmd_list_p->add(buf);
            cmd_list_p->data(n, (void*)i);
            //printf("Manage... Added assoc[%d] to list: %s\n", i, buf); fflush(0);
            if (i == sel_assoc) {
                sel_line = n;
            }
            n++;
        }
    }
    if (cmd_list_p->size() && (sel_line > 0)) {
        cmd_list_p->value(sel_line);
    }
    else {
        disable_editing();
    }
}

void Manage_Associations_Window::swap_assoc_order(int sel, int other)
{
    association* sel_assoc_p   = &assoc[sel];
    association* other_assoc_p = &assoc[other];
    //printf("Manage... planning to move assoc '%s' up to the place of assoc '%s'\n",
    //       assoc[sel].cmd_spec, assoc[other].cmd_spec);
    swap_associations(sel_assoc_p, other_assoc_p);
    // update selected assoc pointer and refresh gui
    sel_assoc = other;
    update();
}



Manage_Filetypes_Window::Manage_Filetypes_Window(int x, int y, int w, int h)
    :   Fl_Window(x, y, w, h, "Manage File Types and Hints")
{
    sel_hint = -1;    
    strclr(new_filetype_name_str);
    new_hint.filetype = -1;
    new_hint.hintmethod = HINTMETHOD_EXTENSION;
    strclr(new_hint.pat);
    new_hint.patlen = 0;
    new_info_ready = 0;
}

Manage_Filetypes_Window::~Manage_Filetypes_Window()
{
}

void Manage_Filetypes_Window::present(int init_filetype)
{
    memcpy(filetype_name_str, FiletypeName_str, sizeof(FiletypeName_str));
    filetypes = Filetypes;
    memcpy(file_hint, FiletypeHint, sizeof(FiletypeHint));
    file_hints = FiletypeHints;
    assocs = Associations;
    memcpy(assoc, Association, sizeof(Association));

    sel_hint = -1; //FIRST_CONFIGURED_FILEHINT;
    if (new_info_ready) {
        strnzcpy(filetype_name_str[filetypes], new_filetype_name_str, MAXFTNAMELEN);
        memcpy(&file_hint[file_hints], &new_hint, sizeof(new_hint));
        file_hint[file_hints].filetype = filetypes;
        new_info_ready = 0;
        sel_hint = file_hints;
        //printf("adding ft %d, hint %d, '%s', matches '%s'\n", 
        //        filetypes, file_hints, filetype_name_str[filetypes], 
        //        file_hint[sel_hint].pat); fflush(0);
        filetypes++;
        file_hints++;
    }
    else if (init_filetype >=0) {
        int h;
        for (h = 0; h < filetypes; h++) {
            if (file_hint[h].filetype == init_filetype) {
                sel_hint = h;
                break;
            }
        }
    }
    else {
        editable_grp_p->deactivate();
    }
//    printf("%s: init_filetype is %d, sel_hint is %d\n", __func__, 
//            init_filetype, sel_hint); fflush(0);
    update();
    Fl_Window::show();
}

void Manage_Filetypes_Window::withdraw(void)
{
    memcpy(FiletypeName_str, filetype_name_str, sizeof(FiletypeName_str));
    Filetypes = filetypes;
    memcpy(FiletypeHint, file_hint, sizeof(FiletypeHint));
    FiletypeHints = file_hints;
    Associations = assocs;
    memcpy(Association, assoc, sizeof(Association));
    sync_gui_with_filesystem();
    Fl_Window::hide();
}

void Manage_Filetypes_Window::update(void)
{
    char buf[512];
    char fmt[32];
    const char* method_str_p;
//    int sel_line = 0;
    hint_list_p->clear();
    int ft, n = 1;
    intptr_t fh;
    hint_list_p->clear();
    const char* bk_clr_str = "";
    //printf("Will now put %d hints into the list\n", FiletypeHints);
    //printf("filetypes update(): Sel filetype is %d (%s), sel hint is %d\n", sel_filetype, 
    //        FiletypeName_str[sel_filetype], sel_hint); fflush(0);
    // first add the generic file entry
    hint_list_p->add("@C88@.generic file\t@C88@.--");
    //printf("   %s\n", buf); fflush(0);
    hint_list_p->data(n, (void*)0);
    n++;

    for (ft = FT_EXEC; ft < filetypes; ft++) {
        for (fh = 0; fh < file_hints; fh++) {
            //printf("File hint #%d has filetype %d\n", fh, FiletypeHint[fh].filetype);
            if (file_hint[fh].filetype == ft) {
                //printf("Adding ft %d, fh %d...", ft, fh);
                bk_clr_str = "";
                if ((sel_hint >= 0) && (ft == file_hint[sel_hint].filetype)) {
                    bk_clr_str = Bg2Color_str;
                }
                if (fh < FIRST_CONFIGURED_FILEHINT) {
                    sprintf(fmt, "@C88%s@.", bk_clr_str);
                }
                else {
                    sprintf(fmt, "@C16%s@.", bk_clr_str);
                }
                switch(file_hint[fh].hintmethod) {
                    case HINTMETHOD_STRING:
                        method_str_p = HinttypeStr_str;
                        break;
                    case HINTMETHOD_BYTES:
                        method_str_p = HinttypeData_str;
                        break;
                    case HINTMETHOD_EXTENSION:
                        method_str_p = HinttypeExt_str;
                        break;
                    default:
                        method_str_p = "";
                        break;
                }
                sprintf(buf, "%s%s\t%s%s\t%s",
                        fmt, filetype_name_str[ft], 
                        fmt, method_str_p,
                        fmt
                        );
                if ( file_hint[fh].hintmethod == HINTMETHOD_BYTES) {
                    for (int b = 0 ; b < file_hint[fh].patlen; b++) {
                        char bytestr[4];
                        sprintf(bytestr, "%02X ", (unsigned char)(file_hint[fh].pat[b]));
                        strcat(buf, bytestr);
                    }
                }
                else {
                    strcat(buf, file_hint[fh].pat);
                }
                if (file_hint[fh].hintmethod != HINTMETHOD_EXTENSION) {
                    strcat(buf, "\t");
                    strcat(buf, fmt);
                    char offsetstr[8];
                    sprintf(offsetstr, "%d", file_hint[fh].offset);
                    strcat(buf, offsetstr);
                }
                
                hint_list_p->add(buf);
                //printf("   %s\n", buf); fflush(0);
                hint_list_p->data(n, (void*)fh);
                n++;
            }
        } 
    }
    //printf("%s: sel_hint is %d\n", __func__, sel_hint); fflush(0);
    if (sel_hint >= 0) 
    {   
        show_hint();
    } 
}

void Manage_Filetypes_Window::show_hint(void)
{
    char match_bytes_str[64];
    int b;
    if (sel_hint == 0) {
        filetype_name_inp_p->value(filetype_name_str[FT_TEXT]);
        embed_str_type_chk_p->clear();
        embed_data_type_chk_p->clear();
        extension_type_chk_p->clear();
        hint_match_inp_p->value("");
        hint_offset_inp_p->deactivate();
        hint_offset_inp_p->value("");
        assoc_btn_p->activate();
        editable_grp_p->deactivate();
        scroll_to_hint(0);
        return;
    }
    filetype_hint* hint_p = &file_hint[sel_hint];
    strclr(match_bytes_str);
    assoc_btn_p->activate();
    addnew_hint_btn_p->activate();
    filetype_name_inp_p->value(filetype_name_str[hint_p->filetype]);
    static char offset_str[8];
    sprintf(offset_str, "%d", hint_p->offset);
    switch(hint_p->hintmethod) {
        case HINTMETHOD_STRING:
            embed_str_type_chk_p->set();
            embed_data_type_chk_p->clear();
            extension_type_chk_p->clear();
            hint_match_inp_p->value(hint_p->pat);
            hint_offset_inp_p->activate();
            hint_offset_inp_p->value(offset_str);
            break;
            
        case HINTMETHOD_BYTES:
            embed_str_type_chk_p->clear();
            embed_data_type_chk_p->set();
            extension_type_chk_p->clear();
            for(b = 0; b < hint_p->patlen; b++) {
                char hexnum_str[4];
                sprintf(hexnum_str, "%02X ", (unsigned char)(hint_p->pat[b]));
                strcat(match_bytes_str, hexnum_str);
            }
            if (!skip_pat_in_update) {
                hint_match_inp_p->value(match_bytes_str);
            }
            hint_offset_inp_p->activate();
            hint_offset_inp_p->value(offset_str);
            break;
            
        case HINTMETHOD_EXTENSION:
            embed_str_type_chk_p->clear();
            embed_data_type_chk_p->clear();
            extension_type_chk_p->set();
            hint_match_inp_p->value(hint_p->pat);
            hint_offset_inp_p->deactivate();
            hint_offset_inp_p->value("");
            break;
    }
//    printf("%s: sel_hint is %d, first config filehint is %d\n", 
//            __func__, sel_hint, 
//            FIRST_CONFIGURED_FILEHINT); fflush(0);
    if (sel_hint < FIRST_CONFIGURED_FILEHINT) {
        editable_grp_p->deactivate();
    }
    else {
        editable_grp_p->activate();
    }
    scroll_to_hint(sel_hint);
}

int int_value_from_input_field(Fl_Input* inp_p, int* value, const char* warning_str)
{
    char numstr[16];
    strnzcpy(numstr, inp_p->value(), 16);
    int n;
    if (1 == sscanf(numstr, "%d", &n)) {
        *value = n;
        return 1;
    }
    else {
        if (warning_str) 
            fl_alert(warning_str);
        return 0;
    }
 
}

int Manage_Filetypes_Window::hint_count_for_filetype(int filetype) 
{
    int h, count = 0;
    for (h = 0; h < file_hints; h++) {
        if (file_hint[h].filetype == filetype) {
            count++;
        }
    }
    return count;
}

int Manage_Filetypes_Window::scroll_to_hints_for_filetype(int filetype)
{
    int h = 1; 
    long int hint;
    for (h = 1; h <= hint_list_p->size(); h++) {
        hint = (long)hint_list_p->data(h);
        //printf("checking ptr 0x%08X (type %d) to match type %d\n", 
        //        hint_p, hint_p->filetype, filetype); fflush(0);
        if (file_hint[hint].filetype == filetype) {
            //printf("Found! Scrolling to line %d\n", h); fflush(0);
            hint_list_p->middleline(h);
            hint_list_p->redraw();
            return h;
        }
    } 
    return h;
}

int Manage_Filetypes_Window::scroll_to_hint(int hint)
{
    if (hint == 0) {
        hint_list_p->middleline(1);
        hint_list_p->value(1);
        hint_list_p->redraw();
        return 1;
    }
    int h = 1;
    //printf("Try to scroll to hint #%d\n", hint); fflush(0);
    for (h = 1; h <= hint_list_p->size(); h++) {
        if ((long)hint_list_p->data(h) == (long)hint) {
            //printf("   Found! Scrolling to line %d\n", h); fflush(0);
            hint_list_p->middleline(h);
            hint_list_p->value(h);
            hint_list_p->redraw();
            return h;
        }
    } 
    return h;
}

#define LIST_MFT_CTRL         1
#define NAME_INP_MFT_CTRL     2
#define ADD_TYPE_BTN_MFT_CTRL 3
#define ASSOC_BTN_MFT_CTRL    4
#define METHOD_CHK_MFT_CTRL   5
#define MATCH_INP_MFT_CTRL    6
#define OFFSET_INP_MFT_CTRL   7
#define REMOVE_BTN_MFT_CTRL   8
#define ADD_HINT_BTN_MFT_CTRL 9
#define UNDO_BTN_MFT_CTRL    10
#define CLOSE_BTN_MFT_CTRL   11

void MFTW__control_cb(Fl_Widget* thewidget_p, long ctrl_id)
{
    Manage_Filetypes_Window* p = ManageFiletypesWnd_p;
    Fl_Hold_Browser* list_p = p->hint_list_p;
    int v = list_p->value();
    int h = p->sel_hint;
    int ft = -1;
    if (h >= 0)  ft = p->file_hint[h].filetype;

    switch(ctrl_id) {
        
    case LIST_MFT_CTRL: 
        h = (int)(long)list_p->data(v);
        //printf("User selected line %d, which has data %d!\n", v, h);
        if (h == p->sel_hint) {
            //printf("User selected same hint!\n"); fflush(0);
            break;
        }
        p->sel_hint = h;
        p->update();
        break;

    case NAME_INP_MFT_CTRL:
        if (strlen( p->filetype_name_inp_p->value())  > 0) {
            strnzcpy(p->filetype_name_str[ft], 
                    p->filetype_name_inp_p->value(), MAXFTNAMELEN);
            p->update();
        }
        break;
        
    case ASSOC_BTN_MFT_CTRL:
        {
            if (v == 1)   ft = FT_TEXT;
            if (ft >= 0) {
                ManageAssocWnd_p->show(ft, p->filetype_name_str[ft],
                                       &(p->assocs), p->assoc);
            }
        }
        break;
    
    case MATCH_INP_MFT_CTRL:
        if (h >= 0 && strlen(p->hint_match_inp_p->value()) > 0) {
            switch(p->file_hint[h].hintmethod) {
                case HINTMETHOD_BYTES:
                    {
                        char match_str[128];
                        char match_buf[64];
                        const char* byte_match_warn_str = "Match byte format not valid";
                        strnzcpy(match_str, p->hint_match_inp_p->value(), 128);
                        int v = 0;
                        int l = 0;
                        char* ip = match_str;
                        char* op = match_buf;
                        while (*ip && l < 11) {
                            //printf("Looking at char '%c'\n", *ip); fflush(0);
                            if (*ip == ' ' || *ip == '\n') {
                                *op++ = (unsigned char)v;
                                //printf("Byte #%d is 0x%02X\n", l, v);
                                l++;
                                while(*ip == ' ') ip++;
                                v = 0;
                            }
                            else if (*ip >= '0' && *ip <= '9') {
                                v *= 16;
                                v += *ip - '0';
                                ip++;
                            }
                            else if (*ip >= 'a' && *ip <= 'f') {
                                v *= 16;
                                v += 10 + *ip - 'a';
                                ip++;
                            }
                            else if (*ip >= 'A' && *ip <= 'F') {
                                v *= 16;
                                v += 10 + *ip - 'A';
                                ip++;
                            }
                            else if (v >= 256) {
                                fl_alert(byte_match_warn_str);
                                return;
                            }
                            else {
                                fl_alert(byte_match_warn_str);
                                return;
                            }
                        }
                        if (v) {
                            *op++ = v; // write the last one
                            //printf("Byte #%d is 0x%02X\n", l, v); fflush(0);
                            l++;
                        }
                        *op++ = '\0';
                        strncpy(p->file_hint[h].pat, match_buf, l+1);
                        p->file_hint[h].patlen = l;
                        p->skip_pat_in_update = 1;
                    } 
                    break;
                    
                case HINTMETHOD_STRING:
                case HINTMETHOD_EXTENSION:
                    if (strcmp( p->hint_match_inp_p->value(), 
                                p->file_hint[h].pat)) {
                        strnzcpy(p->file_hint[h].pat, 
                                p->hint_match_inp_p->value(), MAXPATLEN);
                        p->file_hint[h].patlen = strlen(p->file_hint[h].pat);
                        if (p->file_hint[h].patlen >=  MAXPATLEN) {
                            p->file_hint[h].patlen = MAXPATLEN - 1;
                        }
                    }
                    break;
            }
            p->update();
            p->skip_pat_in_update = 0;
        }
        break;

	#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
    case OFFSET_INP_MFT_CTRL:
        if (    h >= 0 
            &&  p->file_hint[h].hintmethod != HINTMETHOD_EXTENSION) {
            int n = 0;
            if (int_value_from_input_field( p->hint_offset_inp_p, &n, 
                                            NULL) )
            {
                p->file_hint[h].offset = n;
                p->update();
            }
        }
	#pragma GCC diagnostic warning "-Wimplicit-fallthrough"

    case ADD_TYPE_BTN_MFT_CTRL:
        {
            if (p->filetypes + 1 > MAXFILETYPE) {
                fl_alert("Fluff currently supports only %d filetypes.", MAXFILETYPE);
                return;
            }
            if (p->file_hints + 1 > MAXFILEHINT) {
                fl_alert("Fluff currently supports only %d filetype hints.", MAXFILETYPE);
                return;
            }
            //p->filetype_name_inp_p->activate();
            int newtype = p->filetypes;
            int newhint = p->file_hints;
            strnzcpy(p->filetype_name_str[newtype], "newtype", MAXFTNAMELEN);
            p->file_hint[newhint].filetype = newtype;
            p->file_hint[newhint].hintmethod = HINTMETHOD_EXTENSION;
            p->file_hint[newhint].offset = 0;
            strnzcpy(p->file_hint[newhint].pat, ".something", MAXPATLEN);
            p->file_hint[newhint].patlen = 10;    
            p->filetypes++;
            p->file_hints++;
            p->sel_hint = newhint;
            p->update();
        }
        break;

    case METHOD_CHK_MFT_CTRL:
        if (h >= 0) {
            Fl_Check_Button* the_chk_p = (Fl_Check_Button*)thewidget_p;
            int method = HINTMETHOD_EXTENSION;
            if (the_chk_p == p->embed_str_type_chk_p) {
                method = HINTMETHOD_STRING;
            }
            else if (the_chk_p == p->embed_data_type_chk_p) {
                method = HINTMETHOD_BYTES;
            }
            p->file_hint[h].hintmethod = method;
            p->file_hint[h].patlen = strlen(p->file_hint[h].pat);
            if (p->file_hint[h].patlen >=  MAXPATLEN) {
                p->file_hint[h].patlen = MAXPATLEN - 1;
            }
            p->update();
        }
        break;
        
    case ADD_HINT_BTN_MFT_CTRL:
        {   
            if (p->file_hints + 1 > MAXFILEHINT) {
                fl_alert("Fluff currently supports only %d filetype hints.", MAXFILEHINT);
                return;
            }
            int newhint = p->file_hints;
            //printf("New hint #%d\n", newhint);
            p->file_hint[newhint].filetype = p->file_hint[h].filetype;
            p->file_hint[newhint].hintmethod = p->file_hint[h].hintmethod;
            p->file_hint[newhint].offset = p->file_hint[h].offset;
            strnzcpy(p->file_hint[newhint].pat, "pattern", MAXPATLEN);
            p->file_hint[newhint].patlen = 7;    
            p->file_hints++;
            p->sel_hint = newhint;
            p->update();
        }
        break;
        
    case UNDO_BTN_MFT_CTRL:
        p->present(p->sel_hint);
        break;

    case REMOVE_BTN_MFT_CTRL:
        //int filetype = p->file_hint[h].filetype;
        if (p->hint_count_for_filetype(p->file_hint[h].filetype) == 1) {
            fl_alert("Flume will not remove the last filetype hint for a filetype");
            return;
        }
        //printf("Removing hint %d, filetype %d...\n", h, filetype); fflush(0);
        while (h < p->file_hints - 1) {
            //printf("   Copy hint %d to place of hint %d...\n", h + 1, h); fflush(0);
            p->file_hint[h] = p->file_hint[h+1];
            h++;
        }
        p->file_hints--;
        {
            int line = p->scroll_to_hints_for_filetype(ft);
            //printf("First hint for type %d found at row %d.\n", ft, line); fflush(0);
            p->sel_hint = (int)(long)p->hint_list_p->data(line);
            //printf("Filetype hint pointer is 0x%08X", p->sel_hint_p);
        }
        p->update();
        break;

    case CLOSE_BTN_MFT_CTRL:
        p->withdraw();
        break;
    }
}

void Manage_Filetypes_Window::setup(void)
{
    int x = 20, y = 12;
    int h = 24, w = 400, bw = 100, vs = h + 8, hs = 8;
    int tx;
    begin();

    hint_lbl_p = new Fl_Box(x, y, w, h, 
                            "Filetype Name\t\tHint Method\t     Matches\tOffset in file");
    hint_lbl_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += h; 
    hint_list_p = new Fl_Hold_Browser( x, y, w, 6 * h);
    hint_list_p->column_widths(FileTypeColWidths);
    hint_list_p->callback(MFTW__control_cb, LIST_MFT_CTRL);
    
    y += 13*h/2; // 6.5
    tx = x;   
    Fl_Box* b = new Fl_Box(x, y, bw, h, "Filetype Name");
    b->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    tx = x + w - 2*bw - hs;
    addnew_hint_btn_p = new Fl_Button(tx, y - 6, bw, h, "Add new hint");
    addnew_hint_btn_p->callback(MFTW__control_cb, ADD_HINT_BTN_MFT_CTRL);
    tx += bw + hs;
    remove_hint_btn_p = new Fl_Button(tx, y - 6, bw, h, "Remove hint");
    remove_hint_btn_p->callback(MFTW__control_cb, REMOVE_BTN_MFT_CTRL);

    y += h;
    tx = x;
    filetype_name_inp_p = new Fl_Input(tx, y, bw, h); 
    filetype_name_inp_p->callback(MFTW__control_cb, NAME_INP_MFT_CTRL);
    filetype_name_inp_p->when(FL_WHEN_CHANGED);
    //filetype_name_inp_p->deactivate();
    tx = x + w - 2*bw - hs;
    assoc_btn_p = new Fl_Button(tx, y, 2*bw + hs, h, "Associated apps...");
    assoc_btn_p->callback(MFTW__control_cb, ASSOC_BTN_MFT_CTRL);

    y += vs + 8;
    tx = x;
    filehint_lbl_p = new Fl_Box(tx, y, 2*bw, h, "Hint method\t\t\t   Matches\t\tOffset within file");
    filehint_lbl_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += h - 4; 
    tx = x;
    embed_str_type_chk_p = new Fl_Check_Button(tx, y, 3*bw/2, h, "Header string");
    embed_str_type_chk_p->callback(MFTW__control_cb, METHOD_CHK_MFT_CTRL);
    tx = x + w - 2*bw - hs;
    hint_match_inp_p = new Fl_Input(tx, y, bw+hs, h);
    hint_match_inp_p->callback(MFTW__control_cb, MATCH_INP_MFT_CTRL);
    hint_match_inp_p->when(FL_WHEN_CHANGED);
    tx += bw + 2*hs;
    hint_offset_inp_p = new Fl_Int_Input(tx, y, bw-hs, h);
    hint_offset_inp_p->callback(MFTW__control_cb, OFFSET_INP_MFT_CTRL);
    hint_offset_inp_p->when(FL_WHEN_CHANGED);

    y += vs; 
    tx = x;
    embed_data_type_chk_p = new Fl_Check_Button(tx, y, 3*bw/2, h, "Header bytes (Enter bytes like: 32 FE 1F B5)");
    embed_data_type_chk_p->callback(MFTW__control_cb, METHOD_CHK_MFT_CTRL);

    y += vs; 
    tx = x;
    extension_type_chk_p = new Fl_Check_Button(tx, y, 3*bw/2, h, "Filename extension");
    extension_type_chk_p->callback(MFTW__control_cb, METHOD_CHK_MFT_CTRL);
    
    y += vs;
    tx = x;
    addnew_type_btn_p = new Fl_Button(tx, y, 4*bw/3, h, "Add type");
    addnew_type_btn_p->callback(MFTW__control_cb, ADD_TYPE_BTN_MFT_CTRL); 
    tx += 4*bw/3 + hs;    
    undo_changes_btn_p = new Fl_Button(tx, y, 4*bw/3, h, "Undo changes");
    undo_changes_btn_p->callback(MFTW__control_cb, UNDO_BTN_MFT_CTRL);
    tx = x + w - bw;
    close_btn_p = new Fl_Button(tx, y, bw, h, "Close");
    close_btn_p->callback(MFTW__control_cb, CLOSE_BTN_MFT_CTRL);
    
    editable_grp_p = new Fl_Group(0, 0, Fl_Window::w(), Fl_Window::h());
    editable_grp_p->add(filetype_name_inp_p);
    editable_grp_p->add(hint_match_inp_p);
    editable_grp_p->add(hint_offset_inp_p);
    editable_grp_p->add(embed_str_type_chk_p);
    editable_grp_p->add(embed_data_type_chk_p);
    editable_grp_p->add(extension_type_chk_p);
    editable_grp_p->deactivate();
    end();
    skip_pat_in_update = 0;
}

void Fluff_Window::update_btnbar(void) 
{
    int have_sel = (MainWnd_p->list_p->num_selected() > 0);
    enable_menu_item(dirtree_right_click_menu, COPY_TO_MENU_ITEM_INDEX, have_sel);
    enable_menu_item(dirtree_right_click_menu, MOVE_TO_MENU_ITEM_INDEX, have_sel);
    
    if (have_sel) {
        run_btn_p->activate();
        open_btn_p->activate();
        props_btn_p->activate();
        copy_btn_p->activate();
        delete_btn_p->activate();
        int trashed_file = (*(MainWnd_p->sel_fi_p->name) == '~');
        enable_menu_item(file_right_click_menu, RESTORE_FROM_TRASH_ITEM_INDEX, trashed_file);
        enable_menu_item(file_right_click_menu, INSERT_IN_TRASH_ITEM_INDEX, !trashed_file);
        trash_btn_p->label(trashed_file ? "Restore":"Trash");

    }
    else {
        run_btn_p->deactivate();
        open_btn_p->deactivate();
        props_btn_p->deactivate();
        copy_btn_p->deactivate();
        delete_btn_p->deactivate();
        relabel_menu_item(file_right_click_menu, 0, "---");
        enable_menu_item(file_right_click_menu, 0, 0);
        relabel_menu_item(file_right_click_menu, 1, "---");
        enable_menu_item(file_right_click_menu, 1, 0);
        trash_btn_p->label("Trash");
        //path_inp_p->value("");
    }
    
    if (marked_file_items(MarkedFileItems_sz_p)) {
        paste_btn_p->activate();
        enable_menu_item(dirtree_right_click_menu, PASTE_DIRTREE_MENU_ITEM_INDEX, 1);
        enable_menu_item(file_right_click_menu, PASTE_FILELIST_MENU_ITEM_INDEX, 1);
    }
    else {
        paste_btn_p->deactivate();
        enable_menu_item(dirtree_right_click_menu, PASTE_DIRTREE_MENU_ITEM_INDEX, 0);
        enable_menu_item(file_right_click_menu, PASTE_FILELIST_MENU_ITEM_INDEX, 0);
    }
    
    //printf("Re-labeling buttons and menu items\n"); fflush(0);
    if (MainWnd_p->sel_fi_p) {
        //Update button and menu labels based on filetype
        char* run_label = get_action_label_for_filetype(MainWnd_p->sel_fi_p->filetype, SPECTYPE_RUN);
        //printf("Run label prt is 0x%08x %s\n", run_label, run_label ? run_label : ""); fflush(0);
        if (run_label) {
            run_btn_p->label(run_label);
            relabel_menu_item(file_right_click_menu, 0, run_label);
            enable_menu_item(file_right_click_menu, 0, 1);
        }
        else {
            run_btn_p->label("---");
            run_btn_p->deactivate();
            relabel_menu_item(file_right_click_menu, 0, "---");
            enable_menu_item(file_right_click_menu, 0, 0);
        }
        char* open_label = get_action_label_for_filetype(MainWnd_p->sel_fi_p->filetype, SPECTYPE_OPEN);
        //printf("Open label prt is 0x%08x %s\n", open_label, open_label ? open_label : ""); fflush(0);
        if (open_label) {
            open_btn_p->label(open_label);
            relabel_menu_item(file_right_click_menu, 1, open_label);
            enable_menu_item(file_right_click_menu, 1, 1);
        }
        else {
            open_btn_p->label("---");
            open_btn_p->deactivate();
            relabel_menu_item(file_right_click_menu, 1, "---");
            enable_menu_item(file_right_click_menu, 1, 0);
        }
        //printf("Done with update_btnbar()\n"); fflush(0);
    }
    analyze_and_disp_status();
}   

int near_column_break(int x) {
    int col = -1;
    int c = 0;
    int break_x = 0;
    while (ColWidths[c] && col < 0 ) {
        break_x += ColWidths[c];
        if (abs(break_x - x) < 6) {
            col = c;
            //printf("Near Column %d break\n", col); fflush(0);
            break;
        }
        c++;
    } 
    
    return col;
}

int column_in_filelist(int x) 
{
    int col = -1;
    int c = 0;
    int start_x = 0;
    int end_x = 0;
    while (ColWidths[c] && col < 0 ) {
        end_x += ColWidths[c];
        if (x >= start_x   && x < end_x) {
            col = c;
        }
        c++;
        start_x += ColWidths[c];
    } 
    if (col < 0 && x > start_x) {
        col = c;
    }
    
    return col;
}

void File_Detail_List_Browser::draw() 
{
    // Determine where to put sort direction symbol
    int sortcol  = MainWnd_p->sortspec >> 1;
    int sortdown = MainWnd_p->sortspec & 1; // down=descending order (down arrow)
    
    // Draw generic stuff
    Fl_Multi_Browser::draw();
    int X,Y,W,H,colx;
    Fl_Browser::bbox(X,Y,W,H);
    fl_color(42);
    colx = this->x() - hposition();
    
    // DRAW COLUMN SEPARATORS
    colx = this->x() - hposition();
    Fl_Browser::bbox(X,Y,W,H);
    int line_h = partitioning ?  (H - 1) : (textsize());
    for ( int t=0; ColWidths[t]; t++ ) {
        colx += ColWidths[t];
        if ( colx > X && colx < (X+W) && line_h > vposition() ) {
            fl_color(42);
            Y -= vposition();
            fl_line(colx, Y, colx, Y+line_h);
            if (t == sortcol) {
                fl_color(40);
                int mx = colx - ColWidths[t] + textsize();
                int my = Y - vposition();
                if (sortdown) {
                    my += textsize();
                    fl_polygon(mx - 12, my - 8, mx - 7, my - 3, mx - 2, my - 8);
                }
                else {
                    fl_polygon(mx - 12, my + 8, mx - 7, my + 3, mx - 2, my + 8);
                } 
            } 
        }
    }
}

int File_Detail_List_Browser::item_number_under_mouse(int mouse_y) {
    int target_y = vposition() + mouse_y;

    FL_BLINE * it = (FL_BLINE *)item_first();
    if (!it)  return 0;
    int accum_y = item_height(it);
    int line = 1;
	it = (FL_BLINE *)item_next(it);
    while (it != NULL && accum_y < target_y) {
		line++;
		accum_y += item_height(it);
		it = (FL_BLINE *)item_next(it);
	}
	//~ printf("Mouse event y %d is at widget y %d considering scroll pos %d is in list line %d, which goes to y %d\n",
			//~ mouse_y, target_y, vposition(), line, accum_y);
	return line;
}

int File_Detail_List_Browser::item_y_coordinate(int line) {
	int line_y = 0;
	if (line < 1) return line_y;
	
	int i = 2;
	FL_BLINE * it = (FL_BLINE *)item_first();
	line_y = item_height(it); // upper y of line 2 (first real row after header row)
	while (i < line) {
		it = (FL_BLINE *)item_next(it);
		i++;
		line_y += item_height(it); // bump coordinate to next line
	}
	
	//~ printf("Click event y %d is at widget y %d considering scroll pos %d is in list line %d, which goes to y %d\n",
			//~ mouse_y, target_y, vposition(), line, accum_y);
	return line_y;
}

int File_Detail_List_Browser::handle(int e) {
    int ret = 0;
    static int partition_col = -1;
    File_Detail_List_Browser* p = MainWnd_p->list_p;
    //printf("item height is %d\n", height); fflush(0);
    int ex = Fl::event_x();
    int ey = Fl::event_y();
    int wgt_x = ex - 2 - File_Detail_List_Browser::x() 
                + File_Detail_List_Browser::hposition();   // x within scrolling widget
    //~ int wgt_y = ey - 2 - File_Detail_List_Browser::y() 
                //~ + File_Detail_List_Browser::vposition();    // y within scrolling widget
    
    int event_row = item_number_under_mouse(ey - y()); // 1 + (wgt_y) / list_item_height;
    int event_column = column_in_filelist(wgt_x);
    void* data_p = NULL;
    
    //printf("File_Detail_List_Browser event %d, column %d, last click %d, event clicks %d\n", 
    //        e, event_column, MainWnd_p->last_click_browser, Fl::event_clicks()); fflush(0);
    switch ( e ) {
       case FL_FOCUS:
            redraw();
            return 1;

        case FL_UNFOCUS:
            redraw();
            return 1;

        case FL_PUSH:
            //printf("PUSH event in file  list, ptr y %d, scr y %d, iH %d, row %d\n", event_row); fflush(0); 
            MainWnd_p->last_click_browser = LASTCLICK_IN_LIST;            
            dragx = ex;
            dragy = ey;
            take_focus();
            
            //printf("dragx %d, lb_y %d, scroll pos %d, height %d\n", 
            //        dragy, p->y(), p->position(), height); fflush(0);
            if (event_row == 1) {
                // We are in header row
                if (near_column_break(wgt_x) >= 0) {
                    partitioning = 1;
                    partition_col = near_column_break(wgt_x);
                    fl_cursor(FL_CURSOR_WE, FL_BLACK, FL_WHITE);
                }
                //printf("Clicked on header row\n"); fflush(0);
                return 1;
            }
            else if (   (dragy > p->y() + p->h() - p->scrollbar_width())
                     || (dragx > p->x() + p->w() - p->scrollbar_width())
                     ) {
                return Fl_Multi_Browser::handle(e); // on a scroll bar
            }
            
            if (Fl::event_button() == FL_RIGHT_MOUSE ) {
                // Note: right-click is handled in filelist_change_cb()
                return 1;
            }
             
             
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                if (p->value() == 1) {
                    return 1;
                }
                if (Fl::event_state() & (FL_SHIFT | FL_CTRL)) {
                    return Fl_Multi_Browser::handle(e);
                }
                if (Fl::event_clicks() == 0) {
                    if (event_column == 0) {
                        return 1;
                    }
                    else {
                        return Fl_Multi_Browser::handle(e);
                    }
                }                
                else if (Fl::event_button() == FL_LEFT_MOUSE && Fl::event_clicks() == 1) {
                    // Double-click... do run command or change dirs
                    //printf("File List Double-click\n");
                    if (p) {
                        MainWnd_p->sel_fi_p = (file_item*)(p->data(p->value()));
                        btnbar_action1_cb();
                    }
                    return(1);          // (tells caller we handled this event)
                }
            }

            else {
                ret = Fl_Multi_Browser::handle(e);
                return(ret);          
            }
            break;
            
        case FL_MOVE:       // FL_MOVE event means no buttons are held right now,
            dragging = 0;   // so abort any unfinished drag-and-drop
            //printf("Move event, row %d, column %d\n", event_row, event_column); fflush(0);
            if (event_row == 1) {
                // We are in header row
                if (near_column_break(ex - p->x() + p->hposition()) >= 0) {
                    //printf("Near column break %d\n", near_column_break(ex - p->x() + p->hposition())); fflush(0);
                    fl_cursor(FL_CURSOR_WE, FL_BLACK, FL_WHITE);
                    //return 1;
                    break;
                }
            }
            fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
            break;
            
        case FL_DRAG:
            dragging = 1;
            //printf("Mouse dragging...\n"); fflush(0);
            
            if (partitioning) { // Fl::event_y() - p->y() + p->position() < height) {
                //int col = near_column_break(ex - p->x() + p->hposition());
                int x_adj = ex - dragx;
                //printf("Dragging col %d, x_adj %d, ex %d\n", partition_col, x_adj, ex); fflush(0);
                ColWidths[partition_col] += x_adj;
                column_widths(ColWidths);
                redraw();
                dragx = ex;
                dragy = ey;
                
                return 1;
            }

            if (event_column > 0) {
                // Not dragging from name, handle as sweep-selection
                return Fl_Multi_Browser::handle(e);
            }
            
            if (abs(ex - dragx) < 8 && abs(ey - dragy) < 4 ) {
                // Didn't move far, ignore?
                return 1;
            }
                        
            if (!p->selected(event_row)) {
                return 1; // can't drag unselected item... maybe this row will be selected later
            }
            
            mark_file_items(p);
            //printf( "Initializing drag and drop with the following item(s):\n%s\n", 
            //       MarkedFileItems_sz_p);
            
            // Copy the file path(s) to the selection buffer
            Fl::copy(MarkedFileItems_sz_p, strlen(MarkedFileItems_sz_p)+1, 0);

            // initiate the DnD magic.
            // At this point FLTK's system wide DnD kicks in and we lose control
            Fl::dnd();
            return 1;

        case FL_RELEASE:
			//~ printf("RELEASE event in file list, ey %d, bY %d, vpos %d, wgt_y %d, row %d\n", 
					//~ ey,  y(), File_Detail_List_Browser::vposition(),wgt_y, event_row); fflush(0); 

            //analyze_and_disp_status();
            MainWnd_p->last_click_browser = LASTCLICK_IN_LIST;            
            if (MainWnd_p->mode == MAINWND_MODE_RENAMING) {
                //printf("end renaming...\n"); fflush(0);
                MainWnd_p->end_renaming();
            }
            if (partitioning) {
                //printf("end column partitioning...\n"); fflush(0);
                partitioning = 0;
                fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
                redraw();
                return 1;
            }
            if (Fl::event_button() == FL_RIGHT_MOUSE ) {
                // Note: right-click is handled in filelist_change_cb()
                do_callback();
                return 1;
            }
            if (Fl::event_button() == FL_LEFT_MOUSE ) {
                //printf("left click release ...\n"); fflush(0);
                if (dragging) {
                    //printf("end drag-n-dropping...\n"); fflush(0);
                    dragging = 0;
                }

                if (event_row == 1) {
                    //printf("change sort order\n"); fflush(0);
                    int cur_sort_col = MainWnd_p->sortspec >> 1;
                    int descending = MainWnd_p->sortspec & 1;
                    if (event_column >= 0 && event_column <= 3) {
                        if (event_column == cur_sort_col) {
                            MainWnd_p->sortspec ^= 1; // toggle direction
                        }
                        else {
                            MainWnd_p->sortspec = (event_column << 1) | descending; // change sort param, keep current direction
                        }
                        //printf("pruning...\n"); fflush(0);
                        prune_children(MainWnd_p->seldir_fi_p);
                        //printf("reloading...\n"); fflush(0);
                        get_child_file_items(MainWnd_p->seldir_fi_p);
                        //printf("showing...\n"); fflush(0);
                        sync_gui_with_filesystem();
                        //printf("ready.\n"); fflush(0);
                        
                    }
                    return 1;
                }

                if (Fl::event_state() & (FL_SHIFT | FL_CTRL)) {
                    ret = Fl_Multi_Browser::handle(e);
                }                    
                else if (event_column > 0) {
                    ret = Fl_Multi_Browser::handle(e);
                    MainWnd_p->sel_fi_p = (file_item*)p->data(p->value());
                }
                else if (event_row > 1 && event_row <= p->size()) {
                    //printf("single-select row %d\n", event_row);
                    p->deselect();
                    p->select(event_row);
                    p->redraw();
                    ret = Fl_Multi_Browser::handle(e); // 1;
                }
                data_p = p->data(event_row);
                if (data_p) {
                    MainWnd_p->sel_fi_p = (file_item*)data_p;
                    MainWnd_p->update_btnbar();
                }
                return ret;          
            }
            break;

        case FL_MOUSEWHEEL:
            //printf("File_Detail_List_Browser::handle() for MOUSEWHEEL event\n"); fflush(0);
            if (MainWnd_p->mode == MAINWND_MODE_RENAMING) {
                //printf("Don't scroll during renaming!\n"); fflush(0);
                return 1; // eat event -- don't allow scrolling!
            }
            else {
                return Fl_Multi_Browser::handle(e);
            }
            break;

        case FL_KEYDOWN:
            {
                int key = Fl::event_key();
                //printf("Detail List Keydown, key is 0x%04X, FL_Enter is 0x%04X\n", key, FL_Enter);
                if (key == FL_Enter) {
                    return handle_global_keys(e);
                }
                else {
                    ret = Fl_Multi_Browser::handle(e);
                }
            }
            break;
            
        case FL_SHOW:
            return 1;

        default:
            ret = Fl_Multi_Browser::handle(e);
            break;

    }
    return(ret);
}

void dirtree_change_cb(void);

void relabel_menu_item(Fl_Menu_Item* menu_p, int index, const char* newlabel)
{
    menu_p[index].label(newlabel);
}

void enable_menu_item(Fl_Menu_Item* menu_p, int index, int enable)
{
    if (enable) 
        menu_p[index].flags &= ~FL_MENU_INACTIVE;
    else
        menu_p[index].flags |= FL_MENU_INACTIVE;
        
    //printf("Menu item %d flags now show value 0x%04X\n", index, menu_p[index].flags);
}

void do_file_menu(void) {
    const Fl_Menu_Item *m = file_right_click_menu->popup(
                                Fl::event_x(), Fl::event_y(), "Fluff File Menu");
    if ( m ) m->do_callback(NULL, m->user_data());
};

void do_dir_tree_menu(void) {
    const Fl_Menu_Item *m = dirtree_right_click_menu->popup(
                                Fl::event_x(), Fl::event_y(), "Fluff Directory Menu");
    if ( m ) m->do_callback(NULL, m->user_data());
};


Fluff_Window::Fluff_Window() 
    :   Fl_Double_Window(MainXPos, MainYPos, MainWide, MainHigh)
{
    
        btnbarpack_p = NULL;
        //quit_btn_p = NULL;
        run_btn_p = NULL;
        open_btn_p = NULL;
        props_btn_p = NULL;
        showall_chk_p = NULL;
        copy_btn_p = NULL;
        paste_btn_p = NULL;
        trash_btn_p = NULL;
        delete_btn_p = NULL;
        help_btn_p = NULL;
        path_lbl_p = NULL;
        pathhist_menu_p = NULL;
        browspack_p  = NULL;
        treegroup_p = NULL;
        tree_p  = NULL;
        list_p  = NULL;
        mainwnd_status_lbl_p = NULL;
        files_info_lbl_p = NULL;
        seldir_fi_p = NULL;
        targdir_fi_p = NULL;
        fprops_p = NULL;
        rename_inp_p = NULL;
        mode = MAINWND_MODE_VIEWING;
        adjusting = 0;
        last_click_browser = LASTCLICK_NONE;
        showingall = ShowAllFiles;
        sortspec = SortOrder;
        strnzcpy(path, "/", MAXFPATHLEN);
        dragx = 0;
        dragy = 0;
}

Fluff_Window::~Fluff_Window() {
    //printf("Fluff_Window destructor\n"); fflush(0);
    delete browspack_p;
    delete treegroup_p;
    delete tree_p;
    delete list_p; 
    //delete list2_p;
    delete fprops_p;
    delete rename_inp_p;
}

void Fluff_Window::enable_renaming(file_item* fi_p, int x, int y) {
    if (fi_p) {
        int w = ColWidths[0];
        if (last_click_browser == LASTCLICK_IN_TREE) {
            w = DirTreeWide - 8 - MainWnd_p->tree_p->scrollbar_width();
        }
        mode = MAINWND_MODE_RENAMING;
        rename_fi_p = fi_p;
        rename_inp_p = new Fl_Input(x, y, w, 18);
        rename_inp_p->box(FL_FLAT_BOX);
        rename_inp_p->color(FL_YELLOW);
        rename_inp_p->value(rename_fi_p->name);
        //printf("Positioning input field at x %d, y %d\n", x, y);
        add(rename_inp_p);
        rename_inp_p->show();
        rename_inp_p->take_focus();
        redraw();
    }
};

void Fluff_Window::end_renaming(void) {
    mode = MAINWND_MODE_VIEWING;
    rename_inp_p->hide();
    remove(rename_inp_p);
    delete rename_inp_p;
    rename_inp_p = NULL;
    rename_fi_p = NULL;
    
};

void Fluff_Window::commit_renaming(void) {
    conditionally_rename(rename_fi_p, rename_inp_p->value());
    end_renaming();
    sync_gui_with_filesystem();
};

void Fluff_Window::set_titles(void) {
    if (seldir_fi_p == Root_fi_p) {
        sprintf(wintitle, "/ (root) - Fluff File Manager");
    }
    else if (seldir_fi_p) {
        sprintf(wintitle, "%s - Fluff", seldir_fi_p->name);
    }
    else {
        sprintf(wintitle, "Fluff File Manager");
    }
    label(wintitle);
//    resize(x(), y(), w()+1, h());
//    redraw();
//    resize(x(), y(), w()-1, h());
    redraw();
}

enum {ADJUST_NONE = 0, ADJUST_TREE_WIDTH};

void Fluff_Window::do_menu(void) {
    if (Fl::event_inside(MainWnd_p->tree_p)) {
        do_dir_tree_menu();
    }
    if (Fl::event_inside(MainWnd_p->list_p)) {
        do_file_menu();
    }
    else {
        const Fl_Menu_Item *m = main_right_click_menu->popup(
                                    Fl::event_x(), Fl::event_y(), "Fluff File Manager");
        if ( m ) m->do_callback(this, m->user_data());
    }
};

#if 1
int Fluff_Window::handle(int e) {
    int ret = 0;
    static int width = 0;
    int key;
    int x = Fl::event_x();
    int y = Fl::event_y();
    Fluff_Window* p = MainWnd_p;
    int th = p->treegroup_p->h();
    int ty = p->treegroup_p->y();
    int tx = p->treegroup_p->x();
    int tw = p->treegroup_p->w();
    int lx = p->list_p->x();
    //int lw = p->list_p->x();
    
	//printf("MainWnd Event %d %s\n", e,  fl_eventnames[e]);

    switch ( e ) {
        case FL_SHORTCUT:
            if (Fl::event_key()==FL_Escape) {
                return 1;
            }
            return 0;
            break;

        case FL_KEYDOWN:
            key = Fl::event_key();
            if (key == 0 || key == FL_Menu) {                
                do_menu();
                return 1;
            }
            if (key == FL_Enter && mode == MAINWND_MODE_RENAMING) {
                commit_renaming();
                return 1;
            }
            else if (key == FL_Escape && mode == MAINWND_MODE_RENAMING) {             
                end_renaming();
                return 1;
            }
            else if (key == FL_Escape) {             
                // eat the event, don't close app!
                // unselect everything
                for(int i = 1; i <= list_p->size(); i++) {
                    list_p->select(i, 0);
                }
                MainWnd_p->sel_fi_p = NULL;
                return 1;
            }
            else if (key == 'b' && (Fl::event_state() & FL_CTRL)) {
				btnbar_go_bk_cb();
				return 1;
			}
            else if (key == 'f' && (Fl::event_state() & FL_CTRL)) {
				btnbar_go_fw_cb();
				return 1;
			}
            else if (key == 'u' && (Fl::event_state() & FL_CTRL)) {
				btnbar_go_up_cb();
				return 1;
			}
            else {
                return handle_global_keys(e);
            }
            break;

        case FL_PUSH:
            dragx = Fl::event_x();
            dragy = Fl::event_y();
            if ( Fl::event_button() == FL_LEFT_MOUSE ) {
                if (    x > (tx + tw)   && x < lx 
                    &&  y > ty          && y < (ty + th)
                   ) 
                {
                    adjusting = ADJUST_TREE_WIDTH;
                    fl_cursor(FL_CURSOR_WE, FL_BLACK, FL_WHITE);
                }
                if (!Fl_Window::handle(e)) {
                    //fl_message("left click");
                    return 1;
                }
            }
            else if (Fl::event_button() == FL_RIGHT_MOUSE ) {
                if (!Fl_Window::handle(e)) {
                    tooltip("");
                    do_menu();
                    return(1);          // (tells caller we handled this event)
                }
            }
            else {
                ret = Fl_Window::handle(e);
                return(ret);          
            }
            break;

        case FL_MOVE:
            if (mode == MAINWND_MODE_RENAMING) {
                return 1; // eat event -- don't repaint over renaming edit box!
            }
            else {
                //printf("MainWnd mouse move, x %4d, y %3d\n", x, y); fflush(0);
                if (Fl::event_inside(list_p)) { 
                    list_p->handle(e);
                }
                else if (    x > (tx + tw)   && x < lx 
                         &&  y > ty          && y < (ty + th)
                        ) 
                {
                    fl_cursor(FL_CURSOR_WE, FL_BLACK, FL_WHITE);
                }

                else {
                    if (list_p->dragging == 0) {
                        normal_cursor();
                    }
                }
            }
            break;

        case FL_DRAG:
            if (adjusting == ADJUST_TREE_WIDTH) {
                int x_adj = x - dragx;
                DirTreeWide = tw + x_adj;
                //printf("%s: x %d, dragx %d, tw %d, x_adj %d, new DirTreeWide %d, lw %d, new list wide %d\n", 
                //        __func__, x, dragx, tw, x_adj, DirTreeWide, lw, lw - x_adj);
                p->treegroup_p->resize(tx, ty, DirTreeWide, th);
                //p->list_p->resize(lx + x_adj, ty, lw - x_adj, th);
                redraw();
                dragx = x;
                dragy = y;
            }
            break;
            
        case FL_RELEASE:
            adjusting = 0;
            fl_cursor(FL_CURSOR_DEFAULT, FL_BLACK, FL_WHITE);
            if (mode == MAINWND_MODE_RENAMING) {
                end_renaming();
            }
            break;
            
        case FL_HIDE:
            return 1;

        case FL_SHOW:
            return 1;
            
        case FL_ENTER:
            GuiEnterEventTime_rsec = systemtime_as_real();
            //printf("Event is FL_ENTER at time %1.3lf\n", GuiEnterEventTime_rsec); fflush(0);
            
            return 0;

        default:
            if (w() != width && pathhist_menu_p->x() > 0) {
                //printf("path field x coord is %d, old width is %d\n",  path_inp_p->x(), path_inp_p->w());
                adjust_pathhist_menu_size(); 
                width = w();
                p->treegroup_p->resize(tx, ty, DirTreeWide, th);
                //p->listgroup_p->resize(lx, ty, lw, th);
                redraw();

            }
            ret = Fl_Window::handle(e);
            break;

    }
    return(ret);
}
#endif

void btnbar_quit_cb(void)
{
    Running = 0;
}

void perform_run_open(int choice) 
{
    char* filestr = sel_files_str();
    if (filestr) {
        char* cmdspec_str = get_cmd_spec_for_filetype(MainWnd_p->sel_fi_p->filetype, 
                               (choice == MI_RUN) ? SPECTYPE_RUN:SPECTYPE_OPEN);
        try_assoc_app(cmdspec_str, filestr);
    }
    else {
        perror("memory allocation");
    }
    free(filestr);
}

/*
void perform_run_or_browse(void)
{
    file_item* fi_p = MainWnd_p->sel_fi_p;
    if (fi_p) {
        if (    fi_p->is_dir() 
            &&  Fl::event_button() == FL_LEFT_MOUSE 
            &&  Fl::event_clicks() == 1) {
            if (!strcmp(fi_p->name, "..")) {
                if (MainWnd_p->seldir_fi_p->parent_fi_p) {
                    switch_directory(MainWnd_p->seldir_fi_p->parent_fi_p);
                }
                else {
                    switch_directory(Root_fi_p);
                }
            }
            else {
                switch_directory(fi_p);
            }
        }
        else {
            perform_run_open(MI_RUN); 
        }
    } 
}
*/
void perform_run_or_browse(void)
{
    file_item* fi_p = MainWnd_p->sel_fi_p;
    if (fi_p) {
        if (    Fl::event_button() == FL_LEFT_MOUSE 
            &&  Fl::event_clicks() == 1) {
            if (    fi_p->is_dir() ) {
                if (!strcmp(fi_p->name, "..")) {
                    if (MainWnd_p->seldir_fi_p->parent_fi_p) {
                        switch_directory(MainWnd_p->seldir_fi_p->parent_fi_p);
                    }
                    else {
                        switch_directory(Root_fi_p);
                    }
                }
                else {
                    switch_directory(fi_p);
                }
            }
            else {
                perform_run_open(MI_RUN); 
            }
        }
        else {
            perform_run_open(MI_RUN); 
        }
    } 
}

void btnbar_action1_cb(void)
{
    //printf("%s\n", __func__);
    perform_run_or_browse();
}

void btnbar_action2_cb(void)
{
    perform_run_open(MI_OPEN); 
}

void btnbar_props_cb(void)
{
    if (MainWnd_p->sel_fi_p) {
        MainWnd_p->fprops_p->show_file(MainWnd_p->sel_fi_p);
        MainWnd_p->fprops_p->show();
    }
}

void btnbar_showall_cb(void)
{
    MainWnd_p->showingall = MainWnd_p->showall_chk_p->value();
    sync_gui_with_filesystem();
}

void btnbar_sudo_cb(void)
{
    UseSudo = MainWnd_p->sudo_chk_p->value();
}

void btnbar_copy_cb(void)
{
    //printf("copy button callback... last_click_browser is %d\n",
    //        MainWnd_p->last_click_browser); fflush(0);
    if (MainWnd_p->last_click_browser == LASTCLICK_IN_LIST) {
        mark_file_items(MainWnd_p->list_p);
    }
}

void btnbar_paste_cb(void)
{
    perform_file_copy_or_move(COPY_OPER, MainWnd_p->seldir_fi_p);
}

void perform_file_trash(file_item* fi_p)
{
    char cmdstr[2048];
    char newname[640];
    strclr(newname);
    char* p = fi_p->fullpath;
    char* n = newname;
    while(*p) {
        if (*p == '/') {
            *n++ = '~';
            p++;
        }
        else {
            *n++ = *p++;
        }
    }
    *n = '\0';
    
    sprintf(cmdstr, "%smv -f \"%s\" \"%s/%s\"",
            UseSudo ? "sudo " : "", 
            fi_p->name, TrashbinPath_str, newname);
    //printf("Trying cmd '%s'\n", cmdstr); fflush(0);
    int err = system(cmdstr);
    if (err) {
        fl_alert("Command '%s' not successful", cmdstr);
    }
}

void btnbar_trash_cb(void)
{
    int ans = 2;
    int restore_mode = 0;
    File_Detail_List_Browser* lb_p = MainWnd_p->list_p;
    if (MainWnd_p->list_p->num_selected() > 0) {
        if (MainWnd_p->sel_fi_p) {
            if (*(MainWnd_p->sel_fi_p->name) == '~') {
                restore_mode = 1;
            }
        }
        char * sel_file_str = NULL;
        char filestr[1024] = {0};
        if (MainWnd_p->list_p->num_selected() > 2) {
            sprintf(filestr, "%d selected files", MainWnd_p->list_p->num_selected());
            sel_file_str = filestr;
        }
        else {
            sel_file_str = sel_files_str();
            sprintf(filestr, sel_file_str);
            free(sel_file_str);
        }
        ans = fl_choice("%s options for %s...", "cancel", 
                        restore_mode ? "restore from trash":"insert in trasbin", 
                        restore_mode ? NULL:"view trasbin", 
                        restore_mode ? "Restore" : "Trash", filestr);
        if (ans == 0) {
            return;
        }
        else if (ans == 1) {
            if (restore_mode) {
                FileMenuCB((Fl_Widget*)MainWnd_p->list_p, (void*)MI_RESTORE);
            }
            else {
                wait_cursor();
                int t = 0;
                MainWnd_p->init_progress(0, "file", "inserted in trash", 
                                         lb_p->num_selected());
                for (int i = 1; i <= MainWnd_p->list_p->size(); i++) {
                    if (lb_p->selected(i)) {
                        file_item* fi_p = (file_item*)lb_p->data(i);
                        lb_p->select(i, 0);  // unselect
                        perform_file_trash(fi_p);
                        t++;
                        MainWnd_p->update_progress(t);
                        DELAY_FOR_TESTING;
                    }
                }
                MainWnd_p->complete_progress();
            }
        }
        Fl::wait(0.2);
        normal_cursor();
    }
    if (ans == 2) {
        build_tree_for_branch(TrashbinPath_str, Root_fi_p, TrashbinPath_str, &(MainWnd_p->seldir_fi_p));
        sync_gui_with_filesystem();
    }
}

void btnbar_delete_cb(void)
{
    perform_file_delete(DELETE_SELECTED);
}

void btnbar_loc1_cb(void)
{
    if (Loc1_fi_p) {
        switch_directory(Loc1_fi_p);
    }
    else {
        fl_message("Shortcut not available.\n");
    }
}

void btnbar_loc2_cb(void)
{
    if (Loc2_fi_p) {
        switch_directory(Loc2_fi_p);
    }
    else {
		if (!strcmp(Loc2Label_str, "TCE")) {
			fl_message("No permanent TCE drive defined.\nConsider using AppBrowser's 'Set' button to setup TCE drive\nand restart Fluff.\n");
		}
		else {
			fl_message("Shortcut not available.\n");
		}
    }
}

void btnbar_about_cb(void)
{
    fl_message(About_text, APP_VER);
}

void btnbar_go_up_cb(void)
{
	if (MainWnd_p->seldir_fi_p && MainWnd_p->seldir_fi_p->parent_fi_p) {
		switch_directory(MainWnd_p->seldir_fi_p->parent_fi_p);
	}
}

void btnbar_go_bk_cb(void)
{
	int i = VisitedDirCur - 1;
	if (i < 0) i = 0;
    switch_directory(VisitedDir[i]);	
}

void btnbar_go_fw_cb(void)
{
	int i = VisitedDirCur + 1;
	if (i >= VisitedDirs) i = (VisitedDirs - 1);
    switch_directory(VisitedDir[i]);	
}

void btnbar_help_cb(void)
{
    Fl_Help_Dialog hd;
    hd.load("/usr/local/share/doc/fluff/fluff_help.htm");
    hd.textsize(14);
    hd.show();
    while (hd.visible()) {
        Fl::wait(1);
    } 
}

void path_hist_menu_cb(void) {
    int item = MainWnd_p->pathhist_menu_p->value();
    int i = VisitedDirs - item - 1;
    //printf("item %d %s selected\n", item, MainWnd_p->pathhist_menu_p->text());
    //printf("Go to visited directory %d %s\n", i, VisitedDir[i]->fullpath);
    switch_directory(VisitedDir[i]);
}

void Fluff_Window::init_progress(int by_bytes, const char* unit_str, const char* oper_str, int max)
{
    progress_by_bytes = by_bytes;
    strnzcpy(prog_unit_str, unit_str, MAXPROGUNITLEN); 
    strnzcpy(prog_oper_str, oper_str, MAXPROGOPERLEN); 
    if (max > 2000) {
        prog_divider = (float)max / 1000.0;
    }
    else {
        prog_divider = 1.0;
    }
    progress_p->maximum((int)((float)max / prog_divider));
    prog_max_amount = max;
    update_progress(PROG_BY_FILE);
    progress_p->show();
}

void Fluff_Window::update_progress(int cur_amount)
{
    int prog_val = (int)((float)cur_amount/prog_divider);
    progress_p->value(prog_val);
    sprintf(prog_label_str, "%d of %d %s%s %s", 
            cur_amount, prog_max_amount, prog_unit_str, 
            (cur_amount == 1) ? "":"s", prog_oper_str);
    progress_p->label(prog_label_str);
    progress_p->redraw();
    Fl::check();
}

void Fluff_Window::complete_progress(void)
{
    progress_p->hide();
}

void Fluff_Window::adjust_pathhist_menu_size(void) 
{
    int fw = w() - pathhist_menu_p->x() - 4;
    pathhist_menu_p->size(fw, pathhist_menu_p->h());
    if (fw < 20) {
        pathhist_menu_p->label("");
        pathhist_menu_p->hide();
    }
    else if (fw < 60) {
        pathhist_menu_p->label("");
        pathhist_menu_p->show();
    }
    else {
        if (MainWnd_p->seldir_fi_p) {
            pathhist_menu_p->label(MainWnd_p->seldir_fi_p->fullpath);
        }
        pathhist_menu_p->show();
    }
}

void Fluff_Window::setup(void) 
{
    begin();

    Fl_Group* v = new Fl_Group(0,0,w()-2, h()-2);
    v->type(Fl_Pack::VERTICAL);

    //---        
    btnbarpack_p = new Fl_Pack(0,4,w()-2, 28);
    btnbarpack_p->type(Fl_Pack::HORIZONTAL);
    btnbarpack_p->spacing(4);
    btnbarpack_p->begin();
    {
        Fl_Box* o = new Fl_Box(0,0,0,14);  //spacer
        o->box(FL_NO_BOX);
    }
    //~ quit_btn_p = new Fl_Button(0, 0, 34, 14, "Quit");
    //~ quit_btn_p->labelsize(12);
    //~ quit_btn_p->callback((Fl_Callback*)btnbar_quit_cb);

     loc1_btn_p = new Fl_Button(0, 0, 38, 14, Loc1Label_str);
    loc1_btn_p->labelsize(12);
    loc1_btn_p->callback((Fl_Callback*)btnbar_loc1_cb);
    loc2_btn_p = new Fl_Button(0, 0, 38, 14, Loc2Label_str);
    loc2_btn_p->labelsize(12);
    loc2_btn_p->callback((Fl_Callback*)btnbar_loc2_cb);
    {
        Fl_Box* o = new Fl_Box(0,0,12,16);  //spacer
        o->box(FL_NO_BOX);
    }
    about_btn_p = new Fl_Button(0, 0, 18, 16, "A");
    about_btn_p->labelsize(12);
    about_btn_p->callback((Fl_Callback*)btnbar_about_cb);
    about_btn_p->tooltip("About");
    
    help_btn_p = new Fl_Button(0, 0, 36, 16, "Help");
    help_btn_p->labelsize(12);
    help_btn_p->callback((Fl_Callback*)btnbar_help_cb);
    {
        Fl_Box* o = new Fl_Box(0,0,12,16);  //spacer
        o->box(FL_NO_BOX);
    }
   
    run_btn_p = new Fl_Button(0, 0, 50, 14, "---");
    run_btn_p->labelsize(12);
    run_btn_p->callback((Fl_Callback*)btnbar_action1_cb);

    open_btn_p = new Fl_Button(0, 0, 50, 14, "---");
    open_btn_p->labelsize(12);
    open_btn_p->callback((Fl_Callback*)btnbar_action2_cb);

    props_btn_p = new Fl_Button(0, 0, 40, 14, "Props.");
    props_btn_p->labelsize(12);
    props_btn_p->callback((Fl_Callback*)btnbar_props_cb);
    
    {
    Fl_Pack* p = new Fl_Pack(0,0, 68, 14);
    p->begin();
    showall_chk_p = new Fl_Check_Button(0, 0, 68, 14, "show all"); 
    showall_chk_p->value(showingall);
    showall_chk_p->labelsize(12);
    showall_chk_p->callback((Fl_Callback*)btnbar_showall_cb);
    if (AllowUseSudo) {
        sudo_chk_p = new Fl_Check_Button(0, 0, 68, 12, "use sudo"); 
        sudo_chk_p->value(0);
        sudo_chk_p->labelsize(12);
        sudo_chk_p->callback((Fl_Callback*)btnbar_sudo_cb);
    }
    else {
        p->resizable(showall_chk_p);
    }
    p->end();
    }
    copy_btn_p = new Fl_Button(0, 0, 38, 14, "Copy");
    copy_btn_p->labelsize(12);
    copy_btn_p->callback((Fl_Callback*)btnbar_copy_cb);

    paste_btn_p = new Fl_Button(0, 0, 38, 14, "Paste");
    paste_btn_p->labelsize(12);
    paste_btn_p->callback((Fl_Callback*)btnbar_paste_cb);
    
    trash_btn_p = new Fl_Button(0, 0, 50, 14, "Trash");
    trash_btn_p->labelsize(12);
    trash_btn_p->callback((Fl_Callback*)btnbar_trash_cb);

    delete_btn_p = new Fl_Button(0, 0, 40, 14, "Delete");
    delete_btn_p->labelsize(12);
    delete_btn_p->callback((Fl_Callback*)btnbar_delete_cb);

    {
        Fl_Box* o = new Fl_Box(0,0,12,16);  //spacer
        o->box(FL_NO_BOX);
    }
    go_up_btn_p = new Fl_Button(0, 0, 18, 16, "@-38UpArrow");
    go_up_btn_p->labelsize(12);
    go_up_btn_p->callback((Fl_Callback*)btnbar_go_up_cb);
    go_up_btn_p->tooltip("Parent dir");

    go_bk_btn_p = new Fl_Button(0, 0, 18, 16, "@<");
    go_bk_btn_p->labelsize(12);
    go_bk_btn_p->callback((Fl_Callback*)btnbar_go_bk_cb);
    go_bk_btn_p->tooltip("Back");

    go_fw_btn_p = new Fl_Button(0, 0, 18, 16, "@>");
    go_fw_btn_p->labelsize(12);
    go_fw_btn_p->callback((Fl_Callback*)btnbar_go_fw_cb);
    go_fw_btn_p->tooltip("Forward");    
    {
        Fl_Box* o = new Fl_Box(0,0,4,16);  //spacer
        o->box(FL_NO_BOX);
    }
    
    path_lbl_p = new Fl_Box(0, 0, 24, 20, "Path");
    path_lbl_p->labelsize(12);
    
    pathhist_menu_p = new Fl_Menu_Button(0, 0, 150, 20);
    pathhist_menu_p->labelsize(12);
    pathhist_menu_p->callback((Fl_Callback*)path_hist_menu_cb);
    pathhist_menu_p->box(FL_DOWN_BOX);
    pathhist_menu_p->color(FL_BACKGROUND2_COLOR);
    pathhist_menu_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    
    btnbarpack_p->resizable(pathhist_menu_p);
    btnbarpack_p->end();

    //---
    int brows_h = h() - (4 + 28 + 4 + 4 + 16);
    browspack_p  = new Fl_Pack(0, 4 + 28 + 4, w() - 2, brows_h);
    browspack_p->type(Fl_Pack::HORIZONTAL);
    browspack_p->begin();
    
    treegroup_p = new Fl_Group(0, 0, DirTreeWide, brows_h);
    treegroup_p->begin();
    tree_p  = new Dir_Tree_Browser(0, 0, DirTreeWide, brows_h);
    Fl_DND_Box* o = new Fl_DND_Box(0, 0, DirTreeWide, brows_h);
    o->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    o->callback(dirtree_dnd_cb);
    treegroup_p->end();

    Fl_Box* b = new Fl_Box(0, 0, 6, h() - 40);    // spacer
    b->box(FL_FLAT_BOX);

    listgroup_p = new Fl_Group(0, 0, w() - 6 - DirTreeWide, brows_h);
    listgroup_p->begin();
    list_p  = new File_Detail_List_Browser( 0, 0, 
                                    w() - 6 - DirTreeWide, brows_h);
    list_p->column_widths(ColWidths);
    o = new Fl_DND_Box(0, 0, w() - 6 - DirTreeWide, brows_h);
    o->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
    o->callback(filelist_dnd_cb);
    listgroup_p->end();
    listgroup_p->resizable(list_p);
    browspack_p->resizable(listgroup_p);
    browspack_p->end();

    tree_p->callback((Fl_Callback0*)dirtree_change_cb);
    list_p->callback((Fl_Callback0*)filelist_change_cb);
    list_p->when(FL_WHEN_CHANGED);

    //---
    status_grp_p = new Fl_Group(0, h() - 18, w()-2, 16);
    status_grp_p->begin();
    status_grp_p->type(Fl_Pack::HORIZONTAL);
    mainwnd_status_lbl_p = new Fl_Box(0, h() - 18, 300, 16, "Dir Info");
    mainwnd_status_lbl_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    mainwnd_status_lbl_p->labelsize(12);
    progress_p = new Fl_Progress(316, h() - 18, w() - 324, 16, "Progress");
    progress_p->minimum(0);
    progress_p->maximum(1000);
    progress_p->hide();
    progress_by_bytes = 1;
    status_grp_p->resizable(progress_p);
    status_grp_p->end();

    //---
    v->resizable(browspack_p);
    v->end();
    resizable(v);
    end();
   
    fprops_p =  new File_Props_Window(400, 200, 440, 350, Root_fi_p);
    fprops_p->setup();
}

File_Props_Window::File_Props_Window(int x, int y, int w, int h, file_item* fileitem_p)    
    : Fl_Window(x, y, w, h),
    changed(0),
    fi_p(fileitem_p),       window_pack_p(NULL),
    perm_pack_p(NULL),      perm_lbl_p(NULL),
    
    ownperm_pack_p(NULL),
    ownperm_lbl_p(NULL),    ownperm_r_chk_p(NULL),
    ownperm_w_chk_p(NULL),  ownperm_x_chk_p(NULL),

    grpperm_pack_p(NULL),   grpperm_lbl_p(NULL),
    grpperm_r_chk_p(NULL),  grpperm_w_chk_p(NULL),
    grpperm_x_chk_p(NULL),

    othperm_pack_p(NULL),   othperm_lbl_p(NULL),
    othperm_r_chk_p(NULL),  othperm_w_chk_p(NULL),
    othperm_x_chk_p(NULL),  filename_lbl_p(NULL),
    filename_inp_p(NULL),   filepath_lbl_p(NULL),
    filesize_lbl_p(NULL),   fileowner_lbl_p(NULL),
    mdate_lbl_p(NULL),      cdate_lbl_p(NULL),
    adate_lbl_p(NULL),
    
    button_pack_p(NULL),    close_btn_p(NULL),
    revert_btn_p(NULL),     apply_btn_p(NULL)
{
    show_file(fileitem_p);
}

File_Props_Window::~File_Props_Window()   
{
    delete window_pack_p;
    delete perm_pack_p;  
    delete perm_lbl_p;    
    delete ownperm_pack_p;
    delete ownperm_lbl_p;    
    delete ownperm_r_chk_p;
    delete ownperm_w_chk_p;  
    delete ownperm_x_chk_p;

    delete grpperm_pack_p;   
    delete grpperm_lbl_p;
    delete grpperm_r_chk_p;  
    delete grpperm_w_chk_p;
    delete grpperm_x_chk_p;

    delete othperm_pack_p;   
    delete othperm_lbl_p;
    delete othperm_r_chk_p;  
    delete othperm_w_chk_p;
    delete othperm_x_chk_p;  
    delete filename_lbl_p;
    delete filename_inp_p;   
    delete filepath_lbl_p;
    delete filesize_lbl_p;   
    delete fileowner_lbl_p;
    delete mdate_lbl_p;      
    delete cdate_lbl_p;
    delete adate_lbl_p;
    
    delete button_pack_p;    
    delete close_btn_p;
    delete revert_btn_p;     
    delete apply_btn_p;
    
}

void File_Props_Window::close_btn_cb(Fl_Widget* thewidget_p, void* theparent_p)
{
    //printf("FILE PROPERTIES: Close callback\n"); fflush(0);
    File_Props_Window* fpw_p = (File_Props_Window*)theparent_p;
    fpw_p->hide();
}


void File_Props_Window::update_buttons(void) 
{
    int name_changed = 0;
    if (strncmp(fi_p->name, filename_inp_p->value(), MAXFNAMELEN)){
        //printf("    name DID change.\n");
        name_changed = 1;
    }

    int owner_changed = 0;
    if (strncmp(fi_p->ownstr, fileowner_inp_p->value(), 2*MAXNINAME + 8)){
        //printf("    owner DID change.\n");
        owner_changed = 1;
    }
                      
    if (changed || owner_changed || name_changed) {
        revert_btn_p->activate();
        apply_btn_p->activate();
    }
    else {
        revert_btn_p->deactivate();
        apply_btn_p->deactivate();
    }
}

void File_Props_Window::revert_btn_cb(Fl_Widget* thewidget_p, void* theparent_p)
{
    //printf("FILE PROPERTIES: Revert callback\n"); fflush(0);
    File_Props_Window* fpw_p = (File_Props_Window*)theparent_p;
    fpw_p->show_file(fpw_p->fi_p);
}

void File_Props_Window::apply_btn_cb(Fl_Widget* thewidget_p, void* theparent_p)
{
    int err;
    File_Props_Window* fpw_p = (File_Props_Window*)theparent_p;
    if (fpw_p->changed & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH)) {
        char chmod_cmd[1024];
        sprintf(chmod_cmd, "%schmod %o %s\n", UseSudo ? "sudo " : "", 
                (fpw_p->changed ^ fpw_p->fi_p->status.st_mode) & (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH), 
                fpw_p->fi_p->fullpath);
        err = system(chmod_cmd);
        if (err) {
            fl_alert("Permissions change error.  System returned code %d", err);
            return;
        }
    }
    if (strncmp(fpw_p->fi_p->name, fpw_p->filename_inp_p->value(), 128)){
        //printf("... will rename %s to %s now.\n", fpw_p->fi_p->name, fpw_p->filename_inp_p->value());
        conditionally_rename(fpw_p->fi_p, fpw_p->filename_inp_p->value());
    }
    if (strncmp(fpw_p->fi_p->ownstr, fpw_p->fileowner_inp_p->value(), 128)){
        char cmd[512];
        sprintf(cmd, "%schown %s %s", UseSudo ? "sudo " : "",
                fpw_p->fileowner_inp_p->value(), fpw_p->fi_p->name);
        err = system(cmd);
        if (err) {
            fl_alert("Owner change was not successful.\n");
            return;
        }
    }
    fpw_p->fi_p->fetch_status();
    fpw_p->show_file(fpw_p->fi_p);
}

void File_Props_Window::filetype_btn_cb(Fl_Widget* thewidget_p, void* theparent_p)
{
    File_Props_Window* fpw_p = (File_Props_Window*)theparent_p;
    int filetype = fpw_p->fi_p->filetype;
    
    if (filetype == FT_TEXT) {
        const char* ans = fl_input( "Enter a short descriptive name"
                                    " for this type of file");
        if (ans && *ans) {
            Manage_Filetypes_Window* mftw_p = ManageFiletypesWnd_p;
            strnzcpy(mftw_p->new_filetype_name_str, ans, MAXFTNAMELEN);
            
            const char* ext_str = fpw_p->fi_p->extension();
            if (ext_str) {
                strnzcpy(mftw_p->new_hint.pat, ext_str, MAXPATLEN);
                mftw_p->new_hint.patlen = strlen(ext_str);
                mftw_p->new_hint.hintmethod = HINTMETHOD_EXTENSION;
            }
            else {
                strnzcpy(mftw_p->new_hint.pat, "something", MAXPATLEN);
                mftw_p->new_hint.patlen = strlen("something");
                mftw_p->new_hint.hintmethod = HINTMETHOD_STRING;
            }
            mftw_p->new_hint.offset = 0;
            mftw_p->new_info_ready = 1;
            
            ManageFiletypesWnd_p->present();
        }
        else {        
            //printf("Selected filetype is %d (%s)\n", filetype, 
            //        FiletypeName_str[filetype]); fflush(0);
            ManageFiletypesWnd_p->present(filetype);
        }
    }
    else {        
        //printf("Selected filetype is %d (%s)\n", filetype, 
        //        FiletypeName_str[filetype]); fflush(0);
        ManageFiletypesWnd_p->present(filetype);
    }
}

/*
void File_Props_Window::visitlink_btn_cb(Fl_Widget* thewidget_p, void* theparent_p)
{
    File_Props_Window* fpw_p = (File_Props_Window*)theparent_p;
    printf("visitlink_btn_cb; MainWnd_p->path is %s; link path is %s \n", MainWnd_p->path, fpw_p->linkpath_dsp_p->label()); fflush(0);
    
    char abs_path[MAXFFULLPATHLEN];
    char fulllinkpath[MAXFFULLPATHLEN];
    char dir_path[MAXFFULLPATHLEN];
    char item_name[MAXFNAMELEN];
    strnzcpy(fulllinkpath, fpw_p->linkpath_dsp_p->label(), MAXFFULLPATHLEN);
    if (fulllinkpath[0] != '/') {
		strnzcpy(fulllinkpath, MainWnd_p->path, MAXFFULLPATHLEN);	
		strncat(fulllinkpath, "/", MAXFFULLPATHLEN-1);
		strncat(fulllinkpath, fpw_p->linkpath_dsp_p->label(), MAXFFULLPATHLEN-1);
	}
	char * ap = realpath(fulllinkpath, abs_path);
	printf("Resolved absolute path is '%s'\n", ap);

	strnzcpy(dir_path, abs_path, MAXFFULLPATHLEN);
	//trim off last name to make a directory name
	char *q = dir_path + strlen(dir_path) - 1;
	char *n = item_name + MAXFNAMELEN - 1;
	*n = '\0';
	n--;
	while (*q != '/') {
		*n = *q;
		n--;
		*q = '\0';
		q--;
	}
	if (strlen(q) > 1) {
		*q = '\0';
	}
	n++; // adjust back to start of item name
	
	struct stat64 trgstat;
	if (stat64(dir_path, &trgstat)) {
		printf("Link path '%s' not found!\n", dir_path);
		return;
	}
	if (stat64(abs_path, &trgstat)) {
		printf("Link target '%s' not found!\n", abs_path);
		return;
	}
	//int tgt_is_dir = S_ISDIR(trgstat.st_mode);

    file_item* linkdir_fi_p;
    build_tree_for_branch(dir_path, Root_fi_p, dir_path, &linkdir_fi_p);
    if (linkdir_fi_p) {
        switch_directory(linkdir_fi_p);
        // Select the target file
        file_item* file_p;
        int found = 0;
        for (int i = 2; i <= MainWnd_p->list_p->size(); i++) {
             file_p = (file_item*)(MainWnd_p->list_p->data(i));
            //printf(" - - List item is '%s'\n", file_p->fullpath);
            if (!strcmp(abs_path, file_p->fullpath)) {
                MainWnd_p->list_p->select(i, 1);
                MainWnd_p->list_p->middleline(i);
                found = 1;
                break;
            }
        }
		if (found) {
			fpw_p->hide();
		}
		delete linkdir_fi_p;
    }
    else {
        fl_alert("Link target directory '%s' not found!", fulllinkpath);
    }
}
*/

void File_Props_Window::name_inp_cb(Fl_Widget*w_p, void* v_p)
{
    MainWnd_p->fprops_p->update_buttons();
}

void File_Props_Window::fileowner_inp_cb(Fl_Widget*w_p, void* v_p)
{
    MainWnd_p->fprops_p->update_buttons();
}

void File_Props_Window::perm_chk_cb(Fl_Widget*w_p, void* v_p)
{
    //printf("perm_chk_cb"); fflush(0);
    Fl_Check_Button* perm_chk_p = (Fl_Check_Button*)w_p;
    long perm_mask = (long)v_p;
    file_item* fi_p = MainWnd_p->fprops_p->fi_p;
    
    if (perm_chk_p->value() != (int)(fi_p->status.st_mode & perm_mask))  {
        MainWnd_p->fprops_p->changed |= perm_mask;
        //printf("  checkbox differs from file mode, changed is now 0x%04X\n", MainWnd_p->fprops_p->changed);
    }
    else {
        MainWnd_p->fprops_p->changed &= ~perm_mask;
        //printf("  checkbox same as file mode, changed is now 0x%04X\n", MainWnd_p->fprops_p->changed);
    }

    //printf("perm_chk_cb() widget '%s', val %d, mask 0x%04X, filemode 0x%04X, changed is 0x%04X\n", perm_chk_p->label(), perm_chk_p->value(), perm_mask, fi_p->status.st_mode, MainWnd_p->fprops_p->changed); fflush(0);
    MainWnd_p->fprops_p->update_buttons();
}

int File_Props_Window::handle(int e) {
    int ret = 0;
    //printf("File_Props_Window event %d\n", e); fflush(0);
    switch ( e ) {
        case FL_PUSH:
            if ( Fl::event_button() == FL_LEFT_MOUSE ) {
                if (!Fl_Window::handle(e)) {
                    return 1;
                }
            }
            else if (Fl::event_button() == FL_RIGHT_MOUSE ) {
                return(1);          // (tells caller we handled this event)
            }
            else {
                ret = Fl_Window::handle(e);
                return(ret);          
            }
            break;

        case FL_SHOW:
            return 1;

        default:
            ret = Fl_Window::handle(e);
            break;

    }
    return(ret);
}
void File_Props_Window::setup(void)
{
    int y = 10;
    int x0 = 10;
    int x1 = 100;
    int x2 = 160;
    int w1 = x1 - x0 - 16;
    int w2 = x2 - x0 - 16;
    int fw1 = w() - x1 - 10;
    int fw2 = w() - x2 - 10;
    begin();    

    filename_lbl_p = new Fl_Box( x0, y, w1, 24, "Name");
    filename_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    filename_inp_p = new Fl_Input( x1, y, fw1, 24);
    filename_inp_p->callback(name_inp_cb, (void *)0);
    filename_inp_p->when(FL_WHEN_CHANGED);
    y += 28;

    filetype_lbl_p = new Fl_Box( x0, y, w1, 16, "File type:");
    filetype_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    filetype_dsp_p = new Fl_Box( x1, y, fw1, 16);
    filetype_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    filetype_btn_p = new Fl_Button( x1 + fw1 - 50, y, 50, 22, "Type...");
    filetype_btn_p->callback(filetype_btn_cb, (void*)this);
    y += 20;

    filepath_lbl_p = new Fl_Box( x0, y, w1, 16, "Path:");
    filepath_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    filepath_dsp_p = new Fl_Box( x1, y, fw1, 16, "(some path)");
    filepath_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    filepath_dsp_p->labelsize(10);
    y += 20;

    linkpath_lbl_p = new Fl_Box( x0, y, w1, 16, "Link path:");
    linkpath_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    linkpath_lbl_p->deactivate();
    linkpath_dsp_p = new Fl_Box( x1, y, fw1 - 60, 16);
    linkpath_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    linkpath_dsp_p->labelsize(10);
    linkpath_dsp_p->deactivate();
    //visitlink_btn_p = new Fl_Button( x1 + fw1 - 50, y, 50, 22, "Visit...");
    //visitlink_btn_p->callback(visitlink_btn_cb, (void*)this);
    //visitlink_btn_p->deactivate();
    y += 20;

    filesize_lbl_p = new Fl_Box( x0, y, w1, 16, "Size:");
    filesize_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    filesize_dsp_p = new Fl_Box( x1, y, fw1, 16);
    filesize_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += 20;
    
    dir_content_dsp_p = new Fl_Box( x1, y, fw1, 16);
    dir_content_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    dir_content_dsp_p->labelsize(10);
    y += 20;   
    
    fileowner_lbl_p = new Fl_Box( x0, y, w1, 24, "Owner:Grp:");
    fileowner_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    fileowner_inp_p = new Fl_Input( x1, y, fw1, 24);
    fileowner_inp_p->callback(fileowner_inp_cb, (void *)0);
    fileowner_inp_p->when(FL_WHEN_CHANGED);
    y += 28;
    
    mdate_lbl_p = new Fl_Box( x0, y, w2, 16, "Modification date:"); 
    mdate_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    mdate_dsp_p = new Fl_Box( x2, y, fw2, 16); 
    mdate_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += 20;

    cdate_lbl_p = new Fl_Box( x0, y, w2, 16, "Creation date:"); 
    cdate_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    cdate_dsp_p = new Fl_Box( x2, y, fw2, 16); 
    cdate_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += 20;

    adate_lbl_p = new Fl_Box( x0, y, w2, 16, "Access date:"); 
    adate_lbl_p->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);
    adate_dsp_p = new Fl_Box( x2, y, fw2, 16); 
    adate_dsp_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    y += 20;

    perm_lbl_p = new Fl_Box(x0, y, w()/5 - 10, 16, "Permissions");
    perm_lbl_p->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    ownperm_pack_p = new Fl_Pack(x0 + 10 + w()*1/5, y, w()/5, 72);
    ownperm_pack_p->begin();
    ownperm_lbl_p = new Fl_Box( 0, 0, w()/5, 16);
    ownperm_lbl_p->label("Owner");
    ownperm_r_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    ownperm_r_chk_p->label("read");
    ownperm_r_chk_p->callback(perm_chk_cb, (void*)S_IRUSR);
    ownperm_w_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    ownperm_w_chk_p->label("write");
    ownperm_w_chk_p->callback(perm_chk_cb, (void*)S_IWUSR);
    ownperm_x_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    ownperm_x_chk_p->label("execute");
    ownperm_x_chk_p->callback(perm_chk_cb, (void*)S_IXUSR);
    ownperm_pack_p->end();

    grpperm_pack_p = new Fl_Pack(x0 + 10 +  w() * 2 / 5, y, w()/5, 72);
    grpperm_pack_p->begin();
    grpperm_lbl_p = new Fl_Box( 0, 0, w()/5, 16);
    grpperm_lbl_p->label("Group");
    grpperm_r_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    grpperm_r_chk_p->label("read");
    grpperm_r_chk_p->callback(perm_chk_cb, (void*)S_IRGRP);
    grpperm_w_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    grpperm_w_chk_p->label("write");
    grpperm_w_chk_p->callback(perm_chk_cb, (void*)S_IWGRP);
    grpperm_x_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    grpperm_x_chk_p->label("execute"); 
    grpperm_x_chk_p->callback(perm_chk_cb, (void*)S_IXGRP);
    grpperm_pack_p->end();

    othperm_pack_p = new Fl_Pack(x0 + 10 +  w() * 3 / 5, y, w()/5, 72);
    othperm_pack_p->begin();
    othperm_lbl_p = new Fl_Box( 0, 0, w()/5, 16);
    othperm_lbl_p->label("Others");
    othperm_r_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    othperm_r_chk_p->label("read");
    othperm_r_chk_p->callback(perm_chk_cb, (void*)S_IROTH);
    othperm_w_chk_p = new Fl_Check_Button( 0, 0, w()/5, 16);
    othperm_w_chk_p->label("write");
    othperm_w_chk_p->callback(perm_chk_cb, (void*)S_IWOTH);
    othperm_x_chk_p =new Fl_Check_Button( 0, 0, w()/5, 16);
    othperm_x_chk_p->label("execute");
    othperm_x_chk_p->callback(perm_chk_cb, (void*)S_IXOTH);
    othperm_pack_p->end();
    y += 80;
    
    button_pack_p = new Fl_Pack((w() - 392)/2, y, 392, 24);
    button_pack_p->begin();
    button_pack_p->type(Fl_Pack::HORIZONTAL);
    button_pack_p->spacing(16);
    apply_btn_p = new Fl_Button( 0, 0, 120, 24, "Apply");
    apply_btn_p->callback(apply_btn_cb, (void*)this);
    revert_btn_p = new Fl_Button( 0, 0, 120, 24, "Undo changes");
    revert_btn_p->callback(revert_btn_cb, (void*)this);
    close_btn_p = new Fl_Button( 0, 0, 120, 24, "Close");
    close_btn_p->callback(close_btn_cb, (void*)this);
    button_pack_p->end();
    
    label("File Properties");
    end();
}

void File_Props_Window::show_file(file_item* fileitem_p)
{
    if (!fileitem_p) return;
    
    char scratch[1024];
    char trg_path[1024];
    fi_p = fileitem_p;

    //printf("File_Props_Window::show_file() called for file %s\n",  fileitem_p->fullpath); fflush(0);
    sprintf(scratch, "' %s '  Properties", fileitem_p->name);
    copy_label(scratch);
    //printf("Title is set\n"); fflush(0);
    changed = 0;
    apply_btn_p->deactivate();
    revert_btn_p->deactivate();
    filename_inp_p->value(fi_p->name);
    filetype_dsp_p->copy_label(fi_p->typestr);
    filepath_dsp_p->copy_label(fi_p->path);
    //printf("Will check for link target\n"); fflush(0);
    if (fi_p->is_link()) {
        int trglen = readlink(fi_p->fullpath, trg_path, 1023);
        if (trglen <= 0) {
			if (trglen < 0) {
				perror("readlink");
			}
			// link but info is missing
			linkpath_dsp_p->label("");
			linkpath_lbl_p->deactivate();
			linkpath_dsp_p->deactivate();
			//visitlink_btn_p->deactivate();
		}
        else {
            trg_path[trglen] = '\0';
            linkpath_dsp_p->copy_label(trg_path);
			linkpath_lbl_p->activate();
			linkpath_dsp_p->activate();
			//visitlink_btn_p->activate();
        }
    }
    else {
        //printf("Not link, disabling certain fields\n");
        linkpath_dsp_p->label("");
        linkpath_lbl_p->deactivate();
        linkpath_dsp_p->deactivate();
        //visitlink_btn_p->deactivate();
    }

    if (fi_p->status.st_size < (1536)) {
        sprintf(scratch, "%d bytes", (int)fi_p->status.st_size);
    }
    else {
        sprintf(scratch, "%s (%lld bytes)", fi_p->sizestr, (long long)fi_p->status.st_size);
    }
    filesize_dsp_p->copy_label(scratch);
    //printf("Set up size info\n"); fflush(0);
    
    fileowner_inp_p->value(fi_p->ownstr);
    //printf("Set up owner info\n"); fflush(0);
 
    mdate_dsp_p->copy_label(ctime(& fi_p->status.st_mtime));
    cdate_dsp_p->copy_label(ctime(& fi_p->status.st_ctime));
    adate_dsp_p->copy_label(ctime(& fi_p->status.st_atime));
    //printf("Set up date info\n"); fflush(0);

    ownperm_r_chk_p->value(fi_p->status.st_mode & S_IRUSR);
    ownperm_w_chk_p->value(fi_p->status.st_mode & S_IWUSR);
    ownperm_x_chk_p->value(fi_p->status.st_mode & S_IXUSR);

    grpperm_r_chk_p->value(fi_p->status.st_mode & S_IRGRP);
    grpperm_w_chk_p->value(fi_p->status.st_mode & S_IWGRP);
    grpperm_x_chk_p->value(fi_p->status.st_mode & S_IXGRP);

    othperm_r_chk_p->value(fi_p->status.st_mode & S_IROTH);
    othperm_w_chk_p->value(fi_p->status.st_mode & S_IWOTH);
    othperm_x_chk_p->value(fi_p->status.st_mode & S_IXOTH);
    //printf("Set up permission flags\n"); fflush(0);
    
    if (fi_p->is_dir()) {
        char diskspace_str[32];
        char fc_cmd_str[1024];
        char filecount_str[128];
        char dirstatus_str[128];
        int files = 0;
        int incomplete = 0;
        wait_cursor();
        sprintf(fc_cmd_str, "fluff_fc.sh %s%s", 
                fi_p->fullpath, FollowLinks ? " -follow" : "");
                
        FILE* fc_fp = popen(fc_cmd_str, "r");
        if (fc_fp) {
            fgets(filecount_str, 128, fc_fp);
            pclose(fc_fp);
            //printf("file count command returns: '%s'\n",  
            //        filecount_str); fflush(0);
            sscanf(filecount_str, "incomplete?: %d; filecount: %d; space: %s", 
                    &incomplete, &files, diskspace_str);
            char *p = diskspace_str;
            while(*p && *p != ' ') p++;  
            if (*p == ' ') *p = '\0';   // clip string at first space
            sprintf(dirstatus_str, "Dir contents: %s%d file%s, about %s  (links %s)",
                    incomplete ? "? at least " : "",
                    files, (files == 1)?"":"s", 
                    diskspace_str, FollowLinks ? "followed" : "omitted" );
            //printf("dir stats: '%s'\n", dirstatus_str); fflush(0);
        }
        else {
            printf("popen() failed for '%s'\n", fc_cmd_str);
            return;
        }
        dir_content_dsp_p->copy_label(dirstatus_str);
        normal_cursor();
    }
    else {
        dir_content_dsp_p->copy_label("");
    }
}


static void MainMenuCB(Fl_Widget* window_p, void *userdata) 
{
    long choice = (long)userdata;
    Fluff_Window* mainwnd_p = (Fluff_Window *)window_p;
    
    switch (choice) {
        case MI_ABOUT:
            btnbar_about_cb();
            break;
        case MI_UP_DIR:
            btnbar_go_up_cb();
            break;
        case MI_BK_DIR:
            btnbar_go_bk_cb();
            break;
        case MI_FW_DIR:
            btnbar_go_fw_cb();
            break;
        case MI_HELP:
            btnbar_help_cb();
            break;
        case MI_FILETYPES:
            ManageFiletypesWnd_p->present();
            break;
        case MI_QUIT:
            Running = 0;
            break;
            
        default:
            break;
    }
    mainwnd_p->set_titles();

}

char* sel_files_str(void) 
{
    int bufsize = 128;
    char* buf = (char*)malloc(bufsize);
    char* p = buf;
    if (!p) {
        perror("Memory allocation");
        return p;
    }
    *p = '\0';
    Fl_Multi_Browser* mb_p = MainWnd_p->list_p;
    if (mb_p) {
        int i, buflen, fplen;
        buflen = 0;
        for (i = 1; i <= mb_p->size(); i++) {
            if (mb_p->selected(i)) {
                file_item* fi_p = (file_item*)mb_p->data(i);
                fplen = strlen(fi_p->name);
                if (buflen + fplen + 4 > bufsize) {
                    int newsize = bufsize * 2;
                    char* newbuf = (char*)malloc(newsize);
                    if (newbuf) {
                        //printf("Allocated %d byte buffer\n", newsize); fflush(0); 
                        memcpy(newbuf, buf, bufsize);
                        buf = newbuf;
                        bufsize = newsize;
                        p = buf + strlen(buf);
                    }
                    else {
                        perror("memory allocation");
                        free(buf);
                        return NULL;
                    }
                }
                if (buflen) {
                     strcat(p, " ");
                 }
                strcat(p, "\"");
                strcat(p, fi_p->name);
                strcat(p, "\"");
                buflen += fplen + 3;
                //printf("Added item '%s', buflen is now %d\n", fi_p->fullpath, buflen);
            }
        } // end for
        return buf;
    }
    else {
        return NULL;
    }
}

void perform_create_directory(void)
{
    if (!MainWnd_p->targdir_fi_p) return;
    const char *newname = fl_input("Enter new directory name");
    if (newname) {
        int err = chdir(MainWnd_p->targdir_fi_p->fullpath);
        if (err) {
            err = errno;
            fl_alert("Cannot access current directory. System returned (%d):\n%s",
                     err, strerror(err));
            return;
        }
        
        
//        err = mkdir(newname, DEFAULT_DIR_PERMS);
        char cmdstr[1024] = {0};
        sprintf(cmdstr, "%smkdir %s", UseSudo ? "sudo " : "", newname);
        err = system(cmdstr);
        if (err) {
            fl_alert("Cannot create directory.\nCheck ownership, permissions, and mount status.");
            return;
        }
    }
}

int conditionally_rename(file_item* fi_p, const char* newname)
{
    int answer = 1; // Assume it's ok
//    int replacing = 0;
    char cmdstr[1024] = {0};
    char* dir = get_current_dir_name();
    //printf("Might rename '%s'\n", fi_p->fullpath); fflush(0);
    if (-1 == chdir(fi_p->path)) {
        perror("chdir");
    }
    sprintf(cmdstr, "%sls %s 2> /dev/null", UseSudo ? "sudo " : "", newname);
    //printf("Trying command '%s' from path '%s'\n", cmdstr, fi_p->path); fflush(0);
    if (!system(cmdstr)) {
        struct stat64 trgstat;
        sprintf(cmdstr, "%s/%s", dir, newname);
        if (stat64(cmdstr, &trgstat)) {
            printf("Could not check status of file '%s'\n", cmdstr);
            return 1;
        }
        if (S_ISDIR(trgstat.st_mode) || fi_p->is_dir()) {
            fl_message("Cannot change '%s' to the name of an existing\n"
                    "%s.  Please rename or remove '%s' first.\n",
                    fi_p->name,  S_ISDIR(trgstat.st_mode) ? "directory":"file",
                    newname);
            answer = 0;
        }
        else {
            struct tm mod_tm;
            memcpy(&mod_tm, localtime(&trgstat.st_mtime), sizeof(mod_tm));
            char datestr[64];
            sprintf(datestr, "%04d-%02d-%02d %02d:%02d:%02d", 
                    mod_tm.tm_year+1900, mod_tm.tm_mon+1, mod_tm.tm_mday,
                    mod_tm.tm_hour, mod_tm.tm_min, mod_tm.tm_sec);

            answer = fl_choice( "File '%s' already exists.\n"
                                    "Replace the current file (%lld bytes, %s) by\n"
                                    "renaming the source file (%lld bytes, %s)?\n",
                                    "cancel", "replace", NULL, 
                                    newname, 
                                    (long long)trgstat.st_size, datestr,
                                    (long long)fi_p->status.st_size, fi_p->datestr);
//            if (answer == 1 ) {
//                replacing = 1;
//            }
        }
    }
    if (answer == 1) {
        sprintf(cmdstr, "%smv \"%s\" \"%s\"",                 
                UseSudo ? "sudo " : "", fi_p->name, newname);
        //printf("For rename, trying cmd '%s'\n", cmdstr);
        MainWnd_p->list_p->deselect();  //unselect
        MainWnd_p->sel_fi_p = NULL;
        int err = system(cmdstr);
        if (err) {
            fl_alert("Command '%s' not successful", cmdstr);
            return err;
        }
    }
    return 0;
}

void create_clone_name(file_item* fi_p, char* buf) {
    char candidate[MAXFNAMELEN+128];
    char fullpath[MAXFFULLPATHLEN+128];
    int n = 1;
    struct stat64 s;
    while(1) {
        sprintf(candidate, "%s(copy%d)", fi_p->name, n);
        sprintf(fullpath, "%s%s", fi_p->path, candidate);
        //printf("Checking if '%s' is a good clone name\n", fullpath); fflush(0);
        if (-1 == stat64(fullpath, &s)) {
            strnzcpy(buf, candidate, MAXFNAMELEN);
            return;
        }
        n++;
    }    
}

int conditionally_copy_or_move(file_item* targetdir_fi_p, file_item* src_fi_p, fileop_enum cmd)
{
    int fileanswer = FILE_ANSWER_PROCEED;  // default to replacing
    char cmdstr[2048] = {0};
    char filepath[MAXFPATHLEN];
    // Is this a copy into same directory (make a clone?)
    strcpy(filepath, src_fi_p->path);
    filepath[strlen(filepath) - 1] = '\0'; // trim off the trailing '/'
    if (!strcmp(filepath, targetdir_fi_p->fullpath)) {
        char clone_name[MAXFNAMELEN];
        create_clone_name(src_fi_p, clone_name);
        sprintf(cmdstr, "%scp -fpR \"%s\" \"%s\"",
                UseSudo ? "sudo " : "",
                src_fi_p->name, clone_name);
        //printf("Trying to create clone with cmd '%s'\n", cmdstr); fflush(0);
        int err = system(cmdstr);
        if (err) {
            fl_alert("Command '%s' not successful", cmdstr);
            return err;
        }
        return 0;
    }

    file_item* twin_fi_p = targetdir_fi_p->child_fi_p;
    //printf("Checking dir '%s' for existing files\n", targetdir_fi_p->fullpath); fflush(0);
    if (!twin_fi_p) {
        // Perhaps directory not scanned yet, see if it has any files...
        get_child_file_items(targetdir_fi_p);
        twin_fi_p = targetdir_fi_p->child_fi_p;
    }
    while (twin_fi_p && strcmp(twin_fi_p->name, src_fi_p->name)) {
        twin_fi_p = twin_fi_p->next_fi_p;
        if (twin_fi_p) {
            //printf("Does file %s have the same name as %s?\n", twin_fi_p->fullpath, src_fi_p->name); fflush(0);
        }
    }
    //printf("twin_fi_p is 0x%08X \n", twin_fi_p); fflush(0);
    if (twin_fi_p) {
        //printf("found %s with same name as %s\n", twin_fi_p->fullpath, src_fi_p->name);
        if (BatchReplaceMode == BATCH_ASK_EACH) {
            fileanswer = FILE_ANSWER_UNKNOWN;
            while (fileanswer == FILE_ANSWER_UNKNOWN) {
                int answer = fl_choice( "'%s' already exists at the destination.\n"
                                        "Replace it (%lld bytes, %s)\n"
                                        " with file (%lld bytes, %s)?\n",
                                        "(batch...)", "replace", "skip", 
                                        twin_fi_p->name, 
                                        (long long)twin_fi_p->status.st_size, twin_fi_p->datestr,
                                        (long long)src_fi_p->status.st_size, src_fi_p->datestr);
                if (!answer) {
                    answer = fl_choice( "For this and remaining items in the %s operation,\n"
                                        "would you like to...", "specify each", "replace all", "skip all", 
                                        (cmd == COPY_OPER) ? "copy" : "move");
                    switch(answer) {
                        case BATCH_ASK_EACH:
                            break;
                        case BATCH_SKIP_ALL:
                            fileanswer = FILE_ANSWER_SKIP;
                            BatchReplaceMode = BATCH_SKIP_ALL;
                            break;
                        case BATCH_REPLACE_ALL:
                            fileanswer = FILE_ANSWER_PROCEED;
                            BatchReplaceMode = BATCH_REPLACE_ALL;
                            break;
                    }
                }
                else if (answer == FILE_ANSWER_SKIP || answer == FILE_ANSWER_PROCEED) {
                    fileanswer = answer;
                }
            }
        }
        else if (BatchReplaceMode == BATCH_SKIP_ALL) {
            fileanswer = FILE_ANSWER_SKIP;
        }
        else if (BatchReplaceMode == BATCH_REPLACE_ALL) {
            fileanswer = FILE_ANSWER_PROCEED;
        }
    }
    if (fileanswer == FILE_ANSWER_PROCEED) {
        //printf("Going to try to make a copy now...\n"); fflush(0);
        //printf("src_fi_p is 0x%08X ", src_fi_p); fflush(0);
        //printf("src_fi_p->fullpath is '%s'\n", src_fi_p->fullpath); fflush(0);
        //printf("targdir_fi_p is 0x%08X ", targetdir_fi_p); fflush(0);
        //printf("targdir_fi_p->fullpath is '%s'\n", targetdir_fi_p->fullpath); fflush(0);
        sprintf(cmdstr, "%s%s \"%s\" \"%s\"",
                UseSudo ? "sudo " : "",
                (cmd == COPY_OPER) ? "cp -fpR" : "mv -f", 
                src_fi_p->fullpath, targetdir_fi_p->fullpath);
        //printf("Trying cmd '%s'\n", cmdstr); fflush(0);
        int err = system(cmdstr);
        if (err) {
            fl_alert("Command '%s' not successful", cmdstr);
            return err;
        }
    }
    return 0;
}

void begin_renaming(void)
{
    file_item* fi_p;
    int rx;
    int ry;
    //printf("Lastclick in %s\n", (MainWnd_p->last_click_browser == LASTCLICK_IN_LIST) ? "list window" : (MainWnd_p->last_click_browser == LASTCLICK_IN_TREE) ? "tree window" : "???"); fflush(0);
    if (MainWnd_p->last_click_browser == LASTCLICK_IN_LIST) {
        File_Detail_List_Browser* p = MainWnd_p->list_p;
        MainWnd_p->sel_fi_p = (file_item*)(p->data(p->value()));
        //btnbar_action1_cb();

        fi_p = MainWnd_p->sel_fi_p;
        rx = 4 + p->x();
        ry = p->y() + p->item_y_coordinate(p->value()) - p->vposition();
    }
    else {
        fi_p = MainWnd_p->targdir_fi_p;
        Dir_Tree_Browser* p = MainWnd_p->tree_p;
        rx = 4 + p->x();
        ry = p->y() + ((p->value() - 1) * p->list_item_height)
                     - p->vposition();
    }
    //printf("begining to rename %s, rx %d, ry %d\n", fi_p->fullpath, rx, ry); fflush(0);
    chdir(fi_p->path);
    MainWnd_p->enable_renaming(fi_p, rx, ry);
    
}

static void DirTreeMenuCB(Fl_Widget* window_p, void *userdata) 
{
    long choice = (long)userdata;
    file_item* dir_fi_p = MainWnd_p->targdir_fi_p;
    file_item* parent_dir_p = dir_fi_p->parent_fi_p;
	char cmdstr[1024] = {0};
    switch (choice) {
        case MI_NEW_FOLDER:
            perform_create_directory();
            break;
            
        case MI_OPENTERM:
			sprintf(cmdstr, "cd %s; aterm &", dir_fi_p->fullpath);
            system(cmdstr);
            break;
            
        case MI_PASTE:
            perform_file_copy_or_move(COPY_OPER, dir_fi_p);
            break;
            
        case MI_MOVE:
            perform_file_copy_or_move(MOVE_OPER, dir_fi_p, USING_SELECTIONS);
            break;

        case MI_COPYTO:
            perform_file_copy_or_move(COPY_OPER, dir_fi_p, USING_SELECTIONS);
            break;

        case MI_RENAME:
            begin_renaming();
            break;
            
        case MI_PROPERTIES:
            MainWnd_p->sel_fi_p = MainWnd_p->targdir_fi_p;
            switch_directory(MainWnd_p->sel_fi_p->parent_fi_p);
            sync_gui_with_filesystem();
            btnbar_props_cb();
            break;
        case MI_TRASH:
            if (!parent_dir_p) return;  // don't trash root!
            chdir(dir_fi_p->path);
            perform_file_trash(dir_fi_p);
            MainWnd_p->seldir_fi_p = parent_dir_p;            
            break;
        case MI_DELETE:
            if (!parent_dir_p) return;  // don't delete root!
            perform_file_delete(DELETE_DIR);            
            switch_directory(parent_dir_p);
            break;            
        case MI_CANCEL:
            break;            
        case MI_UP_DIR:
            btnbar_go_up_cb();
            break;
        case MI_BK_DIR:
            btnbar_go_bk_cb();
            break;
        case MI_FW_DIR:
            btnbar_go_fw_cb();
            break;
        case MI_ABOUT:
            btnbar_about_cb();
            break;
        case MI_HELP:
            btnbar_help_cb();
            break;
        case MI_QUIT:
            Running = 0;
            break;
        default:
            break;
    }
    MainWnd_p->set_titles();

}

void mark_file_items(File_Detail_List_Browser* mb_p) {
    int i, len;
    len = 0;
    // Write the list of selected file items to a buffer 
    // First, free previous string
    if (MarkedFileItems_sz_p)  free(MarkedFileItems_sz_p);
    
    // first get size of needed buffer
    for (i = 2; i <= mb_p->size(); i++) {
        if (mb_p->selected(i)) {
            len += strlen(((file_item *)(mb_p->data(i)))->fullpath) + strlen("file://") + 2;
        }
    }
    // allocate it
    MarkedFileItems_sz_p = (char *)malloc(len);
    if (!MarkedFileItems_sz_p) {
        perror("Could not allocate memory for copy/paste or drag-and-drop\n");
    }
    strclr(MarkedFileItems_sz_p);
    file_item* fi_p;
    // now write filenames into it
    for (i = 2; i <= mb_p->size(); i++) {
        if (mb_p->selected(i)) {
            fi_p = (file_item *)(mb_p->data(i));
            if (strcmp(fi_p->name, ".") && strcmp(fi_p->name, "..")) { // skip these
                strcat(MarkedFileItems_sz_p, "file://");
                strcat(MarkedFileItems_sz_p, fi_p->fullpath);
                strcat(MarkedFileItems_sz_p, "\n");
            }
        }
    }
    //printf("Marked item string:\n%s\n", MarkedFileItems_sz_p); fflush(0);
    MainWnd_p->update_btnbar();
}

void perform_file_copy_or_move(fileop_enum oper, file_item* dir_fi_p, int using_sel)
{
    int i; //, buflen; //, fplen;        
    int t = 0;
//    buflen = 0;
    int err = 0;
    file_item* fi_p;
    BatchReplaceMode = BATCH_ASK_EACH;
    wait_cursor();
    const char* oper_str = "";
    switch(oper) {
        case MOVE_OPER:   
            oper_str = "perhaps moved";  
            break;
        case COPY_OPER: 
            oper_str = "perhaps copied"; 
            break;
    }
    if (using_sel) {
        File_Detail_List_Browser* mb_p = MainWnd_p->list_p;
        MainWnd_p->init_progress(0, "file", oper_str, mb_p->num_selected());
        for (i = 1; i <= mb_p->size() && !err; i++) {
            if (mb_p->selected(i)) {
                fi_p = (file_item*)mb_p->data(i);
                //printf("Targdir is '%s', file is '%s'\n", dir_fi_p->fullpath, fi_p->fullpath); fflush(0);
                if (oper == MOVE_OPER)  mb_p->select(i, 0); // unselect item
                err = conditionally_copy_or_move(dir_fi_p, fi_p, oper);
                t++;
                MainWnd_p->update_progress(t);
                DELAY_FOR_TESTING;
            }
        }
        if (oper == MOVE_OPER)  {
            MainWnd_p->sel_fi_p = NULL;
        }
    }
    else {
        int cnt = marked_file_items(MarkedFileItems_sz_p);
        MainWnd_p->init_progress(0, "file", oper_str, cnt);
        file_item temp_fi;
        char* c = MarkedFileItems_sz_p;
        while(*c && t < cnt) {
            char* f = temp_fi.fullpath;
            char* p = temp_fi.path;
            while(*c && strncmp(c, "file://", 7)) c++;
            //printf("REMAINING IN COPY:\n%s\n=============\n", c); fflush(0);
            c += 7;
            while(*c && *c != '\n') {
                *f++ = *c;
                *p++ = *c++;
            }
            *f = '\0';
            *p = '\0';
            c++;
            char* n = temp_fi.name;
            char* l = p - 1;
            while(*l != '/')  l--;
            *l++ = '\0'; 
            while(*l  && *l != '\n') {
                *n++ = *l++;
            }
            *n = '\0';
            strcat(temp_fi.path, "/");
            temp_fi.fetch_status();
            //printf("Temp item path '%s'; name '%s', size '%s'\n", 
            //        temp_fi.path, temp_fi.name, temp_fi.sizestr); fflush(0);
            //printf("Targdir is '%s', file is '%s'\n", dir_fi_p->fullpath, temp_fi.fullpath); fflush(0);
            err = conditionally_copy_or_move(dir_fi_p, &temp_fi, oper);
            t++;
            MainWnd_p->update_progress(t);
            DELAY_FOR_TESTING;                    
        }
            
    }
    MainWnd_p->complete_progress();
    normal_cursor();
    if (err) {
        fl_alert("There were errors during the %s command", 
                 (oper == MOVE_OPER) ? "move" : "paste");
    }
}

void perform_file_delete(int what)
{
    char* filestr;
    char dirstr[2048];
    if (what == DELETE_SELECTED) {
        filestr = sel_files_str();
    }
    else {
        sprintf(dirstr, "\"%s\"", MainWnd_p->targdir_fi_p->fullpath);
        filestr = dirstr;
    }
    if (ConfirmDelete) {
        char prompt_str[80];
        if (what == DELETE_SELECTED) {
            if (strlen(filestr) < 40) {
                strnzcpy(prompt_str, filestr, 80);
            }
            else {
                int n = MainWnd_p->list_p->num_selected();
                if (n > 2) {
                    sprintf(prompt_str, "%d file%s", n, (n>1)?"s":"");
                }
                else {
                    strnzcpy(prompt_str, filestr, 80);
                }
            }
        }
        else {
            sprintf(prompt_str, "directory");
        }
        int ans = fl_choice("Delete %s?", "No, cancel", "Yes, delete", NULL, prompt_str);
        if (ans == 0) {
            return;
        }
    }
    //printf("Delete of '%s' requested. Filestr is 0x%08X\n", filestr, filestr); fflush(0);
    char* cmdstr = (char*)malloc(strlen(filestr) + 256);
    //printf("Cmdstr is 0x%08X\n", cmdstr); fflush(0);
    //printf("ConfirmDelete is %d\n", ConfirmDelete);
    if (cmdstr) {
        sprintf(cmdstr, "%srm -rf %s", UseSudo ? "sudo " : "", filestr);
        //printf("From dir '%s', executing '%s'\n", getcwd(NULL, 0), cmdstr); fflush(0);
        MainWnd_p->list_p->deselect(); //unselect items in list
        MainWnd_p->sel_fi_p = NULL;
        //printf("Before delete, %d items selected\n", MainWnd_p->list_p->num_selected());
        wait_cursor();
        system(cmdstr);
        free(cmdstr);
        //printf("Cmd string freed\n"); fflush(0);
        
    }
    else {
        perror("memory allocation");
    }
    if (what == DELETE_SELECTED) {
        if (filestr)  free(filestr);
        //printf("File string freed\n"); fflush(0);
        for (int i = 1; i <= MainWnd_p->list_p->size() ; i++) {
            if (MainWnd_p->list_p->selected(i)) {
                prune_file_item((file_item*) MainWnd_p->list_p->data(i));
            }
        }
    }
    else {
        //printf("Before pruning directory item\n"); fflush(0);
        prune_file_item(MainWnd_p->seldir_fi_p);
        //printf("File item branch pruned\n"); fflush(0);
    }
    sync_gui_with_filesystem();
    normal_cursor();
}
void perform_select_all(void) {
    File_Detail_List_Browser* mb_p = (File_Detail_List_Browser*)MainWnd_p->list_p;
    //printf("Select All...\n"); fflush(0);
    file_item* fi_p = NULL;
    for(int i = 2; i <= mb_p->size(); i++) {
        fi_p = (file_item*)mb_p->data(i);
        if (fi_p && strcmp(fi_p->name, ".") && strcmp(fi_p->name, "..")) { 
            //printf("Selecting list item '%s'\n", fi_p->name); fflush(0);
            mb_p->select(i);
            MainWnd_p->sel_fi_p = fi_p;
        }
    }
    Fl::wait(0.1);
    if (MainWnd_p->sel_fi_p) {
        MainWnd_p->last_click_browser = LASTCLICK_IN_LIST;
        MainWnd_p->update_btnbar();
    }
}

static void FileMenuCB(Fl_Widget* window_p, void *userdata) 
{
    long choice = (long)userdata;
    File_Detail_List_Browser* mb_p = (File_Detail_List_Browser*)MainWnd_p->list_p;
    char cmdstr[1024] = {0};
	char cmd[2048];

    switch (choice) {
        case MI_RUN:
            perform_run_or_browse();
            break;
            
        case MI_OPEN:
            perform_run_open(choice);
            break;

        case MI_OPENWITH:
            OpenWithWnd_p->treat_generic_chk_p->value(0);
            OpenWithWnd_p->update();   
            OpenWithWnd_p->show();   
            break;

        case MI_SELECTALL:
            perform_select_all();
            break;

        case MI_COPY:
            mark_file_items(mb_p);
            break;

        case MI_PASTE:
            perform_file_copy_or_move(COPY_OPER, MainWnd_p->seldir_fi_p);
            break;

        case MI_RENAME:
            begin_renaming();
            break;

        case MI_OPENTERM:
			sprintf(cmdstr, "cd %s; aterm &", MainWnd_p->seldir_fi_p->fullpath);
            system(cmdstr);
            break;
            
        case MI_PROPERTIES:
            btnbar_props_cb();
            break;

        case MI_TRASH:
            btnbar_trash_cb();
            break;
            
        case MI_RESTORE:
            wait_cursor();
            if (mb_p->num_selected()) {
                char fullpath[640];
                int t = 0;
                MainWnd_p->init_progress(0, "file", "restored from trash", 
                                         mb_p->num_selected());
                for (int i = 1 ; i <= mb_p->size(); i++) {
                    if (mb_p->selected(i)) {
                        file_item* sel_fi_p = (file_item*)mb_p->data(i);
                        mb_p->select(i,  0);
                        //strclr(fullpath);
                        bzero(fullpath, 640);
                        char* p = fullpath;
                        char* n = sel_fi_p->name;
                        
                        while(*n) {
                            //printf("name char %c, new fullpath is '%s'\n", *n, fullpath);
                            if (*n == '~') {
                                *p++ = '/';
                                n++;
                            }
                            else {
                                *p++ = *n++;
                            }
                        }
                        *p = '\0';
                        
                        //printf("File '%s' should be restored to '%s'\n", sel_fi_p->name, fullpath); fflush(0);
                        if (*fullpath) {
                            sprintf(cmd, "%smv \"%s\" \"%s\"",
                                    UseSudo ? "sudo " : "",
                                    sel_fi_p->fullpath, fullpath);
                            //printf("Trying restore cmd: %s\n", cmd); fflush(0);
                            int err = system(cmd);
                            if (err) {
                                fl_alert("Unable to restore %s from trash.\nTry manual copy or move", sel_fi_p->name);
                            }
                            else {
                                t++;
                                MainWnd_p->update_progress(t);
                                DELAY_FOR_TESTING;
                            }
                        }
                    }
                }
                MainWnd_p->complete_progress();
            }
            normal_cursor();
            break;
            
        case MI_DELETE:
            perform_file_delete(DELETE_SELECTED);
            break;
            
        case MI_UP_DIR:
            btnbar_go_up_cb();
            break;

        case MI_BK_DIR:
            btnbar_go_bk_cb();
            break;

        case MI_FW_DIR:
            btnbar_go_fw_cb();
            break;

        case MI_HELP:
            btnbar_help_cb();
            break;
            
        case MI_CANCEL:
            break;
            
        default:
            break;
    }
}

void refresh_children_status(file_item* fi_p) {
    file_item* last_fi_p = NULL;
    file_item* child_fi_p = NULL;
    child_fi_p = fi_p->child_fi_p;
    int rc = 0;
    struct stat64 child_status;
    while (child_fi_p) {
        //printf("Checking status: file '%s'\n", child_fi_p->name); fflush(0);
        rc = stat64(child_fi_p->fullpath, &child_status);
        if (rc) {
            // not found or not accessible any more, so prune it out
            //printf("Pruning out missing file '%s'\n", child_fi_p->fullpath); fflush(0);
            last_fi_p = child_fi_p;
            child_fi_p = child_fi_p->next_fi_p; // get next one, if any
            prune_file_item(last_fi_p);
        }
        else {
            child_fi_p->fetch_status();
            child_fi_p = child_fi_p->next_fi_p; // get next one, if any
        }
    }

}

int new_item_sorts_before_existing_item(file_item* new_fi_p, file_item* fi_p, int sortspec) {
    char newname[MAXFNAMELEN];
    char existname[MAXFNAMELEN];
    char* c = new_fi_p->name;
    char* C = newname;
    while (*c) *C++ = (char)toupper(*c++);
    *C = '\0';
    
    c = fi_p->name;
    C = existname;
    while (*c) *C++ = (char)toupper(*c++);
    *C = '\0';
    
    int descending = sortspec & 1;
    int name_cmp = strcmp(newname, existname);
    long long diff = 0;

    switch(sortspec) {
        case SORTSPEC_NAME_ASC:
        case SORTSPEC_NAME_DESC:
            diff = name_cmp;
            break;

        case SORTSPEC_TYPE_ASC:
        case SORTSPEC_TYPE_DESC:
            diff = new_fi_p->filetype - fi_p->filetype;
            break;

        case SORTSPEC_SIZE_ASC:
        case SORTSPEC_SIZE_DESC:
            diff = new_fi_p->status.st_size - fi_p->status.st_size;
            break;

        case SORTSPEC_DATE_ASC:
        case SORTSPEC_DATE_DESC:
            diff = new_fi_p->status.st_mtime - fi_p->status.st_mtime;
            break;

        default:
            break;
    }
    
    if (descending)  diff *= -1;
    if (diff == 0)  diff = name_cmp;    // name is tie-breaker for non-name compares
    return (diff < 0);
}

int get_child_file_items(file_item* fi_p)
{
    int ret = 0;
    char dir_str[MAXFFULLPATHLEN] = {0};
    int curlevel;
    sprintf(dir_str, "%s%s/", fi_p->path, fi_p->name);
    //printf("Getting children of '%s'\n", dir_str);
    DIR *dir = NULL;
    struct dirent *dp;          /* returned from readdir() */
    
    wait_cursor();

    // First, remove any file items now missing 
    // and refresh status if any are still-present
    refresh_children_status(fi_p);
    
    // Now get any additional child file items
    dir = opendir(dir_str);
    if (!dir) {
        perror(dir_str);
    }

    if (dir) {
        curlevel = fi_p->level + 1;
        while ((dp = readdir (dir)) != NULL) {
            if (!strcmp(dp->d_name, "*"))  {
                //printf("Dir item is '%s'\n", dp->d_name); fflush(0);
                continue;
            }
            //printf("--- child '%s%s'\n", dir_str, dp->d_name); fflush(0);
            file_item* new_fi_p = new file_item(dir_str, dp->d_name);
            new_fi_p->parent_fi_p = fi_p;
            new_fi_p->level = curlevel;

            // find place for this newest child
            file_item* p = fi_p->child_fi_p;
            file_item* earlier_fi_p = NULL;
            int pos = 0;
            int added = 0;
            if (!p) {
                fi_p->child_fi_p = new_fi_p;
                //printf("at head of list\n");
                added = 1;
            }
            else {
                while(p) {
                    // Always build the file_item tree in inode order, maybe view parts of it in different order
                    if (new_item_sorts_before_existing_item(new_fi_p, p, SORTSPEC_NAME_ASC)) {
                        //printf("It goes before current item\n");
                        new_fi_p->next_fi_p = p;
                        if (fi_p->child_fi_p == p) {
                            fi_p->child_fi_p = new_fi_p;
                        }
                        if (earlier_fi_p) {
                            earlier_fi_p->next_fi_p = new_fi_p;
                        }
                        added = 1;
                        break;
                    }
                    else if (!strcmp(new_fi_p->name, p->name)) {
                        //printf("It's the same as the current item\n");
                        // It's the same, so update the old one and delete the new one
                        p->fetch_status(); //
                        delete new_fi_p;
                        new_fi_p = NULL;
                        break; // leave while loop without altering list
                    }
                    else {
                        //printf("It goes after the current item\n");
                        pos++;
                        earlier_fi_p = p;
                        p = p->next_fi_p;
                    }
                }
                if (new_fi_p && !added) {
                    earlier_fi_p->next_fi_p = new_fi_p;
                    added = 1;
                }
                if (added) {
                    //printf("%s (%s, %s, %s, %s, %s) added \n", 
                    //        new_fi_p->name, new_fi_p->typestr, new_fi_p->sizestr, new_fi_p->datestr,
                    //        new_fi_p->ownstr, new_fi_p->permstr); fflush(0);
                    fi_p->children++;
                    if (new_fi_p->is_dir()) fi_p->subdirs++;
                }
                //printf("in position %d\n", pos);
            }

        }
        //printf("    Dir %s (fi 0x%08x) has %d children, %d subdirs\n", fi_p->name, 
        //        (unsigned int)fi_p, fi_p->children, fi_p->subdirs);
        ret = fi_p->children;
        closedir(dir);
        //printf("directory %s closed\n", dir_str);
    }
    else {
        //printf("opendir() returned NULL.\n");
    }   
    //printf("%d children found\n", ret); fflush(0);
    normal_cursor();
    return ret;
}

int build_tree_for_branch(char* path, file_item* root_fi_p, char* targpath, file_item** targ_fi_pp) 
{
    file_item* parent_fi_p = root_fi_p;
    char name[MAXFNAMELEN];
    
    char* p = path;
    char* n;
    if (!strcmp(path, "/")) {
        if (targ_fi_pp) *targ_fi_pp = root_fi_p;
        return 1;
    }
    //printf ("Building tree/branch '%s' from node at %s%s/\n", path, root_fi_p->path, root_fi_p->name);
    if (*p == '/') p++;
    if(*p) {
        n = name;
        while (*p && *p != '/') { 
            *n++ = *p++;
        }
        *n++ = '\0';
        //printf("We want to nest into subdir '%s'\n", name);
        if (get_child_file_items(parent_fi_p)) {
            // did we get an item with a matching name?
            file_item* fi_p = parent_fi_p->child_fi_p;
            while (fi_p) {
                //printf("Checking %s as a match for %s\n", fi_p->name, name);
                if (!strcmp(fi_p->name, name)) {
                    //printf("Subdir found! Nesting...\n");
                    if (*p == '/') {
                        p++;
                    }
                    // check to see if this file item is the currently selected dir
                    char fullpathname[1024];
                    sprintf(fullpathname, "%s%s/", fi_p->path, fi_p->name);
                    //printf("Comparing current path '%s' to target path '%s'...", fullpathname, MainWnd_p->path);
                    if (!strncmp(fullpathname, targpath, strlen(targpath))) {
                        if (targ_fi_pp) *targ_fi_pp = fi_p;
                        //printf("found!");
                        get_child_file_items(fi_p);
                    }
                    //printf("\n");
                    // Expand/nest deeper
                    if (*p) {
                        build_tree_for_branch(p, fi_p, targpath, targ_fi_pp);
                    }
                    if (fi_p->subdirs > 1) fi_p->expanded = 1;
                    break;
                }
                fi_p = fi_p->next_fi_p;
            }
            if (parent_fi_p->subdirs > 1) parent_fi_p->expanded = 1;
            //printf("parent %s (fi 0x%08X) subdirs %d, expanded %d\n", parent_fi_p->name, parent_fi_p, parent_fi_p->subdirs, parent_fi_p->expanded);
        }
        
    }
    return 1;
}

void prune_single_file_item(file_item* fi_p)
{
    file_item* parent_p = fi_p->parent_fi_p;
    file_item* prev_sib_p = NULL;
    
    if (parent_p) {
        //printf("parent_p is 0x%08X, parent_p->child_fi_p is 0x%08X, fi_p is 0x%08X\n",
        //        parent_p, parent_p->child_fi_p, fi_p);
        if (parent_p->child_fi_p == fi_p) {
            // prune first child
            parent_p->child_fi_p = fi_p->next_fi_p;
            //printf("Pruned item '%s', parent's first child is now '%s'\n",
            //     fi_p->name, 
            //      parent_p->child_fi_p ? parent_p->child_fi_p->name : "(null)");
            free(fi_p);
            parent_p->children--;
        }
        else {
            prev_sib_p = parent_p->child_fi_p;
            while(prev_sib_p->next_fi_p && (prev_sib_p->next_fi_p != fi_p)) {
                prev_sib_p = prev_sib_p->next_fi_p;
                //printf("   prev. sib. is now '%s'\n", prev_sib_p->name); fflush(0);
            }
            //printf("prev_sib_p->next_fi_p is 0x%08X, fi_p is 0x%08X, fi_p->next_fi_p is 0x%08X\n",
            //        prev_sib_p->next_fi_p, fi_p, fi_p->next_fi_p);
            if (prev_sib_p->next_fi_p == fi_p) {
                prev_sib_p->next_fi_p = fi_p->next_fi_p;
                //printf("Pruned item '%s'\n", fi_p->name); //, now items '%s' and '%s' are close siblings\n",
                //       fi_p->name, prev_sib_p->name, 
                //       prev_sib_p->next_fi_p ? "(the end)" : prev_sib_p->next_fi_p->name); fflush(0);
                free(fi_p);
                parent_p->children--;
            }
        }
    }
    //else {
    //    printf("Won't delete root file item node!\n");
    //}
}

void prune_children(file_item* fi_p)
{
    while (fi_p->child_fi_p) {
        //printf("pruning %s's first child, %s...", fi_p->name, 
        //        fi_p->child_fi_p->name); fflush(0);
        prune_file_item(fi_p->child_fi_p);
    }
}

void prune_file_item(file_item* fi_p)
{
    prune_children(fi_p);
    remove_watch(fi_p);
    if (fi_p == MainWnd_p->seldir_fi_p) {
        MainWnd_p->seldir_fi_p = fi_p->parent_fi_p;
    }
    prune_single_file_item(fi_p);
}

// return -1 if dir_p is not in the visited directory set
// will start checking at index start_i and and scan downwards, 
// returning the first match backwards
int dirp_index_in_visted_set(file_item* dir_p, int start_i)
{
	int idx = -1;
	if (VisitedDirs < VISITED_DIR_MAX) {
		for (int i = start_i; i >= 0; i--) {
			if (VisitedDir[i] == dir_p) {
				idx = i;
				break;
			}
		}
	}
	else {
		int n = 1;
		int i = start_i;
		while (n < VISITED_DIR_MAX) {
			if (VisitedDir[i] == dir_p) {
				idx = i;
				break;
			}
			i = (i + 1) % VISITED_DIR_MAX;
			n++;
		} 
	}
	return idx;
}

void switch_directory(file_item* new_dir_fi_p, bool is_fwd) 
{
	unsigned int initVistedDirCur = VisitedDirCur;
    if (new_dir_fi_p) {
        //~ printf("switch_dir on entry: cur %d, newest %d, dirs %d, switch to  %s, cur dir is %s\n",
                //~ VisitedDirCur, VisitedDirNewest, VisitedDirs, 
                //~ new_dir_fi_p->fullpath, MainWnd_p->seldir_fi_p->fullpath); fflush(0);
        // make sure we know where the current one is in the list, if any
        if (MainWnd_p->seldir_fi_p) {
            // Check to see if current dir is already in list
			if (MainWnd_p->seldir_fi_p != VisitedDir[VisitedDirCur] && VisitedDirs > 0) {
				int i = dirp_index_in_visted_set(MainWnd_p->seldir_fi_p, VisitedDirs - 1);
				if (i >= 0) {
					VisitedDirCur = i;
				}
				else {
					// remember the current one
					if (VisitedDirs < VISITED_DIR_MAX) {
						// add to end
						VisitedDirCur = VisitedDirs;
						VisitedDirs++;
					}
					else {
						// add after current newest, perhaps replacing an much older one
						VisitedDirCur = (VisitedDirNewest + 1) % VISITED_DIR_MAX;
					}
					VisitedDir[VisitedDirCur] = MainWnd_p->seldir_fi_p;
					VisitedDirNewest = VisitedDirCur;
				}
			}
        }
		if (MainWnd_p->seldir_fi_p) {
			if (is_fwd) {
				// first check to see whether we are stepping forward through visited list
				int fwd_from_cur_slot = (VisitedDirCur + 1) % VISITED_DIR_MAX;
				if (VisitedDir[fwd_from_cur_slot] == new_dir_fi_p) {
					// yes, just move the current one to the fwd slot position
					VisitedDirCur = fwd_from_cur_slot;
				}
				else {
					int i = dirp_index_in_visted_set(new_dir_fi_p, VisitedDirs - 1);
					if (i >= 0) {
						VisitedDirCur = i;
					}
					else {
						// branch off from prior history, perhaps pruning some of it
						VisitedDirNewest = fwd_from_cur_slot;
						VisitedDir[VisitedDirNewest] = new_dir_fi_p; // put it in history
						VisitedDirCur = VisitedDirNewest;
						VisitedDirs = VisitedDirNewest + 1;
						if (VisitedDirs > VISITED_DIR_MAX) {
							VisitedDirs = VISITED_DIR_MAX;
						}
					}
				}
			}
			else {
				int rev_from_cur_slot = VisitedDirCur - 1;
				if (rev_from_cur_slot < 0) {
					rev_from_cur_slot = 0;
				}
				int i = dirp_index_in_visted_set(new_dir_fi_p, VisitedDirCur - 1);
				if (i >= 0) {
					// just move back there, 
					VisitedDirCur = i;
				}
				else {
					// shouldn't happen if using the backward mode!
					printf("Trying to go back in history but didn't find expected directory!\n");
				}				
			}
		}
		else {
			VisitedDirNewest = (VisitedDirNewest + 1) % VISITED_DIR_MAX;
			VisitedDir[VisitedDirNewest] = new_dir_fi_p; // put it in history
			VisitedDirCur = VisitedDirNewest;
			VisitedDirs = VisitedDirNewest + 1;
			if (VisitedDirs > VISITED_DIR_MAX) {
				VisitedDirs = VISITED_DIR_MAX;
			}
		}

        MainWnd_p->pathhist_menu_p->clear();
        //printf("switch_dir before filling menu: oldest %d, newest %d, dirs %d\n",
        //        VisitedDirCur, VisitedDirNewest, VisitedDirs); fflush(0);
        
        //~ for(unsigned int i = (VisitedDirNewest - 1) & (VISITED_DIR_MAX-1) ;
                //~ i != VisitedDirCur ;
                //~ i = (i - 1) & (VISITED_DIR_MAX-1)) {
            //~ MainWnd_p->pathhist_menu_p->add(VisitedDir[i]->fullpath);
            //~ //printf("   added from slot %d, %s\n", i, VisitedDir[i]->fullpath); fflush(0);
        //~ }
        //~ MainWnd_p->pathhist_menu_p->add(VisitedDir[VisitedDirCur]->fullpath);
        //~ //printf("   added from slot %d, %s\n", VisitedDirCur, VisitedDir[VisitedDirCur]->fullpath); fflush(0);
        for (int i = (VisitedDirs - 1); i >= 0; i--) {
			MainWnd_p->pathhist_menu_p->add(VisitedDir[i]->fullpath);
		}
        MainWnd_p->pathhist_menu_p->label(new_dir_fi_p->fullpath);
        MainWnd_p->seldir_fi_p = new_dir_fi_p;
        strnzcpy(MainWnd_p->path, new_dir_fi_p->fullpath, MAXFFULLPATHLEN);
        
/*        // Unselect the new directory if selected in list
        for (int i = 2; i < MainWnd_p->list_p->size(); i++) {
            if (MainWnd_p->list_p->data(i) == (void*)new_dir_fi_p) {
                MainWnd_p->list_p->select(i, 0);
            }
        } */
        sync_gui_with_filesystem();
        
        if (VisitedDirCur != initVistedDirCur) {
			MainWnd_p->list_p->vposition(0);
		}

        //printf("switch_dir on exit: oldest %d, newest %d, dirs %d\n",
//                VisitedDirCur, VisitedDirNewest, VisitedDirs); fflush(0);
    }
}

void dirtree_change_cb(void)
{
    Fl_Select_Browser* p = MainWnd_p->tree_p;
    int selline = p->value();
    if (selline == 0) {
        selline = p->size();
        p->value(selline);
    }
    file_item* newdir_fi_p = (file_item*)p->data(selline);
    MainWnd_p->targdir_fi_p = newdir_fi_p;
    //printf( "You clicked line %d with button %d: %s\n", selline, 
    //        Fl::event_button(), MainWnd_p->seldir_fi_p->fullpath); fflush(0);
    if (Fl::event_button() == FL_LEFT_MOUSE) {
        if (MainWnd_p->mode == MAINWND_MODE_RENAMING) {
            MainWnd_p->end_renaming();
        }
        MainWnd_p->last_click_browser = LASTCLICK_IN_TREE;
        int first_click = (newdir_fi_p->children < 0);
        MainWnd_p->list_p->populate(newdir_fi_p);
        // let user toggle expanded state...
        if (Fl::event_x() < 16) {
            newdir_fi_p->expanded = !newdir_fi_p->expanded;
        }
        // unless this is the first time, in which case expand to show any subdirs 
        if (first_click && newdir_fi_p->subdirs > 1) {
            newdir_fi_p->expanded = 1;
        }
        switch_directory(newdir_fi_p);
    }
    else if (Fl::event_button() == FL_RIGHT_MOUSE) {
        MainWnd_p->last_click_browser = LASTCLICK_IN_TREE;
        //printf("Target directory for menu commands will be '%s'\n", MainWnd_p->targdir_fi_p->fullpath); fflush(0);
        do_dir_tree_menu();
    }
}

void filelist_change_cb(void)
{
    if (Fl::event_button() == FL_RIGHT_MOUSE ) {
        do_file_menu();
    }
    else {
        int row = MainWnd_p->list_p->value();
        if (row > 1) {
            MainWnd_p->sel_fi_p 
                = (file_item*)(MainWnd_p->list_p->data(row));
            if (MainWnd_p->sel_fi_p) {
                MainWnd_p->update_btnbar();
            }
        }
    }
}

void init_dir_watching(void) {
    DirWatch_fd = inotify_init();
    if (-1 == DirWatch_fd) {
        fl_alert("Directory monitoring capability missing!");
    }
}

void close_dir_watching(void) {
    close(DirWatch_fd);
}

const unsigned int INOTIFY_WATCH_FLAGS = IN_ATTRIB | IN_MODIFY | \
IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF | IN_MOVE;

const unsigned int INOTIFY_EVENT_FLAGS  = INOTIFY_WATCH_FLAGS | IN_UNMOUNT;
const char* dir_no_watch_list[64] = {
    "/dev",
    "",
    "",
    "",
};

void setup_dir_watch(file_item* dir_fi_p) {
    int w;//, ret;
    if (!dir_fi_p) {
        //printf("NULL directory item does not require a watch!\n");
        return;
    }
    
    // See if requested directory is on no-watch list
    int n = 0;
    const char* no_watch_dirname = dir_no_watch_list[n];
    while(*no_watch_dirname) {
        //printf("Checking if '%s' matches item '%s' from no-watch liist\n", 
        //        dir_fi_p->fullpath, no_watch_dirname);
        if (!strncmp(dir_fi_p->fullpath, no_watch_dirname, strlen(no_watch_dirname))) {
            //printf("Directory '%s' will not be watched\n", no_watch_dirname);
            return;
        }
        no_watch_dirname = dir_no_watch_list[++n];
    }
    
    //printf("Checking for a pre-existing watch descriptor for '%s'... checking \n", dir_fi_p->name);
    for (w = 0 ; w < DirWatches; w++) {
        //printf("%s, ", (Watched_fi_p[w]) ? Watched_fi_p[w]->name : "(null)");
        if (Watched_fi_p[w] == dir_fi_p) {
            //printf("found! It is already watched\n");
            return;
        }
    }
    DirWatch_wd[DirWatches] = inotify_add_watch(DirWatch_fd, 
                                dir_fi_p->fullpath, 
                                INOTIFY_WATCH_FLAGS);
    if (-1 == DirWatch_wd[DirWatches]) {
        perror("inotify_add_watch");
        //printf("Failed to add watch for '%s'\n", dir_fi_p->fullpath);
        return;
    }
    //printf("Added watch descriptor %d to slot %d for '%s'\n",  
    //        DirWatch_wd[DirWatches], DirWatches, dir_fi_p->fullpath);
    Watched_fi_p[DirWatches] = dir_fi_p;
    DirWatches++;
}

file_item* get_fi_pointer_from_wd(int wd) {
    int w;//, ret;
    //printf("Looking for watch descriptor %d... checking \n", wd);
    for (w = 0 ; w < DirWatches; w++) {
        //printf("[%d] = %d ,", w, DirWatch_wd[w]);
        if (DirWatch_wd[w] == wd) {
            //printf(" found! fi_p is 0x%08X '%s'\n", Watched_fi_p[w], 
            //(Watched_fi_p[w])? Watched_fi_p[w]->name: "(noname)");
            return Watched_fi_p[w];
        }
    }
    //printf("\nNot found.\n");
    return NULL;
}

void remove_dir_watches(void) {
    int w, ret;
    for (w = 0 ; w < DirWatches; w++) {
        ret = inotify_rm_watch(DirWatch_fd, DirWatch_wd[w]);
        if (ret) {
            perror("inotify_rm_watch");
        }
        else {
            //printf("Removing wd %d from slot %d\n", DirWatch_wd[w], w);
            DirWatch_wd[w] = -1;
            Watched_fi_p[w] = NULL;
        }
    }
    DirWatches = 0;
    fflush(0);
}

void remove_watch(file_item* dir_fi_p) {
    int w;//, ret;
    if (!dir_fi_p) {
        //printf("NULL directory item does not need to be removed!\n");
        return;
    }
    //printf("Looking for watch descriptor of '%s'... checking \n", dir_fi_p->name);
    for (w = 0 ; w < DirWatches; w++) {
        //printf("%s,", (Watched_fi_p[w]) ? Watched_fi_p[w]->name : "(null), ");
        if (Watched_fi_p[w] == dir_fi_p) {
            //printf("found! Nulling out Watched_fi_p[] element\n");
            Watched_fi_p[w] = NULL;
            return;
        }
    }
}


int dir_watch_events_pending_len(void) {
    int ret;
    unsigned int len;
    ret = ioctl(DirWatch_fd, FIONREAD, &len);
    if (ret >= 0 && len > 0) return len;
    else return 0;
}

char* get_file_selections(void) {
    int len = strlen(MainWnd_p->seldir_fi_p->fullpath) + 2;
    int i;
    for (i = 1; i <= MainWnd_p->list_p->size(); i++) {
        if (MainWnd_p->list_p->selected(i)) {
            len += strlen(((file_item*)(MainWnd_p->list_p->data(i)))->fullpath);
            len += 2;
        }
    }
    //printf ("File Selections may be buffered in %d bytes\n", len);
    char* buf = (char*)malloc(len);
    strclr(buf);
    strcpy(buf, MainWnd_p->seldir_fi_p->fullpath);
    strcat(buf, "\n");
    for (i = 1; i <= MainWnd_p->list_p->size(); i++) {
        if (MainWnd_p->list_p->selected(i)) {
            strcat(buf, ((file_item*)(MainWnd_p->list_p->data(i)))->fullpath);
            strcat(buf, "\n");
        }
    }
//    printf ("File Selections are:\n%s\n", buf);
    return buf;    
}

void reselect_files(const char* selections) {
    char pathname[MAXFFULLPATHLEN];
    char* p = pathname;
    const char* s = selections;
    int missing_count = 0;
    if (!s) {
        return;
    }
    while (*s != '\n' && *s) {
        *p++ = *s++;
    }
    if (*s == '\n') {
        s++;
        *p = '\0';
    }
    if (strcmp(pathname, MainWnd_p->seldir_fi_p->fullpath)) {
        //printf("Not the same directory... don't try to select\n");
        return;  // not the same directory... don't try to select
    }
    while (*s) {
        p = pathname;
        while (*s != '\n' && *s) {
            *p++ = *s++;
        }
        if (*s == '\n') {
            s++;
        }
        *p = '\0';
        int i;
        int found = 0;
        //printf("Trying to reselect '%s'\n", pathname);
        for (i = 2; i <= MainWnd_p->list_p->size(); i++) {
            file_item* file_p = (file_item*)(MainWnd_p->list_p->data(i));
            //printf(" - - List item is '%s'\n", file_p->fullpath);
            if (!strcmp(pathname, file_p->fullpath)) {
                MainWnd_p->list_p->select(i, 1);
                MainWnd_p->list_p->middleline(i);
                //printf(" ==== It matches ===\n");
                found = 1;
                break;
            }
        }
        if (!found)   missing_count++;
    }
}

void sync_gui_with_filesystem(int try_to_reselect) {
    //printf("Sync-ing GUI with filesystem...\n"); fflush(0);
    int lpos = MainWnd_p->list_p->vposition();
    MainWnd_p->set_titles();
    char* selections_str = NULL;
    if (try_to_reselect)   {
        selections_str = get_file_selections();
    }
    if (MainWnd_p->seldir_fi_p->parent_fi_p)   get_child_file_items(MainWnd_p->seldir_fi_p->parent_fi_p);
    get_child_file_items(MainWnd_p->seldir_fi_p);
    MainWnd_p->tree_p->populate();
    MainWnd_p->list_p->populate(MainWnd_p->seldir_fi_p);
    if (try_to_reselect)   {
        reselect_files(selections_str);
        free(selections_str);
    }
    MainWnd_p->list_p->vposition(lpos);
    MainWnd_p->update_btnbar();
}

void react_to_dir_watch_events(void) {
    ssize_t len = -1; 
    ssize_t i = 0;
    file_item* dir_fi_p = NULL;
    int need_to_sync = 0;
    double resync_delay = 1.0;
    
    //printf("%d dirs watched. %d bytes of event info pending\n", 
    //        DirWatches, dir_watch_events_pending_len());
    while (dir_watch_events_pending_len()) {
        len = read(DirWatch_fd, WatchEventBuf, WATCHBUFLEN);
        //printf("Pending watch event data length is %d\n", len); fflush(0);
        while (i < len) {
            struct inotify_event* event_p =
                (struct inotify_event*) &WatchEventBuf[i];
            // is this an event type we care about?
            if (INOTIFY_EVENT_FLAGS & event_p->mask) {
//                printf("Filesystem event: wd=%d, mask=0x%08X, cookie=%d, len=%d, isdir=%s",
//                        event_p->wd, event_p->mask, event_p->cookie,
//                        event_p->len, (event_p->mask & IN_ISDIR) ? "yes":"no"); fflush(0);
//                if (event_p->len) printf(", name='%s'\n", event_p->name);
//                else              printf("\n");
                
                dir_fi_p = get_fi_pointer_from_wd(event_p->wd);
                if (dir_fi_p) { 
                    
//                    printf("*** Watch event matches a file item '%s' ***\n", dir_fi_p->fullpath);
                    
                    if (    (MainWnd_p->seldir_fi_p->parent_fi_p != NULL)
                        &&  !strcmp(dir_fi_p->fullpath, MainWnd_p->seldir_fi_p->parent_fi_p->fullpath)
                        &&  (   (event_p->mask & IN_MOVED_FROM)
                                 || (event_p->mask & IN_DELETE)
                            )
                        &&  (event_p->len > 0)
                        &&  !strcmp(event_p->name, MainWnd_p->seldir_fi_p->name)
                    ) {
//                      printf("Selected directory '%s' has moved or been deleted...\n", 
//                              MainWnd_p->seldir_fi_p->name);
                        switch_directory(MainWnd_p->seldir_fi_p->parent_fi_p);
                        need_to_sync = 0;   // done by switch_directory() call
                    }
                    else if (   dir_fi_p == MainWnd_p->seldir_fi_p
                             || (   MainWnd_p->seldir_fi_p->parent_fi_p != NULL
                                 && dir_fi_p == MainWnd_p->seldir_fi_p->parent_fi_p
                                )
                    ) {
                        need_to_sync = 1;
                    }                       
                } 
            }
            i += sizeof(struct inotify_event) + event_p->len;
        }
    }
    //printf("Events handled. %d bytes of event info pending.  Need to sync? %d\n", 
    //        dir_watch_events_pending_len(), need_to_sync); fflush(0);
    double now = systemtime_as_real();
    if (need_to_sync)  {
        if (now > TimeToResync_rsec) {
            TimeToResync_rsec = now + resync_delay;
;
        }
    }
//    printf("t now is %1.3lf, rsync at %1.3lf (or in %1.3lf seconds)\n", 
//            now, TimeToResync_rsec, (TimeToResync_rsec - now));
}

int marked_file_items(const char* urls) 
{
    if (!urls) return 0;
    int cnt = 0;
    for(const char *c = urls; *c != '\0'; ++c)
    {
        if(*c == '\n')
            cnt++;
    }
    return cnt;
}

void dnd_receive(const char *urls, file_item *target_fi_p)
{
    int cnt = marked_file_items(urls);
    
    BatchReplaceMode = BATCH_ASK_EACH;
    int answer = fl_choice( "You dropped %d file%s on %s.\nWould you like to...", 
                            "cancel", "move to here", "copy to here", cnt, 
                            (cnt == 1) ? "" : "s", target_fi_p->fullpath);
    if (answer == 0) {
        return;
    }
    else {
        free(MarkedFileItems_sz_p);
        MarkedFileItems_sz_p = (char*)urls;
        perform_file_copy_or_move((answer == 2) ? COPY_OPER : MOVE_OPER, target_fi_p); 
        MarkedFileItems_sz_p = 0;
    }
    
    MainWnd_p->list_p->dragging = 0;
    sync_gui_with_filesystem();
}

void dirtree_dnd_cb(Fl_Widget *o, void *v)
{
    Fl_DND_Box *dnd = (Fl_DND_Box*)o;

    switch(dnd->event())
    {
        case FL_DND_DRAG:           // User is dragging widget
            {
                MainWnd_p->tree_p->drag_targ_itemnum 
                    = MainWnd_p->tree_p->item_number_under_mouse(Fl::event_y() - MainWnd_p->tree_p->y());
                MainWnd_p->tree_p->select(MainWnd_p->tree_p->drag_targ_itemnum);
            }
            break;

        case FL_PASTE:          // DND Drop
        {
            if (MainWnd_p->tree_p->drag_targ_itemnum) {
                file_item *droptarg_dir_fi_p 
                    = (file_item *)(MainWnd_p->tree_p->data(MainWnd_p->tree_p->drag_targ_itemnum));
                dnd_receive(dnd->event_text(), droptarg_dir_fi_p);
            }
            break;
        }
    }
}

void filelist_dnd_cb(Fl_Widget *o, void *v)
{
    Fl_DND_Box *dnd = (Fl_DND_Box*)o;

    switch(dnd->event())
    {
        case FL_DND_DRAG:           // User is dragging widget
            {
                MainWnd_p->list_p->deselect();
            }
            break;

        case FL_PASTE:          // DND Drop
        {
            if (MainWnd_p->seldir_fi_p) {
                //printf("directory '%s' will try to receive:\n%s\n------\n",
                //        MainWnd_p->seldir_fi_p->fullpath, dnd->event_text()); fflush(0);
                dnd_receive(dnd->event_text(), MainWnd_p->seldir_fi_p);
            }
            break;
        }
    }
}

void analyze_and_disp_status(void) 
{
        char status_str[256];
        char selfiles_str[128];
        char selfiles_sizestr[64];
        char pastefiles_str[128];
        
        strclr(selfiles_str);
        int selcount = MainWnd_p->list_p->num_selected();
        if (selcount > 0) {
            long long totsize = MainWnd_p->list_p->size_selected();
            bytecount_size_str(selfiles_sizestr, totsize);
            sprintf(selfiles_str, "%d file%s selected (%s)",
                    selcount, (selcount == 1)?"":"s", selfiles_sizestr);
        }
        
        strclr(pastefiles_str);
        int pastecnt = marked_file_items(MarkedFileItems_sz_p);
        if (pastecnt > 0) {
            sprintf(pastefiles_str, "%s%d file%s ready to paste",
                    (selcount > 0)?" | ":"",
                    pastecnt, (pastecnt==1)?"":"s");
        }
        sprintf(status_str, "%s%s", selfiles_str, pastefiles_str);
        //printf("%s\n", dirstatus_str); fflush(0);
        MainWnd_p->mainwnd_status_lbl_p->copy_label(status_str);
}

int handle_global_keys(int e) {
    int ctrl_active = Fl::event_state(FL_CTRL);
    int alt_active = Fl::event_state(FL_ALT | FL_META);
    int tree_click = (MainWnd_p->last_click_browser == LASTCLICK_IN_TREE);
    int key;
    Fluff_Window* p = MainWnd_p;
    file_item* new_dir_fi_p;
    int v = MainWnd_p->tree_p->value();
    int n = MainWnd_p->tree_p->size();
            
    key = Fl::event_key();
    //printf("Global keys fn, event %d, key %d '%c', keystate 0x%04X, last click %d\n", e, key, key, 
    //        Fl::event_state(), MainWnd_p->last_click_browser); fflush(0);
    switch ( e ) {
        case FL_SHORTCUT:
            if (Fl::event_key()==FL_Escape) {
                return 1;
            }
            return 0;
            break;

        case FL_KEYDOWN:
            key = Fl::event_key();
            //printf("Global Keydown... key is 0x%04X\n", key); fflush(0);
            if (key == 0) {                
                p->do_menu();
                return 1;
            } 
            if (    tree_click 
                &&  (p->mode != MAINWND_MODE_RENAMING)
                &&  ((key == '=') || (key == FL_Right))
            ) {
                MainWnd_p->seldir_fi_p->expanded = 1;
                MainWnd_p->tree_p->populate();
                return 1;
            }
            //else if (tree_click && ((key == '-') || (key == FL_Left))) {
            if (    tree_click 
                &&  (p->mode != MAINWND_MODE_RENAMING)
                &&  ((key == '-') || (key == FL_Left))
            ) {
                MainWnd_p->seldir_fi_p->expanded = 0;
                MainWnd_p->tree_p->populate();
                return 1;
            }
            else if (tree_click && (key == FL_Up) &&  v > 1) {
                new_dir_fi_p = (file_item*)MainWnd_p->tree_p->data(v - 1);
                switch_directory(new_dir_fi_p);
                //printf("seldir is %s\n", MainWnd_p->seldir_fi_p->fullpath); fflush(0);
                return 1;
            }
            else if (tree_click && (key == FL_Down) &&  v < n) {
                new_dir_fi_p = (file_item*)MainWnd_p->tree_p->data(v + 1);
                switch_directory(new_dir_fi_p);    
                //printf("seldir is %s\n", MainWnd_p->seldir_fi_p->fullpath); fflush(0);
                return 1;
            }
            else if (tree_click && (key == FL_Insert)) {
                DirTreeMenuCB((Fl_Widget*)NULL, (void*)MI_TRASH);
                return 1;
            }
            else if (key == FL_Enter && p->mode == MAINWND_MODE_RENAMING) {
                p->commit_renaming();
                return 1;
            }
            else if (key == (FL_F + 1)) {             
                btnbar_help_cb();
                return 1;
            }
            else if (key == (FL_F + 2) && p->mode == MAINWND_MODE_VIEWING) { 
                MainWnd_p->targdir_fi_p = MainWnd_p->seldir_fi_p;            
                begin_renaming();
                return 1;
            }
            else if (   key == (FL_F + 3)
                     || ((alt_active == 0) && (key == FL_Enter))) { 
                if (Fl::event_clicks() < 1) {
                    btnbar_action1_cb();
                }
                Fl::event_clicks(0);    // clear it so next enter is ok
                return 1;
            }
            else if (key == (FL_F + 4)) {
                btnbar_action2_cb();
                return 1;
            }
            else if (key == (FL_F + 5)) {             
                sync_gui_with_filesystem();
                return 1;
            }
            else if (key == (FL_F + 10)) {             
                btnbar_about_cb();
                return 1;
            }
            else if (key == (FL_F + 8)) {             
                OpenWithWnd_p->treat_generic_chk_p->value(0);
                OpenWithWnd_p->update();   
                OpenWithWnd_p->show();   
                return 1;
            }
            else if ((key == FL_Escape) && (p->mode == MAINWND_MODE_RENAMING)) {             
                p->end_renaming();
                return 1;
            }
            else if (ctrl_active && (key == 'a')) {             
                perform_select_all();
                return 1;
            }
            else if (ctrl_active && (key == 'c')) {             
                if (p->mode == MAINWND_MODE_RENAMING) 
                    return 0;
                else 
                    btnbar_copy_cb();
                return 1;
            }
            else if (ctrl_active && (key == 'v')) {
                if (p->mode == MAINWND_MODE_RENAMING) 
                    return 0;
                else if (MainWnd_p->paste_btn_p->active()) 
                    btnbar_paste_cb();
                return 1;
            }
            else if (ctrl_active && (key == 'q')) { 
                double now = systemtime_as_real();
                //printf("Enter event at %1.3lf, now is %1.3lf, delta %1.3lf\n", 
                //        GuiEnterEventTime_rsec, now, now - GuiEnterEventTime_rsec);
                if ((now - GuiEnterEventTime_rsec) > 0.150) {
                    btnbar_quit_cb();
                }
//                else {
//                    printf("Enter event too soon... ignoring spurious(?) key event!\n");
//                }
                return 1;
            }
            else if (   (ctrl_active && (key == 'h'))
                     || (key == FL_Home)) {
                btnbar_loc1_cb();
                return 1;
            }
            else if (ctrl_active && (key == 'e')) {             
                btnbar_loc2_cb();
                return 1;
            }
            else if (ctrl_active && (key == FL_BackSpace)) {
                if (MainWnd_p->seldir_fi_p->parent_fi_p) {
                    switch_directory(MainWnd_p->seldir_fi_p->parent_fi_p);
                }
                else {
                    switch_directory(Root_fi_p);
                } 
                return 1;
            } 
            else if (   (alt_active && key == FL_Enter)
                     || (key == (FL_F + 7))) {
                btnbar_props_cb();
                return 1;
            }
            else if (alt_active && (key == 't')) {
                system("xterm &");
                return 1;
            }
            else if (key == FL_Delete) {
                if (MainWnd_p->mode == MAINWND_MODE_RENAMING) {
                    return 1;   // ignore delete here, handled elsewhere
                }
                if (MainWnd_p->last_click_browser == LASTCLICK_IN_LIST) {           
                    btnbar_delete_cb();
                }
                else {
                    DirTreeMenuCB((Fl_Widget*)NULL, (void*)MI_DELETE);
                }
                return 1;
            }
            else if (key == FL_Insert) {             
                btnbar_trash_cb();
                return 1;
            }
            else if (key == FL_Escape) {             
                // eat the event, don't close app!
                // unselect everything
                for(int i = 1; i <= p->list_p->size(); i++) {
                    p->list_p->select(i, 0);
                }
                MainWnd_p->sel_fi_p = NULL;
                return 1;
            }
            return 0;
            break;

        default:
            return 0;
            break;

    }
    return 0;
}

Fl_Color faded(Fl_Color& clr, float amount)
{
    uchar r, g, b;
    Fl::get_color(clr, r, g, b);
    //float avg = (float)(r + g + b) / 3.0;
    
    r += (int)(amount*(150 - r));
    g += (int)(amount*(150 - g));
    b += (int)(amount*(150 - b));
    return fl_rgb_color(r, g, b);
}

void configure_colors(void) 
{
    uchar r, g, b;

    Fl_Color fg = FL_FOREGROUND_COLOR;  // text
    Fl::get_color(fg, r, g, b);
    //printf("Fg color components are %d, %d, %d\n", r, g, b);
    Fl::set_color(FLUFF_NORMTEXT_COLOR, r, g, b);

    Fl_Color fg2 = faded(fg, 0.8);
    Fl::get_color(fg2, r, g, b);
    //printf("Fg2 color components are %d, %d, %d\n", r, g, b);
    Fl::set_color(FLUFF_HIDETEXT_COLOR, r, g, b);

    Fl_Color bg = FL_BACKGROUND2_COLOR;  // list background
    Fl::get_color(bg, r, g, b);
    //printf("Bg color components are %d, %d, %d\n", r, g, b);
    Fl::set_color(FLUFF_LISTBG_COLOR, r, g, b);
    
    Fl_Color bg2 = faded(bg, 0.20);
    Fl::get_color(bg2, r, g, b);
    //printf("Bg2 color components are %d, %d, %d\n", r, g, b);
    Fl::set_color(FLUFF_LISTBG2_COLOR, r, g, b);
}

int main(int argc, char** argv) 
{
    int a = 1;
    //char fullfilename[MAXFFULLPATHLEN] = {0};

    int i;
    Fl::args(argc, argv, i);
    Fl::scheme(NULL);  // NULL causes libr. to look up style in .Xdefaults

    scan_users_and_groups();
    load_configuration();
    int err = mkdir(TrashbinPath_str, DEFAULT_DIR_PERMS);
    if (err && errno != EEXIST) {
        err = errno;
        fl_alert("error: cannot create trashbin directory '%s'. System returned (%d)\n%s",
                 TrashbinPath_str, err, strerror(err));
    }

    MainWnd_p = new Fluff_Window();
    MainWnd_p->setup();
    
    OpenWithWnd_p = new Open_With_Window(100, 100, 380, 280);
    OpenWithWnd_p->setup();        
    
    ManageAssocWnd_p = new Manage_Associations_Window(100, 100, 432, 300);
    ManageAssocWnd_p->setup();       
     

    ManageFiletypesWnd_p = new Manage_Filetypes_Window(100, 100, 432, 408);
    ManageFiletypesWnd_p->setup();

    Root_fi_p = new file_item();
    strclr(Root_fi_p->name);
    strclr(Root_fi_p->path);
    strnzcpy(Root_fi_p->fullpath, "/", MAXFFULLPATHLEN);
    stat64("/", &(Root_fi_p->status));
    Root_fi_p->expanded = 1;

    build_tree_for_branch(Loc1Path_str, Root_fi_p, Loc1Path_str, &Loc1_fi_p);
    struct stat dummy;
    if (!strcmp(Loc2Label_str, "TCE")) {
        if (!stat("/etc/sysconfig/tcedir", &dummy)) {
			strnzcpy(Loc2Path_str, "/etc/sysconfig/tcedir", MAXFPATHLEN);
		}
    }
    if (!strlen(Loc2Path_str)) {
		strnzcpy(Loc2Path_str, "/opt", MAXFPATHLEN);  // let's just use /opt
		MainWnd_p->loc2_btn_p->label("/opt");
	}	
    build_tree_for_branch(Loc2Path_str, Root_fi_p, Loc2Path_str, &Loc2_fi_p);

    if (argc > 1) { 
        strnzcpy(MainWnd_p->path, argv[a], MAXFFULLPATHLEN);
    }
    else {
        strnzcpy(MainWnd_p->path, get_current_dir_name(), MAXFFULLPATHLEN);
    }
    build_tree_for_branch(MainWnd_p->path, Root_fi_p, MainWnd_p->path, 
                          & (MainWnd_p->seldir_fi_p));
    
    init_dir_watching();
    if (!MainWnd_p->seldir_fi_p) {
        MainWnd_p->seldir_fi_p = Root_fi_p;
    }


    VisitedDir[0] = MainWnd_p->seldir_fi_p;
    VisitedDirs = 1;

    Fl::visual(FL_DOUBLE|FL_INDEX);
    MainWnd_p->show(argc, argv);
//    MainWnd_p->show();
    configure_colors();
    MainWnd_p->list_p->populate(MainWnd_p->seldir_fi_p);
    MainWnd_p->tree_p->populate();
    
    const char* iconfilepath1 = {"/usr/local/share/pixmaps/fluff.png"};
    const char* iconfilepath2 = {"/usr/local/share/pixmaps/core.png"};
    const char* iconfilepath = NULL;
    Fl_Box* icon_p = NULL;
    Fl_PNG_Image* icon_img_p = NULL;
    
    if (access(iconfilepath1, F_OK) == 0) {
		iconfilepath = iconfilepath1;
	}
	else if (access(iconfilepath2, F_OK) == 0) {
		iconfilepath = iconfilepath2;
	}
	if (iconfilepath) {
		icon_p = (Fl_Box*)fl_message_icon();
		icon_p->label("");
		icon_p->align(FL_ALIGN_IMAGE_BACKDROP);
		icon_img_p = new Fl_PNG_Image(iconfilepath);
		icon_p->image(icon_img_p);
	}
    
    MainWnd_p->set_titles();
    if (MainWnd_p->seldir_fi_p) {
		switch_directory(MainWnd_p->seldir_fi_p);
        //MainWnd_p->pathhist_menu_p->add(MainWnd_p->seldir_fi_p->fullpath);
        //MainWnd_p->pathhist_menu_p->label(MainWnd_p->seldir_fi_p->fullpath);
    }    
    MainWnd_p->last_click_browser = LASTCLICK_IN_TREE;

    OpenWithWnd_p->update();

    Running = 1;
    int sized = 0;
    while(Running && (0 <= Fl::wait(0.5))) {
        if (MainWnd_p->pathhist_menu_p->x() > 0 && !sized) {    
            MainWnd_p->adjust_pathhist_menu_size();
            MainWnd_p->redraw();
            sized = 1;
        }

        if (!MainWnd_p->visible()) {
            //printf("MainWnd_p->shown() is %d\n", MainWnd_p->shown());
            if (MainWnd_p->shown()) {
                Running = 1;    // it's only iconified
            }
            else {
                Running = 0;    // it's really going away
            }
        }
        else if (dir_watch_events_pending_len()) {
            react_to_dir_watch_events();
        }
        else if (   !OperationInProgress 
                 && (systemtime_as_real() > TimeToResync_rsec) 
                 && (TimeToResync_rsec >= 0.0)
                ) 
        {
            sync_gui_with_filesystem(1); // 1 = try to reselect
            TimeToResync_rsec = -1.0;   // flag for "no resync needed
        }
        

    };
    save_configuration();
    close_dir_watching();
    printf("Fluff exited normally.\n");
    return 0;
} 

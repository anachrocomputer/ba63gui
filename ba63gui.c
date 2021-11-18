/* ba63gui --- simple GTK+ GUI for BA63 display             2014-06-28 */
/* Copyright (c) 2014 John Honniball, Froods Software Development      */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>


#define ESC    (0x1b)

#define MAXMSGS (6)
#define MAXCOLS (20)    // Display has 20 characters per line
#define MAXROWS (4)     // BA66 display has four rows, BA63 has two
#define MAXNAME (16)    // Max length of name of a preset

#define LUG_PRESET      (1)
#define MFUK_PRESET     (2)
#define BRISTOL_PRESET  (3)
#define DMMF_PRESET     (4)
#define BVOS_PRESET     (5)
#define BRIGHTON_PRESET (6)
#define MEME_PRESET     (7)

int Fd = 0;

struct Item {
   int which;
   GtkEntry *entry[MAXROWS];
   GtkButton *button;
};

struct PresetButton {
   int which;
   char label[MAXNAME];
   GtkButton *button;
};

struct Item Message[MAXMSGS];
struct PresetButton Preset[] = {
   {MFUK_PRESET,     "MFUK",     NULL},
   {BRISTOL_PRESET,  "Bristol",  NULL},
   {LUG_PRESET,      "LUG",      NULL},
   {DMMF_PRESET,     "Derby",    NULL},
   {BVOS_PRESET,     "BV",       NULL},
   {BRIGHTON_PRESET, "Brighton", NULL},
   {MEME_PRESET,     "Memes",    NULL},
   {0,               "",         NULL}
};
int Curmsg = 0;
int Autoadvance = FALSE;
GtkSpinButton *Time_spin;
GtkToggleButton *Auto_button;


static int openBA63Port(const char *const port)
{
   struct termios tbuf;
   long int fdflags;

   const int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
   
   if (fd < 0) {
      perror(port);
      exit(1);
   }
   
   if ((fdflags = fcntl(fd, F_GETFL, NULL)) < 0) {
      perror("fcntl GETFL");
      exit(1);
   }
   
   fdflags &= ~O_NONBLOCK;
   
   if (fcntl(fd, F_SETFL, fdflags) < 0) {
      perror("fcntl SETFL");
      exit(1);
   }

   if (tcgetattr(fd, &tbuf) < 0) {
      perror("tcgetattr");
      exit(1);
   }
   
   cfsetospeed(&tbuf, B9600);
   cfsetispeed(&tbuf, B9600);
   cfmakeraw(&tbuf);
   
   tbuf.c_cflag |= PARENB | PARODD | CLOCAL;
   
   if (tcsetattr(fd, TCSAFLUSH, &tbuf) < 0) {
      perror("tcsetattr");
      exit(1);
   }
   
   return (fd);
}


void ba63send(const char *const str)
{
   const int n = strlen(str);
   
   if (write(Fd, str, n) != n)
      perror("write");
}


void ba63cls(void)
{
   ba63send("\x1b[2J"); // Clear screen
}


void ba63charset(const int countryCode)
{
   char str[4];
   
   str[0] = ESC;
   str[1] = 'R';
   str[2] = countryCode;
   str[3] = '\0';
   
   ba63send(str); // Select national character set
}


void ba63home(void)
{
   ba63send("\x1b[H"); // Home cursor
}


int isBlank(const int msg)
{
   const char *str1 = gtk_entry_get_text(Message[msg].entry[0]);
   const char *str2 = gtk_entry_get_text(Message[msg].entry[1]);
   
   return ((*str1 == '\0') && (*str2 == '\0'));
}


/* show_next --- show next pair of messages in sequence */

void show_next(void)
{
   int n = 0;

   do {
      Curmsg = (Curmsg + 1) % MAXMSGS;
   } while (isBlank (Curmsg) && (++n <= MAXMSGS));

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(Message[Curmsg].button), TRUE);
}


/* timer_callback --- function called from timeout if in auto mode */

static gboolean timer_callback(gpointer data)
{
   if (Autoadvance) {
      show_next();
      return (TRUE);
   }
   else
      return (FALSE);
}


/* show_message --- show a pair of text strings on the BA63 display */

void show_message(const int i)
{
   const char *str1 = gtk_entry_get_text(Message[i].entry[0]);
   const char *str2 = gtk_entry_get_text(Message[i].entry[1]);

   ba63home();
   ba63cls();

   ba63send(str1);
   ba63send("\r\n");
   ba63send(str2);
}


/* show_button --- respond to a click on the "Show" radio button */

static void show_button(GtkWidget *widget, gpointer data)
{
   const struct Item *p = (const struct Item *)data;
   const int i = p->which;

   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
//    g_print("Show button %d was pressed\n", i);
      show_message(i);
      Curmsg = i;
   }
   else {
//    g_print("Show button %d was released\n", i);
   }
}


/* auto_button --- switch on or off auto-advance mode */

static void auto_button(GtkWidget *widget, gpointer data)
{
   if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
//    g_print("Auto button was pressed\n");

      /* Re-select Curmsg here to clear test mode */
      show_message(Curmsg);

      const int seconds = gtk_spin_button_get_value_as_int(Time_spin);
//    g_print("seconds = %d\n", seconds);

      if (seconds > 0) {
         g_timeout_add_seconds(seconds, timer_callback, NULL);
         Autoadvance = TRUE;
      }
   }
   else {
//    g_print("Auto button was released\n");
      Autoadvance = FALSE;
   }
}


/* test_button --- light all pixels on the BA63 display */

static void test_button(GtkWidget *widget, gpointer data)
{
   char str[MAXCOLS + 1];
   int i;
   
// g_print("Test button was pressed\n");

   gtk_toggle_button_set_active(Auto_button, FALSE);

   for (i = 0; i < MAXCOLS; i++)
      str[i] = 0xdb;
      
   str[MAXCOLS] = '\0';
   
   ba63home();
   ba63cls();
   ba63send(str);
   ba63send("\r\n");
   ba63send(str);
}


/* next_button --- manual advance to next message */

static void next_button(GtkWidget *widget, gpointer data)
{
// g_print("Manual Next button was pressed\n");

   show_next();
}


/* preset_click --- fill in text fields with pre-set strings */

static void preset_click(GtkWidget *widget, gpointer data)
{
   const struct PresetButton *const p = (const struct PresetButton *const)data;
   int i;

// g_print ("Preset button %d was clicked\n", p->which);

   for (i = 0; i < MAXMSGS; i++) {
      gtk_entry_set_text(Message[i].entry[2], "");
      gtk_entry_set_text(Message[i].entry[3], "");
   }

   switch (p->which) {
   case LUG_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "  BRISTOL AND BATH");
      gtk_entry_set_text(Message[0].entry[1], "  LINUX USER GROUP");
      gtk_entry_set_text(Message[1].entry[0], "  Bristol and Bath");
      gtk_entry_set_text(Message[1].entry[1], "  Linux User Group");
      gtk_entry_set_text(Message[2].entry[0], "");
      gtk_entry_set_text(Message[2].entry[1], "");
      gtk_entry_set_text(Message[3].entry[0], "");
      gtk_entry_set_text(Message[3].entry[1], "");
      gtk_entry_set_text(Message[4].entry[0], "");
      gtk_entry_set_text(Message[4].entry[1], "");
      gtk_entry_set_text(Message[5].entry[0], "");
      gtk_entry_set_text(Message[5].entry[1], "");
      break;
   case MFUK_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "   MAKER FAIRE UK");
      gtk_entry_set_text(Message[0].entry[1], "   NEWCASTLE 2013");
      gtk_entry_set_text(Message[1].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[1].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[2].entry[0], "JOHN HONNIBALL");
      gtk_entry_set_text(Message[2].entry[1], "BRISTOL HACKSPACE");
      gtk_entry_set_text(Message[3].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[3].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[4].entry[0], "@anachrocomputer");
      gtk_entry_set_text(Message[4].entry[1], "   #MakerFaireUK");
      gtk_entry_set_text(Message[5].entry[0], "");
      gtk_entry_set_text(Message[5].entry[1], "");
      break;
   case BRISTOL_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "BRISTOL MINI");
      gtk_entry_set_text(Message[0].entry[1], "MAKER FAIRE 2014");
      gtk_entry_set_text(Message[1].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[1].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[2].entry[0], "JOHN HONNIBALL");
      gtk_entry_set_text(Message[2].entry[1], "BRISTOL HACKSPACE");
      gtk_entry_set_text(Message[3].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[3].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[4].entry[0], "@anachrocomputer");
      gtk_entry_set_text(Message[4].entry[1], "           #bmmf");
      gtk_entry_set_text(Message[5].entry[0], "");
      gtk_entry_set_text(Message[5].entry[1], "");
      break;
   case DMMF_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "DERBY MINI");
      gtk_entry_set_text(Message[0].entry[1], "MAKER FAIRE 2014");
      gtk_entry_set_text(Message[1].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[1].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[2].entry[0], "JOHN HONNIBALL");
      gtk_entry_set_text(Message[2].entry[1], "BRISTOL HACKSPACE");
      gtk_entry_set_text(Message[3].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[3].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[4].entry[0], "@anachrocomputer");
      gtk_entry_set_text(Message[4].entry[1], "         #DMMF14");
      gtk_entry_set_text(Message[5].entry[0], "");
      gtk_entry_set_text(Message[5].entry[1], "");
      break;
   case BVOS_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "BRISTOL HACKSPACE");
      gtk_entry_set_text(Message[0].entry[1], "  BV Studios 2014");
      gtk_entry_set_text(Message[1].entry[0], "BRISTOL HACKSPACE");
      gtk_entry_set_text(Message[1].entry[1], "John Honniball");
      gtk_entry_set_text(Message[2].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[2].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[3].entry[0], "EARTH DEMOLITION");
      gtk_entry_set_text(Message[3].entry[1], "SIMULATOR GAME");
      gtk_entry_set_text(Message[4].entry[0], "@anachrocomputer");
      gtk_entry_set_text(Message[4].entry[1], "");
      gtk_entry_set_text(Message[5].entry[0], "");
      gtk_entry_set_text(Message[5].entry[1], "");
      break;
   case BRIGHTON_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "BRIGHTON MINI");
      gtk_entry_set_text(Message[0].entry[1], "MAKER FAIRE 2014");
      gtk_entry_set_text(Message[1].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[1].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[2].entry[0], "JOHN HONNIBALL");
      gtk_entry_set_text(Message[2].entry[1], "BRISTOL HACKSPACE");
      gtk_entry_set_text(Message[3].entry[0], "FUN WITH FLAT-BED");
      gtk_entry_set_text(Message[3].entry[1], "PEN PLOTTERS");
      gtk_entry_set_text(Message[4].entry[0], "@anachrocomputer");
      gtk_entry_set_text(Message[4].entry[1], "           #bmmf");
      gtk_entry_set_text(Message[5].entry[0], "");
      gtk_entry_set_text(Message[5].entry[1], "");
      break;
   case MEME_PRESET:
      gtk_entry_set_text(Message[0].entry[0], "   ALL YOUR BASE");
      gtk_entry_set_text(Message[0].entry[1], "  ARE BELONG TO US");
      gtk_entry_set_text(Message[1].entry[0], "   FOUR SEASONS");
      gtk_entry_set_text(Message[1].entry[1], " TOTAL LANDSCAPING");
      gtk_entry_set_text(Message[2].entry[0], "SOON MAY THE");
      gtk_entry_set_text(Message[2].entry[1], " WELLERMAN COME");
      gtk_entry_set_text(Message[2].entry[2], "TO BRING US SUGAR");
      gtk_entry_set_text(Message[2].entry[3], " AND TEA AND RUM");
      gtk_entry_set_text(Message[3].entry[0], "        EVER");
      gtk_entry_set_text(Message[3].entry[1], "        GIVEN");
      gtk_entry_set_text(Message[4].entry[0], "NOTES ARE DISPENSED");
      gtk_entry_set_text(Message[4].entry[1], " BELOW THE SCANNER");
      gtk_entry_set_text(Message[5].entry[0], "  UNEXPECTED ITEM");
      gtk_entry_set_text(Message[5].entry[1], "  IN BAGGING AREA");
      break;
   }
}


/* delete_event --- callback for window deletion */
static gboolean delete_event(GtkWidget *widget,
                       GdkEvent  *event,
                       gpointer   data)
{
   gtk_main_quit();
   ba63cls();

   return (FALSE);
}


/* make_preset --- make a button for selecting a set of strings */

static void make_preset(GtkWidget *hbox, struct PresetButton *p)
{
   GtkWidget *button;
   
   button = gtk_button_new_with_label(p->label);

   g_signal_connect(button, "clicked", G_CALLBACK(preset_click), (gpointer)p);

   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

   gtk_widget_show(button);
   
   p->button = GTK_BUTTON(button);
}


/* make_message_controls --- make a group of controls for each message */

static void make_message_controls(GtkWidget *grid, const int i, const int col, const int row, const int lines)
{
   static GtkRadioButton *group = NULL;
   GtkWidget *radio;
   GtkWidget *hbox;
   GtkWidget *ebox;
   GtkWidget *entry;
   char showtip[32];
   int j;
   PangoFontDescription *mono_font;
// GdkColor cyan  = {0, 0x0000, 0xffff, 0xffff};
// GdkColor black = {0, 0x0000, 0x0000, 0x0000};
// GdkColor red   = {0, 0xffff, 0x0000, 0x0000};
   
   mono_font = pango_font_description_from_string("monospace");

   /* Make a horizontal box for the two text fields and a button */
   hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

   gtk_widget_show(hbox);

   gtk_grid_attach(GTK_GRID(grid), hbox, col, row, 1, 1);

   /* Make a vertical box for the two entry fields */
   ebox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

   /* Put the vertical box into the horizontal box */
   gtk_box_pack_start(GTK_BOX(hbox), ebox, FALSE, TRUE, 0);

   gtk_widget_show(ebox);

   /* Make text entry fields */
   for (j = 0; j < MAXROWS; j++) {
      entry = gtk_entry_new();
      gtk_widget_override_font(entry, mono_font);
      gtk_entry_set_max_length(GTK_ENTRY(entry), MAXCOLS);
      gtk_entry_set_width_chars(GTK_ENTRY(entry), MAXCOLS);
//    gtk_widget_modify_fg(entry, GTK_STATE_NORMAL, &cyan);
//    gtk_widget_modify_text(entry, GTK_STATE_NORMAL, &cyan);
//    gtk_widget_modify_base(entry, GTK_STATE_NORMAL, &black);
//    gtk_widget_modify_bg(entry, GTK_STATE_NORMAL, &black);
//    gtk_widget_modify_fg(entry, GTK_STATE_PRELIGHT, &red);
//    gtk_widget_modify_bg(entry, GTK_STATE_PRELIGHT, &black);
//    gtk_widget_modify_fg(entry, GTK_STATE_ACTIVE, &red);
//    gtk_widget_modify_bg(entry, GTK_STATE_ACTIVE, &black);
//    gtk_widget_modify_fg(entry, GTK_STATE_SELECTED, &black);
//    gtk_widget_modify_bg(entry, GTK_STATE_SELECTED, &red);
      
      Message[i].entry[j] = GTK_ENTRY(entry);
      
      gtk_box_pack_start(GTK_BOX(ebox), entry, TRUE, TRUE, 0);
      
      if (j >= lines)
         gtk_widget_set_sensitive(entry, FALSE);

      gtk_widget_show(entry);
   }

   /* Each message has a "Show" radio button */
   radio = gtk_radio_button_new_with_label_from_widget(group, "Show");

   Message[i].which = i;
   Message[i].button = GTK_BUTTON(radio);
   
   group = GTK_RADIO_BUTTON(radio);
   
   gtk_box_pack_start(GTK_BOX(hbox), radio, TRUE, TRUE, 0);

   g_signal_connect(radio, "clicked", G_CALLBACK(show_button), (gpointer)&Message[i]);

   sprintf(showtip, "Show message %d", i + 1);
   gtk_widget_set_tooltip_text(radio, showtip);
   gtk_widget_show(radio);
}


int main(int argc, char *argv[])
{
   /* GtkWidget is the storage type for widgets */
   GtkWidget *window;
   GtkWidget *button;
   GtkWidget *frame;
   GtkWidget *vbox;
   GtkWidget *grid;
   GtkWidget *hbox;
   GtkWidget *check;
   GtkWidget *label;
   GtkAdjustment *adjustment;
   int i;

   printf("GTK V%d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
   
   gtk_init(&argc, &argv);

   /* Create the main window */
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

   gtk_window_set_title(GTK_WINDOW(window), "BA63 Display");

   /* Set a handler for delete-event that exits */
   g_signal_connect(window, "delete-event", G_CALLBACK(delete_event), NULL);

   /* Set the border width of the window */
   gtk_container_set_border_width(GTK_CONTAINER(window), 10);

   /* Vertical box contains rows of text fields and buttons */
   vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
   
   /* Put the box into the main window */
   gtk_container_add(GTK_CONTAINER(window), vbox);

   /* Grid contains rows and columns of text fields and radio buttons */
   grid = gtk_grid_new();

   gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 0);
   
   gtk_widget_show(grid);
   
   /* Make text fields and radio button for each message */
   for (i = 0; i < MAXMSGS; i++)
      make_message_controls(grid, i, i % 3, i / 3, 2);

   /* Create a new button with the label "Lamp Test" */
   button = gtk_button_new_with_label("Lamp Test");
   gtk_widget_set_tooltip_text(button, "Test with all pixels lit");
   
   g_signal_connect(button, "clicked", G_CALLBACK(test_button), (gpointer)NULL);

   gtk_box_pack_start(GTK_BOX(vbox), button, TRUE, TRUE, 0);

   gtk_widget_show(button);

   /* Make a frame to contain the preset buttons */
   frame = gtk_frame_new("Presets");
   gtk_widget_set_tooltip_text(frame, "Messages for any occasion");
   gtk_widget_show(frame);
   gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

   /* Make a horizontal box for the preset buttons */
   hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_widget_show(hbox);
   gtk_container_add(GTK_CONTAINER(frame), hbox);

   for (i = 0; Preset[i].which != 0; i++)
      make_preset(hbox, &Preset[i]);

   /* Make a frame to contain the auto controls */
   frame = gtk_frame_new("Advance to Next");
   gtk_widget_show(frame);
   gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);

   /* Make a horizontal box for the auto controls */
   hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_widget_show(hbox);
   gtk_container_add(GTK_CONTAINER(frame), hbox);

   check = gtk_check_button_new_with_label("Auto");
   gtk_widget_set_tooltip_text(check, "Automatically advance to next message");
   gtk_widget_show(check);
   gtk_box_pack_start(GTK_BOX(hbox), check, TRUE, TRUE, 0);
   g_signal_connect(check, "clicked", G_CALLBACK(auto_button), (gpointer)NULL);
   Auto_button = GTK_TOGGLE_BUTTON(check);

   /* Spin-box for timeout value in seconds */
   adjustment = gtk_adjustment_new(5.0, 2.0, 3600.0, 1.0, 5.0, 0.0);
   button = gtk_spin_button_new(adjustment, 1.0, 0);
   gtk_widget_set_tooltip_text(button, "Delay in seconds before auto advance");
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   gtk_widget_show(button);
   Time_spin = GTK_SPIN_BUTTON(button);
   
   label = gtk_label_new("seconds");
   gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   /* Button for manual advance */
   button = gtk_button_new_with_label("Manual");
   gtk_widget_set_tooltip_text(button, "Manually advance to next message");
   gtk_widget_show(button);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   g_signal_connect(button, "clicked", G_CALLBACK(next_button), (gpointer)NULL);

   gtk_widget_show(vbox);

   gtk_widget_show(window);
   
   /* Open the serial port connection to the BA63 */
   Fd = openBA63Port("/dev/ttyUSB0");
   
   ba63home();
   ba63cls();
   ba63charset(6);
   
   gtk_main();

   return (0);
}

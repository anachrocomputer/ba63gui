/* ba63gui --- simple GTK+ GUI for BA63 display             2014/06/28 */
/* Copyright (c) 2014 John Honniball, Froods Software Development      */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MAXMSGS (6)

#define LUG_PRESET (1)
#define MFUK_PRESET (2)
#define BRISTOL_PRESET (3)
#define DMMF_PRESET (4)
#define BVOS_PRESET (5)
#define BRIGHTON_PRESET (6)

int Fd = 0;

struct Item {
   GtkEntry *entry1;
   GtkEntry *entry2;
   GtkButton *button;
};

struct Item Message[MAXMSGS];
int Curmsg = 0;
int Autoadvance = FALSE;
GtkSpinButton *Time_spin;
GtkToggleButton *Auto_button;

const int ESC = 0x1b;


static int openBA63Port (const char *port)
{
   int fd;
   struct termios tbuf;
   long int fdflags;

   fd = open (port, O_RDWR | O_NOCTTY | O_NDELAY);
   
   if (fd < 0) {
      perror (port);
      exit (1);
   }
   
   if ((fdflags = fcntl (fd, F_GETFL, NULL)) < 0) {
      perror ("fcntl GETFL");
      exit (1);
   }
   
   fdflags &= ~O_NDELAY;
   
   if (fcntl (fd, F_SETFL, fdflags) < 0) {
      perror ("fcntl SETFL");
      exit (1);
   }

   if (tcgetattr (fd, &tbuf) < 0) {
      perror ("tcgetattr");
      exit (1);
   }
   
   cfsetospeed (&tbuf, B9600);
   cfsetispeed (&tbuf, B9600);
   cfmakeraw (&tbuf);
   
   tbuf.c_cflag |= PARENB | PARODD | CLOCAL;
   
   if (tcsetattr (fd, TCSAFLUSH, &tbuf) < 0) {
      perror ("tcsetattr");
      exit (1);
   }
   
   return (fd);
}


void ba63send (const char *str)
{
   int n = strlen (str);
   
   if (write (Fd, str, n) != n)
      perror ("write");
}


void ba63cls (void)
{
   ba63send ("\x1b[2J"); // Clear screen
}


void ba63charset (int countryCode)
{
   char str[4];
   
   str[0] = ESC;
   str[1] = 'R';
   str[2] = countryCode;
   str[3] = '\0';
   
   ba63send (str); // Select national character set
}


void ba63home (void)
{
   ba63send ("\x1b[H"); // Home cursor
}


int isBlank (int msg)
{
   const char *str1, *str2;
   
   str1 = gtk_entry_get_text (Message[msg].entry1);
   str2 = gtk_entry_get_text (Message[msg].entry2);
   
   return ((*str1 == '\0') && (*str2 == '\0'));
}


/* show_next --- show next pair of messages in sequence */

void show_next (void)
{
   int n = 0;

   do {
      Curmsg = (Curmsg + 1) % MAXMSGS;
   } while (isBlank (Curmsg) && (++n <= MAXMSGS));

   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (Message[Curmsg].button), TRUE);
}


/* timer_callback --- function called from timeout if in auto mode */

static gboolean timer_callback (gpointer data)
{
   if (Autoadvance) {
      show_next ();
      return (TRUE);
   }
   else
      return (FALSE);
}


/* show_message --- show a pair of text strings on the BA63 display */

void show_message (int i)
{
   const char *str;

   ba63home ();
   ba63cls ();

   str = gtk_entry_get_text (Message[i].entry1);

   ba63send (str);
   ba63send ("\r\n");

   str = gtk_entry_get_text (Message[i].entry2);

   ba63send (str);
}


/* show_button --- respond to a click on the "Show" radio button */

static void show_button (GtkWidget *widget, gpointer data)
{
   int i = (int)data;

   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
//    g_print ("Show button %d was pressed\n", (int)data);
      show_message (i);
      Curmsg = i;
   }
   else {
//    g_print ("Show button %d was released\n", (int)data);
   }
}


/* auto_button --- switch on or off auto-advance mode */

static void auto_button (GtkWidget *widget, gpointer data)
{
   int seconds;

   if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
//    g_print ("Auto button was pressed\n");

   /* Re-select Curmsg here to clear test mode */
   show_message (Curmsg);

      seconds = gtk_spin_button_get_value_as_int (Time_spin);
//    g_print ("seconds = %d\n", seconds);

      if (seconds > 0) {
         g_timeout_add_seconds (seconds, timer_callback, 0);
         Autoadvance = TRUE;
      }
   }
   else {
//    g_print ("Auto button %d was released\n", (int)data);
      Autoadvance = FALSE;
   }
}


/* test_button --- light all pixels on the BA63 display */

static void test_button (GtkWidget *widget, gpointer data)
{
   char str[21];
   int i;
   
// g_print ("Test button was pressed\n");

   gtk_toggle_button_set_active (Auto_button, FALSE);

   for (i = 0; i < 20; i++)
      str[i] = 0xdb;
      
   str[20] = '\0';
   
   ba63home ();
   ba63cls ();
   ba63send (str);
   ba63send ("\r\n");
   ba63send (str);
}


/* next_button --- manual advance to next message */

static void next_button (GtkWidget *widget, gpointer data)
{
// g_print ("Manual Next button was pressed\n");

   show_next ();
}


/* preset_button --- fill in text fields with pre-set strings */

static void preset_button (GtkWidget *widget, gpointer data)
{
// g_print ("Preset button %d was pressed\n", (int)data);

   switch ((int)data) {
   case LUG_PRESET:
      gtk_entry_set_text (Message[0].entry1, "  BRISTOL AND BATH");
      gtk_entry_set_text (Message[0].entry2, "  LINUX USER GROUP");
      gtk_entry_set_text (Message[1].entry1, "  Bristol and Bath");
      gtk_entry_set_text (Message[1].entry2, "  Linux User Group");
      gtk_entry_set_text (Message[2].entry1, "");
      gtk_entry_set_text (Message[2].entry2, "");
      gtk_entry_set_text (Message[3].entry1, "");
      gtk_entry_set_text (Message[3].entry2, "");
      gtk_entry_set_text (Message[4].entry1, "");
      gtk_entry_set_text (Message[4].entry2, "");
      gtk_entry_set_text (Message[5].entry1, "");
      gtk_entry_set_text (Message[5].entry2, "");
      break;
   case MFUK_PRESET:
      gtk_entry_set_text (Message[0].entry1, "   MAKER FAIRE UK");
      gtk_entry_set_text (Message[0].entry2, "   NEWCASTLE 2013");
      gtk_entry_set_text (Message[1].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[1].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[2].entry1, "JOHN HONNIBALL");
      gtk_entry_set_text (Message[2].entry2, "BRISTOL HACKSPACE");
      gtk_entry_set_text (Message[3].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[3].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[4].entry1, "@anachrocomputer");
      gtk_entry_set_text (Message[4].entry2, "   #MakerFaireUK");
      gtk_entry_set_text (Message[5].entry1, "");
      gtk_entry_set_text (Message[5].entry2, "");
      break;
   case BRISTOL_PRESET:
      gtk_entry_set_text (Message[0].entry1, "BRISTOL MINI");
      gtk_entry_set_text (Message[0].entry2, "MAKER FAIRE 2014");
      gtk_entry_set_text (Message[1].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[1].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[2].entry1, "JOHN HONNIBALL");
      gtk_entry_set_text (Message[2].entry2, "BRISTOL HACKSPACE");
      gtk_entry_set_text (Message[3].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[3].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[4].entry1, "@anachrocomputer");
      gtk_entry_set_text (Message[4].entry2, "           #bmmf");
      gtk_entry_set_text (Message[5].entry1, "");
      gtk_entry_set_text (Message[5].entry2, "");
      break;
   case DMMF_PRESET:
      gtk_entry_set_text (Message[0].entry1, "DERBY MINI");
      gtk_entry_set_text (Message[0].entry2, "MAKER FAIRE 2014");
      gtk_entry_set_text (Message[1].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[1].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[2].entry1, "JOHN HONNIBALL");
      gtk_entry_set_text (Message[2].entry2, "BRISTOL HACKSPACE");
      gtk_entry_set_text (Message[3].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[3].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[4].entry1, "@anachrocomputer");
      gtk_entry_set_text (Message[4].entry2, "         #DMMF14");
      gtk_entry_set_text (Message[5].entry1, "");
      gtk_entry_set_text (Message[5].entry2, "");
      break;
   case BVOS_PRESET:
      gtk_entry_set_text (Message[0].entry1, "BRISTOL HACKSPACE");
      gtk_entry_set_text (Message[0].entry2, "  BV Studios 2014");
      gtk_entry_set_text (Message[1].entry1, "BRISTOL HACKSPACE");
      gtk_entry_set_text (Message[1].entry2, "John Honniball");
      gtk_entry_set_text (Message[2].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[2].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[3].entry1, "EARTH DEMOLITION");
      gtk_entry_set_text (Message[3].entry2, "SIMULATOR GAME");
      gtk_entry_set_text (Message[4].entry1, "@anachrocomputer");
      gtk_entry_set_text (Message[4].entry2, "");
      gtk_entry_set_text (Message[5].entry1, "");
      gtk_entry_set_text (Message[5].entry2, "");
      break;
   case BRIGHTON_PRESET:
      gtk_entry_set_text (Message[0].entry1, "BRIGHTON MINI");
      gtk_entry_set_text (Message[0].entry2, "MAKER FAIRE 2014");
      gtk_entry_set_text (Message[1].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[1].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[2].entry1, "JOHN HONNIBALL");
      gtk_entry_set_text (Message[2].entry2, "BRISTOL HACKSPACE");
      gtk_entry_set_text (Message[3].entry1, "FUN WITH FLAT-BED");
      gtk_entry_set_text (Message[3].entry2, "PEN PLOTTERS");
      gtk_entry_set_text (Message[4].entry1, "@anachrocomputer");
      gtk_entry_set_text (Message[4].entry2, "           #bmmf");
      gtk_entry_set_text (Message[5].entry1, "");
      gtk_entry_set_text (Message[5].entry2, "");
      break;
   }
}


/* delete_event --- callback for window deletion */
static gboolean delete_event (GtkWidget *widget,
                       GdkEvent  *event,
                       gpointer   data)
{
   gtk_main_quit ();
   return FALSE;
}


/* make_preset --- make a button for selecting a set of strings */

static void make_preset (GtkWidget *hbox, const char label[], int id)
{
   GtkWidget *button;
   
   button = gtk_button_new_with_label (label);

   g_signal_connect (button, "clicked",
       G_CALLBACK (preset_button), (gpointer)id);

   gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);

   gtk_widget_show (button);
}


/* make_message_controls --- make a group of controls for each message */

static void make_message_controls (GtkWidget *vbox, int i)
{
   GtkWidget *radio;
   GtkWidget *hbox;
   GtkWidget *ebox;
   static GtkRadioButton *group = NULL;
   GtkWidget *entry;
   char showtip[32];

   /* Make a horizontal box for the two text fields and a button */
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

   gtk_widget_show (hbox);

   gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

   /* Make a vertical box for the two entry fields */
   ebox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

   /* Put the vertical box into the horizontal box */
   gtk_box_pack_start (GTK_BOX(hbox), ebox, TRUE, TRUE, 0);

   gtk_widget_show (ebox);

   /* Make upper text entry field */
   entry = gtk_entry_new ();
   
   Message[i].entry1 = GTK_ENTRY (entry);
   
   gtk_box_pack_start (GTK_BOX(ebox), entry, TRUE, TRUE, 0);
   
   gtk_widget_show (entry);

   /* Make lower text entry field */
   entry = gtk_entry_new ();
   
   Message[i].entry2 = GTK_ENTRY (entry);
   
   gtk_box_pack_start (GTK_BOX(ebox), entry, TRUE, TRUE, 0);
   
   gtk_widget_show (entry);

   /* Each message has a "Show" radio button */
   radio = gtk_radio_button_new_with_label_from_widget (group, "Show");

   Message[i].button = GTK_BUTTON (radio);
   
   group = GTK_RADIO_BUTTON(radio);
   
   gtk_box_pack_start (GTK_BOX(hbox), radio, TRUE, TRUE, 0); 

   g_signal_connect (radio, "clicked", G_CALLBACK (show_button), (gpointer)i);

   sprintf (showtip, "Show message %d", i + 1);
   gtk_widget_set_tooltip_text (radio, showtip);
   gtk_widget_show (radio);
}


int main (int argc, char *argv[])
{
   /* GtkWidget is the storage type for widgets */
   GtkWidget *window;
   GtkWidget *button;
   GtkWidget *frame;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *check;
   GtkWidget *label;
   GtkAdjustment *adjustment;
   int i;

   gtk_init (&argc, &argv);

   /* Create the main window */
   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

   gtk_window_set_title (GTK_WINDOW (window), "BA63 Display");

   /* Set a handler for delete-event that exits */
   g_signal_connect (window, "delete-event", G_CALLBACK (delete_event), NULL);

   /* Set the border width of the window */
   gtk_container_set_border_width (GTK_CONTAINER (window), 10);

   /* Vertical box contains rows of text fields and buttons */
   vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

   /* Put the box into the main window */
   gtk_container_add (GTK_CONTAINER (window), vbox);

   /* Make a pair of text fields for each message */
   for (i = 0; i < MAXMSGS; i++) {
      make_message_controls (vbox, i);
   }

   /* Create a new button with the label "Test" */
   button = gtk_button_new_with_label ("Test");
   gtk_widget_set_tooltip_text (button, "Test with all pixels lit");
   
   g_signal_connect (button, "clicked",
       G_CALLBACK (test_button), (gpointer) NULL);

   gtk_box_pack_start (GTK_BOX(vbox), button, TRUE, TRUE, 0);

   gtk_widget_show (button);

   /* Make a frame to contain the preset buttons */
   frame = gtk_frame_new ("Presets");
   gtk_widget_set_tooltip_text (frame, "Messages for any occasion");
   gtk_widget_show (frame);
   gtk_box_pack_start(GTK_BOX (vbox), frame, TRUE, TRUE, 0);

   /* Make a horizontal box for the preset buttons */
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_widget_show (hbox);
   gtk_container_add (GTK_CONTAINER (frame), hbox);

   make_preset (hbox, "MFUK", MFUK_PRESET);
   make_preset (hbox, "Bristol", BRISTOL_PRESET);
   make_preset (hbox, "LUG", LUG_PRESET);
   make_preset (hbox, "Derby", DMMF_PRESET);
   make_preset (hbox, "BV", BVOS_PRESET);
   make_preset (hbox, "Brighton", BRIGHTON_PRESET);

   /* Make a frame to contain the auto controls */
   frame = gtk_frame_new ("Advance to Next");
   gtk_widget_show (frame);
   gtk_box_pack_start(GTK_BOX (vbox), frame, TRUE, TRUE, 0);

   /* Make a horizontal box for the auto controls */
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
   gtk_widget_show (hbox);
   gtk_container_add (GTK_CONTAINER (frame), hbox);

   check = gtk_check_button_new_with_label ("Auto");
   gtk_widget_set_tooltip_text (check, "Automatically advance to next message");
   gtk_widget_show (check);
   gtk_box_pack_start(GTK_BOX (hbox), check, TRUE, TRUE, 0);
   g_signal_connect (check, "clicked",
       G_CALLBACK (auto_button), (gpointer)0);
   Auto_button = GTK_TOGGLE_BUTTON(check);

   /* Spin-box for timeout value in seconds */
   adjustment = gtk_adjustment_new (5.0, 2.0, 3600.0, 1.0, 5.0, 0.0);
   button = gtk_spin_button_new (adjustment, 1.0, 0);
   gtk_widget_set_tooltip_text (button, "Delay in seconds before auto advance");
   gtk_box_pack_start (GTK_BOX(hbox), button, TRUE, TRUE, 0);
   gtk_widget_show (button);
   Time_spin = GTK_SPIN_BUTTON(button);
   
   label = gtk_label_new ("seconds");
   gtk_box_pack_start (GTK_BOX(hbox), label, TRUE, TRUE, 0);
   gtk_widget_show (label);

   /* Button for manual advance */
   button = gtk_button_new_with_label ("Manual");
   gtk_widget_set_tooltip_text (button, "Manually advance to next message");
   gtk_widget_show (button);
   gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect (button, "clicked",
       G_CALLBACK (next_button), (gpointer) NULL);

   gtk_widget_show (vbox);

   gtk_widget_show (window);
   
   /* Open the serial port connection to the BA63 */
   Fd = openBA63Port ("/dev/ttyUSB0");
   
   ba63home ();
   ba63cls ();
   ba63charset (6);
   
   gtk_main ();

   return 0;
}

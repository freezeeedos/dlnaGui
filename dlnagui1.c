#include <gtk/gtk.h>
#include <sys/types.h>
#include <dirent.h>
#include<unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

pid_t proc_find(const char* name);
void *checkifrunning(GtkWidget *statusbar);
void execdlna(GtkWidget *startbtn, GtkWidget *statusbar);
void killdlna(GtkWidget *startbtn, GtkWidget *statusbar);
char *readcfg(char *filename);
static void load_cfg (GtkWidget *view);
static void write_cfg(GtkWidget *savebtn, GtkWidget *view);
static void onclick_load_cfg (GtkWidget *loadbtn, GtkWidget *view);
static void activate (GtkApplication *app, gpointer user_data);

int
main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    app = gtk_application_new ("org.gtk.dlnagui", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}

pid_t proc_find(const char* name)
{
    DIR* dir;
    struct dirent* ent;
    char buf[512];

    long  pid;
    char pname[100] = {0,};
    char state;
    FILE *fp=NULL;

    if (!(dir = opendir("/proc"))) {
        perror("proc_find(): can't open /proc");
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        long lpid = atol(ent->d_name);
        if(lpid < 0)
            continue;
        snprintf(buf, sizeof(buf), "/proc/%ld/stat", lpid);
        fp = fopen(buf, "r");

        if (fp) {
            if ( (fscanf(fp, "%ld (%[^)]) %c", &pid, pname, &state)) != 3 ){
                perror("proc_find(): fscanf failed");
                fclose(fp);
                closedir(dir);
                return -1;
            }
            if (!strcmp(pname, name)) {
                fclose(fp);
                closedir(dir);
                return (pid_t)lpid;
            }
            fclose(fp);
        }
    }


    closedir(dir);
    return -1;
}

void *checkifrunning(GtkWidget *statusbar)
{
    char *statusMessage;
    guint context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar),
                                                            "dlnagui statusbar");
    if(proc_find("minidlna") == -1)
    {
        statusMessage = "Minidlna is not running...";
    }
    else
    {
        statusMessage = "Minidlna is running !";
    }
    
    gtk_statusbar_push (GTK_STATUSBAR (statusbar),
		    context_id,
		    statusMessage);
}

void execdlna(GtkWidget *startbtn, GtkWidget *statusbar)
{
    if(proc_find("minidlna") == -1)
    {
        if(system("minidlna &") == -1)
        {
	    perror("system");
            return;
        }
    }
    usleep(1500);
    checkifrunning(statusbar);
}

void killdlna(GtkWidget *stopbtn, GtkWidget *statusbar)
{
    pid_t pid;
    
    checkifrunning(statusbar);
    pid = proc_find("minidlna");
    
    if(pid != -1){
        if(kill(pid, SIGTERM) == -1)
	{
	    perror("kill");
	}
    }
    sleep(1);
    checkifrunning(statusbar);
}

char *readcfg(char *filename)
{
    FILE *f;
    char *string = NULL;
    long fsize;

    f = fopen(filename, "r");
    if(f == NULL)
    {
        fprintf(stderr, "Unable to open %s: ", filename);
        perror("");
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;
    return string;
}

static void load_cfg (GtkWidget *view)
{
//     printf("Loading cfg file...\n");
    GtkTextBuffer *buffer;
    char *text = NULL;
    buffer = gtk_text_buffer_new (NULL);
    text = readcfg("/etc/minidlna.conf");
    if(text == NULL)
    {
        text = "Unable to open file.\nRecommended: start the program in a terminal to see error messages\n";
    }
    gtk_text_buffer_set_text (buffer, text, -1);
    gtk_text_view_set_buffer(GTK_TEXT_VIEW (view), buffer);
    return;
}

static void write_cfg(GtkWidget *savebtn, GtkWidget *view)
{
    FILE *cfgfile;
    GtkTextBuffer *buffer;
    gchar *text;
    GtkTextIter start;
    GtkTextIter end;
//     buffer = gtk_text_buffer_new(NULL);
//     printf("Writing cfg file...\n");
    cfgfile = fopen("/etc/minidlna.conf", "w");
    if(cfgfile == NULL)
    {
        perror("write_cfg(): Unable to open file");
        return;
    }
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(view));
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
//     g_printf("%s", text);
    g_fprintf(cfgfile, text);
    g_free (text);
// //     g_free(buffer);
//     gtk_text_buffer_set_modified(buffer, FALSE);
    fclose(cfgfile);
}

static void onclick_load_cfg (GtkWidget *loadbtn, GtkWidget *view)
{
    load_cfg(view);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    GtkWidget *window;
    GtkWidget *box;
    GtkWidget *box1;
    GtkWidget *statusbar;

    GtkWidget *gtk_frame;
    GtkWidget *text_view;
    GtkWidget *scrolled_window;

    GtkWidget *startbtn;
    GtkWidget *stopbtn;
    GtkWidget *loadbtn;
    GtkWidget *savebtn;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "DlnaGui");
    gtk_window_set_default_size (GTK_WINDOW (window), 640, 640);

    gtk_frame = gtk_frame_new("Config editor");
    
    text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);

    gtk_widget_set_hexpand(GTK_WIDGET (scrolled_window), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET (scrolled_window), TRUE);

    box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
    gtk_widget_set_hexpand(GTK_WIDGET (box), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET (box), TRUE);

    box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_widget_set_hexpand(GTK_WIDGET (box), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET (box), TRUE);

    startbtn = gtk_button_new_with_label ("Start");
    gtk_widget_set_hexpand(GTK_WIDGET (startbtn), TRUE);

    stopbtn = gtk_button_new_with_label ("Stop");
    gtk_widget_set_hexpand(GTK_WIDGET (stopbtn), TRUE);

    loadbtn = gtk_button_new_with_label ("Load CFG");
    gtk_widget_set_hexpand(GTK_WIDGET (loadbtn), TRUE);

    savebtn = gtk_button_new_with_label ("Save CFG");
    gtk_widget_set_hexpand(GTK_WIDGET (savebtn), TRUE);

    statusbar = gtk_statusbar_new ();

//     gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 5);


    gtk_container_add (GTK_CONTAINER (window), gtk_frame);
    gtk_container_add (GTK_CONTAINER (gtk_frame), GTK_WIDGET (box));
    gtk_container_add (GTK_CONTAINER (box), scrolled_window);
    gtk_container_add (GTK_CONTAINER (scrolled_window),
                       text_view);
    gtk_container_add (GTK_CONTAINER (box), box1);
    gtk_container_add (GTK_CONTAINER (box), statusbar);
    gtk_container_add (GTK_CONTAINER (box1), startbtn);
    gtk_container_add (GTK_CONTAINER (box1), stopbtn);
    gtk_container_add (GTK_CONTAINER (box1), loadbtn);
    gtk_container_add (GTK_CONTAINER (box1), savebtn);
    checkifrunning(statusbar);
    load_cfg(text_view);
    g_signal_connect (startbtn, "clicked", G_CALLBACK (execdlna), statusbar);
    g_signal_connect (stopbtn, "clicked", G_CALLBACK (killdlna), statusbar);
    g_signal_connect (loadbtn, "clicked", G_CALLBACK (onclick_load_cfg), text_view);
    g_signal_connect (savebtn, "clicked", G_CALLBACK (write_cfg), text_view);
//     gtk_grid_insert_row(grid, 1);

    gtk_widget_show_all (window);
}

/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Rogério Ferro do Nascimento 2009 <rogerioferro@gmail.com>
 * 
 * main.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * main.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "color.h"
#include "toolbar.h"
#include "cv_drawing.h"
#include "file.h"
#include "undo.h"


#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define UI_FILE		PACKAGE_DATA_DIR G_DIR_SEPARATOR_S "gnome-paint" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "gnome_paint.ui"
#define ICON_DIR	PACKAGE_DATA_DIR G_DIR_SEPARATOR_S "gnome-paint" G_DIR_SEPARATOR_S "icons"


GtkWidget	*create_window		(   void	);
void		on_window_destroy   (   GtkObject   *object, 
									gpointer	user_data   );
void		gnome_paint_init	( int argc, char *argv[] );
void		
on_menu_new_activate( GtkMenuItem *menuitem, gpointer user_data)
{
	g_spawn_command_line_async (g_get_prgname(), NULL);
}

int
main (int argc, char *argv[])
{

 	GtkWidget   *window;

//	g_mem_set_vtable (glib_mem_profiler_table);

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
	gtk_set_locale ();
	gtk_init (&argc, &argv);

	/* Add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), ICON_DIR);
	gtk_window_set_default_icon_name ("gp");
	window = create_window ();
	gnome_paint_init (argc, argv);
	gtk_widget_show (window);

	gtk_main ();

//	g_mem_profile ();
	
	return 0;	
}


void
gnome_paint_init	( int argc, char *argv[] )
{
	if (argc > 1)
	{
		if( argc > 2 )
		{   /*open others images*/
			gchar   *new_argv[argc];
			gint	i,n;
			n = argc - 1;
			new_argv[0] = argv[0];
			new_argv[n] = NULL;
			for (i=1; i < n ; i++)
			{
				new_argv[i] = argv[i+1];
			}
			g_spawn_async_with_pipes ( NULL,
				                  	   new_argv,
			                           NULL,
			                           G_SPAWN_SEARCH_PATH,
			                           NULL,NULL,NULL,NULL,NULL,NULL,NULL);
		}
		/*open the first image*/
		file_open (argv[1]);
	}
}

gboolean
on_window_delete_event (GtkWidget       *widget,
                        GdkEvent        *event,
                        gpointer         user_data)
{
	return file_save_dialog ();
}

void 
on_window_destroy ( GtkObject   *object, 
				    gpointer	user_data )
{
	gtk_main_quit();
}

GtkWidget*
create_window (void)
{
	GtkWidget		*window;
	GtkWidget		*widget;
	GtkBuilder		*builder;

	builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, UI_FILE, NULL);
    window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
	g_assert ( window );
	file_set_parent_window ( GTK_WINDOW(window) );	
    gtk_builder_connect_signals (builder, NULL);          
    g_object_unref (G_OBJECT (builder));	
	
	/* To show all widget that is set invisible on Glade */
	/* and call realize event */
	gtk_widget_show_all(window);

	return window;
}

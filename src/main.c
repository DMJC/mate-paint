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
#include "color-picker.h"

#include "cv_eraser_tool.h"
#include "cv_paintbrush_tool.h"

#include <glib/gi18n.h>
#include <locale.h>
#include <gtk/gtk.h>

#define UI_FILE		PACKAGE_DATA_DIR G_DIR_SEPARATOR_S "mate-paint" G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "mate_paint.ui"
#define ICON_DIR	PACKAGE_DATA_DIR G_DIR_SEPARATOR_S "mate-paint" G_DIR_SEPARATOR_S "icons"

#ifndef GETTEXT_PACKAGE
#define GETTEXT_PACKAGE "mate-paint"
#endif

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

GtkWidget	*create_window			( void );
void		mate_paint_init		( int argc, char *argv[] );
void		on_window_destroy		( GtkWidget *object, gpointer user_data );
void		on_menu_about_activate  ( GtkMenuItem *menuitem, gpointer user_data );

static void init_eraser				(GtkBuilder *builder);
static void init_paint_brush		(GtkBuilder *builder);
static void save_the_children		(GtkBuilder *builder);
static GtkWidget *create_fallback_window	( void );

void		
on_menu_new_activate( GtkMenuItem *menuitem, gpointer user_data)
{
	g_spawn_command_line_async (g_get_prgname(), NULL);
}

void main_color_changed	(ColorPicker *color_picker, gpointer user_data)
{
	GdkColor *color;
	color = color_picker_get_color (color_picker);
	foreground_set_color ( color );
}

void color_picker_released (ColorPicker *color_picker, gpointer user_data)
{
	toolbar_go_to_previous_tool ();
}


int
main (int argc, char *argv[])
{

 	GtkWidget   *window;
	ColorPicker *color_picker; 

//	g_mem_set_vtable (glib_mem_profiler_table);

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
	setlocale (LC_ALL, "");
	gtk_init (&argc, &argv);

	/* Add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), ICON_DIR);
	gtk_window_set_default_icon_name ("gp");
	window = create_window ();
	mate_paint_init (argc, argv);
	gtk_widget_show (window);

	color_picker = color_picker_new ();
	toolbar_set_color_picker ( color_picker );
	g_signal_connect (color_picker, "color-changed",
		            G_CALLBACK (main_color_changed), NULL);

	g_signal_connect (color_picker, "released",
		            G_CALLBACK (color_picker_released), NULL);
	
	gtk_main ();

//	g_mem_profile ();
	
	return 0;	
}


void
mate_paint_init	( int argc, char *argv[] )
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
on_window_destroy ( GtkWidget *object, 
				    gpointer	user_data )
{
	gtk_main_quit();
}

GtkWidget*
create_window (void)
{
	GtkWidget		*window;
	GtkWidget		*widget;
	GtkWidget		*drawing;
	GtkBuilder		*builder;

	builder = gtk_builder_new ();
    gtk_builder_add_from_file (builder, UI_FILE, NULL);
    window = GTK_WIDGET (gtk_builder_get_object (builder, "window"));
	g_assert ( window );

    widget = GTK_WIDGET (gtk_builder_get_object (builder, "flip_roate_dialog"));
    drawing = GTK_WIDGET (gtk_builder_get_object (builder, "cv_drawing"));
	g_object_set_data(G_OBJECT(drawing), "flip_roate_dialog", widget);

	file_set_parent_window ( GTK_WINDOW(window) );	
    gtk_builder_connect_signals (builder, NULL);          

#if GTK_MAJOR_VERSION >= 3
	/*
	 * Older UI definitions can still wire the canvas to "expose-event".
	 * GTK3 only repaints through "draw", so hook it explicitly to avoid
	 * a blank canvas when the builder file doesn't define the new signal.
	 */
	g_signal_connect (drawing, "draw",
	                  G_CALLBACK (on_cv_drawing_expose_event), NULL);
#endif

	init_eraser (builder);
	init_paint_brush (builder);
	save_the_children (builder);

    g_object_unref (G_OBJECT (builder));	
	
	/* To show all widget that is set invisible on Glade */
	/* and call realize event */
	gtk_widget_show_all(window);

	return window;
}

void 
on_menu_about_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	const gchar *authors[] = { "Rogério Ferro do Nascimento",
							   "Juan Balderas",
							   "Special thanks to:",
							   "  Aron Xu",
							   NULL };
	GtkAboutDialog *dlg;
	
	dlg = GTK_ABOUT_DIALOG ( gtk_about_dialog_new () ); 
	g_set_application_name( "mate-paint" );
	gtk_about_dialog_set_version ( dlg, PACKAGE_VERSION); 
	gtk_about_dialog_set_copyright ( dlg, 
									"(c) Rogério Ferro do Nascimento");
	gtk_about_dialog_set_comments ( dlg, 
									_("mate-paint is a simple, easy to use paint program for MATE.") );
	gtk_about_dialog_set_license ( dlg, 
								_( "This program is free software;"
								   " you may redistribute it and/or modify it"
								   " under the terms of the GNU General Public License"
								   " as published by the Free Software Foundation, "
								   "either version 3, or (at your opinion) any later version.\n") );
	gtk_about_dialog_set_wrap_license ( dlg, TRUE );
	gtk_about_dialog_set_authors ( dlg, authors );
	gtk_about_dialog_set_website ( dlg, 
									"https://launchpad.net/mate-paint");
	//gtk_about_dialog_set_logo ( dlg, pixbuf);
	gtk_dialog_run ( GTK_DIALOG(dlg) );
	gtk_widget_destroy ( GTK_WIDGET(dlg) );
}

static void init_eraser(GtkBuilder *builder)
{
	GtkWidget *erase;
	gchar name[20];
	gint i;
	static gint size[4] = {
							GP_ERASER_RECT_TINY,
							GP_ERASER_RECT_SMALL,
							GP_ERASER_RECT_MEDIUM,
							GP_ERASER_RECT_LARGE
						};
	
	if(NULL == builder){
		return;
	}
	
	for(i = 0; i < 4; i++)
	{
		sprintf(name, "erase%d", i);
		erase = GTK_WIDGET (gtk_builder_get_object (builder, name));
		if(!GTK_IS_WIDGET(erase))
		{
			continue;
		}
		g_signal_connect (erase, "toggled",
		            G_CALLBACK (on_eraser_size_toggled), (gpointer)&(size[i]));
	}
}

static void init_paint_brush(GtkBuilder *builder)
{
	GtkWidget *brush;
	gchar name[20];
	gint i;
	static gint size[12] = {
							GP_BRUSH_RECT_LARGE,
							GP_BRUSH_RECT_MEDIUM,
							GP_BRUSH_RECT_SMALL,
							GP_BRUSH_ROUND_LARGE,
							GP_BRUSH_ROUND_MEDIUM,
							GP_BRUSH_ROUND_SMALL,
							GP_BRUSH_FWRD_LARGE,
							GP_BRUSH_FWRD_MEDIUM,
							GP_BRUSH_FWRD_SMALL,
							GP_BRUSH_BACK_LARGE,
							GP_BRUSH_BACK_MEDIUM,
							GP_BRUSH_BACK_SMALL,
						};
	
	if(NULL == builder){
		return;
	}
	
	for(i = 0; i < GP_BRUSH_MAX; i++)
	{
		sprintf(name, "brush%d", i);
		brush = GTK_WIDGET (gtk_builder_get_object (builder, name));
		if(!GTK_IS_WIDGET(brush))
		{
			continue;
		}
		g_signal_connect (brush, "toggled",
		            G_CALLBACK (on_brush_size_toggled), (gpointer)&(size[i]));
	}
}

static void save_the_children (GtkBuilder *builder)
{
	GtkWidget *child, *drawing;

	drawing = GTK_WIDGET (gtk_builder_get_object (builder, "cv_drawing"));
	if (!GTK_IS_WIDGET (drawing))
	{
		return;
	}
	child = GTK_WIDGET (gtk_builder_get_object (builder, "tool-rect-select"));
	g_object_set_data(G_OBJECT(drawing), "tool-rect-select", (gpointer)child);

	/* Flip rotate dlg radio buttons */
	child = GTK_WIDGET (gtk_builder_get_object (builder, "radiobutton_rotate"));
	g_object_set_data(G_OBJECT(drawing), "radiobutton_rotate", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "radiobutton_rotate_90"));
	g_object_set_data(G_OBJECT(drawing), "radiobutton_rotate_90", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "radiobutton_rotate_180"));
	g_object_set_data(G_OBJECT(drawing), "radiobutton_rotate_180", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "radiobutton_rotate_270"));
	g_object_set_data(G_OBJECT(drawing), "radiobutton_rotate_270", (gpointer)child);

	/* Attributes dlg */
	child = GTK_WIDGET (gtk_builder_get_object (builder, "dialog_attributes"));
	g_object_set_data(G_OBJECT(drawing), "dialog_attributes", (gpointer)child);

	/* Opaque/transparent buttons */
	child = GTK_WIDGET (gtk_builder_get_object (builder, "sel1"));
	g_object_set_data(G_OBJECT(drawing), "sel1", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "sel2"));
	g_object_set_data(G_OBJECT(drawing), "sel2", (gpointer)child);

	/* Attributes dlg widgets */
	child = GTK_WIDGET (gtk_builder_get_object (builder, "attributes_entry1"));
	g_object_set_data(G_OBJECT(drawing), "attributes_entry1", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "attributes_entry2"));
	g_object_set_data(G_OBJECT(drawing), "attributes_entry2", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "attributes_radiobutton1"));
	g_object_set_data(G_OBJECT(drawing), "attributes_radiobutton1", (gpointer)child);
	child = GTK_WIDGET (gtk_builder_get_object (builder, "attributes_radiobutton2"));
	g_object_set_data(G_OBJECT(drawing), "attributes_radiobutton2", (gpointer)child);
	
	child = GTK_WIDGET (gtk_builder_get_object (builder, "attributes_label6"));
	g_object_set_data(G_OBJECT(drawing), "attributes_label6", (gpointer)child);
	

	//child = GTK_WIDGET (gtk_builder_get_object (builder, ""));
	//g_object_set_data(G_OBJECT(drawing), "", (gpointer)child);
}



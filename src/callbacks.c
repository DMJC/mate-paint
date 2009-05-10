/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.c
 * Copyright (C) Rogério Ferro do Nascimento 2009 <rogerioferro@gmail.com>
 * 
 * callbacks.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * callbacks.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "color.h"
#include "toolbar.h"

typedef enum
{
    LEFT_BUTTON		=   1,
    MIDDLE_BUTTON	=   2,
	RIGHT_BUTTON	=   3
} gnome_paint_button;


void 
on_window_destroy ( GtkObject   *object, 
				    gpointer	user_data )
{
	gtk_main_quit();
}

void
on_background_color_picker_realize  ( GtkObject *object,
                              		  gpointer  user_data )
{
	background_set_widget ( GTK_WIDGET(object) );
}

void
on_foreground_color_picker_realize  ( GtkObject *object,
                              		  gpointer  user_data )
{
	foreground_set_widget ( GTK_WIDGET(object) );
}

/*
 * Initialize the colors on the color palette tool bar.
 */
void
on_color_palette_entry_realize  ( GtkObject *object,
                                  gpointer  user_data )
{
	const gchar *name;
	guint i;
	name = gtk_widget_get_name ( GTK_WIDGET(object) );
	i = ( (guint)(name[0] - '0') * 10 ) + (guint)(name[1] - '0');
	//g_print("%d\n",i);
	palette_set_widget( i, GTK_WIDGET(object) );
}


gboolean 
on_background_color_picker_button_release_event ( GtkWidget			*widget, 
												  GdkEventButton	*event,
												  gpointer			user_data )
{
	if ( event->button == LEFT_BUTTON )
	{
		background_color_picker();
	}
	return TRUE;
}

gboolean 
on_foreground_color_picker_button_release_event	(   GtkWidget	   *widget, 
													GdkEventButton *event,
													gpointer       user_data )
{
	if ( event->button == LEFT_BUTTON )
	{
		foreground_color_picker();
	}
	return TRUE;
}

gboolean 
on_color_palette_entry_button_press_event ( GtkWidget	   *widget, 
											GdkEventButton *event,
											gpointer       user_data )
{
	const gchar *name;
	guint i;
	name = gtk_widget_get_name ( widget );
	i = ( (guint)(name[0] - '0') * 10 ) + (guint)(name[1] - '0');
	
	if ( event->type == GDK_2BUTTON_PRESS )
	{
		palette_color_picker ( i );
	}
	if ( event->button == LEFT_BUTTON )
	{
		foreground_set_color_from_palette ( i );
	}
	else if ( event->button == RIGHT_BUTTON )
	{
		background_set_color_from_palette ( i );
	}

	
	return TRUE;
}




void 
on_tool_free_select_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_free_select_toggled\n");
	}
}

void 
on_tool_eraser_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_eraser_toggled\n");
	}
}

void 
on_tool_color_picker_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_color_picker_toggled\n");
	}
}

void 
on_tool_pencil_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_pencil_toggled\n");
	}
}

void 
on_tool_airbrush_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_airbrush_toggled\n");
	}
}

void 
on_draw_line_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_line_toggled\n");
	}
}

void 
on_draw_rectangle_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_rectangle_toggled\n");
	}
}

void 
on_draw_ellipse_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_ellipse_toggled\n");
	}
}

void
on_tool_rect_select_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_rect-select_toggled\n");
	}
}

void
on_tool_bucket_fill_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_bucket_fill_toggled\n");
	}
}

void
on_tool_zoom_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_zoom_toggled\n");
	}
}

void
on_tool_paintbrush_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_paintbrush_toggled\n");
	}
}

void
on_tool_text_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_text_toggled\n");
	}
}

void
on_draw_curve_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_curve_toggled\n");
	}
}

void
on_draw_polygon_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_polygon_toggled\n");
	}
}

void
on_draw_rounded_rectangle_toggled  (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_rounded_rectangle_toggled\n");
	}
}





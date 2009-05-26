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
#include "canvas.h"

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
		set_free_select_tool();

	}
}

void 
on_tool_eraser_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_eraser_toggled\n");
		set_eraser_tool();
	}
}

void 
on_tool_color_picker_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_color_picker_toggled\n");
		set_color_picker_tool();
	}
}

void 
on_tool_pencil_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_pencil_toggled\n");
		set_pencil_tool();
	}
}

void 
on_tool_airbrush_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_airbrush_toggled\n");
		set_airbrush_tool();
	}
}

void 
on_draw_line_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_line_toggled\n");
		set_draw_line_tool();
	}
}

void 
on_draw_rectangle_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_rectangle_toggled\n");
		set_draw_rectangle_tool();
	}
}

void 
on_draw_ellipse_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_ellipse_toggled\n");
		set_draw_ellipse_tool();
	}
}

void
on_tool_rect_select_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_rect-select_toggled\n");
		set_rect_select_tool();
	}
}

void
on_tool_bucket_fill_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_bucket_fill_toggled\n");
		set_bucket_fill_tool();
	}
}

void
on_tool_zoom_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_zoom_toggled\n");
		set_zoom_tool();
	}
}

void
on_tool_paintbrush_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_paintbrush_toggled\n");
		set_paintbrush_tool();
	}
}

void
on_tool_text_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_tool_text_toggled\n");
		set_text_tool();
	}
}

void
on_draw_curve_toggled (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_curve_toggled\n");
		set_draw_curve_tool();
	}
}

void
on_draw_polygon_toggled	(GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_polygon_toggled\n");
		set_draw_polygon_tool();
	}
}

void
on_draw_rounded_rectangle_toggled  (GtkObject *object, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(object) ) )
	{
		g_print("on_draw_rounded_rectangle_toggled\n");
		set_draw_rounded_rectangle_tool();
	}
}


void 
on_notebook_realize   (GtkObject *object, gpointer user_data)
{
	//g_print("on_notebook_realize\n");
	notebook_set_widget ( GTK_WIDGET(object) );
}

void 
on_frame_rect_realize  (GtkObject *object, gpointer user_data)
{
	//g_print("on_frame_rect_realize\n");
	frame_rect_set_widget ( GTK_WIDGET(object) );
}


void
on_line0_realize   (GtkObject *object, gpointer user_data)
{
	//g_print("on_line0_realize\n");
	line0_init ( GTK_WIDGET(object) );
}

void
on_line1_realize   (GtkObject *object, gpointer user_data)
{
	line1_init ( GTK_WIDGET(object) );
}

void
on_line2_realize   (GtkObject *object, gpointer user_data)
{
	line2_init ( GTK_WIDGET(object) );
}

void
on_line3_realize   (GtkObject *object, gpointer user_data)
{
	line3_init ( GTK_WIDGET(object) );
}

void
on_line4_realize   (GtkObject *object, gpointer user_data)
{
	line4_init ( GTK_WIDGET(object) );
}


void 
on_sel1_realize   (GtkObject *object, gpointer user_data)
{
	sel1_init ( GTK_WIDGET(object) );
}

void 
on_sel2_realize   (GtkObject *object, gpointer user_data)
{
	sel2_init ( GTK_WIDGET(object) );
}

void 
on_rect0_realize   (GtkObject *object, gpointer user_data)
{
	rect0_init ( GTK_WIDGET(object) );
}

void 
on_rect1_realize   (GtkObject *object, gpointer user_data)
{
	rect1_init ( GTK_WIDGET(object) );
}

void 
on_rect2_realize   (GtkObject *object, gpointer user_data)
{
	rect2_init ( GTK_WIDGET(object) );
}

void 
on_statusbar_realize   (GtkObject *object, gpointer user_data)
{
	//statusbar_set_widget ( GTK_WIDGET(object) );
}

void 
on_canvas_realize   (GtkObject *object, gpointer user_data)
{
	canvas_set_widget ( GTK_WIDGET(object) );
}

void on_cv_ev_box_realize	(GtkObject *object, gpointer user_data)
{
	cv_ev_box_set_widget ( GTK_WIDGET(object) );
}


void
on_cv_right_realize (GtkObject *object, gpointer user_data)
{
	cv_right_set_widget ( GTK_WIDGET(object) );
}

void 
on_cv_bottom_right_realize (GtkObject *object, gpointer user_data)
{
	cv_bottom_right_set_widget ( GTK_WIDGET(object) );
}

void
on_cv_bottom_realize (GtkObject *object, gpointer user_data)
{
	cv_bottom_set_widget ( GTK_WIDGET(object) );
}

void
on_cv_other_edge_realize (GtkObject *object, gpointer user_data)
{
	cv_other_edge_set_widget ( GTK_WIDGET(object) );
}


gboolean 
on_cv_other_edge_expose_event	(   GtkWidget	   *widget, 
									GdkEventButton *event,
                                    gpointer       user_data )
{
	cv_other_edge_draw( widget );
	return TRUE;
}


gboolean 
on_canvas_expose_event	(   GtkWidget	   *widget, 
							GdkEventButton *event,
               				gpointer       user_data )
{
//	g_print("canvas expose\n");
	cv_canvas_draw ();
	return TRUE;
}

gboolean 
on_cv_ev_box_expose_event				(GtkWidget	   *widget, 
												 GdkEventButton *event,
                                                 gpointer       user_data )
{
	//g_print("cv_ev_box_expose\n");
	cv_ev_box_draw	();
	return TRUE;
}

gboolean 
on_cv_bottom_right_button_press_event	(   GtkWidget	   *widget, 
											GdkEventButton *event,
                                            gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_PRESS) && (event->button == LEFT_BUTTON) )
	{
		//g_print("init (%f,%f) \n", event->x, event->y);
		cv_resize_start();
	}
	return TRUE;
}


gboolean
on_cv_bottom_right_motion_notify_event (	GtkWidget      *widget,
                                     		GdkEventMotion *event,
                                            gpointer        user_data)
{
	//g_print("moving (%f,%f) \n", event->x, event->y);
	cv_resize_move( event->x, event->y );
	return TRUE;
}

gboolean 
on_cv_bottom_right_button_release_event (   GtkWidget	   *widget, 
                                            GdkEventButton *event,
                                            gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		//g_print("end (%f,%f) \n", event->x, event->y);
		cv_resize_stop ( event->x,  event->y );
	}
	return TRUE;
}


gboolean 
on_cv_bottom_button_press_event (   GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_PRESS) && (event->button == LEFT_BUTTON) )
	{
		//g_print("init (%f,%f) \n", event->x, event->y);
		cv_resize_start();
	}
	return TRUE;
}

gboolean 
on_cv_bottom_motion_notify_event (  GtkWidget      *widget,
		                            GdkEventMotion *event,
                                    gpointer        user_data)
{
	cv_resize_move( 0.0, event->y );
	return TRUE;
}

gboolean
on_cv_bottom_button_release_event ( GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		//g_print("end (%f,%f) \n", event->x, event->y);
		cv_resize_stop ( 0.0,  event->y );
	}
	return TRUE;
}

gboolean
on_cv_right_button_press_event (	GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_PRESS) && (event->button == LEFT_BUTTON) )
	{
		//g_print("init (%f,%f) \n", event->x, event->y);
		cv_resize_start();
	}
	return TRUE;
}

gboolean
on_cv_right_motion_notify_event (   GtkWidget      *widget,
		                            GdkEventMotion *event,
                                    gpointer        user_data)
{
	cv_resize_move( event->x, 0.0 );
	return TRUE;
}

gboolean
on_cv_right_button_release_event (  GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		//g_print("end (%f,%f) \n", event->x, event->y);
		cv_resize_stop ( event->x,  0.0 );
	}
	return TRUE;
}


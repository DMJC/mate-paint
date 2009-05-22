/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * callbacks.h
 * Copyright (C) Rogério Ferro do Nascimento 2009 <rogerioferro@gmail.com>
 * 
 * callbacks.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * callbacks.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>


void on_window_destroy					(GtkObject *object, gpointer user_data);

/*Color Bar realize functions*/
void on_color_palette_entry_realize     (GtkObject *object, gpointer user_data);
void on_background_color_picker_realize	(GtkObject *object, gpointer user_data);
void on_foreground_color_picker_realize	(GtkObject *object, gpointer user_data);


/*Color Bar Events*/
gboolean on_background_color_picker_button_release_event	(   GtkWidget	   *widget, 
																GdkEventButton *event,
																gpointer       user_data );
gboolean on_foreground_color_picker_button_release_event	(   GtkWidget	   *widget, 
																GdkEventButton *event,
																gpointer       user_data );

gboolean on_color_palette_entry_button_press_event			(   GtkWidget	   *widget, 
																GdkEventButton *event,
			                                                   gpointer       user_data );

/*Toolbar toggled functions*/
void on_tool_free_select_toggled		(GtkObject *object, gpointer user_data);
void on_tool_eraser_toggled				(GtkObject *object, gpointer user_data);
void on_tool_color_picker_toggled		(GtkObject *object, gpointer user_data);
void on_tool_pencil_toggled				(GtkObject *object, gpointer user_data);
void on_tool_airbrush_toggled			(GtkObject *object, gpointer user_data);
void on_draw_line_toggled				(GtkObject *object, gpointer user_data);
void on_draw_rectangle_toggled			(GtkObject *object, gpointer user_data);
void on_draw_ellipse_toggled			(GtkObject *object, gpointer user_data);
void on_tool_rect_select_toggled		(GtkObject *object, gpointer user_data);
void on_tool_bucket_fill_toggled		(GtkObject *object, gpointer user_data);
void on_tool_zoom_toggled				(GtkObject *object, gpointer user_data);
void on_tool_paintbrush_toggled			(GtkObject *object, gpointer user_data);
void on_tool_text_toggled				(GtkObject *object, gpointer user_data);
void on_draw_curve_toggled				(GtkObject *object, gpointer user_data);
void on_draw_polygon_toggled			(GtkObject *object, gpointer user_data);
void on_draw_rounded_rectangle_toggled  (GtkObject *object, gpointer user_data);

/*Option toolbar realize funcitons*/
void on_notebook_realize	(GtkObject *object, gpointer user_data);
void on_frame_rect_realize  (GtkObject *object, gpointer user_data);


/*Line toolbar realize functions*/
void on_line0_realize   (GtkObject *object, gpointer user_data);
void on_line1_realize   (GtkObject *object, gpointer user_data);
void on_line2_realize   (GtkObject *object, gpointer user_data);
void on_line3_realize   (GtkObject *object, gpointer user_data);
void on_line4_realize   (GtkObject *object, gpointer user_data);

/*Selection toolbar realize functions*/
void on_sel1_realize   (GtkObject *object, gpointer user_data);
void on_sel2_realize   (GtkObject *object, gpointer user_data);

/*rect toolbar realize functions*/
void on_rect0_realize   (GtkObject *object, gpointer user_data);
void on_rect1_realize   (GtkObject *object, gpointer user_data);
void on_rect2_realize   (GtkObject *object, gpointer user_data);




		                         	




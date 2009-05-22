/***************************************************************************
 *            toolbar.h
 *
 *  Sat May  9 15:13:23 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <gtk/gtk.h>


void set_free_select_tool				( void );
void set_eraser_tool					( void );
void set_color_picker_tool				( void );
void set_pencil_tool					( void );
void set_airbrush_tool					( void );
void set_draw_line_tool					( void );
void set_draw_rectangle_tool			( void );
void set_draw_ellipse_tool				( void );
void set_rect_select_tool				( void );
void set_bucket_fill_tool				( void );
void set_zoom_tool						( void );
void set_paintbrush_tool				( void );
void set_text_tool						( void );
void set_draw_curve_tool				( void );
void set_draw_polygon_tool				( void );
void set_draw_rounded_rectangle_tool	( void );



void notebook_set_widget 	( GtkWidget *widget );
void frame_rect_set_widget	( GtkWidget *widget );

/*Line Bar*/
void line0_init ( GtkWidget *widget );
void line1_init ( GtkWidget *widget );
void line2_init ( GtkWidget *widget );
void line3_init ( GtkWidget *widget );
void line4_init ( GtkWidget *widget );


/*Select Bar*/
void sel1_init ( GtkWidget *widget );
void sel2_init ( GtkWidget *widget );

/*Rect Bar*/
void rect0_init ( GtkWidget *widget );
void rect1_init ( GtkWidget *widget );
void rect2_init ( GtkWidget *widget );

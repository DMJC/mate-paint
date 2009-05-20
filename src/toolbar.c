/***************************************************************************
 *            toolbar.c
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

#include "toolbar.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "./pixmaps/line_0.xpm"
#include "./pixmaps/line_1.xpm"
#include "./pixmaps/line_2.xpm"
#include "./pixmaps/line_3.xpm"
#include "./pixmaps/line_4.xpm"

#include "./pixmaps/sel1.xpm"
#include "./pixmaps/sel2.xpm"


/*private vars*/
static GtkNotebook *notebook = NULL;

/*private functions*/
static GtkWidget * get_gtk_image( GtkWidget *widget, gchar** xpm );

void 
set_free_select_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_eraser_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 1 );
}

void 
set_color_picker_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 2 );
}

void 
set_pencil_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 3 );
}

void 
set_airbrush_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 4 );
}

void 
set_draw_line_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 5 );
}

void 
set_draw_rectangle_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 6 );
}

void 
set_draw_ellipse_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_rect_select_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_bucket_fill_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_zoom_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_paintbrush_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_text_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_draw_curve_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_draw_polygon_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}

void 
set_draw_rounded_rectangle_tool( void )
{
	g_return_if_fail( notebook != NULL );
	gtk_notebook_set_current_page ( notebook, 0 );
}



void
notebook_set_widget ( GtkWidget *widget )
{
	notebook	=	GTK_NOTEBOOK( widget );
}


void
line0_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)line_0_xpm ) );
}

void
line1_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)line_1_xpm ) );
}

void
line2_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)line_2_xpm ) );
}

void
line3_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)line_3_xpm ) );
}

void
line4_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)line_4_xpm ) );
}


void 
sel1_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)sel1_xpm ) );
}

void 
sel2_init ( GtkWidget *widget )
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(widget), 
	                                 	get_gtk_image( widget, (gchar**)sel2_xpm ) );
}


/*private*/

GtkWidget * 
get_gtk_image ( GtkWidget *widget, gchar** xpm )
{
	GdkPixmap *gdkpixmap	= NULL;
    GdkBitmap *mask 		= NULL;
    GtkWidget *gtkimage 	= NULL;
	gdkpixmap = gdk_pixmap_create_from_xpm_d(widget->window, &mask, NULL, xpm);
	g_assert(gdkpixmap);
	gtkimage = gtk_image_new_from_pixmap(gdkpixmap, mask);
    g_assert(gtkimage);
	
	gdk_pixmap_unref(gdkpixmap);
    gdk_pixmap_unref(mask); 

	gtk_widget_show(gtkimage);

	return gtkimage;
}

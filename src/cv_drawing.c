/***************************************************************************
 *            cv_drawing.c
 *
 *  Sun Jun  7 11:31:18 2009
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

#include "cv_drawing.h"
#include "tool_line.h"


/*Member functions*/
GdkGC * 	cv_create_new_gc ( char * name );


/* private data  */
static gnome_paint_canvas	cv;
static gnome_paint_tool		*cv_tool		=	NULL;
static GdkColor 			white_color		=	{ 0, 0xffff, 0xffff, 0xffff  };
static GdkColor 			black_color		=	{ 0, 0x0000, 0x0000, 0x0000  };


/*
 *   CODE
 */

void
cv_set_color_bg	( GdkColor *color )
{
	gdk_gc_set_rgb_bg_color ( cv.gc_fg, color );		
	gdk_gc_set_rgb_fg_color ( cv.gc_bg, color );		
}

void
cv_set_color_fg	( GdkColor *color )
{
	gdk_gc_set_rgb_fg_color ( cv.gc_fg, color );	
	gdk_gc_set_rgb_bg_color ( cv.gc_bg, color );	
}

void
cv_set_line_width	( gint width )
{
	gdk_gc_set_line_attributes ( cv.gc_fg, width, GDK_LINE_SOLID, 
	                             GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );
	gdk_gc_set_line_attributes ( cv.gc_bg, width, GDK_LINE_SOLID, 
	                             GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );
}

void cv_sel_none_tool	( void )
{
	gdk_window_set_cursor ( cv.drawing, NULL);
	cv_tool = NULL;
}

void cv_sel_line_tool	( void )
{
	static gnome_paint_tool	*line_tool	=	NULL;
	if ( line_tool == NULL )
	{
		line_tool = tool_line_init ( &cv );
	}
	cv_tool = line_tool;
	cv_tool->reset ();
}


void  my_g_object_unref(gpointer data)
{
	g_print("g_object_unref\n");
	g_object_unref( G_OBJECT(data) );
}

void
cv_create_pixmap ( gint width, gint height )
{
	GdkPixmap *		px;
	px = gdk_pixmap_new ( cv.drawing, width, height, -1);
	g_assert( px );
	/* initial drawing is filled with background color */ 
	gdk_draw_rectangle( px, cv.gc_bg, TRUE, 0, 0, width, height );
	if ( cv.pixmap != NULL )
	{
		gint w,h;
		gdk_drawable_get_size ( cv.pixmap, &w, &h );
		if ( width < w ) w = width;
		if ( height < h ) h = height;
		gdk_draw_drawable (	px,
		                	cv.gc_fg,
			                cv.pixmap,
			                0, 0,
			                0, 0,
			                w, h );
	}
	/*set new data to be destroyed and destroy old data*/
	g_object_set_data_full (	G_OBJECT(cv.widget), "cv_pixmap", 
	                       		(gpointer)px, 
	                        	(GDestroyNotify)g_object_unref );
	cv.pixmap	=	px;
}

/* GUI CallBacks */

void
on_cv_drawing_realize (GtkWidget *widget, gpointer user_data)
{
	cv.widget	=	widget;
	cv.drawing	=	cv.widget->window;
	cv.gc_fg	=	cv_create_new_gc( "cv_gc_fg" );
	cv.gc_bg	=	cv_create_new_gc( "cv_gc_bg" );
	cv.pixmap	=	NULL;
	cv_set_color_fg ( &black_color );
	cv_set_color_bg ( &white_color );
	cv_set_line_width ( 1 );
	cv_create_pixmap ( 300, 300);
	cv_resize_set_canvas ( &cv );
}


/* events */
gboolean
on_cv_drawing_button_press_event (	GtkWidget	   *widget, 
                               		GdkEventButton *event,
                                	gpointer       user_data )
{
	gboolean ret = TRUE;

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_press( event );
	}

	return ret;
}

gboolean
on_cv_drawing_button_release_event (	GtkWidget	   *widget, 
                                    	GdkEventButton *event,
                                    	gpointer       user_data )
{
	gboolean ret = TRUE;

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_release( event );
	}

	return ret;
}
									
gboolean
on_cv_drawing_motion_notify_event (	GtkWidget      *widget,
		                        	GdkEventMotion *event,
                                	gpointer        user_data)
{
	gboolean ret = TRUE;

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_motion( event );
	}

	return ret;
}

gboolean 
on_cv_drawing_expose_event	(   GtkWidget	   *widget, 
								GdkEventExpose *event,
               					gpointer       user_data )
{

    gdk_draw_drawable (	widget->window,
                    	widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
    	                cv.pixmap,
    	                event->area.x, event->area.y,
    	                event->area.x, event->area.y,
    	                event->area.width, event->area.height);	
	
	if ( cv_tool != NULL )
	{
		cv_tool->draw();
	}

	cv_resize_draw();
	
	return TRUE;
}

/*private functions*/

GdkGC *
cv_create_new_gc ( char * name )
{
	GdkGC  * gc;
	gc	=	gdk_gc_new ( cv.widget->window );
	g_assert( gc );
	/*set data to be destroyed*/
	g_object_set_data_full (	G_OBJECT( cv.widget ), name, 
	                       		(gpointer)gc , 
	                        	(GDestroyNotify)g_object_unref );
	return gc;
}



/***************************************************************************
 *            cv_rectangle_tool.c
 *
 *  Thu Set 10 22:35:13 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
 *  by Jacson
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

#include "cv_pencil_tool.h"
#include "file.h"

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( void );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void		draw_in_pixmap	( GdkDrawable *drawable );


/*private data*/
static gp_tool			tool;
static gp_canvas *		cv		= NULL;
static GdkGC *			gc		= NULL;
static gint 			x0,y0,x1,y1;
static guint			button	= 0;
static gboolean 		is_draw = FALSE;

const gp_tool * 
tool_pencil_init ( gp_canvas * canvas )
{
	cv					=	canvas;
	tool.button_press	= 	button_press;
	tool.button_release	= 	button_release;
	tool.button_motion	= 	button_motion;
	tool.draw			= 	draw;
	tool.reset			= 	reset;
	tool.destroy		= 	destroy;
	/*set data to be destroyed*/
	g_object_set_data_full (	G_OBJECT(cv->widget), "pencil_tool", 
	                        	(gpointer)&tool, 
	                        	(GDestroyNotify)(tool.destroy) );	
	return &tool;
}

static gboolean
button_press ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_PRESS )
	{
		if ( event->button == LEFT_BUTTON )
		{
			gc = cv->gc_fg_pencil;
		}
		else if ( event->button == RIGHT_BUTTON )
		{
			gc = cv->gc_bg_pencil;
		}
		is_draw = !is_draw;
		if( is_draw ) button = event->button;
		x0 = x1 = (gint)event->x;
		y0 = y1 = (gint)event->y;
		if( !is_draw ) gtk_widget_queue_draw ( cv->widget );
	}
	return TRUE;
}

static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
		if( button == event->button )
		{
			if( is_draw )
			{
				draw_in_pixmap (cv->pixmap);
				file_set_unsave ();
			}
			gtk_widget_queue_draw ( cv->widget );
			is_draw = FALSE;
		}
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if( is_draw )
	{
		x1 = (gint)event->x;
		y1 = (gint)event->y;
		gtk_widget_queue_draw ( cv->widget );
	}
	return TRUE;
}

static void	
draw ( void )
{
	if ( is_draw )
	{
		draw_in_pixmap ( cv->pixmap );
	}
}

static void 
reset ( void )
{
	GdkCursor *cursor = gdk_cursor_new ( GDK_PENCIL );
	g_assert(cursor);
	gdk_window_set_cursor ( cv->drawing, cursor );
	gdk_cursor_unref( cursor );
	is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	//g_print("rectangle tool destroy\n");
}

static void
draw_in_pixmap ( GdkDrawable *drawable )
{
	gdk_draw_line (drawable, gc, x0, y0, x1, y1 );
	x0 = x1;
	y0 = y1;
}


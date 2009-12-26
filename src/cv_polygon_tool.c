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

#include "cv_polygon_tool.h"
#include "file.h"

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( void );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void		draw_in_pixmap	( GdkDrawable *drawable );

static void		change_point	( gint x, gint y );
static void		add_point 		( gint x, gint y );
static void		clear_points	( void );

/*private data*/
typedef enum
{
	TOOL_NONE,
	TOOL_DRAWING,
	TOOL_WAITING
} gp_tool_state;

typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
	GdkGC *			gcf;
	GdkGC *			gcb;
	guint			button;
	gboolean 		is_draw;
	GArray			*garray;
	gp_tool_state	state;
} private_data;

static private_data		*m_priv = NULL;
	
static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv			=	NULL;
		m_priv->gcf			=	NULL;
		m_priv->gcb			=	NULL;
		m_priv->button		=	NONE_BUTTON;
		m_priv->is_draw		=	FALSE;
		m_priv->garray		=	NULL;
		m_priv->state		=	TOOL_NONE;		
	}
}

static void
destroy_private_data( void )
{
	clear_points ();
	g_free (m_priv);
	m_priv = NULL;
}

gp_tool * 
tool_polygon_init ( gp_canvas * canvas )
{
	create_private_data ();
	m_priv->cv					= canvas;
	m_priv->tool.button_press	= button_press;
	m_priv->tool.button_release	= button_release;
	m_priv->tool.button_motion	= button_motion;
	m_priv->tool.draw			= draw;
	m_priv->tool.reset			= reset;
	m_priv->tool.destroy		= destroy;
	return &m_priv->tool;
}

static gboolean
button_press ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_PRESS )
	{
		switch ( m_priv->state )
		{
			case TOOL_NONE:
			{
				if ( event->button == LEFT_BUTTON )
				{
					m_priv->gcf = m_priv->cv->gc_fg;
					m_priv->gcb = m_priv->cv->gc_bg;
				}
				else if ( event->button == RIGHT_BUTTON )
				{
					m_priv->gcf = m_priv->cv->gc_bg;
					m_priv->gcb = m_priv->cv->gc_fg;
				}
				m_priv->state = TOOL_DRAWING;
				m_priv->is_draw	= TRUE;
				m_priv->button	= event->button;

				clear_points ();
				/*New Array*/
				m_priv->garray = g_array_new (FALSE, FALSE, sizeof (GdkPoint));
				/*add two point*/
				add_point ( (gint)event->x, (gint)event->y );
				add_point ( (gint)event->x, (gint)event->y );
				break;
			}
			case TOOL_DRAWING:
			{
				if ( m_priv->button != event->button )
				{
					/*cancel*/
					m_priv->state 		= TOOL_NONE;
					m_priv->is_draw		= FALSE;
					clear_points ();
				}
				break;
			}
			case TOOL_WAITING:
			{
				if ( m_priv->button == event->button )
				{
					/*next point*/
					m_priv->state = TOOL_DRAWING;
					add_point ( (gint)event->x, (gint)event->y );
				}
				else
				{
					/*finish*/
					m_priv->state = TOOL_NONE;
					m_priv->is_draw	= FALSE;
					draw_in_pixmap (m_priv->cv->pixmap);
					file_set_unsave ();
					clear_points ();
				}
				break;
			}
		}
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
		if ( m_priv->state == TOOL_DRAWING )
		{
			if( m_priv->button == event->button )
			{
				m_priv->state = TOOL_WAITING;
			}
		}
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if( m_priv->is_draw )
	{
		change_point( (gint)event->x, (gint)event->y );
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static void	
draw ( void )
{
	if ( m_priv->is_draw )
	{
		draw_in_pixmap (m_priv->cv->drawing);
	}
}

static void 
reset ( void )
{
	GdkCursor *cursor = gdk_cursor_new ( GDK_CROSSHAIR );
	g_assert(cursor);
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	gdk_cursor_unref( cursor );
	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	gtk_widget_queue_draw ( m_priv->cv->widget );
	destroy_private_data ();
	g_print("polygon tool destroy\n");
}

static void
draw_in_pixmap ( GdkDrawable *drawable )
{
	if ( m_priv->garray != NULL )
	{
		GdkPoint *	points		=	(GdkPoint *)m_priv->garray->data;
		gint		n_points	=	(gint)m_priv->garray->len;
		if ( m_priv->cv->filled == FILLED_BACK )
		{
			gdk_draw_polygon ( drawable, m_priv->gcb, TRUE, points, n_points);
		}
		else
		if ( m_priv->cv->filled == FILLED_FORE )
		{
			gdk_draw_polygon ( drawable, m_priv->gcf, TRUE, points, n_points);
		}

		if ( m_priv->cv->filled != FILLED_FORE )
		{
			gdk_draw_polygon ( drawable, m_priv->gcf, FALSE, points, n_points);
		}
	}
}

static void
change_point ( gint x, gint y)
{
	GdkPoint *	point	=	&g_array_index (m_priv->garray, GdkPoint, m_priv->garray->len - 1 );
	point->x = x;
	point->y = y;
}

static void
add_point ( gint x, gint y)
{
	GdkPoint	point	=	{ x, y };
	g_array_append_val ( m_priv->garray, point );
}

static void
clear_points( void )
{
	if( m_priv->garray != NULL )
	{
		g_array_free ( m_priv->garray, TRUE);
		m_priv->garray	=	NULL;
	}
}


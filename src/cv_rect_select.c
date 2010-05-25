/***************************************************************************
 *            cv_rect_select.c
 *
 *  Sun May 23 16:37:33 2010
 *  Copyright  2010  rogerio
 *  <rogerioferro@gmail.com>
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

#include "cv_rect_select.h"
#include "file.h"
#include "undo.h"
#include "gp_point_array.h"

/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( void );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void		draw_in_pixmap	( GdkDrawable *drawable );
static void     save_undo       ( void );


/*private data*/
typedef struct {
	gp_tool			tool;
	gp_canvas *		cv;
    gp_point_array  *pa;
	gboolean 		b_selecting;
} private_data;

static private_data		*m_priv = NULL;
	
static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv		=	NULL;
        m_priv->pa      =   gp_point_array_new();
        m_priv->b_selecting     =   FALSE;
	}
}

static void
destroy_private_data( void )
{
    gp_point_array_free( m_priv->pa );
	g_free (m_priv);
	m_priv = NULL;
}

gp_tool * 
tool_rect_select_init ( gp_canvas * canvas )
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
        gp_point_array_clear ( m_priv->pa );
		/*add two point*/
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
        m_priv->b_selecting =   TRUE;      
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
        m_priv->b_selecting =   FALSE;
        gp_point_array_set ( m_priv->pa, 1, (gint)event->x, (gint)event->y );
        gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if ( m_priv->b_selecting )
    {
        gp_point_array_set ( m_priv->pa, 1, (gint)event->x, (gint)event->y );
        gtk_widget_queue_draw ( m_priv->cv->widget );
    }
	return TRUE;
}

static void	
draw ( void )
{
	if ( gp_point_array_size (m_priv->pa) == 2 )
    {
        gint        x,y,w,h;
        cairo_t     *cr;
        GdkPoint    *p;
        gdouble     dashes[2];
        p   =   gp_point_array_data (m_priv->pa);
        cr  =   gdk_cairo_create ( m_priv->cv->drawing );

	    x = MIN(p[0].x,p[1].x);
	    y = MIN(p[0].y,p[1].y);
	    w = ABS(p[1].x-p[0].x);
	    h = ABS(p[1].y-p[0].y);
        
        cairo_set_line_width (cr, 1.0);
        cairo_set_source_rgba (cr, 0.7, 0.9, 1, 0.2);
        cairo_rectangle (cr, x, y, w+1, h+1);
        cairo_fill (cr);
        cairo_set_antialias ( cr, CAIRO_ANTIALIAS_NONE );
        cairo_set_line_join ( cr, CAIRO_LINE_JOIN_ROUND );
        cairo_set_line_cap ( cr, CAIRO_LINE_CAP_ROUND );
        dashes[0] = 3;
        dashes[1] = 3;
        cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
        cairo_set_dash ( cr, &dashes, 2, 1 );
        cairo_rectangle (cr, x, y, w, h);
        cairo_stroke (cr);
        cairo_destroy (cr);
    }
}

static void 
reset ( void )
{
    GdkCursor *cursor = gdk_cursor_new ( GDK_DOTBOX );
	g_assert(cursor);
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	gdk_cursor_unref( cursor );
	m_priv->b_selecting = FALSE;
}

static void 
destroy ( gpointer data  )
{
    gtk_widget_queue_draw ( m_priv->cv->widget );
	destroy_private_data ();
	g_print("rectangle tool destroy\n");
}

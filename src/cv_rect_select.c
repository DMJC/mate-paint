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
#include "cv_drawing.h"
#include "undo.h"
#include "gp_point_array.h"
#include "cv_resize.h"

#include "selection.h"
#include "gp-image.h"

/*private data*/
typedef enum
{
	SEL_NONE,
    SEL_WAITING,
	SEL_DRAWING,
    SEL_ACTION,
} gp_sel_state;


typedef struct {
	gp_tool			tool;
	gp_canvas       *cv;
    gp_sel_state    state;
} private_data;


/*Member functions*/

static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void     set_cursor      ( GdkCursorType cursor_type );
static void     set_point       ( GdkPoint *p );
static void     change_cursor   ( GdkPoint *p );
/* Draw functions */
static void		draw			( void );


static private_data		*m_priv = NULL;
	
static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv		    =	NULL;
        m_priv->state       =   SEL_NONE;
	}
}

static void
destroy_private_data( void )
{
    gp_selection_clear ();
	g_free (m_priv);
	m_priv = NULL;
}

gp_tool * 
tool_rect_select_init ( gp_canvas * canvas )
{
    GdkPoint s = {0, 0};
    GdkPoint e = {0, 0};

    gp_selection_init ();
	create_private_data ();
	m_priv->cv					= canvas;
	m_priv->tool.button_press	= button_press;
	m_priv->tool.button_release	= button_release;
	m_priv->tool.button_motion	= button_motion;
	m_priv->tool.draw			= draw;
	m_priv->tool.reset			= reset;
	m_priv->tool.destroy		= destroy;
	
	/* Create from clipboard pixbuf */
	if(GDK_IS_PIXBUF(canvas->pb_clipboard))
	{	
		e.x = gdk_pixbuf_get_width(canvas->pb_clipboard);
		e.x += s.x;
		e.y = gdk_pixbuf_get_height(canvas->pb_clipboard);
		e.y += s.y;
		if(gp_selection_create (&s, &e, canvas->pb_clipboard)){
			m_priv->state   =   SEL_WAITING;
		}
		g_object_unref(canvas->pb_clipboard);
		canvas->pb_clipboard = NULL;
	}
	
	return &m_priv->tool;
}

static gboolean
button_press ( GdkEventButton *event )
{
    GdkPoint p;
    p.x = (gint)event->x;
    p.y = (gint)event->y;
	if ( event->type == GDK_BUTTON_PRESS )
	{
        if ( gp_selection_start_action ( &p ) )
        {
            m_priv->state   =   SEL_ACTION;
        }
        else
        {
            gp_selection_set_floating ( TRUE );
            gp_selection_set_active ( FALSE );
            gp_selection_clipbox_set_start_point ( &p );
            gp_selection_clipbox_set_end_point ( &p );
            gp_selection_set_active ( TRUE );
            m_priv->state   =   SEL_DRAWING;
        }
        gp_selection_set_borders ( FALSE );
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
    else
    if ( event->type == GDK_2BUTTON_PRESS )
    {
        if ( gp_selection_start_action ( &p ) )
        {
            gp_selection_set_floating ( FALSE );
        }        
    }
        
	return TRUE;
}


static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
        if ( m_priv->state == SEL_DRAWING )
        {
            GdkPoint p;
            p.x = (gint)event->x;
            p.y = (gint)event->y;
            set_point ( &p );
        }
        m_priv->state = SEL_WAITING;
        gp_selection_set_borders ( TRUE );
        gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
    GdkPoint p;
    p.x = (gint)event->x;
    p.y = (gint)event->y;
    switch ( m_priv->state )
    {
        case SEL_DRAWING:
        {
            set_point ( &p );
            gtk_widget_queue_draw ( m_priv->cv->widget );
            break;
        }
        case SEL_WAITING:
        {
            change_cursor ( &p );
            break;
        }
        case SEL_ACTION:
        {
            gp_selection_do_action ( &p );
            gtk_widget_queue_draw ( m_priv->cv->widget );
            break;
        }
    }
    return TRUE;
}

    
static void	
draw ( void )
{
    gp_selection_draw (NULL);
}

static void 
reset ( void )
{
    set_cursor ( GDK_DOTBOX );
}

static void 
destroy ( gpointer data  )
{
	g_print("rect select tool destroy\n");
	gp_selection_draw (m_priv->cv->pixmap);
    gtk_widget_queue_draw ( m_priv->cv->widget );
	destroy_private_data ();
}


static void 
set_cursor ( GdkCursorType cursor_type )
{
    static GdkCursorType last_cursor = GDK_LAST_CURSOR;
    if ( cursor_type != last_cursor )
    {
        GdkCursor *cursor = gdk_cursor_new ( cursor_type );
	    g_assert(cursor);
	    gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	    gdk_cursor_unref( cursor );
        last_cursor = cursor_type;
    }
}

static void 
set_point ( GdkPoint *p )
{
    gint x1,y1;
    GdkRectangle rect;
    cv_get_rect_size ( &rect);
    x1 = rect.width - 1;
    y1 = rect.height - 1;
    if ( p->x < 0 ) p->x = 0;
    else if ( p->x > x1 ) p->x = x1;
    if ( p->y < 0 ) p->y = 0;
    else if ( p->y > y1 ) p->y = y1;
    gp_selection_clipbox_set_end_point ( p );
}

static void 
change_cursor ( GdkPoint *p )
{
    GdkCursorType cursor;
    cursor = gp_selection_get_cursor ( p );
    if ( cursor == GDK_BLANK_CURSOR ) 
    {
        set_cursor ( GDK_DOTBOX );
    }
    else
    {
        set_cursor ( cursor );
    }
}




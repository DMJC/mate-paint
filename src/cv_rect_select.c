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


/*private data*/
typedef enum
{
	SEL_NONE,
    SEL_WAITING,
	SEL_DRAWING,
    SEL_RESIZING_TL,
    SEL_RESIZING_TM,
    SEL_RESIZING_TR,
    SEL_RESIZING_ML,
    SEL_RESIZING_MR,
    SEL_RESIZING_BL,
    SEL_RESIZING_BM,
    SEL_RESIZING_BR,
	SEL_MOVING
} gp_sel_state;

typedef enum {
    SEL_TOP_LEFT,
    SEL_TOP_MID,
    SEL_TOP_RIGHT,
    SEL_MID_RIGHT,
    SEL_BOTTOM_RIGHT,
    SEL_BOTTOM_MID,
    SEL_BOTTOM_LEFT,
    SEL_MID_LEFT,
    SEL_N_BORDERS
} gp_sel_border;

typedef struct {
    gint x0;
    gint y0;
    gint x1;
    gint y1;
} gp_sel_rect;


typedef struct {
	gp_tool			tool;
	gp_canvas       *cv;
    gp_point_array  *pa;
    gp_sel_state    state;
    gp_sel_state    command;
    gp_sel_rect     r_sel;
    gp_sel_rect     r_borders[SEL_N_BORDERS];
    GdkPoint        p_drag;
} private_data;


/*Member functions*/
static gboolean	button_press	( GdkEventButton *event );
static gboolean	button_release	( GdkEventButton *event );
static gboolean	button_motion	( GdkEventMotion *event );
static void		draw			( void );
static void		reset			( void );
static void		destroy			( gpointer data  );
static void     set_cursor      ( GdkCursorType cursor_type );
static void     set_point       ( gint x, gint y );
static gboolean point_in        ( GdkPoint *point, gp_sel_rect *rect);
static void     update_sel      ( void );
static void     change_cursor   ( GdkPoint *p );
static void     set_sel_rect    ( gp_sel_rect *rect, gint x0, gint y0, gint x1, gint y1 );
static void     draw_sel_area   ( cairo_t *cr, GdkGC *gc, GdkRectangle *rect );
static void     draw_sel_borders( cairo_t *cr, GdkGC *gc, GdkRectangle *rect );
static void     do_command      ( gint dx, gint dy );


static private_data		*m_priv = NULL;
	
static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv		    =	NULL;
        m_priv->pa          =   gp_point_array_new();
        m_priv->state       =   SEL_NONE;
        m_priv->command     =   SEL_NONE;
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
        if ( m_priv->command == SEL_NONE )
        {
            gp_point_array_clear ( m_priv->pa );
		    /*add two point*/
            gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
            gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
            m_priv->state   =   SEL_DRAWING;
        }
        else
        {
            m_priv->p_drag.x    =   (gint)event->x;
            m_priv->p_drag.y    =   (gint)event->y;
            m_priv->state       =   m_priv->command;
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
        if ( m_priv->state == SEL_DRAWING )
        {
            set_point ( (gint)event->x, (gint)event->y );
        }
        m_priv->state = SEL_WAITING;
        update_sel ();
        gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
    switch ( m_priv->state )
    {
        case SEL_DRAWING:
        {
            set_point ( (gint)event->x, (gint)event->y );
            gtk_widget_queue_draw ( m_priv->cv->widget );
            break;
        }
        case SEL_WAITING:
        {
            GdkPoint p;
            p.x = event->x;
            p.y = event->y;
            change_cursor ( &p );
            break;
        }
        case SEL_MOVING:
        case SEL_RESIZING_TL:
        case SEL_RESIZING_TM:
        case SEL_RESIZING_TR:
        case SEL_RESIZING_ML:
        case SEL_RESIZING_MR:
        case SEL_RESIZING_BL:
        case SEL_RESIZING_BM:
        case SEL_RESIZING_BR:
        {
            gint dx = event->x - m_priv->p_drag.x;
            gint dy = event->y - m_priv->p_drag.y;
            do_command ( dx, dy );
            m_priv->p_drag.x += dx;
            m_priv->p_drag.y += dy;
            gtk_widget_queue_draw ( m_priv->cv->widget );
            break;
        }
    }
    return TRUE;
}


static void	
draw ( void )
{
	if ( gp_point_array_size (m_priv->pa) == 2 )
    {
        GdkRectangle    rect;
        gp_point_array_get_clipbox ( m_priv->pa, &rect, 0, NULL );

        if ( rect.width != 1 &&  rect.height != 1 )
        {
         	gint8 dash_list[]	=	{ 3, 3 };
	        GdkGC *gc	=	gdk_gc_new ( m_priv->cv->widget->window );
            cairo_t     *cr;
            cr  =   gdk_cairo_create ( m_priv->cv->drawing );
	        gdk_gc_set_function ( gc, GDK_INVERT );
	        gdk_gc_set_dashes ( gc, 0, dash_list, 2 );
	        gdk_gc_set_line_attributes ( gc, 1, GDK_LINE_ON_OFF_DASH,
	                                     GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );

            draw_sel_area (cr, gc, &rect );
            if ( m_priv->state == SEL_WAITING )
            {
                draw_sel_borders (cr, gc, &rect );
            }
            else
            {
                gdk_draw_rectangle ( m_priv->cv->drawing, gc, FALSE, 
                                    rect.x, rect.y, rect.width-1, rect.height-1);
            }
            cairo_destroy (cr);
            g_object_unref ( gc );
        }
    }
}

static void 
reset ( void )
{
    set_cursor ( GDK_DOTBOX );
}

static void 
destroy ( gpointer data  )
{
    gtk_widget_queue_draw ( m_priv->cv->widget );
	destroy_private_data ();
	g_print("rect select tool destroy\n");
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
update_sel ( void )
{
    GdkPoint    *p;
    p   =   gp_point_array_data (m_priv->pa);
    m_priv->r_sel.x0 = MIN(p[0].x,p[1].x);
    m_priv->r_sel.y0 = MIN(p[0].y,p[1].y);
    m_priv->r_sel.x1 = MAX(p[0].x,p[1].x);
    m_priv->r_sel.y1 = MAX(p[0].y,p[1].y);
    p[0].x = m_priv->r_sel.x0;
    p[0].y = m_priv->r_sel.y0;
    p[1].x = m_priv->r_sel.x1;
    p[1].y = m_priv->r_sel.y1;
}

static gboolean 
point_in ( GdkPoint *point,  gp_sel_rect *rect)
{
    if (    point->x >= rect->x0 && 
            point->x <= rect->x1 && 
            point->y >= rect->y0 && 
            point->y <= rect->y1 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void set_point ( gint x, gint y )
{
    gint x1,y1;
    GdkRectangle rect;
    cv_get_rect_size ( &rect);
    x1 = rect.width - 1;
    y1 = rect.height - 1;
    if ( x < 0 ) x = 0;
    else if ( x > x1 ) x = x1;
    if ( y < 0 ) y = 0;
    else if ( y > y1 ) y = y1;
    gp_point_array_set ( m_priv->pa, 1, x, y);
}

static void 
change_cursor ( GdkPoint *p )
{
    if ( point_in (p, &m_priv->r_borders[SEL_TOP_LEFT] ) )
    {
        set_cursor ( GDK_TOP_LEFT_CORNER );
        m_priv->command = SEL_RESIZING_TL;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_TOP_MID] ) )
    {
        set_cursor ( GDK_TOP_SIDE );
        m_priv->command = SEL_RESIZING_TM;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_TOP_RIGHT] ) )
    {
        set_cursor ( GDK_TOP_RIGHT_CORNER );
        m_priv->command = SEL_RESIZING_TR;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_MID_LEFT] ) )
    {
        set_cursor ( GDK_LEFT_SIDE );
        m_priv->command = SEL_RESIZING_ML;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_MID_RIGHT] ) )
    {
        set_cursor ( GDK_RIGHT_SIDE );
        m_priv->command = SEL_RESIZING_MR;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_BOTTOM_LEFT] ) )
    {
        set_cursor ( GDK_BOTTOM_LEFT_CORNER );
        m_priv->command = SEL_RESIZING_BL;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_BOTTOM_MID] ) )
    {
        set_cursor ( GDK_BOTTOM_SIDE );
        m_priv->command = SEL_RESIZING_BM;
    }
    else
    if ( point_in (p, &m_priv->r_borders[SEL_BOTTOM_RIGHT] ) )
    {
        set_cursor ( GDK_BOTTOM_RIGHT_CORNER );
        m_priv->command = SEL_RESIZING_BR;
    }
    else
    if ( point_in (p, &m_priv->r_sel ) )
    {
        set_cursor ( GDK_FLEUR );
        m_priv->command = SEL_MOVING;
    }
    else
    {
        m_priv->command = SEL_NONE;
        set_cursor ( GDK_DOTBOX );
    }
}


static void
draw_sel_area ( cairo_t *cr, GdkGC *gc, GdkRectangle *rect )
{
    cairo_set_line_width (cr, 1.0);
    cairo_set_source_rgba (cr, 0.7, 0.9, 1.0, 0.2);
    cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
    cairo_fill (cr);

//    cairo_set_source_rgba (cr, 0.7, 0.9, 1.0, 1.0);
//    cairo_set_operator ( cr, CAIRO_OPERATOR_HSL_LUMINOSITY );
 //   cairo_rectangle (cr, 0, 0, 30, 0);
 //   cairo_fill (cr);
    
        
}

static void 
draw_sel_rect ( cairo_t *cr, GdkGC *gc, gp_sel_rect *rect )
{
    gint x,y,w,h;
    x = rect->x0;
    y = rect->y0;
    w = (rect->x1 - rect->x0 + 1);
    h = (rect->y1 - rect->y0 + 1);
    gdk_draw_rectangle ( m_priv->cv->drawing, gc, TRUE, 
                         x, y, w, h);
}

static void 
set_sel_rect ( gp_sel_rect *rect, gint x0, gint y0, gint x1, gint y1 )
{
    rect->x0 = x0;
    rect->y0 = y0;
    rect->x1 = x1;
    rect->y1 = y1;
}

static void
draw_sel_borders ( cairo_t *cr, GdkGC *gc, GdkRectangle *rect )
{
    const gint s = 4;
    gint i;
    gint xl = rect->x;
    gint xm = rect->x + (rect->width - 1)/2;
    gint xr = rect->x + (rect->width - 1);
    gint yt = rect->y;
    gint ym = rect->y + (rect->height - 1)/2;
    gint yb = rect->y + (rect->height - 1);

    set_sel_rect ( &m_priv->r_borders[SEL_TOP_LEFT],
                   xl,yt,xl+s,yt+s);
    set_sel_rect ( &m_priv->r_borders[SEL_TOP_MID],
                   xm-s/2,yt,xm+s/2,yt+s);
    set_sel_rect ( &m_priv->r_borders[SEL_TOP_RIGHT],
                   xr-s,yt,xr,yt+s);
    set_sel_rect ( &m_priv->r_borders[SEL_MID_LEFT],
                   xl,ym-s/2,xl+s,ym+s/2);
    set_sel_rect ( &m_priv->r_borders[SEL_MID_RIGHT],
                   xr-s,ym-s/2,xr,ym+s/2);
    set_sel_rect ( &m_priv->r_borders[SEL_BOTTOM_LEFT],
                   xl,yb-s,xl+s,yb);
    set_sel_rect ( &m_priv->r_borders[SEL_BOTTOM_MID],
                   xm-s/2,yb-s,xm+s/2,yb);
    set_sel_rect ( &m_priv->r_borders[SEL_BOTTOM_RIGHT],
                   xr-s,yb-s,xr,yb);

    
    for ( i = 0; i < SEL_N_BORDERS; i++ )
    {
        draw_sel_rect ( cr, gc, &m_priv->r_borders[i]);
    }

    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[0].x1+1,m_priv->r_borders[0].y0,
                    m_priv->r_borders[1].x0-1,m_priv->r_borders[1].y0);
    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[1].x1+1,m_priv->r_borders[1].y0,
                    m_priv->r_borders[2].x0-1,m_priv->r_borders[2].y0);

    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[2].x1,m_priv->r_borders[2].y1+1,
                    m_priv->r_borders[3].x1,m_priv->r_borders[3].y0-1);
    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[3].x1,m_priv->r_borders[3].y1+1,
                    m_priv->r_borders[4].x1,m_priv->r_borders[4].y0-1);

    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[4].x0-1,m_priv->r_borders[4].y1,
                    m_priv->r_borders[5].x1+1,m_priv->r_borders[5].y1);
    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[5].x0-1,m_priv->r_borders[5].y1,
                    m_priv->r_borders[6].x1+1,m_priv->r_borders[6].y1);

    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[6].x0,m_priv->r_borders[6].y0-1,
                    m_priv->r_borders[7].x0,m_priv->r_borders[7].y1+1);
    gdk_draw_line ( m_priv->cv->drawing, gc, 
                    m_priv->r_borders[7].x0,m_priv->r_borders[7].y0-1,
                    m_priv->r_borders[0].x0,m_priv->r_borders[0].y1+1);
}

static void
do_command ( gint dx, gint dy )
{
    GdkPoint *p = gp_point_array_data ( m_priv->pa );
    switch ( m_priv->state )
    {
        case SEL_MOVING:
            gp_point_array_offset ( m_priv->pa, dx, dy );
            break;
        case SEL_RESIZING_TL:
            p[0].y  += dy;
        case SEL_RESIZING_ML:
            p[0].x  += dx;
            break;
        case SEL_RESIZING_TM:
            p[0].y  += dy;
            break;
        case SEL_RESIZING_TR:
            p[0].y  += dy;
            p[1].x  += dx;
            break;
        case SEL_RESIZING_BL:
            p[1].y  += dy;
            p[0].x  += dx;
            break;
        case SEL_RESIZING_BM:
            p[1].y  += dy;
            break;
        case SEL_RESIZING_BR:
            p[1].y  += dy;
        case SEL_RESIZING_MR:
            p[1].x  += dx;
            break;
    }
}

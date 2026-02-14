/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Contributed by Juan Balderas

	This file is part of mate-paint.

	mate-paint is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	mate-paint is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with mate-paint.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/
 
 #include <gtk/gtk.h>

#include "cv_rounded_rectangle_tool.h"
#include "file.h"
#include "undo.h"
#include "gp_point_array.h"
#include "cv_drawing.h"

static void draw_rounded_rectangle(cairo_t *cr, GdkGC *gc, gboolean filled, gint x,
									gint y, gint width, gint height, gint line_width);

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
	GdkGC *			gcf;
	GdkGC *			gcb;
    gp_point_array  *pa;
	guint			button;
	gboolean 		is_draw;
} private_data;

static private_data		*m_priv = NULL;
	
static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv		=	NULL;
		m_priv->gcf		=	NULL;
		m_priv->gcb		=	NULL;
		m_priv->button	=	NONE_BUTTON;
		m_priv->is_draw	=	FALSE;
        m_priv->pa      =   gp_point_array_new();
        
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
tool_rounded_rectangle_init ( gp_canvas * canvas )
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
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw ) m_priv->button = event->button;

        gp_point_array_clear ( m_priv->pa );
		/*add two point*/
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
        gp_point_array_append ( m_priv->pa, (gint)event->x, (gint)event->y );
        
		if( !m_priv->is_draw ) gtk_widget_queue_draw ( m_priv->cv->widget );
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static gboolean
button_release ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_RELEASE )
	{
		if( m_priv->button == event->button )
		{
			if( m_priv->is_draw )
			{
   	            save_undo ();
				draw_in_pixmap (m_priv->cv->pixmap);
				file_set_unsave ();
			}
			gtk_widget_queue_draw ( m_priv->cv->widget );
            gp_point_array_clear ( m_priv->pa );
            m_priv->is_draw = FALSE;
		}
	}
	return TRUE;
}

static gboolean
button_motion ( GdkEventMotion *event )
{
	if( m_priv->is_draw )
	{
        gp_point_array_set ( m_priv->pa, 1, (gint)event->x, (gint)event->y );
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
    GdkCursor *cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_DOTBOX);
	g_assert(cursor);
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	g_object_unref ( cursor );
	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	destroy_private_data ();
	g_print("rounded_rectangle tool destroy\n");
}

static void
draw_in_pixmap ( GdkDrawable *drawable )
{
    GdkRectangle    rect;
    GdkPoint        *p = gp_point_array_data (m_priv->pa);
	rect.x      = MIN(p[0].x,p[1].x);
	rect.y      = MIN(p[0].y,p[1].y);
	rect.width  = ABS(p[1].x-p[0].x);
	rect.height = ABS(p[1].y-p[0].y);

	cairo_t         *cr;

	cr = gdk_cairo_create (drawable);
	if (cr == NULL) {
		return;
	}

	if ( m_priv->cv->filled == FILLED_BACK )
	{
		draw_rounded_rectangle (cr, m_priv->gcb, TRUE, rect.x, rect.y, rect.width, rect.height, m_priv->cv->line_width);
	}
	else
	if ( m_priv->cv->filled == FILLED_FORE )
	{
		draw_rounded_rectangle (cr, m_priv->gcf, TRUE, rect.x, rect.y, rect.width, rect.height, m_priv->cv->line_width);
	}
	draw_rounded_rectangle (cr, m_priv->gcf, FALSE, rect.x, rect.y, rect.width, rect.height, m_priv->cv->line_width);
	cairo_destroy (cr);
}

static void     
save_undo ( void )
{
    GdkRectangle    rect;
    GdkRectangle    rect_max;
    GdkBitmap       *mask = NULL;

    cv_get_rect_size ( &rect_max );
    gp_point_array_get_clipbox ( m_priv->pa, &rect, m_priv->cv->line_width, &rect_max );
    if ( m_priv->cv->filled == FILLED_NONE )
    {
        GdkPoint    *p = gp_point_array_data (m_priv->pa);
        cairo_t     *cr_mask;
        gint        x,y,w,h;
        x   = MIN(p[0].x,p[1].x);
        y   = MIN(p[0].y,p[1].y);
        w   = ABS(p[1].x-p[0].x);
        h   = ABS(p[1].y-p[0].y);
        undo_create_mask ( rect.width, rect.height, &mask, &cr_mask );
        draw_rounded_rectangle ( cr_mask, NULL, FALSE,
                                 x - rect.x, y-rect.y, w, h, m_priv->cv->line_width );

        cairo_destroy (cr_mask);
    }                
    undo_add ( &rect, mask, NULL, TOOL_ROUNDED_RECTANGLE );
    if ( mask != NULL ) g_object_unref (mask);
}

static void add_rounded_rectangle_path (cairo_t *cr, gint x, gint y, gint width, gint height, gint warc, gint harc)
{
	double radius_x = warc / 2.0;
	double radius_y = harc / 2.0;
	double x0 = x;
	double y0 = y;
	double x1 = x + width;
	double y1 = y + height;

	cairo_save (cr);
	cairo_translate (cr, x0 + radius_x, y0 + radius_y);
	cairo_scale (cr, radius_x, radius_y);
	cairo_arc (cr, (x1 - x0 - warc) / (2.0 * radius_x), (y1 - y0 - harc) / (2.0 * radius_y), 1.0, -G_PI_2, 0);
	cairo_arc (cr, (x1 - x0 - warc) / (2.0 * radius_x), (y1 - y0 - harc) / (2.0 * radius_y), 1.0, 0, G_PI_2);
	cairo_arc (cr, 0, (y1 - y0 - harc) / (2.0 * radius_y), 1.0, G_PI_2, G_PI);
	cairo_arc (cr, 0, 0, 1.0, G_PI, 3 * G_PI_2);
	cairo_close_path (cr);
	cairo_restore (cr);
}

static void draw_rounded_rectangle(cairo_t *cr, GdkGC *gc, gboolean filled, gint x,
					   gint y, gint width, gint height, gint line_width)
{
	GdkGCValues values;
	gint warc = 16;
	gint harc = 16;

	if ((width <= 0) || (height <= 0))
		return;

	if (gc != NULL)
	{
		gdk_gc_get_values (gc, &values);
		gdk_cairo_set_source_color (cr, &values.foreground);
	}

	if ((width < warc) && (height < harc)) {
		cairo_arc (cr,
			x + (width / 2.0),
			y + (height / 2.0),
			MIN (width, height) / 2.0,
			0,
			2 * G_PI);
	} else {
		if (width < warc) warc = width;
		if (height < harc) harc = height;
		add_rounded_rectangle_path (cr, x, y, width, height, warc, harc);
	}

	if (filled) {
		cairo_fill (cr);
	} else {
		cairo_set_line_width (cr, MAX (1, line_width));
		cairo_stroke (cr);	
	}
}
















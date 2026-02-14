/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Copyright (C) Juan Balderas 2010

	cv_curve_tool.c is part of mate-paint.

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

#include "cv_curve_tool.h"
#include "cv_drawing.h"
#include "file.h"
#include "undo.h"
#include "gp_point_array.h"

typedef enum{
	GP_CURVE_DO_LINE,
	GP_CURVE_DO_CURVE,
	GP_CURVE_SET
}GPCurveAction;

static void draw_line_with_cairo (GdkDrawable *drawable,
                                  GdkGC *gc,
                                  gint x0,
                                  gint y0,
                                  gint x1,
                                  gint y1);
static void draw_bezier_with_cairo (GdkDrawable *drawable,
                                    GdkGC *gc,
                                    GdkPoint pt1,
                                    GdkPoint pt2,
                                    GdkPoint pt3);

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
	guint			button;
	gboolean 		is_draw;
	GdkPoint		start, crv, end;
	gint			action;
} private_data;

static void
draw_line_with_cairo (GdkDrawable *drawable, GdkGC *gc, gint x0, gint y0, gint x1, gint y1)
{
	GdkGCValues values;
	cairo_t *cr;

	g_return_if_fail (drawable != NULL);
	g_return_if_fail (gc != NULL);

	cr = gdk_cairo_create (drawable);
	gdk_gc_get_values (gc, &values);

	cairo_set_source_rgb (cr,
	                      values.foreground.red / 65535.0,
	                      values.foreground.green / 65535.0,
	                      values.foreground.blue / 65535.0);
	cairo_set_line_width (cr, MAX (values.line_width, 1));
	cairo_move_to (cr, x0 + 0.5, y0 + 0.5);
	cairo_line_to (cr, x1 + 0.5, y1 + 0.5);
	cairo_stroke (cr);
	cairo_destroy (cr);
}

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
        m_priv->action	=	GP_CURVE_DO_LINE;
	}
}

static void
destroy_private_data( void )
{
	g_free (m_priv);
	m_priv = NULL;
}

gp_tool * 
tool_curve_init ( gp_canvas * canvas )
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

        switch(m_priv->action){
        	case GP_CURVE_DO_LINE:
        		m_priv->start.x = (gint)event->x;
        		m_priv->start.y = (gint)event->y;
        		m_priv->end = m_priv->start;
				break;
			case GP_CURVE_DO_CURVE:
				m_priv->crv.x = (gint)event->x;
        		m_priv->crv.y = (gint)event->y;
				break;
			case GP_CURVE_SET:
				break;
			default:
				break;
        }
        
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
   	            switch(m_priv->action)
   	            {
        			case GP_CURVE_DO_LINE:
        				m_priv->is_draw = TRUE;
        				m_priv->end.x = (gint)event->x;
        				m_priv->end.y = (gint)event->y;
       			 		m_priv->action = GP_CURVE_DO_CURVE;
       			 		m_priv->crv.x = (gint)event->x;
        				m_priv->crv.y = (gint)event->y;
						m_priv->is_draw = FALSE;
						return TRUE;
					case GP_CURVE_DO_CURVE:
						m_priv->crv.x = (gint)event->x;
        				m_priv->crv.y = (gint)event->y;
        				m_priv->action = GP_CURVE_SET;
						m_priv->is_draw = FALSE;
						return TRUE;
					case GP_CURVE_SET:
						m_priv->crv.x = (gint)event->x;
        				m_priv->crv.y = (gint)event->y;
						save_undo ();
						draw_in_pixmap (m_priv->cv->pixmap);
						file_set_unsave ();
            			m_priv->action = GP_CURVE_DO_LINE;
						break;
					default:
						return TRUE;
				}
			}
			gtk_widget_queue_draw ( m_priv->cv->widget );
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
        switch(m_priv->action){
      	  	case GP_CURVE_DO_LINE:
      	  		m_priv->end.x = (gint)event->x;
      	  		m_priv->end.y = (gint)event->y;
				break;
			case GP_CURVE_DO_CURVE:
				m_priv->crv.x = (gint)event->x;
        		m_priv->crv.y = (gint)event->y;
				break;
			case GP_CURVE_SET:
				break;
			default:
				return TRUE;
        }
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
	GdkCursor *cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
	g_assert(cursor);
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	g_object_unref ( cursor );
	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	destroy_private_data ();
	g_print("curve tool destroy\n");
}

static void
draw_in_pixmap ( GdkDrawable *drawable )
{
    switch(m_priv->action)
    {
       	case GP_CURVE_DO_LINE:
		draw_line_with_cairo (drawable, m_priv->gcf,
		                     m_priv->start.x, m_priv->start.y,
		                     m_priv->end.x, m_priv->end.y);
			break;
		case GP_CURVE_DO_CURVE:
			draw_bezier_with_cairo (drawable, m_priv->gcf,
			                       m_priv->start, m_priv->crv, m_priv->end);
			break;
		case GP_CURVE_SET:
			draw_bezier_with_cairo (drawable, m_priv->gcf,
			                       m_priv->start, m_priv->crv, m_priv->end);
			break;
		default:
			break;
    }
}

static void
draw_bezier_with_cairo (GdkDrawable *drawable, GdkGC *gc, GdkPoint pt1, GdkPoint pt2, GdkPoint pt3)
{
	gint x, y, x2, y2;
	double t, t2;
	
	x2 = pt1.x;
	y2 = pt1.y;
	
	for(t = 0.0; t < 1.0 + .02; t += .02)
	{
		t2 = t;

		x = ((1 - t2) * (1 - t2)) *
			pt1.x + 2 * t2 * (1 -t2) *
			pt2.x + (t2 * t2) *
			pt3.x ;

		y = ((1 - t2) * (1 - t2)) *
			pt1.y + 2 * t2 * (1 -t2) *
			pt2.y + (t2 * t2) *
			pt3.y ;

		draw_line_with_cairo (drawable, gc, x, y, x2, y2);
		x2 = x; y2 = y;
	}

}

static void     
save_undo ( void )
{
    GdkRectangle    rect;
    GdkRectangle    rect_max;
    GdkBitmap       *mask = NULL;
	GdkGC	        *gc_mask;
	gp_point_array  *pa;
	GdkPoint		*points;

    cv_get_rect_size ( &rect_max );
    pa = gp_point_array_new ();
    gp_point_array_append ( pa, m_priv->start.x, m_priv->start.y );
    gp_point_array_append ( pa, m_priv->crv.x, m_priv->crv.y );
    gp_point_array_append ( pa, m_priv->end.x, m_priv->end.y );
    
    gp_point_array_get_clipbox ( pa, &rect, m_priv->cv->line_width, &rect_max );
        
	undo_create_mask ( rect.width, rect.height, &mask, &gc_mask );
	
	gp_point_array_offset ( pa, -rect.x, -rect.y);
	points = gp_point_array_data ( pa );

	draw_bezier_with_cairo (mask, gc_mask, points[0], points[1], points[2]);

	g_object_unref (gc_mask);
	gp_point_array_free ( pa );

    undo_add ( &rect, mask, NULL, TOOL_CURVE );
    if ( mask != NULL ) g_object_unref (mask);
}






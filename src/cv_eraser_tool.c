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
#include <math.h>

#include "cv_eraser_tool.h"
#include "file.h"
#include "gp-image.h"
#include "toolbar.h"
#include "undo.h"

#define ERASER_WIDTH	17
#define ERASER_HEIGHT	17

typedef enum{
	GP_ERASER_TYPE_ROUND,
	GP_ERASER_TYPE_RECTANGLE
}GPEraserType;

typedef void (DrawEraserFunc)(GdkDrawable *drawable, int x, int y);

static const double EPSILON = 0.00001;

static gint m_last_eraser = GP_ERASER_RECT_TINY;

static GdkPixbuf *g_pixbuf = NULL;

static void b_rush_interpolate(GdkDrawable *drawable, DrawEraserFunc *draw_eraser_func, int x, int y);
static GdkCursor *create_eraser_cursor(GPEraserType type);
static void set_eraser_values(GPEraserType type, GPEraserSize size);
static void set_eraser_size(GPEraserSize size);

/* Private drawing functions */
static void draw_round_eraser(GdkDrawable *drawable, int x, int y);
static void draw_rectangular_eraser(GdkDrawable *drawable, int x, int y);
static void draw_back_slash_eraser(GdkDrawable *drawable, int x, int y);
static void draw_fwd_slash_eraser(GdkDrawable *drawable, int x, int y);
static void draw_pixbuf_eraser(GdkDrawable *drawable, int x, int y);

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
	GdkGC *			gc;
	gint 			x0,y0;
	gint			x_min,y_min,x_max,y_max;
	guint			button;
	gboolean 		is_draw;
	double			spacing;        
    double			distance;
    GdkPoint		drag;
    DrawEraserFunc	*draw_eraser;
    GPEraserType	eraser_type;
    gint			width, height; /* eraser width & height */
    GdkPixmap *		bg_pixmap;
    gint			xprev;
    gint			yprev;
} private_data;

static private_data		*m_priv = NULL;

static void
destroy_background ( void )
{
    if (m_priv->bg_pixmap != NULL) 
    {
        g_object_unref (m_priv->bg_pixmap);
        m_priv->bg_pixmap = NULL;
    }
}

static void
save_background ( void )
{
	gint w,h;
    destroy_background ();
	gdk_drawable_get_size ( m_priv->cv->pixmap, &w, &h );
	m_priv->bg_pixmap = gdk_pixmap_new ( m_priv->cv->drawing, w, h, -1);
	gdk_draw_drawable (	m_priv->bg_pixmap,
		            	m_priv->cv->gc_fg,
			            m_priv->cv->pixmap,
			            0, 0,
			            0, 0,
			            w, h );
}

static void
restore_background ( void )
{
    if ( m_priv->bg_pixmap != NULL )
    {
	    gdk_draw_drawable (	m_priv->cv->pixmap,
		                	m_priv->cv->gc_fg,
			                m_priv->bg_pixmap,
			                0, 0,
			                0, 0,
			                -1, -1 );
        destroy_background ();
    }    
}

static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_slice_new0 (private_data);
		m_priv->cv			=	NULL;
		m_priv->gc			=	NULL;
		m_priv->button		=	0;
		m_priv->is_draw		=	FALSE;
		m_priv->distance	=	0;
		m_priv->eraser_type	=	GP_ERASER_TYPE_RECTANGLE; /* change type here to test other eraseres */
		m_priv->width		=	ERASER_WIDTH;
		m_priv->height		=	ERASER_HEIGHT;
        m_priv->bg_pixmap   =   NULL;
		
		set_eraser_values(m_priv->eraser_type, m_last_eraser);
	}
}

static void
destroy_private_data( void )
{
    destroy_background ();
	g_slice_free (private_data, m_priv);
	m_priv = NULL;
}


gp_tool * 
tool_eraser_init ( gp_canvas * canvas )
{
	create_private_data ();
	m_priv->cv					=	canvas;
	m_priv->tool.button_press	= 	button_press;
	m_priv->tool.button_release	= 	button_release;
	m_priv->tool.button_motion	= 	button_motion;
	m_priv->tool.draw			= 	draw;
	m_priv->tool.reset			= 	reset;
	m_priv->tool.destroy		= 	destroy;
	return &m_priv->tool;
}

static gboolean
button_press ( GdkEventButton *event )
{
	if ( event->type == GDK_BUTTON_PRESS )
	{
		if ( event->button == LEFT_BUTTON )
		{
			m_priv->gc = m_priv->cv->gc_bg_pencil;
		}
		else if ( event->button == RIGHT_BUTTON )
		{
			m_priv->gc = m_priv->cv->gc_fg_pencil;
		}
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw )
		{
			m_priv->button = event->button;
            save_background();
		}
        else
        {
            restore_background ();
        }
		m_priv->drag.x = (gint)event->x;
		m_priv->drag.y = (gint)event->y;
		m_priv->x0 = m_priv->drag.x;
		m_priv->y0 = m_priv->drag.y;
        m_priv->x_min = G_MAXINT;
        m_priv->x_max = G_MININT;
		m_priv->y_min = G_MAXINT;
        m_priv->y_max = G_MININT;

		if( !m_priv->is_draw )
		{
			gtk_widget_queue_draw ( m_priv->cv->widget );
		}
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
				draw_in_pixmap (m_priv->cv->pixmap);
                save_undo ();
				file_set_unsave ();
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
	GdkModifierType state;
	gint x, y;
	gdouble xd, yd;

	if( m_priv->is_draw )
	{
		
		if (gdk_event_get_coords ((GdkEvent *) event, &xd, &yd))
		{
			x = (gint) xd;
			y = (gint) yd;
		}
		else
		{
			x = (gint) event->x;
			y = (gint) event->y;
		}
		state = event->state;
		
		m_priv->xprev = m_priv->x0;
		m_priv->yprev = m_priv->y0;
		
		m_priv->x0 = x;
		m_priv->y0 = y;
		gtk_widget_queue_draw ( m_priv->cv->widget );
	}
	return TRUE;
}

static void	
draw ( void )
{
	if ( m_priv->is_draw )
	{
		draw_in_pixmap ( m_priv->cv->pixmap );
	}
}

static void 
reset ( void )
{
	GdkCursor *cursor;
	
	cursor = create_eraser_cursor(m_priv->eraser_type);
	
	if(!cursor)
	{
		printf("Debug eraser reset\n");
		cursor = gdk_cursor_new ( GDK_CROSSHAIR );
		g_assert(cursor);
	}
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	gdk_cursor_unref( cursor );

	m_priv->is_draw = FALSE;
}

static void 
destroy ( gpointer data  )
{
	destroy_private_data ();
	if(GDK_IS_PIXBUF(g_pixbuf))
	{
		g_object_unref(g_pixbuf);
	}
	g_print("eraser tool destroy\n");
}

static void
draw_in_pixmap ( GdkDrawable *drawable )
{
	b_rush_interpolate(drawable, m_priv->draw_eraser, m_priv->x0, m_priv->y0);
	m_priv->drag.x = m_priv->x0;
	m_priv->drag.y = m_priv->y0;
}

/* Create a new cursor after bg color change */
void notify_eraser_of_bg_color_change(void)
{
	GdkCursor *cursor;

    if ( m_priv == NULL ) return;
	
	cursor = create_eraser_cursor(m_priv->eraser_type);
	
	if(!cursor)
	{
		cursor = gdk_cursor_new ( GDK_CROSSHAIR );
		g_assert(cursor);
	}
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	gdk_cursor_unref( cursor );
}

/*
	The following code was copped & modified from gpaint
*/
static void 
b_rush_interpolate(GdkDrawable *drawable, DrawEraserFunc *draw_eraser_func, int x, int y)
{
	double dx;		/* delta x */
	double dy;		/* delta y */
	double moved;	/* mouse movement */	   
	double initial;	/* initial paint distance */
	double final;	/* final paint distance   */

	double points;	/* number of paint points */
	double next;	/* distance to the next point */
	double percent;	/* linear interplotation, 0 to 1.0 */
  
	/* calculate mouse move distance */
	dx = (double)(x - m_priv->drag.x);
	dy = (double)(y - m_priv->drag.y);
	moved = sqrt(dx*dx + dy*dy);

	initial = m_priv->distance;
	final = initial + moved;

	/* paint along the movement */
	while (m_priv->distance < final)
	{
		/* calculate distance to the next point */
		points = (int) (m_priv->distance / m_priv->spacing + 1.0 + EPSILON); 
		next = points * m_priv->spacing - m_priv->distance;
		m_priv->distance += next;		  
		if (m_priv->distance <= (final + EPSILON))
		{   
		   /* calculate interpolation */ 
			percent = (m_priv->distance - initial) / moved;
			x = (int)(m_priv->drag.x + percent * dx);
			y = (int)(m_priv->drag.y + percent * dy);

		    if (m_priv->x_min>x)m_priv->x_min=x;
		    if (m_priv->x_max<x)m_priv->x_max=x;
		    if (m_priv->y_min>y)m_priv->y_min=y;
		    if (m_priv->y_max<y)m_priv->y_max=y;
            
			draw_eraser_func(drawable, x, y);
		}
	 }
	 m_priv->distance = final;
}

static void draw_round_eraser(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_arc( drawable, m_priv->gc, TRUE, x, y, m_priv->width, m_priv->height, 0, 360 * 64);
}

static void draw_rectangular_eraser(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_rectangle( drawable, m_priv->gc, TRUE, x, y, m_priv->width, m_priv->height);
}

static GdkCursor *create_eraser_cursor(GPEraserType type)
{
	GdkCursor *cursor = NULL;
	GdkPixmap *pixmap;
	GdkPixbuf *pixbuf, *tmp;

	pixmap = gdk_pixmap_new(m_priv->cv->widget->window, m_priv->width, m_priv->height, -1);
	if(!GDK_IS_PIXMAP(pixmap))
	{
		printf("Debug: create_eraser_cursor() !GDK_IS_PIXMAP(pixmap)\n");
		goto CURSOR_CLEANUP;
	}

	/* Draw eraser onto pixmap with back color */
	switch(type)
	{
		case GP_ERASER_TYPE_ROUND:
			gdk_draw_arc( pixmap, m_priv->cv->gc_bg_pencil, TRUE, 0, 0,
						  m_priv->width, m_priv->height, 0, 360 * 64 );
			gdk_draw_arc( pixmap, m_priv->cv->widget->style->black_gc, FALSE, 0, 0,
						  m_priv->width, m_priv->height, 0, 360 * 64 );
			break;
		case GP_ERASER_TYPE_RECTANGLE:
			gdk_draw_rectangle( pixmap, m_priv->cv->gc_bg_pencil, TRUE, 0, 0,
								m_priv->width, m_priv->height );
			gdk_draw_rectangle( pixmap, m_priv->cv->widget->style->black_gc, FALSE, 0, 0,
								m_priv->width - 1, m_priv->height - 1);
			break;
		default:
			printf("Debug: create_eraser_cursor() unknown eraser type: %d\n", type);
			break;
	}

	/* Create pixbuf for cursor */
	pixbuf =  gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, m_priv->width, m_priv->height);
	if(!GDK_IS_PIXBUF(pixbuf))
	{
		printf("Debug: create_eraser_cursor() !GDK_IS_PIXBUF(pixbuf)\n");
		goto CURSOR_CLEANUP;
	}
	
	/* Copy pixmap data to pixbuf */
	gdk_pixbuf_get_from_drawable(pixbuf, pixmap, NULL, 0, 0, 0, 0, m_priv->width, m_priv->height);

	cursor = gdk_cursor_new_from_pixbuf ( gdk_display_get_default (), pixbuf,
										  m_priv->width / 2, m_priv->height / 2 );
	
	CURSOR_CLEANUP: {}

	if(GDK_IS_PIXMAP(pixmap))
	{
		g_object_unref(pixmap);
	}
	if(GDK_IS_PIXBUF(pixbuf))
	{
		g_object_unref(pixbuf);
	}

	return cursor;
}

static void     
save_undo ( void )
{

	gint w,h;
	GdkRectangle rect;
	
	rect.x		=	m_priv->x_min - m_priv->width/2;
	rect.y		=	m_priv->y_min - m_priv->height/2;
	rect.width	=	m_priv->x_max - m_priv->x_min + m_priv->width;
	rect.height	=	m_priv->y_max - m_priv->y_min + m_priv->height;

	if (rect.x<0) rect.x = 0;
	if (rect.y<0) rect.y = 0;
	gdk_drawable_get_size ( m_priv->cv->pixmap, &w, &h );
	if (rect.width>w) rect.width=w;
	if (rect.height>h) rect.height=h;

    undo_add ( &rect, NULL, m_priv->bg_pixmap, TOOL_ERASER);

	
	g_print ("save_undo\n");
	
}

void on_eraser_size_toggled(GtkWidget *widget, gpointer data)
{
	static gint size;
	GdkCursor *cursor;

	if(NULL != data)
	{
		size = *((gint *)data);
		if(m_last_eraser != size)
		{
			if((size >= GP_ERASER_RECT_LARGE) && (size <= GP_ERASER_RECT_TINY))
			{
				set_eraser_values(GP_ERASER_TYPE_RECTANGLE, size);
			}
			else if((size >= GP_ERASER_ROUND_LARGE) && (size <= GP_ERASER_ROUND_TINY))
			{
				set_eraser_values(GP_ERASER_TYPE_ROUND, size);
			}
			else
			{
				return;
			}

			m_last_eraser = *((gint *)data);
	
			cursor = create_eraser_cursor(m_priv->eraser_type);
	
			if(!cursor)
			{
				cursor = gdk_cursor_new ( GDK_CROSSHAIR );
				g_assert(cursor);
			}
			gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
			gdk_cursor_unref( cursor );
		}
		
	}
}

static void set_eraser_values(GPEraserType type, GPEraserSize size)
{
	switch(type)
		{
			case GP_ERASER_TYPE_ROUND:
				m_priv->eraser_type  =   type;
				m_priv->draw_eraser	=	draw_round_eraser;
				m_priv->spacing 	=	2.0;
				set_eraser_size(size);
				break;
			case GP_ERASER_TYPE_RECTANGLE:
				m_priv->eraser_type  =   type;
				m_priv->draw_eraser	=	draw_rectangular_eraser;
				m_priv->spacing 	=	2.0;
				set_eraser_size(size);
				break;
			default:
				printf("Debug: eraser set_eraser_values() unknown eraser type %d\n", m_priv->eraser_type);
				break;
		}
}

static void set_eraser_size(GPEraserSize size)
{
	switch(size)
		{
			case GP_ERASER_RECT_LARGE:	/* Fall through*/
			case GP_ERASER_ROUND_LARGE:
				m_priv->width  = ERASER_WIDTH;
				m_priv->height = ERASER_HEIGHT;
				break;
			case GP_ERASER_RECT_MEDIUM:	/* Fall through*/
			case GP_ERASER_ROUND_MEDIUM:
				m_priv->width  = ERASER_WIDTH / 2;
				m_priv->height = ERASER_HEIGHT / 2;
				break;
			case GP_ERASER_RECT_SMALL:	/* Fall through*/
			case GP_ERASER_ROUND_SMALL:	
				m_priv->width  = ERASER_WIDTH / 3;
				m_priv->height = ERASER_HEIGHT / 3;
				break;
			case GP_ERASER_RECT_TINY:	/* Fall through*/
			case GP_ERASER_ROUND_TINY:
				m_priv->width  = ERASER_WIDTH / 4;
				m_priv->height = ERASER_HEIGHT / 4;
				break;
			default:
				printf("Debug: set_eraser_size() Unknown eraser size: %d\n", size);
				return;
		}
}

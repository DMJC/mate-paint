/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Contributed by Juan Balderas

	This file is part of gnome-paint.

	gnome-paint is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	gnome-paint is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with gnome-paint.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <gtk/gtk.h>
#include <math.h>

#include "cv_paintbrush_tool.h"
#include "file.h"

#define BRUSH_WIDTH		17
#define BRUSH_HEIGHT	17

typedef enum{
	GP_BRUSH_TYPE_ROUND,
	GP_BRUSH_TYPE_RECTANGLE,
	GP_BRUSH_TYPE_FWRD_SLASH,
	GP_BRUSH_TYPE_BACK_SLASH,
	GP_BRUSH_TYPE_PIXBUF
}GPBrushType;

typedef void (DrawBrushFunc)(GdkDrawable *drawable, int x, int y);

static const double EPSILON = 0.00001;

/* Test XPM for pixbuf brush */
const char * happyface_xpm[] = {
"30 30 3 1",
" 	c None",
".	c #000000",
"+	c #FFFF00",
"           ........           ",
"        ...++++++++...        ",
"       ..++++++++++++..       ",
"     ..++++++++++++++++..     ",
"    ..++++++++++++++++++..    ",
"   ..++++++++++++++++++++..   ",
"   .++++++++++++++++++++++.   ",
"  .++++++..++++++++..++++++.  ",
" ..+++++....++++++....+++++.. ",
" .+++++......++++......+++++. ",
" .+++++......++++......+++++. ",
".++++++......++++......++++++.",
".++++++......++++......++++++.",
".+++++++....++++++....+++++++.",
".++++++++..++++++++..++++++++.",
".++++++++++++++++++++++++++++.",
".++++++++++++++++++++++++++++.",
".++++++++++++++++++++++++++++.",
".+++++.++++++++++++++++.+++++.",
" .+++++.++++++++++++++.+++++. ",
" .++++++..++++++++++..++++++. ",
" ..+++++++..++++++..+++++++.. ",
"  .+++++++++......+++++++++.  ",
"   .++++++++++++++++++++++.   ",
"   ..++++++++++++++++++++..   ",
"    ..++++++++++++++++++..    ",
"     ..++++++++++++++++..     ",
"       ..++++++++++++..       ",
"        ...++++++++...        ",
"           ........           "};
static GdkPixbuf *g_pixbuf = NULL;

static void brush_interpolate(GdkDrawable *drawable, DrawBrushFunc *draw_brush_func, int x, int y);
static GdkCursor *create_brush_cursor(GPBrushType type);

/* Private drawing functions */
static void draw_round_brush(GdkDrawable *drawable, int x, int y);
static void draw_rectangular_brush(GdkDrawable *drawable, int x, int y);
static void draw_back_slash_brush(GdkDrawable *drawable, int x, int y);
static void draw_fwd_slash_brush(GdkDrawable *drawable, int x, int y);
static void draw_pixbuf_brush(GdkDrawable *drawable, int x, int y);

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
	guint			button;
	gboolean 		is_draw;
	double			spacing;        
    double			distance;
    GdkPoint		drag;
    DrawBrushFunc	*draw_brush;
    GPBrushType		brush_type;
    gint			width, height; /* Brush width & height */
} private_data;

static private_data		*m_priv = NULL;

static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_new0 (private_data,1);
		m_priv->cv			=	NULL;
		m_priv->gc			=	NULL;
		m_priv->button		=	0;
		m_priv->is_draw		=	FALSE;
		m_priv->distance	=	0;
		m_priv->brush_type	=	GP_BRUSH_TYPE_ROUND; /* Rogerio: change type here to test other brushes */
		m_priv->width		=	BRUSH_WIDTH;
		m_priv->height		=	BRUSH_HEIGHT;
		
		switch(m_priv->brush_type)
		{
			case GP_BRUSH_TYPE_ROUND:
				m_priv->draw_brush	=	draw_round_brush;
				m_priv->spacing 	=	2.0;
				break;
			case GP_BRUSH_TYPE_RECTANGLE:
				m_priv->draw_brush	=	draw_rectangular_brush;
				m_priv->spacing 	=	2.0;
				break;
			case GP_BRUSH_TYPE_FWRD_SLASH:
				m_priv->draw_brush	=	draw_fwd_slash_brush;
				m_priv->spacing 	=	1.0;
				break;
			case GP_BRUSH_TYPE_BACK_SLASH:
				m_priv->draw_brush	=	draw_back_slash_brush;
				m_priv->spacing 	=	1.0;
				break;
			case GP_BRUSH_TYPE_PIXBUF:
				g_pixbuf = gdk_pixbuf_new_from_xpm_data(happyface_xpm);
				m_priv->draw_brush	=	draw_pixbuf_brush;
				m_priv->spacing 	=	gdk_pixbuf_get_width(g_pixbuf);
				m_priv->width		=	gdk_pixbuf_get_width(g_pixbuf);
				m_priv->height		=	gdk_pixbuf_get_height(g_pixbuf);
				break;
			default:
				printf("Debug: brush create_private_data unknown brush type %d\n", m_priv->brush_type);
				break;
		}
	}
}

static void
destroy_private_data( void )
{
	g_free (m_priv);
	m_priv = NULL;
}


gp_tool * 
tool_paintbrush_init ( gp_canvas * canvas )
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
			m_priv->gc = m_priv->cv->gc_fg_pencil;
		}
		else if ( event->button == RIGHT_BUTTON )
		{
			m_priv->gc = m_priv->cv->gc_bg_pencil;
		}
		m_priv->is_draw = !m_priv->is_draw;
		if( m_priv->is_draw )
		{
			m_priv->button = event->button;
		}
		m_priv->drag.x = (gint)event->x;
		m_priv->drag.y = (gint)event->y;
		m_priv->x0 = m_priv->drag.x;
		m_priv->y0 = m_priv->drag.y;
		
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

	if( m_priv->is_draw )
	{
		
		if (event->is_hint)
		{
			gdk_window_get_pointer (event->window, &x, &y, &state);
		}
		else
		{
			x = event->x;
			y = event->y;
			state = event->state;
		}
		
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
	
	cursor = create_brush_cursor(m_priv->brush_type);
	
	if(!cursor)
	{
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
	g_print("paintbrush tool destroy\n");
}

static void
draw_in_pixmap ( GdkDrawable *drawable )
{
	brush_interpolate(drawable, m_priv->draw_brush, m_priv->x0, m_priv->y0);
	m_priv->drag.x = m_priv->x0;
	m_priv->drag.y = m_priv->y0;
}

/* Create a new cursor after fg color change */
void notify_brush_of_fg_color_change(void)
{
	GdkCursor *cursor;

    if ( m_priv == NULL ) return;
	
	/* No need to change cursor if pixbuf */
	if(GP_BRUSH_TYPE_PIXBUF == m_priv->brush_type)
	{
		return;
	}
	
	cursor = create_brush_cursor(m_priv->brush_type);
	
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
brush_interpolate(GdkDrawable *drawable, DrawBrushFunc *draw_brush_func, int x, int y)
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

			draw_brush_func(drawable, x, y);
		}
	 }
	 m_priv->distance = final;
}

/* Rogerio: m_priv->spacing = 2.0 is best for this brush */
static void draw_round_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_arc( drawable, m_priv->gc, TRUE, x, y, m_priv->width, m_priv->height, 0, 360 * 64);
}

/* Rogerio: m_priv->spacing = 2.0 is best for this brush */
static void draw_rectangular_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_rectangle( drawable, m_priv->gc, TRUE, x, y, m_priv->width, m_priv->height);
}

/* Rogerio: m_priv->spacing = 1.0 is best for this brush */
static void draw_back_slash_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_line(drawable, m_priv->gc, x, y, x + m_priv->width, y + m_priv->height);
	/* Draw another line to fill in gaps */
	gdk_draw_line(drawable, m_priv->gc, x + 1, y, x + 1 + m_priv->width, y + m_priv->height);
}

/* Rogerio: m_priv->spacing = 1.0 is best for this brush */
static void draw_fwd_slash_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);
	
	gdk_draw_line(drawable, m_priv->gc, x , y + m_priv->height, x + m_priv->width, y);
	/* Draw another line to fill in gaps */
	gdk_draw_line(drawable, m_priv->gc, x + 1 , y + m_priv->height, x + m_priv->width, y + 1);
}

/* Rogerio: m_priv->spacing = gdk_get_pixbuf_width() and you get nice tiling effect */
static void draw_pixbuf_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_pixbuf(drawable, m_priv->gc, g_pixbuf, 0, 0, x, y,
					-1, -1, GDK_RGB_DITHER_NONE, 0, 0);
}

static GdkCursor *create_brush_cursor(GPBrushType type)
{
	GdkCursor *cursor = NULL;
	GdkPixmap *pixmap;
	GdkPixbuf *pixbuf, *tmp;

	pixmap = gdk_pixmap_new(m_priv->cv->widget->window, m_priv->width, m_priv->height, -1);
	if(!GDK_IS_PIXMAP(pixmap))
	{
		printf("Debug: create_brush_cursor() !GDK_IS_PIXMAP(pixmap)\n");
		goto CURSOR_CLEANUP;
	}

	/* Fill with background with white */
	gdk_draw_rectangle(pixmap, m_priv->cv->widget->style->white_gc,
					   TRUE, 0, 0, m_priv->width, m_priv->height);
	
	/* Draw crosshair onto pixmap */
	/*gdk_draw_line(pixmap, m_priv->cv->gc_fg_pencil, 0 , m_priv->height / 2, m_priv->width, m_priv->height / 2);
	gdk_draw_line(pixmap, m_priv->cv->gc_fg_pencil, m_priv->width / 2, 0, m_priv->width / 2, m_priv->height);
	*/

	/* Draw brush onto pixmap with fore color */
	switch(type)
	{
		case GP_BRUSH_TYPE_ROUND:
			gdk_draw_arc( pixmap, m_priv->cv->gc_fg_pencil, TRUE, 0, 0,
						  m_priv->width, m_priv->height, 0, 360 * 64 );
			break;
		case GP_BRUSH_TYPE_RECTANGLE:
			gdk_draw_rectangle( pixmap, m_priv->cv->gc_fg_pencil, TRUE, 0, 0,
								m_priv->width, m_priv->height );
			break;
		case GP_BRUSH_TYPE_FWRD_SLASH:
			gdk_draw_line( pixmap, m_priv->cv->gc_fg_pencil, 0 ,
						   m_priv->height, m_priv->width, 0 );
			break;
		case GP_BRUSH_TYPE_BACK_SLASH:
			gdk_draw_line( pixmap, m_priv->cv->gc_fg_pencil, 0, 0,
							m_priv->width, m_priv->height );
			break;
		case GP_BRUSH_TYPE_PIXBUF:
			gdk_draw_pixbuf( pixmap, m_priv->cv->gc_fg_pencil, g_pixbuf,
							 0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NONE, 0, 0 );
			break;
		default:
			printf("Debug: create_brush_cursor() unknown brush type: %d\n", type);
			break;
	}
	
	/* Create pixbuf without aplha, will add alpha in next steps */
	pixbuf =  gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, m_priv->width, m_priv->height);
	if(!GDK_IS_PIXBUF(pixbuf))
	{
		printf("Debug: create_brush_cursor() !GDK_IS_PIXBUF(pixbuf)\n");
		goto CURSOR_CLEANUP;
	}
	
	/* Copy pixmap data to pixbuf */
	gdk_pixbuf_get_from_drawable(pixbuf, pixmap, NULL, 0, 0, 0, 0, m_priv->width, m_priv->height);
	
	/* Make background pixels transparent. */
	tmp = gdk_pixbuf_add_alpha(pixbuf, TRUE, 0xFF, 0xFF, 0xFF);
	if(!GDK_IS_PIXBUF(tmp))
	{
		printf("Debug: create_brush_cursor() !GDK_IS_PIXBUF(tmp)\n");
		goto CURSOR_CLEANUP;
	}

	g_object_unref(pixbuf);
	pixbuf = tmp;
	
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










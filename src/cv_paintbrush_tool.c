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

#include "cv_paintbrush_tool.h"
#include "file.h"
#include "gp-image.h"
#include "toolbar.h"
#include "undo.h"

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

/* TODO: These should be part of gp_canvas */
static gint g_prev_brush_size = GP_BRUSH_ROUND_LARGE;
static gint g_brush_type = GP_BRUSH_TYPE_ROUND;

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
static void set_brush_values(GPBrushType type, GPBrushSize size);
static void set_brush_size(GPBrushSize size);
static GdkGC *color_for_alphaing(GtkWidget *widget, GdkGC *fg, GdkGC *bg);
static void brush_set_pixel(GdkPixbuf *pixbuf, gint color, gint x, gint y);
static void draw_crosshair(GdkPixbuf *pixbuf, GPBrushType type);
static void copy_drawable_with_cairo(GdkDrawable *dst, GdkDrawable *src, gint width, gint height);

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
	gint			x_min,y_min,x_max,y_max;
	guint			button;
	gboolean 		is_draw;
	double			spacing;        
    double			distance;
    GdkPoint		drag;
    DrawBrushFunc	*draw_brush;
    GPBrushType		brush_type;
    gint			width, height; /* Brush width & height */
    GdkPixmap *		bg_pixmap;
} private_data;

static private_data		*m_priv = NULL;

static void
copy_drawable_with_cairo ( GdkDrawable *dst, GdkDrawable *src, gint width, gint height )
{
	cairo_t *cr;

	cr = gdk_cairo_create (dst);
	gdk_cairo_set_source_pixmap (cr, GDK_PIXMAP (src), 0, 0);
	cairo_rectangle (cr, 0, 0, width, height);
	cairo_fill (cr);
	cairo_destroy (cr);
}

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
	copy_drawable_with_cairo ( GDK_DRAWABLE (m_priv->bg_pixmap), m_priv->cv->pixmap, w, h );
}

static void
restore_background ( void )
{
    if ( m_priv->bg_pixmap != NULL )
    {
		gint w, h;

		gdk_drawable_get_size ( GDK_DRAWABLE (m_priv->bg_pixmap), &w, &h );
	    copy_drawable_with_cairo ( m_priv->cv->pixmap, GDK_DRAWABLE (m_priv->bg_pixmap), w, h );
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
		m_priv->brush_type	=	g_brush_type; /* change type here to test other brushes */
		m_priv->width		=	BRUSH_WIDTH;
		m_priv->height		=	BRUSH_HEIGHT;
        m_priv->bg_pixmap   =   NULL;

		set_brush_values(m_priv->brush_type, g_prev_brush_size);
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
		cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
		g_assert(cursor);
	}
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	g_object_unref ( cursor );

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
		cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
		g_assert(cursor);
	}
	gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
	g_object_unref ( cursor );
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

		    if (m_priv->x_min>x)m_priv->x_min=x;
		    if (m_priv->x_max<x)m_priv->x_max=x;
		    if (m_priv->y_min>y)m_priv->y_min=y;
		    if (m_priv->y_max<y)m_priv->y_max=y;
            
			draw_brush_func(drawable, x, y);
		}
	 }
	 m_priv->distance = final;
}

/* m_priv->spacing = 2.0 is best for this brush */
static void draw_round_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_arc( drawable, m_priv->gc, TRUE, x, y, m_priv->width, m_priv->height, 0, 360 * 64);
}

/* m_priv->spacing = 2.0 is best for this brush */
static void draw_rectangular_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_rectangle( drawable, m_priv->gc, TRUE, x, y, m_priv->width, m_priv->height);
}

/* m_priv->spacing = 1.0 is best for this brush */
static void draw_back_slash_brush(GdkDrawable *drawable, int x, int y)
{
    gint w, h;
   
    w = m_priv->width - 1;
    h = m_priv->height - 1;
   
    x -= (m_priv->width / 2);
    y -= (m_priv->height / 2);

    gdk_draw_line(drawable, m_priv->gc, x, y, x + w, y + h);
    /* Draw another line to fill in gaps */
    gdk_draw_line(drawable, m_priv->gc, x + 1, y, x + w, y + h - 1);
}

/* m_priv->spacing = 1.0 is best for this brush */
static void draw_fwd_slash_brush(GdkDrawable *drawable, int x, int y)
{
    gint w, h;
   
    w = m_priv->width - 1;
    h = m_priv->height - 1;
   
    x -= (m_priv->width / 2);
    y -= (m_priv->height / 2);
   
    gdk_draw_line(drawable, m_priv->gc, x , y + h, x + w, y);
    /* Draw another line to fill in gaps */
    gdk_draw_line(drawable, m_priv->gc, x + 1 , y + h, x + w, y + 1);
}

/* m_priv->spacing = gdk_get_pixbuf_width() and you get nice tiling effect */
static void draw_pixbuf_brush(GdkDrawable *drawable, int x, int y)
{
	x -= (m_priv->width / 2);
	y -= (m_priv->height / 2);

	gdk_draw_pixbuf(drawable, m_priv->gc, g_pixbuf, 0, 0, x, y,
					-1, -1, GDK_RGB_DITHER_NONE, 0, 0);
}

/* TODO: Finish up GP_BRUSH_TYPE_PIXBUF cursor */
#define MIN_CURSOR_WIDTH	19
#define MIN_CURSOR_HEIGHT	MIN_CURSOR_WIDTH
#define CENTERED(a,b)	(((a)/(2))-((b)/(2)))
static GdkCursor *create_brush_cursor(GPBrushType type)
{
	GdkCursor *cursor = NULL;
	GdkPixmap *pixmap;
	GdkPixbuf *pixbuf, *tmp;
	GdkGCValues bgvalues;
	GdkColor color;
	GdkGC *gc;
	/* No cursor is less than 19 pixels ie. w & h of crosshair */
	gint wcur = MIN_CURSOR_WIDTH;
	gint hcur = MIN_CURSOR_HEIGHT;

	if(m_priv->width > wcur){
		wcur = m_priv->width;
	}

	if(m_priv->height > hcur){
		hcur = m_priv->height;
	}

	pixmap = gdk_pixmap_new(gtk_widget_get_window(m_priv->cv->widget), wcur, hcur, -1);
	if(!GDK_IS_PIXMAP(pixmap))
	{
		printf("Debug: create_brush_cursor() !GDK_IS_PIXMAP(pixmap)\n");
		goto CURSOR_CLEANUP;
	}

	/* Fill with background color. See if fg & bg are different */
	gc = color_for_alphaing(m_priv->cv->widget, m_priv->cv->gc_fg_pencil, m_priv->cv->gc_bg_pencil);
	if(GDK_IS_GC(gc)){
		gdk_draw_rectangle(pixmap, gc, TRUE, 0, 0, wcur, hcur);
		gdk_gc_get_values(gc, &bgvalues);
		gdk_gc_unref(gc);
	}
	else{
		gdk_draw_rectangle(pixmap, m_priv->cv->gc_bg_pencil, TRUE, 0, 0, wcur, hcur);
		gdk_gc_get_values(m_priv->cv->gc_bg_pencil, &bgvalues);
		color = bgvalues.foreground;
	}

	/* Draw brush shape centered onto pixmap with fore color */
	switch(type)
	{
		case GP_BRUSH_TYPE_ROUND:
			gdk_draw_arc( pixmap, m_priv->cv->gc_fg_pencil, TRUE, CENTERED(wcur, m_priv->width),
						    CENTERED(hcur, m_priv->height),
						  m_priv->width - 1, m_priv->height - 1, 0, 360 * 64 );
			/* Draw twice because gdk has a bug where circles don't look round! */
			gdk_draw_arc( pixmap, m_priv->cv->gc_fg_pencil, FALSE, CENTERED(wcur, m_priv->width),
						    CENTERED(hcur, m_priv->height),
						  m_priv->width - 1, m_priv->height - 1, 0, 360 * 64 );
			break;
		case GP_BRUSH_TYPE_RECTANGLE:
			gdk_draw_rectangle( pixmap, m_priv->cv->gc_fg_pencil, TRUE, CENTERED(wcur, m_priv->width),
						    CENTERED(hcur, m_priv->height),
								m_priv->width, m_priv->height);
			break;
		case GP_BRUSH_TYPE_FWRD_SLASH:
			gdk_draw_line( pixmap, m_priv->cv->gc_fg_pencil, wcur / 2 +  m_priv->width / 2,
						   CENTERED(hcur, m_priv->height), CENTERED(wcur, m_priv->width),
						   hcur / 2 +  m_priv->height / 2);
			break;
		case GP_BRUSH_TYPE_BACK_SLASH:
			gdk_draw_line( pixmap, m_priv->cv->gc_fg_pencil, CENTERED(wcur, m_priv->width),
						    CENTERED(hcur, m_priv->height),
							wcur / 2 +  m_priv->width / 2, 
							hcur / 2 +  m_priv->height / 2 );
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
	pixbuf =  gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, wcur, hcur);
	if(!GDK_IS_PIXBUF(pixbuf))
	{
		printf("Debug: create_brush_cursor() !GDK_IS_PIXBUF(pixbuf)\n");
		goto CURSOR_CLEANUP;
	}
	
	/* Copy pixmap data to pixbuf */
	gdk_pixbuf_get_from_drawable(pixbuf, pixmap, NULL, 0, 0, 0, 0, wcur, hcur);
	
	color = bgvalues.foreground;
	
	bgvalues.foreground.red /= 256;
	bgvalues.foreground.green /= 256;
	bgvalues.foreground.blue /= 256;
	
	
	/* Make background pixels transparent. */
	tmp = gdk_pixbuf_add_alpha(pixbuf, TRUE, bgvalues.foreground.red,
											 bgvalues.foreground.green,
											 bgvalues.foreground.blue);
	
	if(!GDK_IS_PIXBUF(tmp))
	{
		printf("Debug: create_brush_cursor() !GDK_IS_PIXBUF(tmp)\n");
		goto CURSOR_CLEANUP;
	}
	
	draw_crosshair(tmp, type);

	g_object_unref(pixbuf);
	pixbuf = tmp;
	
	cursor = gdk_cursor_new_from_pixbuf ( gdk_display_get_default (), pixbuf,
										  wcur / 2, hcur / 2 );
	
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

    undo_add ( &rect, NULL, m_priv->bg_pixmap, TOOL_PAINTBRUSH);

	
	g_print ("save_undo\n");
	
}

void on_brush_size_toggled(GtkWidget *widget, gpointer data)
{
	static gint size;
	GdkCursor *cursor;
 
	if(NULL != data)
	{
		size = *((gint *)data);
		if(g_prev_brush_size != size)
		{
			if((size >= GP_BRUSH_RECT_LARGE) && (size <= GP_BRUSH_RECT_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_ROUND, size);
			}
			else if((size >= GP_BRUSH_ROUND_LARGE) && (size <= GP_BRUSH_ROUND_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_RECTANGLE, size);
			}
			else if((size >= GP_BRUSH_FWRD_LARGE) && (size <= GP_BRUSH_FWRD_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_FWRD_SLASH, size);
			}
			else if((size >= GP_BRUSH_BACK_LARGE) && (size <= GP_BRUSH_BACK_SMALL))
			{
				set_brush_values(GP_BRUSH_TYPE_BACK_SLASH, size);
			}
			else
			{
				return;
			}
 
			g_prev_brush_size = *((gint *)data);
	
			cursor = create_brush_cursor(m_priv->brush_type);
	
			if(!cursor)
			{
				cursor = gdk_cursor_new_for_display (gdk_display_get_default (), GDK_CROSSHAIR);
				g_assert(cursor);
			}
			gdk_window_set_cursor ( m_priv->cv->drawing, cursor );
			g_object_unref( cursor );
		}
		
	}
}
 
static void set_brush_values(GPBrushType type, GPBrushSize size)
{
	switch(type)
		{
			case GP_BRUSH_TYPE_ROUND:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_round_brush;
				m_priv->spacing 	=	2.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_RECTANGLE:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_rectangular_brush;
				m_priv->spacing 	=	2.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_FWRD_SLASH:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_fwd_slash_brush;
				m_priv->spacing 	=	1.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_BACK_SLASH:
				m_priv->brush_type  =   type;
				m_priv->draw_brush	=	draw_back_slash_brush;
				m_priv->spacing 	=	1.0;
				set_brush_size(size);
				break;
			case GP_BRUSH_TYPE_PIXBUF:
				m_priv->brush_type  =   type;
				g_pixbuf = gdk_pixbuf_new_from_xpm_data(happyface_xpm);
				m_priv->draw_brush	=	draw_pixbuf_brush;
				m_priv->spacing 	=	gdk_pixbuf_get_width(g_pixbuf);
				m_priv->width		=	gdk_pixbuf_get_width(g_pixbuf);
				m_priv->height		=	gdk_pixbuf_get_height(g_pixbuf);
				break;
			default:
				printf("Debug: brush set_brush_values() unknown brush type %d\n", m_priv->brush_type);
				break;
		}

	g_brush_type = type;
}

static void set_brush_size(GPBrushSize size)
{
	switch(size)
		{
			case GP_BRUSH_RECT_LARGE:	/* Fall through*/
			case GP_BRUSH_ROUND_LARGE:	/* Fall through*/
			case GP_BRUSH_FWRD_LARGE:	/* Fall through*/
			case GP_BRUSH_BACK_LARGE:
				m_priv->width  = BRUSH_WIDTH;
				m_priv->height = BRUSH_HEIGHT;
				break;
			case GP_BRUSH_RECT_MEDIUM:	/* Fall through*/
			case GP_BRUSH_ROUND_MEDIUM:	/* Fall through*/
			case GP_BRUSH_FWRD_MEDIUM:	/* Fall through*/
			case GP_BRUSH_BACK_MEDIUM:
				m_priv->width = BRUSH_WIDTH / 2;
				m_priv->height = BRUSH_WIDTH / 2;
				break;
			case GP_BRUSH_RECT_SMALL:	/* Fall through*/
			case GP_BRUSH_ROUND_SMALL:	/* Fall through*/
			case GP_BRUSH_FWRD_SMALL:	/* Fall through*/
			case GP_BRUSH_BACK_SMALL:	/* Fall through*/
				m_priv->width = BRUSH_WIDTH / 4;
				m_priv->height = BRUSH_WIDTH / 4;
				break;
			default:
				return;
		}
		
	/* Favor odd sized brush sizes. They look better */
	if(0 == m_priv->width % 2){
		m_priv->width++;
	}
	if(0 == m_priv->height % 2){
		m_priv->height++;
	}
}

/* Create a gc if the current fore and back colors are the same
 */
static GdkGC *color_for_alphaing(GtkWidget *widget, GdkGC *fg, GdkGC *bg)
{
	GdkGC *gc = NULL;
	GdkGCValues fgvalues, bgvalues;
	gint i;

	gdk_gc_get_values(fg, &fgvalues);
	gdk_gc_get_values(bg, &bgvalues);

	/* Create a gc if the colors are the same */
	if(fgvalues.foreground.pixel == bgvalues.background.pixel)
	{
		/* Create a color different from the fore color */
		bgvalues.background.red   = 0;
		bgvalues.background.green = 0;
		bgvalues.background.blue  = 0;

		for(i = 0; i < 255; i++)
		{
			bgvalues.background.pixel = i;
			bgvalues.background.red   = i * 256;
			
			/* Create the gc and set the new color */
			if(fgvalues.foreground.pixel != bgvalues.background.pixel)
			{
				gc = gdk_gc_new(gtk_widget_get_window(widget));
				gdk_gc_set_rgb_bg_color(gc, &bgvalues.background);
				gdk_gc_set_rgb_fg_color(gc, &bgvalues.background);

				gdk_gc_get_values(gc, &bgvalues);

				return gc;
			}
		}
	}
	
	/* Return NULL gc if not */
	return gc;
}

/* This func should go in another module */
static void draw_crosshair(GdkPixbuf *pixbuf, GPBrushType type)
{
	int width, height, rowstride, n_channels;
	guint color = 0x00000000;

	/* Ignore pixbuf brushes */
	if(GP_BRUSH_TYPE_PIXBUF == type){ return; }
	
	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
	g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
	g_assert (gdk_pixbuf_get_has_alpha (pixbuf));
	g_assert (n_channels == 4);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	{
		int j, i, npix = 19;
		int center; /* Center of pixbuf in both x & y */
		
		center = width / 2;
		color = 0;

		for(i = center - npix / 2, j = npix - 1; i < 8; i++, j--){
			brush_set_pixel(pixbuf, color, center, i);
			brush_set_pixel(pixbuf, color, center, j);
			brush_set_pixel(pixbuf, color, i, center);
			brush_set_pixel(pixbuf, color, j, center);
			color = ~color;
		}
	}
}

/* This func should go in another module */
#define getr(x) ((((x)>>(16))&(0x0FF)))
#define getg(x) ((((x)>>(8))&(0x0FF)))
#define getb(x) (((x)&(0x0FF)))
static void brush_set_pixel(GdkPixbuf *pixbuf, gint color, gint x, gint y)
{
	guchar *pixels, *p;
	int width, height, rowstride, n_channels;

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	
	if((x >= 0) && (x <= width))
	{
		if((y >= 0) && (y <= height))
		{
			p = pixels + (y * rowstride + x * n_channels);
			p[0] = getr(color);/*red*/
			p[1] = getg(color);/*green*/
			p[2] = getb(color);/*blue*/
			p[3] = 0xFF;
		}
	}	
}

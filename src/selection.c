/***************************************************************************
 *            selection.c
 *
 *  Sat Jun 19 16:12:19 2010
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

#include "selection.h"
#include "cv_drawing.h"
#include "gp-image.h"
#include "undo.h"

typedef enum {
    SEL_TOP_LEFT,
    SEL_TOP_MID,
    SEL_TOP_RIGHT,
    SEL_MID_RIGHT,
    SEL_BOTTOM_RIGHT,
    SEL_BOTTOM_MID,
    SEL_BOTTOM_LEFT,
    SEL_MID_LEFT,
    SEL_CLIPBOX,
    SEL_NONE
} GpSelBoxEnum;

typedef struct {
    GdkPoint p0;
    GdkPoint p1;
} GpSelBox;

typedef struct {
    GpSelBoxEnum    action;
    GdkPoint        p_drag;
    GdkPoint        sp; /*Start Point*/
    GdkPoint        ep; /*End Poind*/   
    GpSelBox        boxes[SEL_NONE];
    GpImage         *image;
    gint            active : 1;
    gint            show_borders : 1;
    gint            floating : 1;
    gboolean		transparent;
    GdkPixbuf		*pb_clipboard;
} PrivData;

static gboolean gp_selection_get_bg_color_rgb(guchar *r, guchar *g, guchar *b);

/* private data */
static PrivData *m_priv = NULL;

static void
create_private_data( void )
{
	if (m_priv == NULL)
	{
		m_priv = g_slice_new0 (PrivData);
        m_priv->image = NULL;
        g_print ("init\n");
	}
}

static void
destroy_image ( void )
{
    if ( m_priv->image != NULL )
    {
        g_object_unref ( m_priv->image );
        m_priv->image = NULL;
    }
}

static void
destroy_private_data( void )
{
    if ( m_priv != NULL )
    {
        destroy_image ();
	    g_slice_free (PrivData, m_priv);
	    m_priv = NULL;
        g_print ("clear\n");
    }
}

static void
update_clipbox ( void )
{
    GdkPoint *sp        = &m_priv->sp;
    GdkPoint *ep        = &m_priv->ep;
    GpSelBox *clipbox   = &m_priv->boxes[SEL_CLIPBOX];
    
    if ( ep->x > sp->x)
    {
        clipbox->p0.x = sp->x;
        clipbox->p1.x = ep->x;
    }
    else
    {
        clipbox->p1.x = sp->x;
        clipbox->p0.x = ep->x;
    }

    if ( ep->y > sp->y)
    {
        clipbox->p0.y = sp->y;
        clipbox->p1.y = ep->y;
    }
    else
    {
        clipbox->p1.y = sp->y;
        clipbox->p0.y = ep->y;
    }
}

static void 
set_sel_box ( GpSelBox *box, gint x0, gint y0, gint x1, gint y1 )
{
    box->p0.x = x0;
    box->p0.y = y0;
    box->p1.x = x1;
    box->p1.y = y1;
}

static void 
update_borders ( void )
{
    
    if ( m_priv->show_borders )
    {
        GdkPoint *sp        = &m_priv->sp;
        GdkPoint *ep        = &m_priv->ep;
        GpSelBox *clipbox   = &m_priv->boxes[SEL_CLIPBOX];
        const gint s = 4;
        gint xl,yt,xr,yb,xm,ym;

        sp->x = clipbox->p0.x;
        sp->y = clipbox->p0.y;
        ep->x = clipbox->p1.x;
        ep->y = clipbox->p1.y;
        update_clipbox ();

        xl = clipbox->p0.x;
        yt = clipbox->p0.y;
        xr = clipbox->p1.x;
        yb = clipbox->p1.y;
        xm = (xr + xl)/2;
        ym = (yb + yt)/2;
        
        set_sel_box ( &m_priv->boxes[SEL_TOP_LEFT],
                       xl,yt,xl+s,yt+s);
        set_sel_box ( &m_priv->boxes[SEL_TOP_MID],
                       xm-s/2,yt,xm+s/2,yt+s);
        set_sel_box ( &m_priv->boxes[SEL_TOP_RIGHT],
                       xr-s,yt,xr,yt+s);
        set_sel_box ( &m_priv->boxes[SEL_MID_LEFT],
                       xl,ym-s/2,xl+s,ym+s/2);
        set_sel_box ( &m_priv->boxes[SEL_MID_RIGHT],
                       xr-s,ym-s/2,xr,ym+s/2);
        set_sel_box ( &m_priv->boxes[SEL_BOTTOM_LEFT],
                       xl,yb-s,xl+s,yb);
        set_sel_box ( &m_priv->boxes[SEL_BOTTOM_MID],
                       xm-s/2,yb-s,xm+s/2,yb);
        set_sel_box ( &m_priv->boxes[SEL_BOTTOM_RIGHT],
                       xr-s,yb-s,xr,yb);
    }
}

static gboolean 
point_in ( GdkPoint *point, GpSelBox *box)
{
    if (    point->x >= box->p0.x && 
            point->x <= box->p1.x && 
            point->y >= box->p0.y && 
            point->y <= box->p1.y )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static GpSelBoxEnum
get_box_in ( GdkPoint *point )
{
    GpSelBoxEnum box;
    for ( box = SEL_TOP_LEFT; box < SEL_NONE; box++ )
    {
        if ( point_in ( point, &m_priv->boxes[box] ) )
            break;
    }
    return box;
}


void
gp_selection_init ( void )
{
    create_private_data ();
}

void
gp_selection_clear ( void )
{
    destroy_private_data ();
}

void
gp_selection_clipbox_set_start_point ( GdkPoint *p )
{
    g_return_if_fail ( m_priv != NULL );
    m_priv->sp.x = p->x;
    m_priv->sp.y = p->y;
    update_clipbox ();
}

void
gp_selection_clipbox_set_end_point ( GdkPoint *p )
{
    g_return_if_fail ( m_priv != NULL );
    m_priv->ep.x = p->x;
    m_priv->ep.y = p->y;
    update_clipbox ();
}

void
gp_selection_set_active ( gboolean active )
{
    g_return_if_fail ( m_priv != NULL );
    m_priv->active = active;
}

void
gp_selection_set_borders ( gboolean borders )
{
    g_return_if_fail ( m_priv != NULL );
    m_priv->show_borders = borders;
    update_borders ();
}

/* If pixbuf is NULL, a selection is created from the two points
 * 's' is the top left point of a rectangle and 'e' is the bottom
 * right. If pixbuf is not NULL a selection is created from the pixbuf
 * and 's' is the x,y position to place the selection.
 */
gboolean gp_selection_create (GdkPoint *s, GdkPoint *e, GdkPixbuf *pixbuf)
{
	GdkPoint tmp = {0, 0};
	gp_canvas *cv;
	GdkPoint S, E;
	gint w, h;

	if((NULL == s) || (NULL == e)){
		return FALSE;
	}
	if(NULL == m_priv){
		return FALSE;
	}
	
	S = *s ; E = *e ;
	
	if(GDK_IS_PIXBUF(pixbuf)){
		m_priv->pb_clipboard = gdk_pixbuf_copy(pixbuf);
		w = gdk_pixbuf_get_width(m_priv->pb_clipboard);
		h = gdk_pixbuf_get_height(m_priv->pb_clipboard);
		if(ABS(S.x - E.x) != w){
			E.x = S.x + w;
		}
		if(ABS(S.y - E.y) != h){
			E.y = S.y + h;
		}
		/* Allow for selection border */
		E.x-- ; E.y-- ;
	}
	
	gp_selection_set_floating ( TRUE );
	gp_selection_set_active ( FALSE );
	gp_selection_clipbox_set_start_point ( &S );
	gp_selection_clipbox_set_end_point ( &E );
	gp_selection_set_active ( TRUE );

	tmp.x = ABS(S.x - E.x);
	tmp.y = ABS(S.y - E.y);

	gp_selection_start_action ( &tmp );

	gp_selection_set_floating ( FALSE );

	gp_selection_set_borders ( TRUE );
	cv = cv_get_canvas ();
    gtk_widget_queue_draw ( cv->widget );

	return TRUE;
}

/* button press ---+----gp_selection_set_active ( FALSE );
 *					`+--gp_selection_set_active ( TRUE );
 * double click -> gp_selection_set_floating ( FALSE );
 */
void
gp_selection_set_floating ( gboolean floating )
{
    g_return_if_fail ( m_priv != NULL );
    m_priv->floating = floating;
    if ( m_priv->active )
    {
        GpSelBox *clipbox   = &m_priv->boxes[SEL_CLIPBOX];
        gp_canvas *cv;
        GdkRectangle rect;
        cv = cv_get_canvas ();
        rect.x      = MIN(clipbox->p0.x,clipbox->p1.x);
        rect.y      = MIN(clipbox->p0.y,clipbox->p1.y);
        rect.width  = ABS(clipbox->p1.x - clipbox->p0.x)+1;
        rect.height = ABS(clipbox->p1.y - clipbox->p0.y)+1;
        if ( m_priv->floating )
        {
            printf("gp_selection_set_floating() TRUE\n");
            if ( m_priv->image != NULL )
            {
                gp_image_draw ( m_priv->image,
                                cv->pixmap,
                                cv->gc_fg,
                                rect.x, rect.y,
                                rect.width, rect.height );            
                destroy_image ();
            }
        }
        else
        {
            printf("gp_selection_set_floating() FALSE\n");
            destroy_image ();
            /* Create selection from a pixbuf */
            if(GDK_IS_PIXBUF(m_priv->pb_clipboard))
            {
            	GdkPixmap *pm;
            	gint w, h;

            	w = gdk_pixbuf_get_width(m_priv->pb_clipboard);
				h = gdk_pixbuf_get_height(m_priv->pb_clipboard);
            	
                pm = gdk_pixmap_new (gtk_widget_get_window(cv->widget), w, h, -1);
            	gdk_draw_pixbuf (pm, cv->gc_fg, m_priv->pb_clipboard, 0, 0,
								 0, 0, w, h, GDK_RGB_DITHER_NONE, 0, 0);
				
				m_priv->image = gp_image_new_from_pixmap ( pm, &rect, TRUE );
            	g_object_unref(pm);
            	g_object_unref(m_priv->pb_clipboard);
            	m_priv->pb_clipboard = NULL;
            }
            /* Create selection */
            else
            {
            	GdkRectangle undo_area = {0, 0, 0, 0};

            	/* Save all pixmap so redraw where selection was copied from
            	 * and where sel will be pasted - which we don't know where
            	 * that will be. */
            	gdk_drawable_get_size (cv->pixmap, &undo_area.width, &undo_area.height);
            	undo_add ( &undo_area, NULL, NULL, TOOL_RECT_SELECT);

            	m_priv->image = gp_image_new_from_pixmap ( cv->pixmap, &rect, TRUE );
            	
            	printf("   x: %d, y: %d, w: %d, h: %d\n", rect.x, rect.y,
            											  rect.width, rect.height);

            	gdk_draw_rectangle ( cv->pixmap, cv->gc_bg, TRUE, 
                                     rect.x, rect.y, rect.width, rect.height ); 
            }       
        	/* Add transparancy if necessary */
        	if(cv->transparent)
        	{
            	guchar r, g, b;

            	m_priv->transparent = cv->transparent;
            	gp_selection_get_bg_color_rgb(&r, &g, &b);
            	gp_image_make_color_transparent( m_priv->image, r, g, b, 0);
            	
            }
        }
    }
}

GdkCursorType
gp_selection_get_cursor ( GdkPoint *p )
{
    g_return_val_if_fail ( m_priv != NULL, GDK_BLANK_CURSOR);
    switch ( get_box_in ( p ) )
    {
        case SEL_TOP_LEFT:
            return GDK_TOP_LEFT_CORNER;
        case SEL_TOP_MID:
            return GDK_TOP_SIDE;
        case SEL_TOP_RIGHT:
            return GDK_TOP_RIGHT_CORNER;
        case SEL_MID_LEFT:
            return GDK_LEFT_SIDE;
        case SEL_MID_RIGHT:
            return GDK_RIGHT_SIDE;
        case SEL_BOTTOM_LEFT:
            return GDK_BOTTOM_LEFT_CORNER;
        case SEL_BOTTOM_MID:
            return GDK_BOTTOM_SIDE;
        case SEL_BOTTOM_RIGHT:
            return GDK_BOTTOM_RIGHT_CORNER;
        case SEL_CLIPBOX:
            return GDK_FLEUR;
        default:
            return GDK_BLANK_CURSOR;
    }
}

gboolean
gp_selection_start_action ( GdkPoint *p )
{
    g_return_val_if_fail ( m_priv != NULL, FALSE );
    if ( m_priv->active )
    {
        m_priv->action = get_box_in ( p );
        m_priv->p_drag.x = p->x;
        m_priv->p_drag.y = p->y;

        g_print ("action:%i\n", m_priv->action);

        return (m_priv->action != SEL_NONE);
    }
    return FALSE;
        
}

void
gp_selection_do_action ( GdkPoint *p )
{
    g_return_if_fail ( m_priv != NULL );

    GpSelBox *clipbox   = &m_priv->boxes[SEL_CLIPBOX];
    gint dx = p->x - m_priv->p_drag.x;
    gint dy = p->y - m_priv->p_drag.y;

    switch ( m_priv->action )
    {
        case SEL_TOP_LEFT:
            clipbox->p0.y  += dy;
        case SEL_MID_LEFT:
            clipbox->p0.x  += dx;
            break;            
        case SEL_TOP_RIGHT:
            clipbox->p1.x  += dx;
        case SEL_TOP_MID:
            clipbox->p0.y  += dy;
            break;            
        case SEL_BOTTOM_LEFT:
            clipbox->p0.x  += dx;
        case SEL_BOTTOM_MID:
            clipbox->p1.y  += dy;
            break;
        case SEL_CLIPBOX:
            clipbox->p0.x  += dx;
            clipbox->p0.y  += dy;
        case SEL_BOTTOM_RIGHT:
            clipbox->p1.y  += dy;
        case SEL_MID_RIGHT:
            clipbox->p1.x  += dx;
            break;   
    }    
    m_priv->p_drag.x += dx;
    m_priv->p_drag.y += dy;
}


static void 
draw_sel_box ( GdkDrawable *drawing, GdkGC *gc, GpSelBox *box )
{
    gint x,y,w,h;
    x = box->p0.x;
    y = box->p0.y;
    w = (box->p1.x - x + 1);
    h = (box->p1.y - y + 1);
    gdk_draw_rectangle ( drawing, gc, TRUE, 
                         x, y, w, h);
}

static void
draw_top_line ( GdkDrawable *drawing, GdkGC *gc )
{
    GpSelBox *tlbox = &m_priv->boxes[SEL_TOP_LEFT];
    GpSelBox *tmbox = &m_priv->boxes[SEL_TOP_MID];
    GpSelBox *trbox = &m_priv->boxes[SEL_TOP_RIGHT];
    gdk_draw_line ( drawing, gc, tlbox->p1.x+1, tlbox->p0.y, 
                                 tmbox->p0.x-1, tmbox->p0.y );
    gdk_draw_line ( drawing, gc, tmbox->p1.x+1, tmbox->p0.y,
                                 trbox->p0.x-1, trbox->p0.y );
}

static void
draw_right_line ( GdkDrawable *drawing, GdkGC *gc )
{
    GpSelBox *trbox = &m_priv->boxes[SEL_TOP_RIGHT];
    GpSelBox *mrbox = &m_priv->boxes[SEL_MID_RIGHT];
    GpSelBox *brbox = &m_priv->boxes[SEL_BOTTOM_RIGHT];
    gdk_draw_line ( drawing, gc, trbox->p1.x, trbox->p1.y+1, 
                                 mrbox->p1.x, mrbox->p0.y-1);
    gdk_draw_line ( drawing, gc, mrbox->p1.x, mrbox->p1.y+1,
                                 brbox->p1.x, brbox->p0.y-1 );
}


static void
draw_left_line ( GdkDrawable *drawing, GdkGC *gc )
{
    GpSelBox *tlbox = &m_priv->boxes[SEL_TOP_LEFT];
    GpSelBox *mlbox = &m_priv->boxes[SEL_MID_LEFT];
    GpSelBox *blbox = &m_priv->boxes[SEL_BOTTOM_LEFT];
    gdk_draw_line ( drawing, gc, tlbox->p0.x, tlbox->p1.y+1, 
                                 mlbox->p0.x, mlbox->p0.y-1 );
    gdk_draw_line ( drawing, gc, mlbox->p0.x, mlbox->p1.y+1,
                                 blbox->p0.x, blbox->p0.y-1 );
}

static void
draw_bottom_line ( GdkDrawable *drawing, GdkGC *gc )
{
    GpSelBox *brbox = &m_priv->boxes[SEL_BOTTOM_RIGHT];
    GpSelBox *bmbox = &m_priv->boxes[SEL_BOTTOM_MID];
    GpSelBox *blbox = &m_priv->boxes[SEL_BOTTOM_LEFT];
    gdk_draw_line ( drawing, gc, brbox->p0.x-1, brbox->p1.y, 
                                 bmbox->p1.x+1, bmbox->p1.y );
    gdk_draw_line ( drawing, gc, bmbox->p0.x-1, bmbox->p1.y,
                                 blbox->p1.x+1, blbox->p1.y );
}

static void
draw_borders ( GdkDrawable *drawing, GdkGC *gc )
{
    GpSelBoxEnum box;
    for ( box = SEL_TOP_LEFT; box < SEL_CLIPBOX; box++ )
    {
        draw_sel_box ( drawing, gc, &m_priv->boxes[box] );
    }
    draw_top_line       ( drawing, gc );
    draw_right_line     ( drawing, gc );
    draw_bottom_line    ( drawing, gc );
    draw_left_line      ( drawing, gc );
}


void
gp_selection_draw ( GdkDrawable *gdkd )
{
    g_return_if_fail ( m_priv != NULL );
    if ( m_priv->active )
    {
        GpSelBox *clipbox   = &m_priv->boxes[SEL_CLIPBOX];
        gint x,y,w,h;
        gp_canvas *cv;
        gint8 dash_list[]	=	{ 3, 3 };
        GdkGC *gc;

        cv = cv_get_canvas ();
        gc	=	gdk_gc_new ( gtk_widget_get_window(cv->widget) );
        gdk_gc_set_function ( gc, GDK_INVERT );
        gdk_gc_set_dashes ( gc, 0, dash_list, 2 );
        gdk_gc_set_line_attributes ( gc, 1, GDK_LINE_ON_OFF_DASH,
                                     GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );

        x = MIN(clipbox->p0.x,clipbox->p1.x);
        y = MIN(clipbox->p0.y,clipbox->p1.y);
        w = ABS(clipbox->p1.x - clipbox->p0.x)+1;
        h = ABS(clipbox->p1.y - clipbox->p0.y)+1;


        if ( m_priv->floating )
        {
            cairo_t     *cr;
            cr  =   gdk_cairo_create ( cv->drawing );
            cairo_set_line_width (cr, 1.0);
            cairo_set_source_rgba (cr, 0.7, 0.9, 1.0, 0.3);
            cairo_rectangle ( cr, x, y, w, h); 
            cairo_fill (cr);
            cairo_destroy (cr);
        }
        else
        {
            /* Make transparent/opaque */
            if(cv->transparent != m_priv->transparent)
            {
            	guchar r, g, b, a = 0xFF;

            	m_priv->transparent = cv->transparent;
            	gp_selection_get_bg_color_rgb ( &r, &g, &b );
            	if(cv->transparent)
            	{
            		a = 0;
            	}
            	gp_image_make_color_transparent ( m_priv->image, r, g, b, a );
            	
            }
            
            /* Had to add this here because the selection
             * was being erased when changing tools. Don't
             * know if this was by design or not, but just
             * doesn't seem right. This is just temporary
             * until we find out.
             */
            if(GDK_IS_DRAWABLE(gdkd)){
            	gp_image_draw ( m_priv->image,
                            gdkd,
                            cv->gc_fg,
                            x,y,w,h );
            }
            else{
            	gp_image_draw ( m_priv->image,
                            cv->drawing,
                            cv->gc_fg,
                            x,y,w,h );
        	}
        }


        if ( m_priv->show_borders )
        {
            draw_borders ( cv->drawing, gc );
        }
        else
        {
            gdk_draw_rectangle ( cv->drawing, gc, FALSE, 
                                 x, y, w-1, h-1 );
        }

        
        g_object_unref ( gc );
        
    }    
}

/* Get background color's rgb values */
static gboolean gp_selection_get_bg_color_rgb(guchar *r, guchar *g, guchar *b)
{
	gp_canvas *cv;
	
	cv = cv_get_canvas ();
	if((cv) && (r) && (g) && (b))
	{
		GdkGCValues values;
		
		gdk_gc_get_values (cv->gc_bg, &values);

		*r = values.foreground.red >> 8;
		*g = values.foreground.green >> 8;
		*b = values.foreground.blue >> 8;

		return TRUE;
	}
	
	return FALSE;
}

/* See if there is a selection image */
gboolean gp_selection_query(void)
{
	if(m_priv){
		if(m_priv->image){
			return TRUE;
		}
	}
	
	return FALSE;
}

void gp_selection_invert(void)
{
	if(m_priv){
		if(m_priv->image){
			gp_image_invert_colors ( m_priv->image);
		}
	}
}

void gp_selection_flip(gboolean flip)
{
	if(m_priv){
		if(m_priv->image){
			gp_image_flip ( m_priv->image, flip );
		}
	}
}

void gp_selection_rotate(GdkPixbufRotation angle)
{
	if(m_priv){
		if(m_priv->image){
			GdkPixbuf *pixbuf;
			GdkPoint s, e;
			GpSelBox *clipbox;
			GpImage *new_image;
			gp_canvas *cv = cv_get_canvas ();

			gp_image_rotate ( m_priv->image, angle );
			pixbuf = gp_image_get_pixbuf(m_priv->image);

			clipbox = &m_priv->boxes[SEL_CLIPBOX];
			s.x = MIN(clipbox->p0.x,clipbox->p1.x);
        	s.y = MIN(clipbox->p0.y,clipbox->p1.y);
        	e.x = gdk_pixbuf_get_width(pixbuf);
        	e.y = gdk_pixbuf_get_height(pixbuf);
        	
        	clipbox->p0.x = s.x;
        	clipbox->p0.y = s.y;
        	/* -1 to allow for border width */
        	clipbox->p1.x = clipbox->p0.x + e.x - 1;
        	clipbox->p1.y = clipbox->p0.y + e.y - 1;

        	update_borders ( );
        	
        	g_object_unref ( m_priv->image );
			m_priv->image = gp_image_new_from_pixbuf ( pixbuf, TRUE );

			g_object_unref(pixbuf);
		}
	}
}

/* Return a copy of selection's image pixbuf */
GdkPixbuf *gp_selection_get_pixbuf(void)
{
	if(m_priv){
		if(m_priv->image){
			return gp_image_get_pixbuf(m_priv->image);
		}
	}
	
	return NULL;
}

/* Clear selection. Does not clear
 * as in destroying m_priv.
 * draw == TRUE to draw image
 */
void gp_selection_draw_and_clear (gboolean draw)
{
	GdkPoint pt = {0, 0};
	gp_canvas *cv = cv_get_canvas ();

	g_return_if_fail( gp_selection_query() );
	
	pt.x = m_priv->sp.x - 1;
	pt.y = pt.x;
	
	if(!draw){ g_object_unref (m_priv->image) ; m_priv->image = NULL ; }

	gp_selection_set_floating ( TRUE );
	gp_selection_set_active ( FALSE );
	gp_selection_clipbox_set_start_point ( &pt );
	gp_selection_clipbox_set_end_point ( &pt );
	gp_selection_set_active ( TRUE );

	gp_selection_set_borders ( FALSE );
	gtk_widget_queue_draw ( cv->widget );
}

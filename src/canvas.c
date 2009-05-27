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

static GtkWidget	*canvas		=	NULL;
static GtkWidget	*cv_ev_box	=	NULL;
static GtkWidget	*lb_size	=	NULL;
static GdkColor 	edge_color	=	{ 0, 0xa700, 0xc700, 0xf700  };
static GdkColor 	white_color	=	{ 0, 0xffff, 0xffff, 0xffff  };

/*resize parameters*/
static 	GdkGC 		*gc_resize	=	NULL;
static gboolean		b_resize	=	FALSE;
static gboolean		b_init		=	FALSE;
static gint			x_res		=	0;
static gint			y_res		=	0;


void 
canvas_set_widget 	( GtkWidget *widget )
{
	canvas = widget;
	gtk_widget_modify_bg ( canvas, GTK_STATE_NORMAL , &white_color );
}

void cv_ev_box_set_widget ( GtkWidget *widget )
{
	gint8 dash_list[]	=	{ 1, 1 };
	GdkColor 	color	=	{ 0, 0, 0, 0 };
	cv_ev_box	=	widget;	
	gtk_widget_modify_fg ( widget, GTK_STATE_NORMAL , &color );
	gc_resize = cv_ev_box->style->fg_gc[GTK_WIDGET_STATE (cv_ev_box)];
	gdk_gc_set_dashes ( gc_resize, 0, dash_list, 2 );
	gdk_gc_set_line_attributes ( gc_resize, 1, GDK_LINE_ON_OFF_DASH, 
	                             GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );
}


void
lb_size_set_widget ( GtkWidget *widget )
{
	lb_size	=	widget;
}


void cv_ev_box_draw	( void )
{
	gboolean paint = FALSE;
	if (b_resize)
	{
		gint x_offset = canvas->allocation.x - cv_ev_box->allocation.x;
		gint y_offset = canvas->allocation.y - cv_ev_box->allocation.y;
		gint x = x_res + x_offset;
		gint y = y_res + y_offset;
		gdk_draw_line ( cv_ev_box->window, gc_resize, x_offset, y_offset, x, y_offset );
		gdk_draw_line ( cv_ev_box->window, gc_resize, x_offset, y, x, y );
		gdk_draw_line ( cv_ev_box->window, gc_resize, x, y_offset, x, y );
		gdk_draw_line ( cv_ev_box->window, gc_resize, x_offset, y_offset, x_offset, y );
		paint = TRUE;
	}
	gtk_widget_set_app_paintable ( cv_ev_box, paint );
}

void cv_canvas_draw ( void )
{
	GString *str = g_string_new("");
	gint x,y;
	if (b_resize)
	{
		gdk_draw_line ( canvas->window, gc_resize, 0, 0, x_res, 0 );
		gdk_draw_line ( canvas->window, gc_resize, 0, y_res, x_res, y_res );
		gdk_draw_line ( canvas->window, gc_resize, x_res, 0, x_res, y_res );
		gdk_draw_line ( canvas->window, gc_resize, 0, 0, 0, y_res );
		x = x_res;
		y = y_res;
	}
	else
	{
		x = canvas->allocation.width;
		y = canvas->allocation.height;
	}
	g_string_printf (str, "%dx%d", x, y );
	gtk_label_set_text( GTK_LABEL(lb_size), str->str );
	g_string_free( str, TRUE);
	
}

void 
cv_right_set_widget ( GtkWidget *widget )
{
	static GdkCursor * cursor = NULL;
	if ( cursor == NULL )
	{
		cursor = gdk_cursor_new ( GDK_SB_H_DOUBLE_ARROW );
		gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &edge_color );
		gdk_window_set_cursor ( widget->window, cursor );
	}
}

void 
cv_bottom_right_set_widget ( GtkWidget *widget )
{
	static GdkCursor * cursor = NULL;
	if ( cursor == NULL )
	{
		cursor = gdk_cursor_new ( GDK_BOTTOM_RIGHT_CORNER );
		gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &edge_color );
		gdk_window_set_cursor ( widget->window, cursor );
	}
}

void
cv_bottom_set_widget ( GtkWidget *widget )
{
	static GdkCursor * cursor = NULL;
	if ( cursor == NULL )
	{
		cursor = gdk_cursor_new ( GDK_SB_V_DOUBLE_ARROW );
		gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &edge_color );
		gdk_window_set_cursor ( widget->window, cursor );
	}
}

void
cv_other_edge_set_widget ( GtkWidget *widget )
{
	gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &white_color  );
	gtk_widget_modify_fg ( widget, GTK_STATE_NORMAL , &edge_color  );

}

void
cv_other_edge_draw ( GtkWidget *widget )
{
	gdk_draw_line ( widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    0,0,0,widget->allocation.height);
	gdk_draw_line ( widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    0,0,widget->allocation.width,0);
}

void
cv_resize_start ( void )
{
	b_init	=	TRUE;
}

void
cv_resize_move ( gdouble x,  gdouble y)
{
	if( b_init )
	{
		b_resize = TRUE;
		x_res = canvas->allocation.width + (gint)x;
		y_res = canvas->allocation.height + (gint)y;
		x_res	= (x_res<1)?1:x_res;
		y_res	= (y_res<1)?1:y_res;
		gtk_widget_queue_draw (cv_ev_box);
	}
}

void
cv_resize_stop ( gdouble x,  gdouble y)
{
	if( b_resize )
	{
		gint width, height;
		width	= canvas->allocation.width + (gint)x;
		width	= (width<0)?0:width;
		height	= canvas->allocation.height + (gint)y;
		height	= (height<0)?0:height;
		gtk_widget_set_size_request ( canvas, width, height );
	}
	b_init		= FALSE;
	b_resize 	= FALSE;
}



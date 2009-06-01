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

#include "canvas.h"
#include "tool_line.h"

/* private functions */
static void cv_resize_start		( void );
static void cv_resize_move		( gdouble x,  gdouble y);
static void cv_resize_stop		( gdouble x,  gdouble y);
static void cv_resize_cancel	( void );
static void cv_resize_draw		( void );


/* private data  */
static gnome_paint_tool	*cv_tool		=	NULL;
static GtkWidget		*canvas			=	NULL;
static GtkWidget		*cv_ev_box		=	NULL;
static GdkGC 			*cv_gc			=	NULL;
static GtkWidget		*cv_top_edge	=	NULL;
static GtkWidget		*cv_bottom_edge	=	NULL;
static GtkWidget		*cv_left_edge	=	NULL;
static GtkWidget		*cv_right_edge	=	NULL;
static GtkWidget		*lb_size		=	NULL;
static GdkGC 			*gc_resize		=	NULL;
static GdkColor 		edge_color		=	{ 0, 0x2f00, 0x3600, 0x9800  };
static GdkColor 		white_color		=	{ 0, 0xffff, 0xffff, 0xffff  };
static GdkColor 		black_color		=	{ 0, 0x0000, 0x0000, 0x0000  };
static gboolean			b_resize		=	FALSE;
static gboolean			b_init			=	FALSE;
static gint				x_res			=	0;
static gint				y_res			=	0;


/*
 *   CODE
 */

void
cv_set_color_bg	( GdkColor *color )
{
	gdk_gc_set_rgb_bg_color ( cv_gc, color );		
}

void
cv_set_color_fg	( GdkColor *color )
{
	gdk_gc_set_rgb_fg_color ( cv_gc, color );	
}

void
cv_set_line_width	( gint width )
{
	gdk_gc_set_line_attributes ( cv_gc, width, GDK_LINE_SOLID, 
	                             GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );
}

void cv_sel_none_tool	( void )
{
	gdk_window_set_cursor ( canvas->window, NULL);
	cv_tool = NULL;
}

void cv_sel_line_tool	( void )
{
	static gnome_paint_tool	*line_tool	=	NULL;
	if ( line_tool == NULL )
	{
		line_tool = tool_line_init ( canvas, cv_gc );
	}
	cv_tool = line_tool;
	cv_tool->reset ();
}


/* GUI CallBacks */

void
on_canvas_realize (GtkWidget *widget, gpointer user_data)
{
	canvas = widget;
	gtk_widget_modify_bg ( canvas, GTK_STATE_NORMAL , &white_color );

	if ( cv_gc == NULL )
	{
		cv_gc	=	gdk_gc_new ( canvas->window );
		g_assert(cv_gc);
		/*set data to be destroyed*/
		g_object_set_data_full (	G_OBJECT(canvas), "cv_gc", 
		                       		(gpointer)cv_gc , 
		                        	(GDestroyNotify)g_object_unref );
	}
	cv_set_color_fg ( &black_color );
	cv_set_color_bg ( &white_color );
	cv_set_line_width ( 1 );
}

void
on_cv_ev_box_realize (GtkWidget *widget, gpointer user_data)
{
	gint8 dash_list[]	=	{ 1, 1 };
	cv_ev_box	=	widget;	
	gtk_widget_modify_fg ( cv_ev_box, GTK_STATE_NORMAL , &black_color );
	gc_resize = cv_ev_box->style->fg_gc[GTK_WIDGET_STATE (cv_ev_box)];
	gdk_gc_set_dashes ( gc_resize, 0, dash_list, 2 );
	gdk_gc_set_line_attributes ( gc_resize, 1, GDK_LINE_ON_OFF_DASH, 
	                             GDK_CAP_NOT_LAST, GDK_JOIN_ROUND );
}

void
on_lb_size_realize (GtkWidget *widget, gpointer user_data)
{
	lb_size	=	widget;
}


void
on_cv_right_realize (GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor;
	cv_right_edge = widget;
	gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &edge_color );
	cursor = gdk_cursor_new_for_display ( gtk_widget_get_display(widget) , GDK_SB_H_DOUBLE_ARROW );
	g_assert (cursor);
	gdk_window_set_cursor ( widget->window, cursor );
	gdk_cursor_unref ( cursor );
}

void 
on_cv_bottom_right_realize (GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor;
	gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &edge_color );
	cursor = gdk_cursor_new_for_display ( gtk_widget_get_display(widget) , GDK_BOTTOM_RIGHT_CORNER );
	g_assert (cursor);
	gdk_window_set_cursor ( widget->window, cursor );
	gdk_cursor_unref ( cursor );
}

void
on_cv_bottom_realize (GtkWidget *widget, gpointer user_data)
{
	GdkCursor *cursor;
	cv_bottom_edge = widget;
	gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &edge_color );
	cursor = gdk_cursor_new_for_display ( gtk_widget_get_display(widget) , GDK_SB_V_DOUBLE_ARROW );
	g_assert (cursor);
	gdk_window_set_cursor ( widget->window, cursor );
	gdk_cursor_unref ( cursor );
}

void
on_cv_top_realize (GtkWidget *widget, gpointer user_data)
{
	cv_top_edge = widget;
	on_cv_other_edge_realize( widget, user_data );
}

void
on_cv_left_realize (GtkWidget *widget, gpointer user_data)
{
	cv_left_edge = widget;
	on_cv_other_edge_realize( widget, user_data );
}


void
on_cv_other_edge_realize (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_modify_bg ( widget, GTK_STATE_NORMAL , &white_color  );
	gtk_widget_modify_fg ( widget, GTK_STATE_NORMAL , &edge_color  );

}

/* events */
gboolean
on_canvas_button_press_event (	GtkWidget	   *widget, 
                                GdkEventButton *event,
                                gpointer       user_data )
{
	gboolean ret = TRUE;

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_press( event );
	}

	return ret;
}

gboolean
on_canvas_button_release_event (	GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	gboolean ret = TRUE;

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_release( event );
	}

	return ret;
}
									
gboolean
on_canvas_motion_notify_event (	GtkWidget      *widget,
		                        GdkEventMotion *event,
                                gpointer        user_data)
{
	gboolean ret = TRUE;

	if ( cv_tool != NULL )
	{
		ret = cv_tool->button_motion( event );
	}

	return ret;
}

gboolean 
on_canvas_expose_event	(   GtkWidget	   *widget, 
							GdkEventButton *event,
               				gpointer       user_data )
{
	if ( cv_tool != NULL )
	{
		cv_tool->draw();
	}

	cv_resize_draw();
	
	return TRUE;
}



gboolean 
on_cv_other_edge_expose_event	(   GtkWidget	   *widget, 
									GdkEventButton *event,
                                    gpointer       user_data )
{
	gdk_draw_line ( widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    0,0,0,widget->allocation.height);
	gdk_draw_line ( widget->window,
                    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                    0,0,widget->allocation.width,0);
	return TRUE;
}



gboolean 
on_cv_ev_box_expose_event (	GtkWidget	   *widget, 
							GdkEventButton *event,
                        	gpointer       user_data )
{
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
	}
	gtk_widget_set_app_paintable ( cv_ev_box, b_resize );
	return TRUE;
}

gboolean 
on_cv_bottom_right_button_press_event	(   GtkWidget	   *widget, 
											GdkEventButton *event,
                                            gpointer       user_data )
{
	if ( event->type == GDK_BUTTON_PRESS )
	{
		if ( event->button == LEFT_BUTTON )
		{
			cv_resize_start();
		}
		else if ( event->button == RIGHT_BUTTON )
		{
			cv_resize_cancel();
		}
	}
	return TRUE;
}


gboolean
on_cv_bottom_right_motion_notify_event (	GtkWidget      *widget,
                                     		GdkEventMotion *event,
                                            gpointer        user_data)
{
	cv_resize_move( event->x, event->y );
	return TRUE;
}

gboolean 
on_cv_bottom_right_button_release_event (   GtkWidget	   *widget, 
                                            GdkEventButton *event,
                                            gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		cv_resize_stop ( event->x,  event->y );
	}
	return TRUE;
}


gboolean 
on_cv_bottom_button_press_event (   GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	return on_cv_bottom_right_button_press_event( widget, event, user_data );
}

gboolean 
on_cv_bottom_motion_notify_event (  GtkWidget      *widget,
		                            GdkEventMotion *event,
                                    gpointer        user_data)
{
	cv_resize_move( 0.0, event->y );
	return TRUE;
}

gboolean
on_cv_bottom_button_release_event ( GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		cv_resize_stop ( 0.0,  event->y );
	}
	return TRUE;
}

gboolean
on_cv_right_button_press_event (	GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	return on_cv_bottom_right_button_press_event( widget, event, user_data );
}

gboolean
on_cv_right_motion_notify_event (   GtkWidget      *widget,
		                            GdkEventMotion *event,
                                    gpointer        user_data)
{
	cv_resize_move( event->x, 0.0 );
	return TRUE;
}

gboolean
on_cv_right_button_release_event (  GtkWidget	   *widget, 
                                    GdkEventButton *event,
                                    gpointer       user_data )
{
	if ( (event->type == GDK_BUTTON_RELEASE) && (event->button == LEFT_BUTTON) )
	{
		cv_resize_stop ( event->x,  0.0 );
	}
	return TRUE;
}

/* private */

static void
cv_resize_start ( void )
{
	b_init	=	TRUE;
}

static void
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

static void
cv_resize_stop ( gdouble x,  gdouble y)
{
	if( b_resize )
	{
		gint w, h, width, height;
		width	= canvas->allocation.width + (gint)x;
		width	= (width<1)?1:width;
		height	= canvas->allocation.height + (gint)y;
		height	= (height<1)?1:height;
		gtk_widget_set_size_request ( canvas, width, height );
		w = ( width  < 3 )?width :3;
		h = ( height < 3 )?height:3;
		gtk_widget_set_size_request ( cv_top_edge		, w, 3 );
		gtk_widget_set_size_request ( cv_bottom_edge	, w, 3 );
		gtk_widget_set_size_request ( cv_left_edge		, 3, h );
		gtk_widget_set_size_request ( cv_right_edge		, 3, h );
	}
	cv_resize_cancel();
}

static void
cv_resize_cancel ( void )
{
	b_init		= FALSE;
	b_resize 	= FALSE;
	gtk_widget_queue_draw (cv_ev_box);
}

static void
cv_resize_draw ( void )
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

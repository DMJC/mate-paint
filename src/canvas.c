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
static GdkColor 	edge_color	=	{ 0, 0xa700, 0xc700, 0xf700  };
static GdkColor 	white_color	=	{ 0, 0xffff, 0xffff, 0xffff  };

/*Control de resize*/
static gboolean		b_resize	=	FALSE;
static gboolean		b_init		=	FALSE;


void 
canvas_set_widget 	( GtkWidget *widget )
{
	canvas = widget;
	gtk_widget_modify_bg ( canvas, GTK_STATE_NORMAL , &white_color );
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
cv_bottom_right_start ( void )
{
	b_init	=	TRUE;
}

void
cv_bottom_right_move ( gdouble x,  gdouble y)
{
	if( b_init )
	{
		b_resize = TRUE;
	}
}

void
cv_bottom_right_stop ( gdouble x,  gdouble y)
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


/***************************************************************************
 *            toolbar.c
 *
 *  Sat May  9 15:13:23 2009
 *  Copyright (C) Rogério Ferro do Nascimento 2009 
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

#include "toolbar.h"
#include "common.h"
#include "cv_drawing.h"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "./pixmaps/line_0.xpm"
#include "./pixmaps/line_1.xpm"
#include "./pixmaps/line_2.xpm"
#include "./pixmaps/line_3.xpm"
#include "./pixmaps/line_4.xpm"
#include "./pixmaps/sel1.xpm"
#include "./pixmaps/sel2.xpm"
#include "./pixmaps/rect0.xpm"
#include "./pixmaps/rect1.xpm"
#include "./pixmaps/rect2.xpm"


typedef enum {
	NONE,
	SELECTION,
	RECT_LINE,
	ERASE,
	
} TypeBar;

/* private data */
static GtkNotebook	*notebook			= NULL;
static GtkFrame		*frame_rect			= NULL;
static gboolean		frame_rect_show		= FALSE;	

/* private functions */
static GtkWidget *	get_gtk_image( GtkWidget *widget, gchar** xpm );
static void			show_frame_rect	( gboolean show );
static void         tool_toggled ( GtkToggleToolButton *button, gp_tool_enum tool );

/* CODE */

void
on_tool_free_select_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_FREE_SELECT );
}

void
on_tool_rect_select_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_RECT_SELECT );
}

void 
on_tool_eraser_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_ERASER );
}

void 
on_tool_color_picker_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_COLOR_PICKER );
}

void 
on_tool_pencil_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_PENCIL );
}

void 
on_tool_airbrush_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_AIRBRUSH );
}

void
on_tool_bucket_fill_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_BUCKET_FILL );
}

void
on_tool_zoom_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_ZOOM );
}

void
on_tool_paintbrush_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_PAINTBRUSH );
}

void
on_tool_text_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_TEXT );
}

void 
on_draw_line_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_LINE );
}

void 
on_draw_rectangle_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_RECTANGLE );
}

void 
on_draw_ellipse_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_ELLIPSE );
}

void
on_draw_curve_toggled (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_CURVE );
}

void
on_draw_polygon_toggled	(GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_POLYGON );
}

void
on_draw_rounded_rectangle_toggled  (GtkToggleToolButton *button, gpointer user_data)
{
    tool_toggled ( button, TOOL_ROUNDED_RECTANGLE );
}

void
on_line0_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_line_width ( 1 );
	}
}

void
on_line1_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_line_width ( 2 );
	}
}

void
on_line2_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_line_width ( 3 );
	}
}

void
on_line3_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_line_width ( 4 );
	}
}

void
on_line4_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_line_width ( 5 );
	}
}

void 
on_rect0_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_filled ( FILLED_NONE );
	}
}

void 
on_rect1_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_filled ( FILLED_BACK );
	}
}

void 
on_rect2_toggled (GtkToggleToolButton *button, gpointer user_data)
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		cv_set_filled ( FILLED_FORE );
	}
}

void 
on_notebook_realize   (GtkObject *object, gpointer user_data)
{
	notebook	=	GTK_NOTEBOOK( object );
	gtk_notebook_set_current_page ( notebook, SELECTION );
}

void 
on_frame_rect_realize  (GtkObject *object, gpointer user_data)
{
	frame_rect	=	GTK_FRAME( object );
	show_frame_rect ( frame_rect_show );
}

void
on_line0_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)line_0_xpm ) );
}

void
on_line1_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)line_1_xpm ) );
}

void
on_line2_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)line_2_xpm ) );
}

void
on_line3_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)line_3_xpm ) );
}

void
on_line4_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)line_4_xpm ) );
}

void 
on_sel1_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)sel1_xpm ) );
}

void 
on_sel2_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)sel2_xpm ) );
}

void 
on_rect0_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)rect0_xpm ) );
}

void 
on_rect1_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)rect1_xpm ) );
}

void 
on_rect2_realize   (GtkObject *object, gpointer user_data)
{
	gtk_tool_button_set_icon_widget (	GTK_TOOL_BUTTON(object), 
	                                 	get_gtk_image( GTK_WIDGET(object), (gchar**)rect2_xpm ) );
}

/*private*/

static GtkWidget * 
get_gtk_image ( GtkWidget *widget, gchar** xpm )
{
	GdkPixmap *gdkpixmap	= NULL;
    GdkBitmap *mask 		= NULL;
    GtkWidget *gtkimage 	= NULL;
	gdkpixmap = gdk_pixmap_create_from_xpm_d(widget->window, &mask, NULL, xpm);
	g_assert ( gdkpixmap );
	gtkimage = gtk_image_new_from_pixmap(gdkpixmap, mask);
    g_assert ( gtkimage );
	g_object_unref ( G_OBJECT(gdkpixmap) );
    g_object_unref ( G_OBJECT(mask) ); 
	gtk_widget_show(gtkimage);
	return gtkimage;
}

static void	
show_frame_rect	( gboolean show )
{
	frame_rect_show = show;
	if( frame_rect != NULL )
	{
		if ( show )
		{
			gtk_widget_show( GTK_WIDGET(frame_rect) );
		}
		else
		{
			gtk_widget_hide( GTK_WIDGET(frame_rect) );
		}
	}
}

static void
tool_toggled ( GtkToggleToolButton *button, gp_tool_enum tool )
{
	if ( gtk_toggle_tool_button_get_active ( button ) )
	{
		/*show tool options */
		g_return_if_fail( notebook != NULL );
        switch ( tool )
        {
            case TOOL_NONE:
            case TOOL_FREE_SELECT:
            case TOOL_RECT_SELECT:
            case TOOL_TEXT:
           		gtk_notebook_set_current_page ( notebook, SELECTION );
                break;
            case TOOL_LINE:
            case TOOL_CURVE:
        		show_frame_rect ( FALSE );
                gtk_notebook_set_current_page ( notebook, RECT_LINE );
                break;
            case TOOL_RECTANGLE:
            case TOOL_ELLIPSE:
            case TOOL_POLYGON:
            case TOOL_ROUNDED_RECTANGLE:
        		show_frame_rect ( TRUE );
                gtk_notebook_set_current_page ( notebook, RECT_LINE );
                break;
            case TOOL_ERASER:
            case TOOL_COLOR_PICKER:
            case TOOL_PENCIL:
            case TOOL_AIRBRUSH:
            case TOOL_BUCKET_FILL:
            case TOOL_ZOOM:
            case TOOL_PAINTBRUSH:
            default:
                gtk_notebook_set_current_page ( notebook, NONE );
                break;
        }
		/*select tool*/
        cv_set_tool ( tool );
	}
}


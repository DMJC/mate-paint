/***************************************************************************
 *            undo.c
 *
 *  Sat Jan 23 14:05:22 2010
 *  Copyright  2010  Rogério Ferro do Nascimento
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

#include "undo.h"
#include "common.h"
#include "cv_drawing.h"
#include "gp-image.h"
#include "gp_point_array.h"


static GdkColor 	white_color		=	{ 0, 0xffff, 0xffff, 0xffff  };
static GdkColor 	black_color		=	{ 0, 0x0000, 0x0000, 0x0000  };
static GdkColor 	back_color		=	{ 0, 0xff00, 0x0000, 0xff00  };


typedef enum
{
	UNDO_PIXBUF,
	UNDO_RESIZE
} undo_type;

typedef struct
{
	GInputStream	*stream;
	gint			x;
	gint			y;
} GpUndoPixbuf;

typedef struct
{
	undo_type	type;
	gpointer	data;
	gchar		*message;
} GpUndo;


/*statics queue*/
GQueue _undo_queue = G_QUEUE_INIT;
GQueue _redo_queue = G_QUEUE_INIT;
/**/

GQueue	*undo_queue	=	&_undo_queue;
GQueue	*redo_queue	=	&_redo_queue;

static GpUndo * 		undo_pixbuf_new		( const GdkPixbuf *pixbuf, gint x, gint y, const gchar * message);
static void				undo_pixbuf_free	( GpUndo *undo );
void DestroyNotify  (gpointer data);

void 
undo_create_mask ( gint width, gint height, GdkBitmap **mask, GdkGC **gc_mask )
{
    GdkColor    color;
	gp_canvas   *cv	=	cv_get_canvas();
     
    *mask 		=	gdk_pixmap_new (NULL, width, height, 1 );
    *gc_mask	=	gdk_gc_new ( *mask );
    gdk_gc_set_line_attributes ( *gc_mask, cv->line_width, GDK_LINE_SOLID, 
                             	GDK_CAP_ROUND, GDK_JOIN_ROUND );

    color.pixel = 0;
    gdk_gc_set_foreground (*gc_mask, &color);
    gdk_draw_rectangle (*mask, *gc_mask, TRUE, 0, 0, width, height);

    color.pixel = 1;
    gdk_gc_set_foreground (*gc_mask, &color);
    return;
}

void
undo_add (GdkRectangle *rect, GdkBitmap * mask, const gchar * message )
{
	GpUndo		*undo;
	GdkPixbuf	*pixbuf;
	GpImage     *image;
    gboolean    has_alpha;
	gp_canvas	*cv	    = cv_get_canvas();

    has_alpha   =   (mask != NULL);
    image = gp_image_new_from_pixmap ( cv->pixmap, rect, has_alpha );
    if (has_alpha)
    {
        gp_image_set_mask ( image, mask );
    }
    pixbuf = gp_image_get_pixbuf (image);
    g_object_unref (image);
    
	undo	=	undo_pixbuf_new (pixbuf, rect->x, rect->y, message );

	g_queue_push_head	( undo_queue, undo );
    
    
	g_object_unref (pixbuf);
}



/* GUI CallBack */
void 
on_menu_undo_activate ( GtkMenuItem *menuitem, gpointer user_data)
{
	GpUndo		*undo	=	g_queue_pop_head (undo_queue);
	if ( undo != NULL )
	{
		if (undo->type == UNDO_PIXBUF)
		{
			GError 			*error	= NULL;
			GdkPixbuf 		*pixbuf	= NULL;
			gp_canvas		*cv	=	cv_get_canvas();
			GpUndoPixbuf	*data	=	(GpUndoPixbuf*)undo->data;

			pixbuf = gdk_pixbuf_new_from_stream ( data->stream, NULL, &error);
			if( error != NULL) 
			{
				g_print("TODO: error\n");
				g_error_free (error);
			}
			gdk_draw_pixbuf	(cv->pixmap,
							 cv->gc_fg,
							 pixbuf,
							 0, 0,
							 data->x, data->y,
							 -1, -1,
							 GDK_RGB_DITHER_NORMAL, 0, 0);
			gtk_widget_queue_draw (cv->widget);
			g_object_unref (pixbuf);
		}	
		undo_pixbuf_free (undo);
		g_print ("on menu undo\n");
	}
}

void 
on_menu_redo_activate ( GtkMenuItem *menuitem, gpointer user_data)
{
	g_print ("on menu redo\n");
}

/*private*/
static GpUndo * 
undo_pixbuf_new ( const GdkPixbuf *pixbuf, gint x, gint y, const gchar * message)
{
	GpUndo	*undo	=	NULL;
	if ( pixbuf != NULL )
	{
		GError 			*error		=	NULL;
		gchar			*buffer 	=	NULL;
		gsize			buffer_size	=	0;
		GpUndoPixbuf	*data		=	g_new (GpUndoPixbuf, 1);
		gdk_pixbuf_save_to_buffer ( (GdkPixbuf *)pixbuf, &buffer, &buffer_size, "png", &error, NULL );
        g_print ("data addr:%d\n",(int)buffer);
		if (error!=NULL)
		{
			g_print("TODO: error\n");
			g_error_free (error);
		}
		data->stream	=	g_memory_input_stream_new_from_data ( buffer, buffer_size, DestroyNotify );
		data->x			=	x;
		data->y			=	y;
		undo			=	g_new (GpUndo, 1);
		undo->type		=	UNDO_PIXBUF;
		undo->data		=	(gpointer)data;
		undo->message	=	g_strdup (message);

		g_print ("buffer_size:%d\n",buffer_size);
	}
	return undo;		
}

static void 
undo_pixbuf_free ( GpUndo *undo )
{
	if (undo->type == UNDO_PIXBUF)
	{
		GpUndoPixbuf	*data	=	(GpUndoPixbuf*)undo->data;
        g_input_stream_close ( data->stream, NULL, NULL);
		g_object_unref (data->stream);
		g_free (data);
		g_free (undo->message);
		g_free (undo);
	}
	return;	
}

void DestroyNotify  (gpointer data)
{
    g_print ("data addr:%d\n",(int)data);
	g_free (data);
	g_print("data freed\n");
}


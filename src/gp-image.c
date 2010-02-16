/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * gnome-paint
 * Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
 * 
 * gnome-paint is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gnome-paint is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gp-image.h"
#include <gdk-pixbuf/gdk-pixbuf.h>


#define GP_IMAGE_GET_PRIVATE(object)	\
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), GP_TYPE_IMAGE, GpImagePrivate))


const int BITS_PER_SAMPLE = 8;


struct _GpImagePrivate
{
	GdkPixbuf *pixbuf;
};


static gint
gp_image_priv_test (void)
{
	/* TODO: Add private function implementation here */
	return 0;
}

G_DEFINE_TYPE (GpImage, gp_image, G_TYPE_OBJECT);

static void
gp_image_init (GpImage *object)
{
	object->priv = GP_IMAGE_GET_PRIVATE (object);
	object->priv->pixbuf = NULL;
}

static void
gp_image_finalize (GObject *object)
{
	G_OBJECT_CLASS (gp_image_parent_class)->finalize (object);
}

static void
gp_image_class_init (GpImageClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GObjectClass* parent_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gp_image_finalize;

	g_type_class_add_private (object_class, sizeof (GpImagePrivate));

}


GpImage * 
gp_image_new ( gint width, gint height )
{
	GpImage *image;

	g_return_val_if_fail (width > 0, NULL);
	g_return_val_if_fail (height > 0, NULL);
	
	image = g_object_new (GP_TYPE_IMAGE, NULL);
    image->priv->pixbuf = gdk_pixbuf_new (
            GDK_COLORSPACE_RGB, TRUE, BITS_PER_SAMPLE, width, height);
    g_assert(image->priv->pixbuf);
	g_object_set_data ( G_OBJECT(image), "pixbuf", image->priv->pixbuf);

	return image;
}


GpImage * 
gp_image_new_from_pixmap ( GdkPixmap* pixmap, GdkRectangle *rect )
{
    GpImage			*image;
	GdkRectangle	r;

	g_return_val_if_fail ( GDK_IS_PIXMAP (pixmap), NULL);

	if ( rect == NULL )
	{
		r.x = r.y = 0;
		gdk_drawable_get_size ( pixmap, &r.width, &r.height );
	}
	else
	{
		r.x		=   rect->x;
		r.y		=   rect->y;
		r.width =   rect->width;
		r.height=   rect->height;
	}
	
	image   = gp_image_new( r.width, r.height );
	g_return_val_if_fail ( image != NULL, NULL);
	
    gdk_pixbuf_get_from_drawable(
                image->priv->pixbuf,
                pixmap,
                gdk_drawable_get_colormap ( pixmap ), 
                r.x, r.y,
                0, 0,
                r.width, r.height);
    return image;
}



GdkPixbuf *
gp_image_get_pixbuf ( GpImage *image )
{
	g_return_val_if_fail ( GP_IS_IMAGE (image), NULL);

	return  gdk_pixbuf_copy ( image->priv->pixbuf );
}

void		
gp_image_set_mask ( GpImage *image, GdkBitmap *mask )
{
	GdkPixbuf *pixbuf;
	GdkPixbuf *m_pixbuf;
	guchar *pixels, *m_pixels;
	guchar *p, *m_p;
	gint w, h;
	gint n_channels, rowstride;

	g_return_if_fail ( GP_IS_IMAGE (image) );
	

	pixbuf		=   image->priv->pixbuf;
	m_pixbuf	=   gdk_pixbuf_copy ( pixbuf );
	
	gdk_pixbuf_get_from_drawable (  m_pixbuf, 
               						mask,
               						gdk_drawable_get_colormap (mask),
               						0,0,
               						0,0,
              						-1,-1);
	n_channels  =   gdk_pixbuf_get_n_channels   ( pixbuf );
	rowstride   =   gdk_pixbuf_get_rowstride	( pixbuf );
	w			=   gdk_pixbuf_get_width		( pixbuf );
	h			=   gdk_pixbuf_get_height		( pixbuf );
	pixels		=   gdk_pixbuf_get_pixels		( pixbuf );
	m_pixels	=   gdk_pixbuf_get_pixels		( m_pixbuf );
	while (h--) 
	{
		guint   i = w;
		p   = pixels;
		m_p = m_pixels;
		while (i--) 
		{
			if(m_p[0] == 0)
			{
				p[0] = 0; 
				p[1] = 0; 
				p[2] = 0; 
				p[3] = 0; 
			}
			p   += n_channels;
			m_p += n_channels;
		}
		pixels		+= rowstride;
		m_pixels	+= rowstride;
	}
	g_object_unref (m_pixbuf);
}





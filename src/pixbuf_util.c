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
#include "pixbuf_util.h"




static void setpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y, guint color);
static gint getpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y);
static gint fill_shape(GdkPixbuf *pixbuf, guint x, guint y, guint nc);

/* Note: There's a problem with using the gd fill algo. The image needs to be surrounded
 * by a border, otherwise the fill won't be right. 
 * 
 * Workaround:
 *             1. Make the pixbuf 2 pixels wider
 *             2. Fill with black
 * After copying the pixmap at pos 1,1, this has the effect of creating a rectangle with a
 * line width of one pixel. 
 */
void fill_draw(GdkDrawable *drawable, GdkGC *gc, guint fill_color, guint x, guint y)
{
	GdkPixbuf *pixbuf;
	gint width, height;
	
	printf("fill_draw fill_color: %.08X\n", fill_color);
	printf("fill_draw x: %d, y: %d\n", x, y);
	
	gdk_drawable_get_size(drawable, &width, &height);
	printf("fill_draw w: %d, h: %d\n", width, height);
	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width + 2, height + 2);

	gdk_pixbuf_fill(pixbuf, 0);

	/* 1 get pixbuf from drawable */
	gdk_pixbuf_get_from_drawable(pixbuf, drawable, NULL, 0, 0, 1, 1, width, height);
	/* 2 draw on pixbuf */
	fill_shape(pixbuf, x, y, fill_color);

	/* 3 draw pixbuf back onto drawable. Notice xdest & ydest */
	gdk_draw_pixbuf(drawable, gc, pixbuf, 1, 1, 0, 0, gdk_pixbuf_get_width(pixbuf),
					gdk_pixbuf_get_height(pixbuf), GDK_RGB_DITHER_NONE, 0, 0);
	
	/* clean up */
	g_object_unref(pixbuf);
}


/* Get a pixel's value at (x,y)
 * For pixbufs with alpha channel only.
 * Sets 'color' as rgb value
 */
gboolean get_pixel_from_pixbuf(GdkPixbuf *pixbuf, guint *color, guint x, guint y)
{
	int width, height, rowstride, n_channels;
	guchar *pixels;

	if(!GDK_IS_PIXBUF(pixbuf)){
		printf("get_pixel_from_pixbuf: !pixbuf\n");
		return 0;
	}

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	
	if (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB){
		printf("get_pixel_from_pixbuf: color space  != GDK_COLORSPACE_RGB\n");
		return FALSE;
	}
	if (gdk_pixbuf_get_bits_per_sample (pixbuf) != 8){
		printf("get_pixel_from_pixbuf: bits per sample != 8\n");
		return FALSE;
	}
	if (!gdk_pixbuf_get_has_alpha (pixbuf)){
		printf("get_pixel_from_pixbuf: no alpha\n");
		return FALSE;
	}
	if (n_channels != 4){
		printf("get_pixel_from_pixbuf: n_channels != 4\n");
		return FALSE;
	}
	if (NULL == color){
		printf("get_pixel_from_pixbuf: NULL == color\n");
		return FALSE;
	}

	*color = getpixel(pixels, rowstride, n_channels, x, y);
	printf("get_pixel_from_pixbuf color: 0x%.08x %s %d\n", *color, __FILE__, __LINE__);

	return TRUE;
}


static void
setpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y, guint color)
{
	guchar *p;
	
	//printf("pp %.08X ", color);

	p = pixels + (y * rowstride + x * n_channels);
	p[0] = getr(color);/*red*/
	p[1] = getg(color);/*green*/
	p[2] = getb(color);/*blue*/
	p[3] = 0xFF; /* alpha */

}



static gint
getpixel(guchar *pixels, gint rowstride, gint n_channels, gint x, gint y)
{
	guchar *p;
	gint color = 0;
	
	p = pixels + (y * rowstride + x * n_channels);

	color = col(p[0], p[1], p[2]);

	return color;
}

/* Flood fill on a pixbuf */
/* The following code was copped and modified from gdlib
 */

/* horizontal segment of scan line y */
struct seg {guint y, xl, xr, dy;};

/* max depth of stack */
#define FILL_MAX 1200000
#define FILL_PUSH(Y, XL, XR, DY) \
	if (sp<stack+FILL_MAX*10 && Y+(DY)>=0 && Y+(DY)<wy2) \
	{sp->y = Y; sp->xl = XL; sp->xr = XR; sp->dy = DY; sp++;}

#define FILL_POP(Y, XL, XR, DY) \
	{sp--; Y = sp->y+(DY = sp->dy); XL = sp->xl; XR = sp->xr;}

gint fill_shape(GdkPixbuf *pixbuf, guint x, guint y, guint nc)
{
	guint l, x1, x2, dy;
	guint oc;   /* old pixel value */
	guint wx2,wy2;
	
	int width, height, rowstride, n_channels;
	guchar *pixels;

	n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	
	if (gdk_pixbuf_get_colorspace (pixbuf) != GDK_COLORSPACE_RGB){
		return FALSE;
	}
	if (gdk_pixbuf_get_bits_per_sample (pixbuf) != 8){
		return FALSE;
	}
	if (!gdk_pixbuf_get_has_alpha (pixbuf)){
		return FALSE;
	}
	if (n_channels != 4){
		return FALSE;
	}
	
	printf("fill.c fill_shape(): \n");

	/* stack of filled segments */
	/* struct seg stack[FILL_MAX],*sp = stack;; */
	struct seg *stack;
	struct seg *sp;

	wx2=width;wy2=height;
	oc = getpixel(pixels, rowstride, n_channels, x, y);
	if (oc==nc || x<0 || x>wx2 || y<0 || y>wy2) {
		printf("fill_shape oc==nc || x<0 || x>wx2 || y<0 || y>wy2\n"
				  "    Fill color is same as back color, or pixel ouside\n"
				  "    of image. x: %d, y: %d, w: %d, h: %d\n"
			      "    fill: %X, bg: %X\n", x, y, width, height, nc, oc);
		return 0;
	}

	/* Do not use the 4 neighbors implementation with
	 * small images
	 */
	if (height < 4) {
		guint ix = x, iy = y, c;
		do {
			c = getpixel(pixels, rowstride, n_channels, ix, iy);
			if (c != oc) {
				printf("1 (c != oc) ");
				goto done;
			}
			setpixel(pixels, rowstride, n_channels, ix, iy, nc);
		} while(ix++ < (height -1));
		ix = x; iy = y + 1;
		do {
			c = getpixel(pixels, rowstride, n_channels, ix, iy);
			if (c != oc) {
				printf("2 (c != oc) ");
				goto done;
			}
			setpixel(pixels, rowstride, n_channels, ix, iy, nc);
		} while(ix++ < (height -1));
		goto done;
	}

	stack = (struct seg *)g_malloc0(sizeof(struct seg) * ((guint)(width*height)/4));
	if (!stack) {
		g_warning("fill_shape !stack\n");
		return 0;
	}
	sp = stack;

	/* required! */
	FILL_PUSH(y,x,x,1);
	/* seed segment (popped 1st) */
 	FILL_PUSH(y+1, x, x, -1);
	while (sp>stack) {
		FILL_POP(y, x1, x2, dy);

		for (x=x1; x>=0 && getpixel(pixels, rowstride, n_channels,x, y)==oc; x--) {
			setpixel(pixels, rowstride, n_channels,x, y, nc);
		}
		if (x>=x1) {
			goto skip;
		}
		l = x+1;

				/* leak on left? */
		if (l<x1) {
			FILL_PUSH(y, l, x1-1, -dy);
		}
		x = x1+1;
		do {
			for (; x<=wx2 && getpixel(pixels, rowstride, n_channels,x, y)==oc; x++) {
				setpixel(pixels, rowstride, n_channels, x, y, nc);
			}
			FILL_PUSH(y, l, x-1, dy);
			/* leak on right? */
			if (x>x2+1) {
				FILL_PUSH(y, x2+1, x-1, -dy);
			}
skip:			for (x++; x<=x2 && (getpixel(pixels, rowstride, n_channels, x, y)!=oc); x++);

			l = x;
		} while (x<=x2);
	}

	g_free(stack);

done:{}
	printf("fill_shape end x: %d, y: %d, w: %d, h: %d\n"
		   "    fill: %X, bg: %X\n", x, y, width, height, nc, oc);
	return 1;
}



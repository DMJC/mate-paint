#ifndef GDK_LEGACY_COMPAT_H
#define GDK_LEGACY_COMPAT_H

#include <gtk/gtk.h>
#include <cairo.h>

#if GTK_MAJOR_VERSION >= 3

typedef cairo_surface_t GdkDrawable;
typedef cairo_surface_t GdkPixmap;
typedef cairo_surface_t GdkBitmap;

#ifndef GDK_LINE_SOLID
typedef enum {
	GDK_LINE_SOLID,
	GDK_LINE_ON_OFF_DASH,
	GDK_LINE_DOUBLE_DASH
} GdkLineStyle;
#endif

#ifndef GDK_CAP_NOT_LAST
typedef enum {
	GDK_CAP_NOT_LAST,
	GDK_CAP_BUTT,
	GDK_CAP_ROUND,
	GDK_CAP_PROJECTING
} GdkCapStyle;
#endif

#ifndef GDK_JOIN_MITER
typedef enum {
	GDK_JOIN_MITER,
	GDK_JOIN_ROUND,
	GDK_JOIN_BEVEL
} GdkJoinStyle;
#endif

#ifndef GDK_RGB_DITHER_NONE
typedef enum {
	GDK_RGB_DITHER_NONE,
	GDK_RGB_DITHER_NORMAL,
	GDK_RGB_DITHER_MAX
} GdkRgbDither;
#endif

typedef struct _GdkColormap GdkColormap;

typedef struct _GdkGC
{
	GdkColor foreground;
	GdkColor background;
	gint line_width;
	cairo_line_cap_t line_cap;
	cairo_line_join_t line_join;
} GdkGC;

typedef struct _GdkGCValues
{
	GdkColor foreground;
	GdkColor background;
	gint line_width;
} GdkGCValues;

static inline GdkGC *
gdk_gc_new (GdkDrawable *drawable)
{
	GdkGC *gc = g_new0 (GdkGC, 1);
	(void) drawable;
	gc->foreground.red = gc->foreground.green = gc->foreground.blue = 0;
	gc->background.red = gc->background.green = gc->background.blue = 65535;
	gc->line_width = 1;
	gc->line_cap = CAIRO_LINE_CAP_ROUND;
	gc->line_join = CAIRO_LINE_JOIN_ROUND;
	return gc;
}

static inline void gdk_gc_unref (GdkGC *gc) { g_free (gc); }

static inline void
gdk_gc_get_values (GdkGC *gc, GdkGCValues *values)
{
	g_return_if_fail (gc != NULL);
	g_return_if_fail (values != NULL);
	values->foreground = gc->foreground;
	values->background = gc->background;
	values->line_width = gc->line_width;
}

static inline void gdk_gc_set_rgb_fg_color (GdkGC *gc, const GdkColor *color) { if (gc && color) gc->foreground = *color; }
static inline void gdk_gc_set_rgb_bg_color (GdkGC *gc, const GdkColor *color) { if (gc && color) gc->background = *color; }

static inline void
gdk_gc_set_line_attributes (GdkGC *gc,
                            gint line_width,
                            GdkLineStyle line_style,
                            GdkCapStyle cap_style,
                            GdkJoinStyle join_style)
{
	(void) line_style;
	if (!gc)
		return;
	gc->line_width = MAX (line_width, 1);
	gc->line_cap = (cap_style == GDK_CAP_PROJECTING) ? CAIRO_LINE_CAP_SQUARE :
	               (cap_style == GDK_CAP_BUTT) ? CAIRO_LINE_CAP_BUTT :
	                                          CAIRO_LINE_CAP_ROUND;
	gc->line_join = (join_style == GDK_JOIN_BEVEL) ? CAIRO_LINE_JOIN_BEVEL :
	                (join_style == GDK_JOIN_MITER) ? CAIRO_LINE_JOIN_MITER :
	                                              CAIRO_LINE_JOIN_ROUND;
}

static inline cairo_t *
gdk_legacy_cairo_create (GdkDrawable *drawable)
{
	g_return_val_if_fail (drawable != NULL, NULL);
	return cairo_create (drawable);
}

#undef gdk_cairo_create
#define gdk_cairo_create gdk_legacy_cairo_create

static inline void
gdk_drawable_get_size (GdkDrawable *drawable, gint *width, gint *height)
{
	if (!drawable)
		return;
	if (width)
		*width = cairo_image_surface_get_width (drawable);
	if (height)
		*height = cairo_image_surface_get_height (drawable);
}

static inline GdkPixmap *
gdk_pixmap_new (GdkDrawable *drawable, gint width, gint height, gint depth)
{
	cairo_format_t format = CAIRO_FORMAT_ARGB32;
	(void) drawable;
	if (depth == 1)
		format = CAIRO_FORMAT_A1;
	else if (depth > 0 && depth <= 24)
		format = CAIRO_FORMAT_RGB24;
	return cairo_image_surface_create (format, width, height);
}

static inline void
legacy_set_source_from_gc (cairo_t *cr, const GdkGC *gc, gboolean foreground)
{
	const GdkColor *color = foreground ? &gc->foreground : &gc->background;
	cairo_set_source_rgb (cr,
	                      color->red / 65535.0,
	                      color->green / 65535.0,
	                      color->blue / 65535.0);
}

static inline void
gdk_draw_line (GdkDrawable *drawable, GdkGC *gc, gint x1, gint y1, gint x2, gint y2)
{
	cairo_t *cr = gdk_cairo_create (drawable);
	legacy_set_source_from_gc (cr, gc, TRUE);
	cairo_set_line_width (cr, MAX (gc->line_width, 1));
	cairo_set_line_cap (cr, gc->line_cap);
	cairo_set_line_join (cr, gc->line_join);
	cairo_move_to (cr, x1 + 0.5, y1 + 0.5);
	cairo_line_to (cr, x2 + 0.5, y2 + 0.5);
	cairo_stroke (cr);
	cairo_destroy (cr);
}

static inline void
gdk_draw_rectangle (GdkDrawable *drawable, GdkGC *gc, gboolean filled, gint x, gint y, gint width, gint height)
{
	cairo_t *cr = gdk_cairo_create (drawable);
	legacy_set_source_from_gc (cr, gc, TRUE);
	cairo_rectangle (cr, x, y, width, height);
	if (filled)
		cairo_fill (cr);
	else {
		cairo_set_line_width (cr, MAX (gc->line_width, 1));
		cairo_stroke (cr);
	}
	cairo_destroy (cr);
}

static inline void
gdk_draw_arc (GdkDrawable *drawable, GdkGC *gc, gboolean filled, gint x, gint y, gint width, gint height, gint angle1, gint angle2)
{
	cairo_t *cr = gdk_cairo_create (drawable);
	double xc = x + width / 2.0;
	double yc = y + height / 2.0;
	double rx = width / 2.0;
	double ry = height / 2.0;
	double a1 = angle1 / 64.0 * (G_PI / 180.0);
	double a2 = (angle1 + angle2) / 64.0 * (G_PI / 180.0);
	legacy_set_source_from_gc (cr, gc, TRUE);
	cairo_save (cr);
	cairo_translate (cr, xc, yc);
	cairo_scale (cr, rx, ry);
	cairo_arc (cr, 0.0, 0.0, 1.0, a1, a2);
	cairo_restore (cr);
	if (filled)
		cairo_fill (cr);
	else {
		cairo_set_line_width (cr, MAX (gc->line_width, 1));
		cairo_stroke (cr);
	}
	cairo_destroy (cr);
}

static inline void
gdk_draw_drawable (GdkDrawable *drawable,
                   GdkGC *gc,
                   GdkDrawable *src,
                   gint xsrc,
                   gint ysrc,
                   gint xdest,
                   gint ydest,
                   gint width,
                   gint height)
{
	cairo_t *cr = gdk_cairo_create (drawable);
	(void) gc;
	cairo_save (cr);
	cairo_rectangle (cr, xdest, ydest, width, height);
	cairo_clip (cr);
	cairo_set_source_surface (cr, src, xdest - xsrc, ydest - ysrc);
	cairo_paint (cr);
	cairo_restore (cr);
	cairo_destroy (cr);
}

static inline void
gdk_draw_pixbuf (GdkDrawable *drawable,
                 GdkGC *gc,
                 const GdkPixbuf *pixbuf,
                 gint src_x,
                 gint src_y,
                 gint dest_x,
                 gint dest_y,
                 gint width,
                 gint height,
                 GdkRgbDither dither,
                 gint x_dither,
                 gint y_dither)
{
	cairo_t *cr = gdk_cairo_create (drawable);
	(void) gc; (void) dither; (void) x_dither; (void) y_dither;
	gdk_cairo_set_source_pixbuf (cr, pixbuf, dest_x - src_x, dest_y - src_y);
	cairo_rectangle (cr, dest_x, dest_y, width, height);
	cairo_fill (cr);
	cairo_destroy (cr);
}

static inline GdkColormap *gdk_drawable_get_colormap (GdkDrawable *drawable) { (void) drawable; return NULL; }

#endif /* GTK_MAJOR_VERSION >= 3 */

#endif /* GDK_LEGACY_COMPAT_H */

## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GNOME_PAINT_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = \
	gnome-paint

EXTRA_DIST = $(glade_DATA)

gnome_paint_SOURCES = \
	color.c  \
	color.h  \
	main.c  \
	toolbar.c  \
	toolbar.h  \
	common.h  \
	cv_resize.c  \
	cv_resize.h  \
	cv_drawing.c  \
	cv_drawing.h  \
	cv_line_tool.c  \
	cv_line_tool.h  \
	file.c  \
	file.h  \
	pixbuf-file-chooser.c  \
	pixbuf-file-chooser.h  \
	pixmaps/line_0.xpm  \
	pixmaps/line_1.xpm  \
	pixmaps/line_2.xpm  \
	pixmaps/line_3.xpm  \
	pixmaps/line_4.xpm  \
	pixmaps/rect0.xpm  \
	pixmaps/rect1.xpm  \
	pixmaps/rect2.xpm  \
	pixmaps/sel1.xpm  \
	pixmaps/sel2.xpm  \
	cv_rectangle_tool.c  \
	cv_rectangle_tool.h  \
	cv_pencil_tool.c  \
	cv_pencil_tool.h  \
	cv_ellipse_tool.c  \
	cv_ellipse_tool.h

gnome_paint_LDFLAGS = \
	-Wl,--export-dynamic

gnome_paint_LDADD = \
	$(GNOME_PAINT_LIBS)

# Remove glade directory on uninstall
uninstall-local:
	-rm -r $(gladedir)
	-rm -r $(datadir)/gnome_paint

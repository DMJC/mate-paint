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
	pixmaps/line_0.xpm  \
	pixmaps/line_1.xpm  \
	pixmaps/line_2.xpm  \
	pixmaps/line_3.xpm  \
	pixmaps/line_4.xpm  \
	canvas.c  \
	canvas.h  \
	common.h  \
	tool_line.c  \
	tool_line.h

gnome_paint_LDFLAGS = \
	-Wl,--export-dynamic

gnome_paint_LDADD = \
	$(GNOME_PAINT_LIBS)

# Remove glade directory on uninstall
uninstall-local:
	-rm -r $(gladedir)
	-rm -r $(datadir)/gnome_paint

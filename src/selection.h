/***************************************************************************
 *            selection.h
 *
 *  Sat Jun 19 16:12:19 2010
 *  Copyright  2010  rogerio
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
 
#ifndef __SELECTION_H__
#define __SELECTION_H__

#include <gtk/gtk.h>

void            gp_selection_init                       ( void );
void            gp_selection_clear                      ( void );
void            gp_selection_clipbox_set_start_point    ( GdkPoint *p );
void            gp_selection_clipbox_set_end_point      ( GdkPoint *p );
void            gp_selection_set_active                 ( gboolean active );
void            gp_selection_set_borders                ( gboolean borders );
void            gp_selection_set_floating               ( gboolean floating );
GdkCursorType   gp_selection_get_cursor                 ( GdkPoint *p );
gboolean        gp_selection_start_action               ( GdkPoint *p );
void            gp_selection_do_action                  ( GdkPoint *p );
void            gp_selection_draw                       ( GdkDrawable *gdkd );

gboolean        gp_selection_query                      ( void );
gboolean		gp_selection_create						( GdkPoint *s,
														  GdkPoint *e,
														  GdkPixbuf *pixbuf );
void			gp_selection_draw_and_clear				( gboolean draw );

#endif /*__SELECTION_H__*/


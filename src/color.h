/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * color.h
 * Copyright (C) Rogério Ferro do Nascimento 2009 <rogerioferro@gmail.com>
 * 
 * color.h is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * color.h is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>



void background_set_widget				( GtkWidget *widget );
void background_color_picker			( void );
void background_set_color_from_palette  ( guint palette );

void foreground_set_widget				( GtkWidget *widget );
void foreground_color_picker			( void );
void foreground_set_color_from_palette  ( guint palette );

void palette_set_widget					( guint palette, GtkWidget *widget );
void palette_color_picker				( guint palette );





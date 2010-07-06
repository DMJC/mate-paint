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

#ifndef _PAINTBRUSH_TOOL_H_
#define _PAINTBRUSH_TOOL_H_

#include "common.h"

typedef enum{
	GP_BRUSH_RECT_LARGE,
	GP_BRUSH_RECT_MEDIUM,
	GP_BRUSH_RECT_SMALL,
	GP_BRUSH_ROUND_LARGE,
	GP_BRUSH_ROUND_MEDIUM,
	GP_BRUSH_ROUND_SMALL,
	GP_BRUSH_FWRD_LARGE,
	GP_BRUSH_FWRD_MEDIUM,
	GP_BRUSH_FWRD_SMALL,
	GP_BRUSH_BACK_LARGE,
	GP_BRUSH_BACK_MEDIUM,
	GP_BRUSH_BACK_SMALL,
	GP_BRUSH_MAX
}GPBrushSize;

gp_tool * tool_paintbrush_init ( gp_canvas * canvas );
void notify_brush_of_fg_color_change(void);

void on_brush_size_toggled(GtkWidget *widget, gpointer data);

#endif

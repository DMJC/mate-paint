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

#ifndef _ERASER_TOOL_H_
#define _ERASER_TOOL_H_

#include "common.h"

typedef enum{
	GP_ERASER_RECT_LARGE,
	GP_ERASER_RECT_MEDIUM,
	GP_ERASER_RECT_SMALL,
	GP_ERASER_RECT_TINY,
	GP_ERASER_ROUND_LARGE,
	GP_ERASER_ROUND_MEDIUM,
	GP_ERASER_ROUND_SMALL,
	GP_ERASER_ROUND_TINY
}GPEraserSize;

gp_tool * tool_eraser_init ( gp_canvas * canvas );
void notify_eraser_of_bg_color_change(void);
void on_eraser_size_toggled(GtkWidget *widget, gpointer data);
#endif

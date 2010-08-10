/***************************************************************************
 *            clipboard.c
 *
 *  Sun May 23 16:37:33 2010
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

#include "clipboard.h"
#include "cv_drawing.h"
#include "selection.h"

void 
on_menu_copy_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
    GdkPixbuf       *pixbuf;
    GtkClipboard    *clipboard;
//    gp_canvas *cv = cv_get_canvas ();

    clipboard   =   gtk_clipboard_get_for_display (gdk_display_get_default (),
	                                             GDK_SELECTION_CLIPBOARD);

    g_return_if_fail ( GTK_IS_CLIPBOARD(clipboard) );

    if(gp_selection_query())
    {
    	pixbuf = GDK_PIXBUF(gp_selection_get_pixbuf());
    }
    else
    {
	    pixbuf = cv_get_pixbuf ();
	}

    gtk_clipboard_set_image ( clipboard, pixbuf );

    g_object_unref(pixbuf);
}

void 
on_menu_paste_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
    GtkClipboard    *clipboard;
    gp_canvas *cv = cv_get_canvas ();
    GtkWidget *wid;
    GdkPixbuf *tmp;

    clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
	                                             GDK_SELECTION_CLIPBOARD);
    
    tmp = gtk_clipboard_wait_for_image ( clipboard );
    
    g_return_if_fail ( GDK_IS_PIXBUF(tmp) );
    
	/* Delete old selection if any */
    if(GDK_IS_PIXBUF ( cv->pb_clipboard ) )
    {
    	g_object_unref(cv->pb_clipboard);
    }

    if(!gdk_pixbuf_get_has_alpha(tmp)){
	    cv->pb_clipboard = gdk_pixbuf_add_alpha(tmp, FALSE, 0, 0, 0);
	    g_object_unref(tmp);
	}
	else{
		cv->pb_clipboard = tmp;
	}
    
    wid = GTK_WIDGET(g_object_get_data(G_OBJECT(cv->widget), "tool-rect-select"));
    /* The rect sel tool is not active, toggle it
     * Selection will be created in tool_rect_select_init ()
     */
    if ( !gtk_toggle_tool_button_get_active ( GTK_TOGGLE_TOOL_BUTTON(wid) ) )
    {
    	gtk_toggle_tool_button_set_active ( GTK_TOGGLE_TOOL_BUTTON(wid), TRUE );
    }
	else
	{
		GdkPoint e, s = {0, 0};

		e.x = gdk_pixbuf_get_width(cv->pb_clipboard);
		e.x += s.x;
		e.y = gdk_pixbuf_get_height(cv->pb_clipboard);
		e.y += s.y;
		/* Create a selection in the upper left corner of canvas */
		gp_selection_create (&s, &e, cv->pb_clipboard);
		g_object_unref(cv->pb_clipboard);
		cv->pb_clipboard = NULL;
	}

}

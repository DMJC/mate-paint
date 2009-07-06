/***************************************************************************
 *            file.c
 *
 *  Thu Jun 11 13:19:47 2009
 *  Copyright  2009  rogerio
 *  <rogerio@<host>>
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

#include "common.h"

#include "file.h"

#include "pixbuf-file-chooser.h"


/*private functions*/
void file_name_free	(gpointer data);

/*private data*/
GtkWindow 	*parent_window	=	NULL;
GString 	*file_name		=	NULL;


/*
 *   CODE
 */

void 
file_set_parent_window	( GtkWindow * wnd )
{
	parent_window	=	wnd;
	file_name		=	g_string_new("");	
	g_assert( file_name );
	g_object_set_data_full (	G_OBJECT(parent_window), "file_name",
			               		(gpointer)file_name , 
			                	(GDestroyNotify)file_name_free );
	file_set_file_name( NULL );
}

void 
file_set_file_name	( const gchar *name )
{
	GString *title	= g_string_new("");
	g_string_assign ( file_name, (name == NULL)?_("Untitled document"):name );
	g_string_printf ( title, _("%s - gnome-paint"), file_name->str );
	gtk_window_set_title ( parent_window, title->str);
	g_string_free( title, TRUE );
}



void 
on_menu_save_activate	( GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWidget *dialog;
	GFile *save_file = NULL;
	gint response;

	dialog = pixbuf_file_chooser_new (GTK_FILE_CHOOSER_ACTION_SAVE);
	pixbuf_file_chooser_set_current_filter( PIXBUF_FILE_CHOOSER (dialog), "png");
	gtk_file_chooser_set_current_name ( GTK_FILE_CHOOSER (dialog), file_name->str);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) 
	{
		gchar * name				= pixbuf_file_chooser_get_name (PIXBUF_FILE_CHOOSER(dialog));
		GdkPixbufFormat * format	= pixbuf_file_chooser_get_format (PIXBUF_FILE_CHOOSER(dialog));
		gchar * type				= gdk_pixbuf_format_get_name (format);
		gchar * filename			= gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		g_print("%s\n",name);

		file_set_file_name( name );
		g_free (name);

		cv_save_file (filename, type);
		
		g_print("%s\n",filename);
		g_free (filename);
		g_free (type);
	}
	gtk_widget_destroy (dialog);
}


/*private function*/
void  
file_name_free(gpointer data)
{
	g_string_free( ( GString * )data, TRUE );
}





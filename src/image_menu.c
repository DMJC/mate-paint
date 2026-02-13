/***************************************************************************
	Copyright (C) Rogério Ferro do Nascimento 2010 <rogerioferro@gmail.com>
	Copyright (C) Juan Balderas 2010

	image_menu.c is part of mate-paint.

	mate-paint is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	mate-paint is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with mate-paint.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <gtk/gtk.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <ctype.h>

#include "common.h"
#include "cv_drawing.h"
#include "selection.h"
#include "image_menu.h"
#include "gp-image.h"
#include "undo.h"

typedef enum{
	GP_FILP_VERT = 0,
	GP_FILP_HORZ = 1,
	GP_ROTATE = 2
}GPEffectType;

typedef enum{
	in,
	cm,
	px
}GPUnitType;

typedef struct{
	GtkWidget		*active;
	GPEffectType	effect;
	gint			degrees;
}GPFlipRotateDlg;

//typedef struct{
//	gdouble w; /* Image width in pixels */
//	gdouble h; /* Image height in pixels */
//	guint vdpi; /* Vertical dpi */
//	guint hdpi; /* Horizontal dpi */
//	gboolean color;
//}GPAttributesDlg;

static void get_dpi(gint *x, gint *y);
static void attributes_dlg_display_size(guint type, gboolean from_drawable);
static gdouble entry_get_number(GtkWidget *widget);
static gdouble convert_units (int from, int to, gdouble d, guint Dpi);

static GPFlipRotateDlg m_fr = {NULL, GP_FILP_HORZ, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE};

static m_cur_unit = px;
static m_prev_unit = px;

/************** Flip and rotate stuff **************************************/
void
on_menu_flip_rotate_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	gint response;
	GtkWidget *dialog;
	GtkWidget *widget90, *widget180, *widget270;
	GtkWidget *flip_vert, *flip_horz, *rotate;
	gp_canvas *cv;
	
	printf("show_flip_rotate_dialog()\n");
	
    cv = (gp_canvas *)cv_get_canvas ( );

	widget90  = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate_90"));
	widget180 = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate_180"));
	widget270 = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate_270"));
	rotate    = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate"));

    if(!gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( rotate )))
    {
	    gtk_widget_set_sensitive (widget90, FALSE);
		gtk_widget_set_sensitive (widget180, FALSE);
		gtk_widget_set_sensitive (widget270, FALSE);
	}
	else{
		gtk_widget_set_sensitive (widget90, TRUE);
		gtk_widget_set_sensitive (widget180, TRUE);
		gtk_widget_set_sensitive (widget270, TRUE);
	}
    
    dialog = g_object_get_data(G_OBJECT(cv->widget), "flip_roate_dialog");

	response = gtk_dialog_run (GTK_DIALOG (dialog));
    printf("  degrees: %d, effect: %d\n", m_fr.degrees, m_fr.effect);
    
    if(GTK_RESPONSE_OK == response)
    {
    	if(gp_selection_query())
    	{
    		switch(m_fr.effect)
    		{
    			case GP_FILP_VERT:
    			case GP_FILP_HORZ:
	    			gp_selection_flip(m_fr.effect);
	    			break;
	    		case GP_ROTATE:
	    			gp_selection_rotate(m_fr.degrees);
	    			break;
    		}
    		gtk_widget_queue_draw ( cv->widget );
    	}
    	else
    	{
    		/* Apply effect to canvas */
    		GpImage *image;
    		GdkPixbuf *pixbuf;
			gp_canvas *cv = cv_get_canvas ( );
			GdkRectangle rect = {0, 0, 0, 0};

    		pixbuf = cv_get_pixbuf();
    		rect.width = gdk_pixbuf_get_width (pixbuf);
    		rect.height = gdk_pixbuf_get_height (pixbuf);
    		printf("old w: %d, h: %d\n", rect.width, rect.height);
    		image = gp_image_new_from_pixbuf ( pixbuf, TRUE );
    		g_object_unref ( pixbuf );

    		switch(m_fr.effect)
    		{
    			case GP_FILP_VERT:
    			case GP_FILP_HORZ:
    				undo_add (&rect, NULL, NULL, TOOL_FLIP_CANVAS );
	    			gp_image_flip(image, m_fr.effect);
	    			break;
	    		case GP_ROTATE:
	    			undo_add (&rect, NULL, NULL, TOOL_ROTATE_CANVAS );
	    			gp_image_rotate(image, m_fr.degrees);
	    			break;
    		}
			pixbuf = gp_image_get_pixbuf ( image );
			cv_set_pixbuf ( pixbuf );

			g_object_unref ( pixbuf );
			g_object_unref ( image );
    	}
    }

    gtk_widget_hide( dialog );
}

void on_radiobutton_flip_horizontal_toggled (GtkRadioButton *object, gpointer user_data)
{
	printf("radiobutton_flip_horizontal_toggled()\n");
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_fr.effect = GP_FILP_HORZ;
		printf("    toggled()\n");
	}
}

void on_radiobutton_flip_vertical_toggled (GtkRadioButton *object, gpointer user_data)
{
	printf("on_radiobutton_flip_vertical_toggled()\n");
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_fr.effect = GP_FILP_VERT;
		printf("    toggled()\n");
	}
}

void on_radiobutton_rotate_toggled (GtkRadioButton *object, gpointer user_data)
{
	GtkWidget *widget90, *widget180, *widget270;
	gp_canvas *cv = cv_get_canvas();

	widget90 = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate_90"));
	widget180 = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate_180"));
	widget270 = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget), "radiobutton_rotate_270"));

	printf("on_radiobutton_rotate_toggled()\n");
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_fr.effect = GP_ROTATE;
		
		gtk_widget_set_sensitive (widget90, TRUE);
		gtk_widget_set_sensitive (widget180, TRUE);
		gtk_widget_set_sensitive (widget270, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (widget90, FALSE);
		gtk_widget_set_sensitive (widget180, FALSE);
		gtk_widget_set_sensitive (widget270, FALSE);
	}
}

void on_radiobutton_rotate_90_toggled (GtkRadioButton *object, gpointer user_data)
{
	printf("on_radiobutton_rotate_90_toggled()\n");
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_fr.degrees = GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE;
		printf("    toggled: %d\n", m_fr.degrees);
	}
}

void on_radiobutton_rotate_180_toggled (GtkRadioButton *object, gpointer user_data)
{
	printf("on_radiobutton_rotate_180_toggled()\n");
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_fr.degrees = GDK_PIXBUF_ROTATE_UPSIDEDOWN;
		printf("    toggled: %d\n", m_fr.degrees);
	}
}

void on_radiobutton_rotate_270_toggled (GtkRadioButton *object, gpointer user_data)
{
	printf("on_radiobutton_rotate_270_toggled()\n");
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_fr.degrees = GDK_PIXBUF_ROTATE_CLOCKWISE;
		printf("    toggled: %d\n", m_fr.degrees);
	}
}

/************** Stretch/Skew stuff *****************************************/
void on_menu_stretch_skew_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	gint width, height, percent = 100;
	/* Resize formula */
	if(100 != percent)
		width = percent / 100 * width;

	if(100 != percent)
		height = percent / 100 * height;

	printf("stretch/skew not implemented\n");
}

/************** Invert menu item *******************************************/
void on_menu_invert_colors_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	GpImage *image;
	GdkRectangle rect = {0, 0, 0, 0};
	gp_canvas *cv = cv_get_canvas ( );
	GdkPixbuf *pixbuf;

	if(gp_selection_query())
    {
    	/* Apply effect to selection */
    	printf("on_menu_invert_colors_activate()\n");
    	gp_selection_invert();
    	/* Update the selection */
		gtk_widget_queue_draw ( cv->widget );
	}
	else
	{
		/* Apply effect to canvas */
		printf("on_menu_invert_colors_activate()\n");
		image = gp_image_new_from_pixmap( cv->pixmap, NULL, TRUE);
		gp_image_invert_colors ( image);

		gdk_drawable_get_size(cv->pixmap, &rect.width, &rect.height);
		undo_add (&rect, NULL, NULL, TOOL_CLEAR_CANVAS );
		
		pixbuf = gp_image_get_pixbuf ( image );
		cv_set_pixbuf ( pixbuf );
		g_object_unref ( pixbuf );
		g_object_unref ( image );
	}
}

/************** Clear image item *******************************************/
void on_menu_clear_image_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	GdkRectangle rect = {0, 0, 0, 0};
	gp_canvas *cv;
	GdkPixbuf *pixbuf;

	printf("on_menu_clear_image_activate()\n");
	
	/* Clear is only for canvas */
	g_return_if_fail(!gp_selection_query() );
	
	cv = cv_get_canvas ( );
	gdk_drawable_get_size(cv->pixmap, &rect.width, &rect.height);

	undo_add (&rect, NULL, NULL, TOOL_CLEAR_CANVAS );
	
	/* "Clear" canvas with bg color */
	gdk_draw_rectangle(cv->pixmap, cv->gc_bg, TRUE, 0, 0,
					   rect.width, rect.height);

	pixbuf = cv_get_pixbuf ( );
	cv_set_pixbuf ( pixbuf );
	g_object_unref ( pixbuf );
}

/************** Toggle opaque/transparent **********************************/
void on_menu_draw_opaque_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	gp_canvas *cv = cv_get_canvas();
	GtkWidget *wid;
	
	printf("on_menu_draw_opaque_activate() ");
	

	if(gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(menuitem)))
	{
		printf("  checked\n");
		wid = GTK_WIDGET( g_object_get_data(G_OBJECT(cv->widget), "sel1") );
		gtk_toggle_tool_button_set_active ( GTK_TOGGLE_TOOL_BUTTON(wid), TRUE );
	}
	else
	{
		printf("  unchecked\n");
		wid = GTK_WIDGET( g_object_get_data(G_OBJECT(cv->widget), "sel2") );
		gtk_toggle_tool_button_set_active ( GTK_TOGGLE_TOOL_BUTTON(wid), TRUE );
	}
	gtk_widget_queue_draw( cv->widget );
}

/************** Attributes dlg stuff ***************************************/
/* TODO: Show file size & last saved. Make grey scaled
 */
void on_menu_attributes_activate ( GtkMenuItem *menuitem, gpointer user_data )
{
	gint response;
	GtkWidget *dialog, *radio_inch, *radio_cm, *width, *height;
	gp_canvas *cv = cv_get_canvas();
	gdouble dw, dh, xdpi, ydpi;
	gdouble ow, oh; /* old height & width */
	gchar *text = NULL;
	gint w, h;

	if( gp_selection_query() )
	{
		gp_selection_draw_and_clear ( TRUE );
	}
	
	dialog = g_object_get_data(G_OBJECT(cv->widget), "dialog_attributes");
	radio_inch = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_radiobutton1"));
	radio_cm = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_radiobutton2"));
	width = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_entry1"));
	height = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_entry2"));

	gdk_drawable_get_size(cv->pixmap, &w, &h);
	ow = (double)w ; oh = (double)h;

	m_prev_unit = px;

	if( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( radio_inch ) ))
	{
		m_cur_unit = in;
		attributes_dlg_display_size(0, TRUE);
	}
	else if( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( radio_cm ) ))
	{
		m_cur_unit = cm;
		attributes_dlg_display_size(1, TRUE);
	}
	else /* pixels */
	{
		m_cur_unit = px;
		attributes_dlg_display_size(2, TRUE);
	}

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	if( GTK_RESPONSE_OK == response )
	{
		dw = entry_get_number(width);
		dh = entry_get_number(height);
		
		get_dpi( &w, &h );
		xdpi = (double)w ; ydpi = (double)h;

		if(gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( radio_inch ) ))
		{
			dw = dw * xdpi;
			dh = dh * ydpi;
		}
		else if(gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( radio_cm ) ))
		{
			dw = (dw * xdpi) / 2.54;
			dh = (dh * ydpi) / 2.54;
		}
		else
		{
			/* sizes in pixels */
		}

		printf("Old size: %.02f %.02f\n", ow, oh);
		printf("New size: %.02f %.02f\n", dw, dh);/* Limit size < 2000 */
		if( ((dw > 0) && (dw < 2000)) && ((dh > 0) && (dh < 2000)) )
		{
			if( !(dw == ow && dh == oh) ) /* Resize only if prev size not same */
			{
					undo_add_resize ( (gint)dw, (gint)dh );
					cv_resize_pixmap ( (gint)dw, (gint)dh );
			}
		}
	}
	
	gtk_widget_hide( dialog );
}

/* Inches */
void on_attributes_radiobutton1_toggled (GtkRadioButton *object, gpointer user_data)
{
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_cur_unit = in;
		attributes_dlg_display_size(0, FALSE);
	}
	else
	{
		m_prev_unit = in;
	}
}

/* Centimeters */
void on_attributes_radiobutton2_toggled (GtkRadioButton *object, gpointer user_data)
{
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_cur_unit = cm;
		attributes_dlg_display_size(1, FALSE);
	}
	else
	{
		m_prev_unit = cm;
	}
}

/* Pixels */
void on_attributes_radiobutton3_toggled (GtkRadioButton *object, gpointer user_data)
{
	if ( gtk_toggle_button_get_active ( GTK_TOGGLE_BUTTON( object ) ) )
	{
		m_cur_unit = px;
		attributes_dlg_display_size(2, FALSE);
	}
	else
	{
		m_prev_unit = px;
	}
}

/* Display image size in inches, cm, or pixels 
 * in attributes dlg width & height text entrys,
 * as well as display dpi in label
 */
static void attributes_dlg_display_size(guint type, gboolean from_drawable)
{
	GtkWidget *entry_width, *entry_height, *label_dpi;
	gp_canvas *cv = cv_get_canvas();
	gchar wtext[80], htext[80], disp[80];
	guint w, h, vdpi, hdpi ;
	gdouble dw, dh, dvdpi, dhdpi;

	entry_width = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_entry1"));
	entry_height = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_entry2"));
	label_dpi = GTK_WIDGET (g_object_get_data(G_OBJECT(cv->widget),
							"attributes_label6"));

	gtk_entry_set_max_length (GTK_ENTRY (entry_width), 5);
	gtk_entry_set_max_length (GTK_ENTRY (entry_height), 5);

	get_dpi(&vdpi, &hdpi);
	dvdpi = (gdouble)vdpi; dhdpi = (gdouble)hdpi;

	/* Display from drawable's size */
	if(from_drawable)
	{
		gdk_drawable_get_size(cv->pixmap, (gint *)&w, (gint *)&h);
		dw = (gdouble)w ; dh = (gdouble)h ;
	}
	else
	{
		dw = entry_get_number(entry_width);
		dh = entry_get_number(entry_height);
	}

	dw = convert_units (m_prev_unit, m_cur_unit, dw, dvdpi);
	dh = convert_units (m_prev_unit, m_cur_unit, dh, dhdpi);

	switch(type)
	{
		case 0: /* inches */
				/* Fall through */
		case 1: /* centimeters */
			sprintf(wtext, "%.02f", dw );
			sprintf(htext, "%.02f", dh );
			break;
		default: /* pixels */
			sprintf(wtext, "%d", (gint)dw );
			sprintf(htext, "%d", (gint)dh );
			break;
	}

	gtk_entry_set_text ( GTK_ENTRY (entry_width), wtext );
	gtk_entry_set_text ( GTK_ENTRY (entry_height), htext );
	
	sprintf(disp, "%d x %d dots per inch", vdpi, hdpi);
	gtk_label_set_text ( GTK_LABEL (label_dpi), disp );
}

static gdouble entry_get_number(GtkWidget *widget)
{	
	gchar *str = NULL;
	gdouble d = 0.0;

	g_return_val_if_fail(GTK_IS_ENTRY(widget), 0);

	str = (gchar *)gtk_entry_get_text (GTK_ENTRY(widget));
	g_return_val_if_fail(str != NULL, 0);
	sscanf(str, "%lf", &d);
	return d;
}

/* Convert between pixels inches cm */
static gdouble convert_units (int from, int to, gdouble d, guint Dpi)
{
	gdouble dpi = (gdouble)Dpi;
	
	if(from == to){ printf("convert_units() Same! unit: %d\n", to); return d ; }
	
	switch(from)
	{
		case in: /* in to pixels */
			printf("from in ");
			d = d * dpi;
			break;
		case cm: /* cm to pixels */
			printf("from cm ");
			d = (d * dpi) / 2.54;
			break;
		case px: /* Fall through */
			printf("from px ");
		default:
			break;
	}
	
	switch(to)
	{
		case in: /* pix to in */
			printf("to in");
			d = d / dpi;
			break;
		case cm: /* pix to cm */
			printf("to cm");
			d = (d * 2.54) / dpi;
			break;
		case px: /* Fall through */
			printf("to px");
		default:
			printf(" %f ->", d);
			d += 0.5;
			break;
	}
	
	printf(" %f\n", d);
	
	return d;
}

/* Make text entries accept numbers & periods only */
/* TODO: Gtk doc says event->string is deprecated */
gboolean on_attributes_entry_key_press_event(GtkWidget *widget, 
											 GdkEventKey *event,
											 gpointer     user_data)
{
	if( ( isprint(event->string[0])  ) && 
	    ( !isdigit(event->string[0]) ) &&
	    ( '.' != event->string[0]    ) )
	{
		return TRUE;
	}
	return FALSE;
}

/* Get dots per inch 
 * Modified from xdpyinfo
 * TODO: Make sure we have right screen
 */
static void get_dpi(gint *x, gint *y)
{
	double xres, yres;
	Display *dpy;
	char *displayname = NULL;
	int scr = 0; /* Screen number */

	g_return_if_fail((NULL != x) && (NULL != y));

	dpy = XOpenDisplay (displayname);

	/*
     * there are 2.54 centimeters to an inch; so there are 25.4 millimeters.
     *
     *     dpi = N pixels / (M millimeters / (25.4 millimeters / 1 inch))
     *         = N pixels / (M inch / 25.4)
     *         = N * 25.4 pixels / M inch
     */
    xres = ((((double) DisplayWidth(dpy,scr)) * 25.4) / 
	    ((double) DisplayWidthMM(dpy,scr)));
    yres = ((((double) DisplayHeight(dpy,scr)) * 25.4) / 
	    ((double) DisplayHeightMM(dpy,scr)));

	*x = (int) (xres + 0.5);
	*y = (int) (yres + 0.5);

	XCloseDisplay (dpy);
}

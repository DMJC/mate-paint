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
 
void canvas_set_widget		 	( GtkWidget *widget );


void cv_right_set_widget		( GtkWidget *widget );

void cv_bottom_right_set_widget	( GtkWidget *widget );
void cv_bottom_right_start		( void );
void cv_bottom_right_move		( gdouble x,  gdouble y);
void cv_bottom_right_stop		( gdouble x,  gdouble y);

void cv_bottom_set_widget		( GtkWidget *widget );

void cv_other_edge_set_widget	( GtkWidget *widget );
void cv_other_edge_draw 		( GtkWidget *widget );

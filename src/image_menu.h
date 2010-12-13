
#ifndef _IMAGE_MENU_H_
#define _IMAGE_MENU_H_

/* Menu callbacks */
void on_menu_invert_colors_activate         ( GtkMenuItem *menuitem,
                                              gpointer user_data );
void on_menu_flip_rotate_activate			( GtkMenuItem *menuitem,
											  gpointer user_data );

/* Flip & rotate callbacks */
void on_radiobutton_flip_horizontal_toggled	( GtkRadioButton *object,
											  gpointer user_data );
void on_radiobutton_flip_vertical_toggled	( GtkRadioButton *object,
											  gpointer user_data );
void on_radiobutton_rotate_toggled			( GtkRadioButton *object,
											  gpointer user_data );
void on_radiobutton_rotate_90_toggled		( GtkRadioButton *object,
											  gpointer user_data );
void on_radiobutton_rotate_180_toggled		( GtkRadioButton *object,
											  gpointer user_data );
void on_radiobutton_rotate_270_toggled		( GtkRadioButton *object,
											  gpointer user_data );
void on_menu_clear_image_activate			( GtkMenuItem *menuitem,
											  gpointer user_data );
void on_menu_draw_opaque_activate           ( GtkMenuItem *menuitem,
                                              gpointer user_data );

/* Attributes dialog */
/* Inches */
void on_attributes_radiobutton1_toggled     ( GtkRadioButton *object,
                                              gpointer user_data);
/* Mili meters */
void on_attributes_radiobutton2_toggled     ( GtkRadioButton *object,
                                              gpointer user_data);
/* Pixels */
void on_attributes_radiobutton3_toggled     ( GtkRadioButton *object, 
                                              gpointer user_data);

/* Make text entrys accept numbers & periods only */
gboolean on_attributes_entry_key_press_event(GtkWidget *widget, 
											 GdkEventKey *event,
											 gpointer     user_data);

#endif

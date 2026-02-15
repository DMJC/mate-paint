// drawing_app.cpp
// Compile with: g++ drawing_app.cpp -o drawing_app `pkg-config --cflags --libs gtk+-3.0` -std=c++11

#include <gtk/gtk.h>
#include <cairo.h>
#include <vector>
#include <cmath>
#include <memory>
#include <string>

// Tool types
enum Tool {
    TOOL_LASSO_SELECT,
    TOOL_RECT_SELECT,
    TOOL_ERASER,
    TOOL_FILL,
    TOOL_EYEDROPPER,
    TOOL_ZOOM,
    TOOL_PENCIL,
    TOOL_PAINTBRUSH,
    TOOL_AIRBRUSH,
    TOOL_TEXT,
    TOOL_LINE,
    TOOL_CURVE,
    TOOL_RECTANGLE,
    TOOL_POLYGON,
    TOOL_ELLIPSE,
    TOOL_ROUNDED_RECT
};

// Forward declarations
void update_color_indicators();
void save_image_dialog(GtkWidget* parent);
void open_image_dialog(GtkWidget* parent);
void clear_selection();
void copy_selection();
void cut_selection();
void paste_selection();
void finalize_text();
void cancel_text();
void update_text_box_size();

// Application state
struct AppState {
    Tool current_tool = TOOL_PENCIL;
    GdkRGBA fg_color = {0.0, 0.5, 0.0, 1.0}; // Green
    GdkRGBA bg_color = {1.0, 1.0, 1.0, 1.0}; // White
    cairo_surface_t* surface = nullptr;
    int canvas_width = 800;
    int canvas_height = 600;
    double last_x = 0;
    double last_y = 0;
    bool is_drawing = false;
    bool is_right_button = false;
    bool shift_pressed = false;
    double line_width = 2.0;
    
    // For shape tools and preview
    double start_x = 0;
    double start_y = 0;
    double current_x = 0;
    double current_y = 0;
    std::vector<std::pair<double, double>> polygon_points;
    std::vector<std::pair<double, double>> lasso_points;
    
    // Selection state
    bool has_selection = false;
    bool selection_is_rect = false;
    double selection_x1 = 0;
    double selection_y1 = 0;
    double selection_x2 = 0;
    double selection_y2 = 0;
    std::vector<std::pair<double, double>> selection_path;
    
    // Text tool state
    bool text_active = false;
    double text_x = 0;
    double text_y = 0;
    double text_box_width = 200;
    double text_box_height = 100;
    std::string text_content;
    std::string text_font_family = "Sans";
    int text_font_size = 14;
    GtkWidget* text_window = nullptr;
    GtkWidget* text_entry = nullptr;
    
    // Clipboard
    cairo_surface_t* clipboard_surface = nullptr;
    int clipboard_width = 0;
    int clipboard_height = 0;
    
    // Ant path animation
    double ant_offset = 0;
    guint ant_timer_id = 0;
    
    // UI elements
    GtkWidget* fg_button = nullptr;
    GtkWidget* bg_button = nullptr;
    GtkWidget* drawing_area = nullptr;
    GtkWidget* window = nullptr;
    
    std::string current_filename;
};

AppState app_state;

// Color palette
const GdkRGBA palette_colors[] = {
    {0.0, 0.5, 0.0, 1.0},   // Green
    {0.0, 0.0, 0.0, 1.0},   // Black
    {0.2, 0.2, 0.2, 1.0},   // Dark gray
    {0.5, 0.5, 0.5, 1.0},   // Gray
    {0.5, 0.0, 0.0, 1.0},   // Dark red
    {0.8, 0.0, 0.0, 1.0},   // Red
    {1.0, 0.4, 0.0, 1.0},   // Orange
    {1.0, 0.8, 0.0, 1.0},   // Yellow-orange
    {1.0, 1.0, 0.0, 1.0},   // Yellow
    {0.8, 1.0, 0.0, 1.0},   // Yellow-green
    {0.0, 1.0, 0.0, 1.0},   // Bright green
    {0.0, 1.0, 0.5, 1.0},   // Cyan-green
    {0.0, 1.0, 1.0, 1.0},   // Cyan
    {0.0, 0.5, 1.0, 1.0},   // Light blue
    {0.0, 0.0, 1.0, 1.0},   // Blue
    {0.5, 0.0, 1.0, 1.0},   // Purple
    {0.8, 0.0, 0.8, 1.0},   // Magenta
    {1.0, 1.0, 1.0, 1.0},   // White
    {0.7, 0.7, 0.7, 1.0},   // Light gray
    {0.4, 0.2, 0.0, 1.0},   // Brown
    {1.0, 0.7, 0.7, 1.0},   // Light pink
    {1.0, 0.9, 0.7, 1.0},   // Cream
    {1.0, 1.0, 0.8, 1.0},   // Light yellow
    {0.8, 1.0, 0.8, 1.0},   // Light green
    {0.7, 0.9, 1.0, 1.0},   // Light cyan
    {0.7, 0.7, 1.0, 1.0},   // Light blue
    {0.9, 0.7, 1.0, 1.0},   // Light purple
};

// Check if tool needs preview
bool tool_needs_preview(Tool tool) {
    return tool == TOOL_LASSO_SELECT || tool == TOOL_RECT_SELECT ||
           tool == TOOL_LINE || tool == TOOL_CURVE ||
           tool == TOOL_RECTANGLE || tool == TOOL_POLYGON ||
           tool == TOOL_ELLIPSE || tool == TOOL_ROUNDED_RECT;
}

// Check if point is inside selection
bool point_in_selection(double x, double y) {
    if (!app_state.has_selection) return false;
    
    if (app_state.selection_is_rect) {
        double x1 = fmin(app_state.selection_x1, app_state.selection_x2);
        double y1 = fmin(app_state.selection_y1, app_state.selection_y2);
        double x2 = fmax(app_state.selection_x1, app_state.selection_x2);
        double y2 = fmax(app_state.selection_y1, app_state.selection_y2);
        return x >= x1 && x <= x2 && y >= y1 && y <= y2;
    }
    
    return false;
}

// Check if point is inside text box
bool point_in_text_box(double x, double y) {
    if (!app_state.text_active) return false;
    return x >= app_state.text_x && x <= app_state.text_x + app_state.text_box_width &&
           y >= app_state.text_y && y <= app_state.text_y + app_state.text_box_height;
}

// Calculate required text box size based on content and font
void update_text_box_size() {
    if (!app_state.text_active) return;
    
    // Create a temporary cairo surface for measurements
    cairo_surface_t* temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* cr = cairo_create(temp_surface);
    
    // Set font
    cairo_select_font_face(cr, app_state.text_font_family.c_str(), 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, app_state.text_font_size);
    
    // Calculate size needed for text
    double max_width = 200.0; // Minimum width
    double total_height = app_state.text_font_size + 10; // Start with padding
    
    if (!app_state.text_content.empty()) {
        std::string text = app_state.text_content;
        cairo_text_extents_t extents;
        std::string word;
        std::string line;
        double line_width = 0;
        int line_count = 1;
        
        for (size_t i = 0; i <= text.length(); i++) {
            if (i == text.length() || text[i] == ' ' || text[i] == '\n') {
                if (!word.empty()) {
                    std::string test_line = line.empty() ? word : line + " " + word;
                    cairo_text_extents(cr, test_line.c_str(), &extents);
                    
                    if (extents.width > max_width - 10 && !line.empty()) {
                        // Line would be too long, record current line width and start new line
                        cairo_text_extents(cr, line.c_str(), &extents);
                        line_width = fmax(line_width, extents.width);
                        line = word;
                        line_count++;
                    } else {
                        line = test_line;
                        line_width = fmax(line_width, extents.width);
                    }
                    word.clear();
                }
                
                if (i < text.length() && text[i] == '\n') {
                    if (!line.empty()) {
                        cairo_text_extents(cr, line.c_str(), &extents);
                        line_width = fmax(line_width, extents.width);
                        line.clear();
                    }
                    line_count++;
                }
            } else {
                word += text[i];
            }
        }
        
        // Check final line
        if (!line.empty()) {
            cairo_text_extents(cr, line.c_str(), &extents);
            line_width = fmax(line_width, extents.width);
        }
        
        // Set dimensions based on content
        max_width = fmax(max_width, line_width + 20);
        total_height = line_count * (app_state.text_font_size + 2) + 15;
    } else {
        // Empty text, use minimum size based on font
        total_height = app_state.text_font_size * 3 + 20;
    }
    
    cairo_destroy(cr);
    cairo_surface_destroy(temp_surface);
    
    // Update text box dimensions
    app_state.text_box_width = max_width;
    app_state.text_box_height = fmax(total_height, app_state.text_font_size * 2 + 20);
    
    // Make sure box doesn't go off canvas
    if (app_state.text_x + app_state.text_box_width > app_state.canvas_width) {
        app_state.text_box_width = app_state.canvas_width - app_state.text_x;
    }
    if (app_state.text_y + app_state.text_box_height > app_state.canvas_height) {
        app_state.text_box_height = app_state.canvas_height - app_state.text_y;
    }
}

// Stop ant path animation
void stop_ant_animation() {
    if (app_state.ant_timer_id != 0) {
        g_source_remove(app_state.ant_timer_id);
        app_state.ant_timer_id = 0;
    }
}

// Cancel text without rendering
void cancel_text() {
    app_state.text_active = false;
    app_state.text_content.clear();
    
    // Destroy text window if it exists
    if (app_state.text_window) {
        gtk_widget_destroy(app_state.text_window);
        app_state.text_window = nullptr;
        app_state.text_entry = nullptr;
    }
    
    // Stop animation if no selection is active
    if (!app_state.has_selection) {
        stop_ant_animation();
    }
    
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

// Finalize text onto canvas
void finalize_text() {
    if (!app_state.text_active || app_state.text_content.empty() || !app_state.surface) {
        // If text is active but empty, just cancel it
        if (app_state.text_active) {
            cancel_text();
        }
        return;
    }
    
    cairo_t* cr = cairo_create(app_state.surface);
    
    // Set font
    cairo_select_font_face(cr, app_state.text_font_family.c_str(), 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, app_state.text_font_size);
    
    // Set color
    cairo_set_source_rgba(cr, 
        app_state.fg_color.red,
        app_state.fg_color.green,
        app_state.fg_color.blue,
        app_state.fg_color.alpha
    );
    
    // Draw text with word wrapping
    std::string text = app_state.text_content;
    double y = app_state.text_y + app_state.text_font_size + 5;
    double x = app_state.text_x + 5;
    
    cairo_text_extents_t extents;
    std::string word;
    std::string line;
    
    for (size_t i = 0; i <= text.length(); i++) {
        if (i == text.length() || text[i] == ' ' || text[i] == '\n') {
            if (!word.empty()) {
                std::string test_line = line.empty() ? word : line + " " + word;
                cairo_text_extents(cr, test_line.c_str(), &extents);
                
                if (extents.width > app_state.text_box_width - 10) {
                    if (!line.empty()) {
                        cairo_move_to(cr, x, y);
                        cairo_show_text(cr, line.c_str());
                        y += app_state.text_font_size + 2;
                        line = word;
                    } else {
                        cairo_move_to(cr, x, y);
                        cairo_show_text(cr, word.c_str());
                        y += app_state.text_font_size + 2;
                        line.clear();
                    }
                } else {
                    line = test_line;
                }
                word.clear();
            }
            
            if (i < text.length() && text[i] == '\n') {
                if (!line.empty()) {
                    cairo_move_to(cr, x, y);
                    cairo_show_text(cr, line.c_str());
                    y += app_state.text_font_size + 2;
                    line.clear();
                }
            }
        } else {
            word += text[i];
        }
    }
    
    if (!line.empty()) {
        cairo_move_to(cr, x, y);
        cairo_show_text(cr, line.c_str());
    }
    
    cairo_destroy(cr);
    
    // Clear text state
    app_state.text_active = false;
    app_state.text_content.clear();
    
    // Destroy text window if it exists
    if (app_state.text_window) {
        gtk_widget_destroy(app_state.text_window);
        app_state.text_window = nullptr;
        app_state.text_entry = nullptr;
    }
    
    // Stop animation if no selection is active
    if (!app_state.has_selection) {
        stop_ant_animation();
    }
    
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

// Clear selection
void clear_selection() {
    app_state.has_selection = false;
    app_state.selection_path.clear();
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

// Copy selection to clipboard
void copy_selection() {
    if (!app_state.has_selection || !app_state.surface) return;
    
    if (app_state.selection_is_rect) {
        double x1 = fmin(app_state.selection_x1, app_state.selection_x2);
        double y1 = fmin(app_state.selection_y1, app_state.selection_y2);
        double x2 = fmax(app_state.selection_x1, app_state.selection_x2);
        double y2 = fmax(app_state.selection_y1, app_state.selection_y2);
        
        int w = (int)(x2 - x1);
        int h = (int)(y2 - y1);
        
        if (w <= 0 || h <= 0) return;
        
        if (app_state.clipboard_surface) {
            cairo_surface_destroy(app_state.clipboard_surface);
        }
        
        app_state.clipboard_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        app_state.clipboard_width = w;
        app_state.clipboard_height = h;
        
        cairo_t* cr = cairo_create(app_state.clipboard_surface);
        cairo_set_source_surface(cr, app_state.surface, -x1, -y1);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
}

// Cut selection to clipboard
void cut_selection() {
    if (!app_state.has_selection || !app_state.surface) return;
    
    copy_selection();
    
    if (app_state.selection_is_rect) {
        double x1 = fmin(app_state.selection_x1, app_state.selection_x2);
        double y1 = fmin(app_state.selection_y1, app_state.selection_y2);
        double x2 = fmax(app_state.selection_x1, app_state.selection_x2);
        double y2 = fmax(app_state.selection_y1, app_state.selection_y2);
        
        cairo_t* cr = cairo_create(app_state.surface);
        cairo_set_source_rgba(cr, 
            app_state.bg_color.red,
            app_state.bg_color.green,
            app_state.bg_color.blue,
            app_state.bg_color.alpha
        );
        cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
        cairo_fill(cr);
        cairo_destroy(cr);
        
        if (app_state.drawing_area) {
            gtk_widget_queue_draw(app_state.drawing_area);
        }
    }
    
    clear_selection();
}

// Paste from clipboard
void paste_selection() {
    if (!app_state.clipboard_surface || !app_state.surface) return;
    
    double paste_x = 20;
    double paste_y = 20;
    
    cairo_t* cr = cairo_create(app_state.surface);
    cairo_set_source_surface(cr, app_state.clipboard_surface, paste_x, paste_y);
    cairo_paint(cr);
    cairo_destroy(cr);
    
    app_state.has_selection = true;
    app_state.selection_is_rect = true;
    app_state.selection_x1 = paste_x;
    app_state.selection_y1 = paste_y;
    app_state.selection_x2 = paste_x + app_state.clipboard_width;
    app_state.selection_y2 = paste_y + app_state.clipboard_height;
    
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

// Constrain line to horizontal or vertical when shift is pressed
void constrain_line(double start_x, double start_y, double& end_x, double& end_y) {
    double dx = end_x - start_x;
    double dy = end_y - start_y;
    
    if (fabs(dx) > fabs(dy)) {
        end_y = start_y;
    } else {
        end_x = start_x;
    }
}

// Constrain ellipse to circle when shift is pressed
void constrain_to_circle(double start_x, double start_y, double& end_x, double& end_y) {
    double dx = end_x - start_x;
    double dy = end_y - start_y;
    double radius = fmax(fabs(dx), fabs(dy));
    
    end_x = start_x + (dx >= 0 ? radius : -radius);
    end_y = start_y + (dy >= 0 ? radius : -radius);
}

// Ant path timer callback
gboolean ant_path_timer(gpointer data) {
    app_state.ant_offset += 1.0;
    if (app_state.ant_offset >= 8.0) {
        app_state.ant_offset = 0.0;
    }
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
    return TRUE;
}

// Start ant path animation
void start_ant_animation() {
    if (app_state.ant_timer_id == 0) {
        app_state.ant_timer_id = g_timeout_add(50, ant_path_timer, NULL);
    }
}

// Draw ant path (marching ants)
void draw_ant_path(cairo_t* cr) {
    double dashes[] = {4.0, 4.0};
    cairo_set_dash(cr, dashes, 2, app_state.ant_offset);
    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 0, 0, 0);
}

// Text entry changed callback
void on_text_entry_changed(GtkTextBuffer* buffer, gpointer data) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar* text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    app_state.text_content = text ? text : "";
    g_free(text);
    
    // Update text box size when content changes
    update_text_box_size();
    
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

// Font selection callback
void on_font_selected(GtkFontButton* button, gpointer data) {
    const gchar* font_name = gtk_font_button_get_font_name(button);
    PangoFontDescription* desc = pango_font_description_from_string(font_name);
    
    app_state.text_font_family = pango_font_description_get_family(desc);
    app_state.text_font_size = pango_font_description_get_size(desc) / PANGO_SCALE;
    
    pango_font_description_free(desc);
    
    // Update text box size when font changes
    update_text_box_size();
    
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

// Create text input window
void create_text_window(double x, double y) {
    if (app_state.text_window) {
        gtk_widget_destroy(app_state.text_window);
    }
    
    app_state.text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_state.text_window), "Text Tool");
    gtk_window_set_default_size(GTK_WINDOW(app_state.text_window), 300, 200);
    gtk_window_set_transient_for(GTK_WINDOW(app_state.text_window), GTK_WINDOW(app_state.window));
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(app_state.text_window), vbox);
    
    // Font selector
    GtkWidget* font_button = gtk_font_button_new();
    gchar* font_str = g_strdup_printf("%s %d", app_state.text_font_family.c_str(), app_state.text_font_size);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(font_button), font_str);
    g_free(font_str);
    g_signal_connect(font_button, "font-set", G_CALLBACK(on_font_selected), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), font_button, FALSE, FALSE, 0);
    
    // Text view
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    GtkWidget* text_view = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
    app_state.text_entry = text_view;
    
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    if (!app_state.text_content.empty()) {
        gtk_text_buffer_set_text(buffer, app_state.text_content.c_str(), -1);
    }
    g_signal_connect(buffer, "changed", G_CALLBACK(on_text_entry_changed), NULL);
    
    gtk_container_add(GTK_CONTAINER(scrolled), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
    
    gtk_widget_show_all(app_state.text_window);
    gtk_widget_grab_focus(text_view);
}

// Custom draw function for color indicator buttons
gboolean on_color_button_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    bool is_foreground = GPOINTER_TO_INT(data);
    GdkRGBA* color = is_foreground ? &app_state.fg_color : &app_state.bg_color;
    
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    
    cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
    cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_set_line_width(cr, 2.0);
    cairo_rectangle(cr, 1, 1, alloc.width - 2, alloc.height - 2);
    cairo_stroke(cr);
    
    return TRUE;
}

// Update the foreground/background color indicator buttons
void update_color_indicators() {
    if (app_state.fg_button) {
        gtk_widget_queue_draw(app_state.fg_button);
    }
    
    if (app_state.bg_button) {
        gtk_widget_queue_draw(app_state.bg_button);
    }
}

// Initialize drawing surface
void init_surface(GtkWidget* widget) {
    if (app_state.surface) {
        cairo_surface_destroy(app_state.surface);
    }
    
    app_state.surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, 
        app_state.canvas_width, 
        app_state.canvas_height
    );
    
    cairo_t* cr = cairo_create(app_state.surface);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
}

// Get active color based on mouse button
GdkRGBA get_active_color() {
    return app_state.is_right_button ? app_state.bg_color : app_state.fg_color;
}

// Drawing functions for each tool
void draw_line(cairo_t* cr, double x1, double y1, double x2, double y2) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, app_state.line_width);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
}

void draw_rectangle(cairo_t* cr, double x1, double y1, double x2, double y2, bool filled) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    
    double x = fmin(x1, x2);
    double y = fmin(y1, y2);
    double w = fabs(x2 - x1);
    double h = fabs(y2 - y1);
    
    cairo_rectangle(cr, x, y, w, h);
    
    if (filled) {
        cairo_fill(cr);
    } else {
        cairo_set_line_width(cr, app_state.line_width);
        cairo_stroke(cr);
    }
}

void draw_ellipse(cairo_t* cr, double x1, double y1, double x2, double y2, bool filled) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    
    double cx = (x1 + x2) / 2.0;
    double cy = (y1 + y2) / 2.0;
    double rx = fabs(x2 - x1) / 2.0;
    double ry = fabs(y2 - y1) / 2.0;
    
    if (rx < 0.1 || ry < 0.1) return;
    
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
    cairo_restore(cr);
    
    if (filled) {
        cairo_fill(cr);
    } else {
        cairo_set_line_width(cr, app_state.line_width);
        cairo_stroke(cr);
    }
}

void draw_rounded_rectangle(cairo_t* cr, double x1, double y1, double x2, double y2, bool filled) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    
    double x = fmin(x1, x2);
    double y = fmin(y1, y2);
    double w = fabs(x2 - x1);
    double h = fabs(y2 - y1);
    double r = fmin(w, h) * 0.1;
    
    if (w < 1 || h < 1) return;
    
    cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, x + w - r, y + r, r, 3 * M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    cairo_close_path(cr);
    
    if (filled) {
        cairo_fill(cr);
    } else {
        cairo_set_line_width(cr, app_state.line_width);
        cairo_stroke(cr);
    }
}

void draw_pencil(cairo_t* cr, double x, double y) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, 1.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    if (app_state.last_x != 0 && app_state.last_y != 0) {
        cairo_move_to(cr, app_state.last_x, app_state.last_y);
        cairo_line_to(cr, x, y);
        cairo_stroke(cr);
    }
}

void draw_paintbrush(cairo_t* cr, double x, double y) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, app_state.line_width * 2);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    if (app_state.last_x != 0 && app_state.last_y != 0) {
        cairo_move_to(cr, app_state.last_x, app_state.last_y);
        cairo_line_to(cr, x, y);
        cairo_stroke(cr);
    }
}

void draw_airbrush(cairo_t* cr, double x, double y) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, 0.1);
    
    for (int i = 0; i < 20; i++) {
        double angle = g_random_double() * 2 * M_PI;
        double radius = g_random_double() * 10;
        double px = x + cos(angle) * radius;
        double py = y + sin(angle) * radius;
        cairo_arc(cr, px, py, 0.5, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

void draw_eraser(cairo_t* cr, double x, double y) {
    cairo_set_source_rgba(cr, 
        app_state.bg_color.red, 
        app_state.bg_color.green, 
        app_state.bg_color.blue, 
        app_state.bg_color.alpha
    );
    cairo_set_line_width(cr, app_state.line_width * 3);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    
    if (app_state.last_x != 0 && app_state.last_y != 0) {
        cairo_move_to(cr, app_state.last_x, app_state.last_y);
        cairo_line_to(cr, x, y);
        cairo_stroke(cr);
    }
}

// Draw text box overlay
void draw_text_overlay(cairo_t* cr) {
    if (!app_state.text_active) return;
    
    // Draw text box with ant path
    draw_ant_path(cr);
    cairo_rectangle(cr, app_state.text_x, app_state.text_y, 
                   app_state.text_box_width, app_state.text_box_height);
    cairo_stroke(cr);
    
    // Draw text preview
    if (!app_state.text_content.empty()) {
        cairo_select_font_face(cr, app_state.text_font_family.c_str(), 
                              CAIRO_FONT_SLANT_NORMAL, 
                              CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, app_state.text_font_size);
        cairo_set_source_rgba(cr, 
            app_state.fg_color.red,
            app_state.fg_color.green,
            app_state.fg_color.blue,
            app_state.fg_color.alpha
        );
        
        // Simple preview (first line)
        std::string text = app_state.text_content;
        double y = app_state.text_y + app_state.text_font_size + 5;
        double x = app_state.text_x + 5;
        
        cairo_text_extents_t extents;
        std::string word;
        std::string line;
        
        for (size_t i = 0; i <= text.length(); i++) {
            if (i == text.length() || text[i] == ' ' || text[i] == '\n') {
                if (!word.empty()) {
                    std::string test_line = line.empty() ? word : line + " " + word;
                    cairo_text_extents(cr, test_line.c_str(), &extents);
                    
                    if (extents.width > app_state.text_box_width - 10) {
                        if (!line.empty()) {
                            cairo_move_to(cr, x, y);
                            cairo_show_text(cr, line.c_str());
                            y += app_state.text_font_size + 2;
                            line = word;
                        } else {
                            cairo_move_to(cr, x, y);
                            cairo_show_text(cr, word.c_str());
                            y += app_state.text_font_size + 2;
                            line.clear();
                        }
                    } else {
                        line = test_line;
                    }
                    word.clear();
                }
                
                if (i < text.length() && text[i] == '\n') {
                    if (!line.empty()) {
                        cairo_move_to(cr, x, y);
                        cairo_show_text(cr, line.c_str());
                        y += app_state.text_font_size + 2;
                        line.clear();
                    }
                }
                
                if (y > app_state.text_y + app_state.text_box_height) break;
            } else {
                word += text[i];
            }
        }
        
        if (!line.empty() && y <= app_state.text_y + app_state.text_box_height) {
            cairo_move_to(cr, x, y);
            cairo_show_text(cr, line.c_str());
        }
    }
}

// Draw selection overlay
void draw_selection_overlay(cairo_t* cr) {
    if (!app_state.has_selection) return;
    
    draw_ant_path(cr);
    
    if (app_state.selection_is_rect) {
        double x1 = fmin(app_state.selection_x1, app_state.selection_x2);
        double y1 = fmin(app_state.selection_y1, app_state.selection_y2);
        double x2 = fmax(app_state.selection_x1, app_state.selection_x2);
        double y2 = fmax(app_state.selection_y1, app_state.selection_y2);
        
        cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
        cairo_stroke(cr);
    } else if (app_state.selection_path.size() > 1) {
        cairo_move_to(cr, app_state.selection_path[0].first, app_state.selection_path[0].second);
        for (size_t i = 1; i < app_state.selection_path.size(); i++) {
            cairo_line_to(cr, app_state.selection_path[i].first, app_state.selection_path[i].second);
        }
        cairo_close_path(cr);
        cairo_stroke(cr);
    }
}

// Draw preview overlays with ant paths
void draw_preview(cairo_t* cr) {
    if (!app_state.is_drawing) return;
    
    cairo_save(cr);
    
    double preview_x = app_state.current_x;
    double preview_y = app_state.current_y;
    
    if (app_state.shift_pressed) {
        if (app_state.current_tool == TOOL_LINE) {
            constrain_line(app_state.start_x, app_state.start_y, preview_x, preview_y);
        } else if (app_state.current_tool == TOOL_ELLIPSE) {
            constrain_to_circle(app_state.start_x, app_state.start_y, preview_x, preview_y);
        }
    }
    
    switch (app_state.current_tool) {
        case TOOL_RECT_SELECT: {
            double x = fmin(app_state.start_x, preview_x);
            double y = fmin(app_state.start_y, preview_y);
            double w = fabs(preview_x - app_state.start_x);
            double h = fabs(preview_y - app_state.start_y);
            
            draw_ant_path(cr);
            cairo_rectangle(cr, x, y, w, h);
            cairo_stroke(cr);
            break;
        }
        
        case TOOL_LASSO_SELECT: {
            if (app_state.lasso_points.size() > 1) {
                draw_ant_path(cr);
                cairo_move_to(cr, app_state.lasso_points[0].first, app_state.lasso_points[0].second);
                for (size_t i = 1; i < app_state.lasso_points.size(); i++) {
                    cairo_line_to(cr, app_state.lasso_points[i].first, app_state.lasso_points[i].second);
                }
                cairo_stroke(cr);
            }
            break;
        }
        
        case TOOL_LINE: {
            draw_ant_path(cr);
            cairo_move_to(cr, app_state.start_x, app_state.start_y);
            cairo_line_to(cr, preview_x, preview_y);
            cairo_stroke(cr);
            break;
        }
        
        case TOOL_RECTANGLE: {
            double x = fmin(app_state.start_x, preview_x);
            double y = fmin(app_state.start_y, preview_y);
            double w = fabs(preview_x - app_state.start_x);
            double h = fabs(preview_y - app_state.start_y);
            
            draw_ant_path(cr);
            cairo_rectangle(cr, x, y, w, h);
            cairo_stroke(cr);
            break;
        }
        
        case TOOL_ELLIPSE: {
            double cx = (app_state.start_x + preview_x) / 2.0;
            double cy = (app_state.start_y + preview_y) / 2.0;
            double rx = fabs(preview_x - app_state.start_x) / 2.0;
            double ry = fabs(preview_y - app_state.start_y) / 2.0;
            
            if (rx > 0.1 && ry > 0.1) {
                draw_ant_path(cr);
                cairo_save(cr);
                cairo_translate(cr, cx, cy);
                cairo_scale(cr, rx, ry);
                cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
                cairo_restore(cr);
                cairo_stroke(cr);
            }
            break;
        }
        
        case TOOL_ROUNDED_RECT: {
            double x = fmin(app_state.start_x, preview_x);
            double y = fmin(app_state.start_y, preview_y);
            double w = fabs(preview_x - app_state.start_x);
            double h = fabs(preview_y - app_state.start_y);
            double r = fmin(w, h) * 0.1;
            
            if (w > 1 && h > 1) {
                draw_ant_path(cr);
                cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
                cairo_arc(cr, x + w - r, y + r, r, 3 * M_PI / 2, 0);
                cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
                cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
                cairo_close_path(cr);
                cairo_stroke(cr);
            }
            break;
        }
        
        case TOOL_POLYGON: {
            if (app_state.polygon_points.size() > 0) {
                draw_ant_path(cr);
                cairo_move_to(cr, app_state.polygon_points[0].first, app_state.polygon_points[0].second);
                for (size_t i = 1; i < app_state.polygon_points.size(); i++) {
                    cairo_line_to(cr, app_state.polygon_points[i].first, app_state.polygon_points[i].second);
                }
                cairo_line_to(cr, preview_x, preview_y);
                cairo_stroke(cr);
            }
            break;
        }
    }
    
    cairo_restore(cr);
}

// Canvas draw callback
gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    if (app_state.surface) {
        cairo_set_source_surface(cr, app_state.surface, 0, 0);
        cairo_paint(cr);
        
        // Draw active selection
        if (app_state.has_selection) {
            draw_selection_overlay(cr);
        }
        
        // Draw text overlay
        if (app_state.text_active) {
            draw_text_overlay(cr);
        }
        
        // Draw preview if needed
        if (tool_needs_preview(app_state.current_tool)) {
            draw_preview(cr);
        }
    }
    return FALSE;
}

// Key press event
gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R) {
        app_state.shift_pressed = true;
        if (app_state.is_drawing && app_state.drawing_area) {
            gtk_widget_queue_draw(app_state.drawing_area);
        }
    } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_c) {
        copy_selection();
    } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_x) {
        cut_selection();
    } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_v) {
        paste_selection();
    }
    return FALSE;
}

// Key release event
gboolean on_key_release(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_Shift_L || event->keyval == GDK_KEY_Shift_R) {
        app_state.shift_pressed = false;
        if (app_state.is_drawing && app_state.drawing_area) {
            gtk_widget_queue_draw(app_state.drawing_area);
        }
    }
    return FALSE;
}

// Mouse button press
gboolean on_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    if ((event->button == 1 || event->button == 3) && app_state.surface) {
        // Handle text tool
        if (app_state.current_tool == TOOL_TEXT) {
            if (app_state.text_active && !point_in_text_box(event->x, event->y)) {
                // Clicked outside text box
                if (event->button == 1) {
                    // Left-click - finalize text
                    finalize_text();
                } else {
                    // Right-click - cancel text
                    cancel_text();
                }
                return TRUE;
            } else if (!app_state.text_active) {
                // Start new text box (only with left-click)
                if (event->button == 1) {
                    app_state.text_active = true;
                    app_state.text_x = event->x;
                    app_state.text_y = event->y;
                    app_state.text_content.clear();
                    
                    // Initialize text box size
                    update_text_box_size();
                    
                    create_text_window(event->x, event->y);
                    start_ant_animation();
                    gtk_widget_queue_draw(widget);
                }
                return TRUE;
            }
            // If clicking inside text box, do nothing (keep editing)
            return TRUE;
        }
        
        // Check if clicking outside selection area - clear selection
        if (app_state.has_selection && !point_in_selection(event->x, event->y)) {
            clear_selection();
        }
        
        // Finalize text if active and clicking with different tool
        if (app_state.text_active && app_state.current_tool != TOOL_TEXT) {
            finalize_text();
        }
        
        app_state.is_drawing = true;
        app_state.is_right_button = (event->button == 3);
        app_state.last_x = event->x;
        app_state.last_y = event->y;
        app_state.start_x = event->x;
        app_state.start_y = event->y;
        app_state.current_x = event->x;
        app_state.current_y = event->y;
        
        if (app_state.current_tool == TOOL_POLYGON) {
            app_state.polygon_points.push_back({event->x, event->y});
        } else if (app_state.current_tool == TOOL_LASSO_SELECT) {
            app_state.lasso_points.clear();
            app_state.lasso_points.push_back({event->x, event->y});
        }
        
        if (tool_needs_preview(app_state.current_tool)) {
            start_ant_animation();
        }
    }
    return TRUE;
}

// Mouse motion
gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    if (app_state.is_drawing && app_state.surface) {
        app_state.current_x = event->x;
        app_state.current_y = event->y;
        
        if (app_state.current_tool == TOOL_LASSO_SELECT) {
            app_state.lasso_points.push_back({event->x, event->y});
            gtk_widget_queue_draw(widget);
        } else if (tool_needs_preview(app_state.current_tool)) {
            gtk_widget_queue_draw(widget);
        } else {
            cairo_t* cr = cairo_create(app_state.surface);
            
            switch (app_state.current_tool) {
                case TOOL_PENCIL:
                    draw_pencil(cr, event->x, event->y);
                    break;
                case TOOL_PAINTBRUSH:
                    draw_paintbrush(cr, event->x, event->y);
                    break;
                case TOOL_AIRBRUSH:
                    draw_airbrush(cr, event->x, event->y);
                    break;
                case TOOL_ERASER:
                    draw_eraser(cr, event->x, event->y);
                    break;
            }
            
            cairo_destroy(cr);
            app_state.last_x = event->x;
            app_state.last_y = event->y;
            gtk_widget_queue_draw(widget);
        }
    }
    return TRUE;
}

// Mouse button release
gboolean on_button_release(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    if ((event->button == 1 || event->button == 3) && app_state.surface) {
        double end_x = event->x;
        double end_y = event->y;
        
        if (app_state.shift_pressed) {
            if (app_state.current_tool == TOOL_LINE) {
                constrain_line(app_state.start_x, app_state.start_y, end_x, end_y);
            } else if (app_state.current_tool == TOOL_ELLIPSE) {
                constrain_to_circle(app_state.start_x, app_state.start_y, end_x, end_y);
            }
        }
        
        cairo_t* cr = cairo_create(app_state.surface);
        
        switch (app_state.current_tool) {
            case TOOL_LINE:
                draw_line(cr, app_state.start_x, app_state.start_y, end_x, end_y);
                stop_ant_animation();
                break;
            case TOOL_RECTANGLE:
                draw_rectangle(cr, app_state.start_x, app_state.start_y, end_x, end_y, false);
                stop_ant_animation();
                break;
            case TOOL_ELLIPSE:
                draw_ellipse(cr, app_state.start_x, app_state.start_y, end_x, end_y, false);
                stop_ant_animation();
                break;
            case TOOL_ROUNDED_RECT:
                draw_rounded_rectangle(cr, app_state.start_x, app_state.start_y, end_x, end_y, false);
                stop_ant_animation();
                break;
            case TOOL_RECT_SELECT:
                app_state.has_selection = true;
                app_state.selection_is_rect = true;
                app_state.selection_x1 = app_state.start_x;
                app_state.selection_y1 = app_state.start_y;
                app_state.selection_x2 = end_x;
                app_state.selection_y2 = end_y;
                break;
            case TOOL_LASSO_SELECT:
                app_state.has_selection = true;
                app_state.selection_is_rect = false;
                app_state.selection_path = app_state.lasso_points;
                app_state.lasso_points.clear();
                break;
        }
        
        cairo_destroy(cr);
        app_state.is_drawing = false;
        app_state.is_right_button = false;
        app_state.last_x = 0;
        app_state.last_y = 0;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// File operations
void save_image_dialog(GtkWidget* parent) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Save Image",
        GTK_WINDOW(parent),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Save", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    
    GtkFileFilter* filter_png = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_png, "PNG Images");
    gtk_file_filter_add_pattern(filter_png, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_png);
    
    GtkFileFilter* filter_jpg = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_jpg, "JPEG Images");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpg");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_jpg);
    
    if (!app_state.current_filename.empty()) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), app_state.current_filename.c_str());
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "untitled.png");
    }
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (filename) {
            std::string fname(filename);
            app_state.current_filename = fname;
            
            bool is_jpeg = (fname.substr(fname.find_last_of(".") + 1) == "jpg" ||
                           fname.substr(fname.find_last_of(".") + 1) == "jpeg");
            
            if (is_jpeg) {
                cairo_surface_t* rgb_surface = cairo_image_surface_create(
                    CAIRO_FORMAT_RGB24, 
                    app_state.canvas_width, 
                    app_state.canvas_height
                );
                cairo_t* cr = cairo_create(rgb_surface);
                cairo_set_source_surface(cr, app_state.surface, 0, 0);
                cairo_paint(cr);
                cairo_destroy(cr);
                
                std::string temp_png = fname + ".temp.png";
                cairo_surface_write_to_png(rgb_surface, temp_png.c_str());
                cairo_surface_destroy(rgb_surface);
                
                GError* error = NULL;
                GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(temp_png.c_str(), &error);
                if (pixbuf) {
                    gdk_pixbuf_save(pixbuf, filename, "jpeg", &error, "quality", "95", NULL);
                    g_object_unref(pixbuf);
                }
                remove(temp_png.c_str());
            } else {
                cairo_surface_write_to_png(app_state.surface, filename);
            }
            
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(dialog);
}

void open_image_dialog(GtkWidget* parent) {
    GtkWidget* dialog = gtk_file_chooser_dialog_new(
        "Open Image",
        GTK_WINDOW(parent),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );
    
    GtkFileFilter* filter_images = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_images, "Image Files");
    gtk_file_filter_add_pattern(filter_images, "*.png");
    gtk_file_filter_add_pattern(filter_images, "*.jpg");
    gtk_file_filter_add_pattern(filter_images, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_images);
    
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        
        if (filename) {
            app_state.current_filename = filename;
            
            GError* error = NULL;
            GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(filename, &error);
            
            if (pixbuf) {
                int width = gdk_pixbuf_get_width(pixbuf);
                int height = gdk_pixbuf_get_height(pixbuf);
                
                app_state.canvas_width = width;
                app_state.canvas_height = height;
                
                if (app_state.surface) {
                    cairo_surface_destroy(app_state.surface);
                }
                
                app_state.surface = cairo_image_surface_create(
                    CAIRO_FORMAT_ARGB32, 
                    width, 
                    height
                );
                
                cairo_t* cr = cairo_create(app_state.surface);
                gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
                cairo_paint(cr);
                cairo_destroy(cr);
                
                g_object_unref(pixbuf);
                
                gtk_widget_set_size_request(app_state.drawing_area, width, height);
                gtk_widget_queue_draw(app_state.drawing_area);
            }
            
            g_free(filename);
        }
    }
    
    gtk_widget_destroy(dialog);
}

// Menu callbacks
void on_file_new(GtkMenuItem* item, gpointer data) {
    init_surface(app_state.drawing_area);
    app_state.current_filename.clear();
    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }
    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_file_open(GtkMenuItem* item, gpointer data) {
    open_image_dialog(app_state.window);
}

void on_file_save(GtkMenuItem* item, gpointer data) {
    if (!app_state.current_filename.empty()) {
        cairo_surface_write_to_png(app_state.surface, app_state.current_filename.c_str());
    } else {
        save_image_dialog(app_state.window);
    }
}

void on_file_save_as(GtkMenuItem* item, gpointer data) {
    save_image_dialog(app_state.window);
}

void on_file_quit(GtkMenuItem* item, gpointer data) {
    gtk_main_quit();
}

// Tool button callback
void on_tool_clicked(GtkButton* button, gpointer data) {
    Tool new_tool = (Tool)GPOINTER_TO_INT(data);
    
    // Cancel text if switching away from text tool (don't finalize empty text)
    if (app_state.text_active && new_tool != TOOL_TEXT) {
        cancel_text();
    }
    
    // Clear selection when switching tools
    if (new_tool != app_state.current_tool) {
        if (new_tool != TOOL_RECT_SELECT && new_tool != TOOL_LASSO_SELECT) {
            clear_selection();
            if (!app_state.text_active) {
                stop_ant_animation();
            }
        }
    }
    
    app_state.current_tool = new_tool;
    app_state.polygon_points.clear();
    app_state.lasso_points.clear();
}

// Color button callback
gboolean on_color_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    int index = GPOINTER_TO_INT(data);
    if (index < sizeof(palette_colors) / sizeof(palette_colors[0])) {
        if (event->button == 1) {
            app_state.fg_color = palette_colors[index];
            update_color_indicators();
        } else if (event->button == 3) {
            app_state.bg_color = palette_colors[index];
            update_color_indicators();
        }
    }
    return TRUE;
}

// Create tool button with tooltip
GtkWidget* create_tool_button(const char* icon_name, Tool tool, const char* tooltip) {
    GtkWidget* button = gtk_button_new();
    gtk_widget_set_size_request(button, 28, 28);
    
    GtkWidget* icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR);
    gtk_button_set_image(GTK_BUTTON(button), icon);
    
    // Set tooltip
    gtk_widget_set_tooltip_text(button, tooltip);
    
    g_signal_connect(button, "clicked", G_CALLBACK(on_tool_clicked), GINT_TO_POINTER(tool));
    
    return button;
}

// Create color button
GtkWidget* create_color_button(GdkRGBA color, int index) {
    GtkWidget* button = gtk_button_new();
    gtk_widget_set_size_request(button, 18, 18);
    
    gchar* css = g_strdup_printf(
        "button { "
        "background-color: rgb(%d,%d,%d); "
        "background-image: none; "
        "border: 1px solid #555; "
        "min-width: 18px; "
        "min-height: 18px; "
        "padding: 0; "
        "margin: 0; "
        "}"
        "button:hover { "
        "border: 1px solid #000; "
        "}",
        (int)(color.red * 255), 
        (int)(color.green * 255), 
        (int)(color.blue * 255)
    );
    
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(button),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    
    g_free(css);
    g_object_unref(provider);
    
    gtk_widget_add_events(button, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(button, "button-press-event", G_CALLBACK(on_color_button_press), GINT_TO_POINTER(index));
    
    return button;
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    
    app_state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_state.window), "Drawing Application");
    gtk_window_set_default_size(GTK_WINDOW(app_state.window), 900, 700);
    g_signal_connect(app_state.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(app_state.window, "key-press-event", G_CALLBACK(on_key_press), NULL);
    g_signal_connect(app_state.window, "key-release-event", G_CALLBACK(on_key_release), NULL);
    
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app_state.window), main_box);
    
    GtkWidget* menubar = gtk_menu_bar_new();
    
    GtkWidget* file_menu = gtk_menu_new();
    GtkWidget* file_menu_item = gtk_menu_item_new_with_label("File");
    
    GtkWidget* file_new = gtk_menu_item_new_with_label("New");
    GtkWidget* file_open = gtk_menu_item_new_with_label("Open...");
    GtkWidget* file_save = gtk_menu_item_new_with_label("Save");
    GtkWidget* file_save_as = gtk_menu_item_new_with_label("Save As...");
    GtkWidget* file_quit = gtk_menu_item_new_with_label("Quit");
    
    g_signal_connect(file_new, "activate", G_CALLBACK(on_file_new), NULL);
    g_signal_connect(file_open, "activate", G_CALLBACK(on_file_open), NULL);
    g_signal_connect(file_save, "activate", G_CALLBACK(on_file_save), NULL);
    g_signal_connect(file_save_as, "activate", G_CALLBACK(on_file_save_as), NULL);
    g_signal_connect(file_quit, "activate", G_CALLBACK(on_file_quit), NULL);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_new);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_save);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_save_as);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), file_quit);
    
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_menu_item);
    
    GtkWidget* edit_menu_item = gtk_menu_item_new_with_label("Edit");
    GtkWidget* image_menu_item = gtk_menu_item_new_with_label("Image");
    GtkWidget* help_menu_item = gtk_menu_item_new_with_label("Help");
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), image_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_menu_item);
    
    gtk_box_pack_start(GTK_BOX(main_box), menubar, FALSE, FALSE, 0);
    
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);
    
    GtkWidget* toolbox = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(toolbox), 2);
    gtk_grid_set_row_spacing(GTK_GRID(toolbox), 2);
    gtk_widget_set_margin_start(toolbox, 5);
    gtk_widget_set_margin_end(toolbox, 5);
    gtk_widget_set_margin_top(toolbox, 5);
    
    // Create tool buttons with tooltips
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("edit-select", TOOL_LASSO_SELECT, 
            "Lasso Select - Draw freehand selection"), 
        0, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("edit-select-all", TOOL_RECT_SELECT, 
            "Rectangle Select - Select rectangular regions (Ctrl+C to copy, Ctrl+X to cut)"), 
        1, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("fill-color", TOOL_FILL, 
            "Fill Tool - Fill areas with color"), 
        0, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("color-picker", TOOL_EYEDROPPER, 
            "Eyedropper - Pick color from canvas"), 
        1, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("edit-clear", TOOL_ERASER, 
            "Eraser - Erase with background color"), 
        0, 2, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("zoom-in", TOOL_ZOOM, 
            "Zoom Tool - Zoom in/out"), 
        1, 2, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-brush", TOOL_PENCIL, 
            "Pencil - Draw thin lines"), 
        0, 3, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-brush", TOOL_PAINTBRUSH, 
            "Paintbrush - Draw with brush strokes"), 
        1, 3, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("weather-fog", TOOL_AIRBRUSH, 
            "Airbrush - Spray paint effect"), 
        0, 4, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("font-x-generic", TOOL_TEXT, 
            "Text Tool - Add text (Left-click outside to finalize, Right-click outside to cancel)"), 
        1, 4, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-line", TOOL_LINE, 
            "Line Tool - Draw straight lines (hold Shift for horizontal/vertical)"), 
        0, 5, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-path", TOOL_CURVE, 
            "Curve Tool - Draw curved lines"), 
        1, 5, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-rectangle", TOOL_RECTANGLE, 
            "Rectangle - Draw rectangles"), 
        0, 6, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-polygon", TOOL_POLYGON, 
            "Polygon - Draw multi-sided shapes"), 
        1, 6, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("draw-circle", TOOL_ELLIPSE, 
            "Ellipse/Circle - Draw ellipses (hold Shift for circles)"), 
        0, 7, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button("object-select", TOOL_ROUNDED_RECT, 
            "Rounded Rectangle - Draw rectangles with rounded corners"), 
        1, 7, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(content_box), toolbox, FALSE, FALSE, 0);
    
    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    app_state.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_state.drawing_area, app_state.canvas_width, app_state.canvas_height);
    
    g_signal_connect(app_state.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(app_state.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(app_state.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(app_state.drawing_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
    
    gtk_widget_set_events(app_state.drawing_area, 
        GDK_BUTTON_PRESS_MASK | 
        GDK_BUTTON_RELEASE_MASK | 
        GDK_POINTER_MOTION_MASK
    );
    
    gtk_container_add(GTK_CONTAINER(scrolled), app_state.drawing_area);
    gtk_box_pack_start(GTK_BOX(content_box), scrolled, TRUE, TRUE, 0);
    
    GtkWidget* bottom_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_start(bottom_box, 5);
    gtk_widget_set_margin_end(bottom_box, 5);
    gtk_widget_set_margin_bottom(bottom_box, 5);
    
    app_state.fg_button = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_state.fg_button, 36, 36);
    gtk_widget_set_tooltip_text(app_state.fg_button, "Foreground color (left-click palette / left-click canvas)");
    g_signal_connect(app_state.fg_button, "draw", G_CALLBACK(on_color_button_draw), GINT_TO_POINTER(1));
    
    app_state.bg_button = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_state.bg_button, 36, 36);
    gtk_widget_set_tooltip_text(app_state.bg_button, "Background color (right-click palette / right-click canvas)");
    g_signal_connect(app_state.bg_button, "draw", G_CALLBACK(on_color_button_draw), GINT_TO_POINTER(0));
    
    gtk_box_pack_start(GTK_BOX(bottom_box), app_state.fg_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_box), app_state.bg_button, FALSE, FALSE, 0);
    
    GtkWidget* palette_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(palette_grid), 2);
    gtk_grid_set_row_spacing(GTK_GRID(palette_grid), 2);
    
    int colors_per_row = 14;
    for (size_t i = 0; i < sizeof(palette_colors) / sizeof(palette_colors[0]); i++) {
        GtkWidget* color_btn = create_color_button(palette_colors[i], i);
        int row = i / colors_per_row;
        int col = i % colors_per_row;
        gtk_grid_attach(GTK_GRID(palette_grid), color_btn, col, row, 1, 1);
    }
    
    gtk_box_pack_start(GTK_BOX(bottom_box), palette_grid, FALSE, FALSE, 10);
    
    GtkWidget* dimensions_label = gtk_label_new("800x600");
    gtk_box_pack_end(GTK_BOX(bottom_box), dimensions_label, FALSE, FALSE, 0);
    
    gtk_box_pack_end(GTK_BOX(main_box), bottom_box, FALSE, FALSE, 0);
    
    init_surface(app_state.drawing_area);
    
    start_ant_animation();
    
    gtk_widget_show_all(app_state.window);
    gtk_main();
    
    stop_ant_animation();
    if (app_state.surface) {
        cairo_surface_destroy(app_state.surface);
    }
    if (app_state.clipboard_surface) {
        cairo_surface_destroy(app_state.clipboard_surface);
    }
    
    return 0;
}


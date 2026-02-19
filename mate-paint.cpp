#include <gtk/gtk.h>
#include <cairo.h>
#include <glib/gstdio.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include <memory>
#include <iterator>
#include <string>
#include <cctype>
#include <queue>
#include <cstdio>

const double line_thickness_options[] = {1.0, 2.0, 4.0, 6.0, 8.0};
const double zoom_options[] = {1.0, 2.0, 4.0, 6.0, 8.0};
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
    TOOL_ROUNDED_RECT,
    TOOL_COUNT
};

// Forward declarations
void update_color_indicators();
void save_image_dialog(GtkWidget* parent);
void open_image_dialog(GtkWidget* parent);
void clear_selection();
void copy_selection();
void cut_selection();
void paste_selection();
void copy_surface_to_system_clipboard(cairo_surface_t* surface);
cairo_surface_t* get_surface_from_system_clipboard(int& width, int& height);
bool should_expand_canvas_for_paste(int pasted_width, int pasted_height);
void resize_canvas_for_paste(int new_width, int new_height);
void commit_floating_selection(bool record_undo = true);
void finalize_text();
void cancel_text();
void update_text_box_size();
void update_line_thickness_visibility();
void update_line_thickness_buttons();
int tool_to_index(Tool tool);
void update_zoom_buttons();
void update_zoom_visibility();
void push_undo_state();
void undo_last_operation();
bool is_transparent_color(const GdkRGBA& color);
bool save_surface_to_file(cairo_surface_t* surface, const std::string& filename);
void load_custom_palette_colors();
void save_custom_palette_colors();

struct UndoSnapshot {
    cairo_surface_t* surface = nullptr;
    int width = 0;
    int height = 0;
};

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
    bool hover_in_canvas = false;
    double hover_x = 0;
    double hover_y = 0;
    std::vector<std::pair<double, double>> polygon_points;
    bool polygon_finished = false;
    std::vector<std::pair<double, double>> lasso_points;
    bool lasso_polygon_mode = false;
    bool ellipse_center_mode = false;
    
    // Curve tool state
    bool curve_active = false;
    bool curve_has_end = false;
    bool curve_has_control = false;
    bool curve_primary_right_button = false;
    double curve_start_x = 0;
    double curve_start_y = 0;
    double curve_end_x = 0;
    double curve_end_y = 0;
    double curve_control_x = 0;
    double curve_control_y = 0;

    // Selection state
    bool has_selection = false;
    bool selection_is_rect = false;
    double selection_x1 = 0;
    double selection_y1 = 0;
    double selection_x2 = 0;
    double selection_y2 = 0;
    std::vector<std::pair<double, double>> selection_path;
    cairo_surface_t* floating_surface = nullptr;
    bool floating_selection_active = false;
    bool dragging_selection = false;
    bool floating_drag_completed = false;
    double selection_drag_offset_x = 0;
    double selection_drag_offset_y = 0;
    
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
    GtkWidget* line_thickness_box = nullptr;
    std::vector<GtkWidget*> line_thickness_buttons;
    int active_line_thickness_index = 1;
    std::vector<int> tool_line_thickness_indices = std::vector<int>(TOOL_COUNT, 1);
    GtkWidget* zoom_box = nullptr;
    std::vector<GtkWidget*> zoom_buttons;
    int active_zoom_index = 0;
    double zoom_factor = 1.0;
    GtkWidget* scrolled_window = nullptr;
    std::vector<GdkRGBA> palette_button_colors;
    std::vector<bool> custom_palette_slots;
    std::vector<GtkWidget*> palette_buttons;

    std::string current_filename;

    std::vector<UndoSnapshot> undo_stack;
    static constexpr size_t max_undo_steps = 50;
    bool drag_undo_snapshot_taken = false;
};

AppState app_state;

// Color palette
const GdkRGBA palette_colors[] = {
    {0.0, 0.0, 0.0, 0.0},   // Transparency
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

const GdkRGBA additional_palette_colors[] = {
    {0.2, 0.0, 0.4, 1.0},   // Deep purple
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 1 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 2 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 3 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 4 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 5 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 6 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 7 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 8 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 9 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 10 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 11 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 12 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 13 default
    {0.6, 0.6, 0.6, 1.0},   // Custom slot 14 default
};

constexpr int custom_palette_slot_count = 14;

std::string get_config_file_path() {
    gchar* path = g_build_filename(g_get_user_config_dir(), "mate", "mate-paint", "mate-paint.cfg", NULL);
    std::string config_path(path);
    g_free(path);
    return config_path;
}

int get_custom_palette_start_index() {
    return static_cast<int>((sizeof(palette_colors) / sizeof(palette_colors[0])) + (sizeof(additional_palette_colors) / sizeof(additional_palette_colors[0])) - custom_palette_slot_count);
}

void load_custom_palette_colors() {
    GKeyFile* key_file = g_key_file_new();
    std::string config_path = get_config_file_path();
    GError* error = NULL;
    if (!g_key_file_load_from_file(key_file, config_path.c_str(), G_KEY_FILE_NONE, &error)) {
        if (error) {
            g_error_free(error);
        }
        g_key_file_unref(key_file);
        return;
    }

    const int custom_start_index = get_custom_palette_start_index();
    for (int i = 0; i < custom_palette_slot_count; ++i) {
        gchar* key = g_strdup_printf("custom_color_%d", i + 1);
        gchar* color_string = g_key_file_get_string(key_file, "palette", key, NULL);
        g_free(key);

        if (!color_string) {
            continue;
        }

        GdkRGBA color;
        if (gdk_rgba_parse(&color, color_string)) {
            app_state.palette_button_colors[custom_start_index + i] = color;
        }

        g_free(color_string);
    }

    g_key_file_unref(key_file);
}

void save_custom_palette_colors() {
    GKeyFile* key_file = g_key_file_new();
    std::string config_path = get_config_file_path();

    const int custom_start_index = get_custom_palette_start_index();
    for (int i = 0; i < custom_palette_slot_count; ++i) {
        gchar* key = g_strdup_printf("custom_color_%d", i + 1);
        gchar* color_string = gdk_rgba_to_string(&app_state.palette_button_colors[custom_start_index + i]);
        g_key_file_set_string(key_file, "palette", key, color_string);
        g_free(color_string);
        g_free(key);
    }

    gsize data_len = 0;
    gchar* data = g_key_file_to_data(key_file, &data_len, NULL);
    if (data) {
        gchar* config_dir = g_path_get_dirname(config_path.c_str());
        g_mkdir_with_parents(config_dir, 0755);
        g_file_set_contents(config_path.c_str(), data, data_len, NULL);
        g_free(config_dir);
        g_free(data);
    }

    g_key_file_unref(key_file);
}

// Check if tool needs preview
bool tool_needs_preview(Tool tool) {
    return tool == TOOL_LASSO_SELECT || tool == TOOL_RECT_SELECT ||
           tool == TOOL_LINE || tool == TOOL_CURVE ||
           tool == TOOL_RECTANGLE || tool == TOOL_POLYGON ||
           tool == TOOL_ELLIPSE || tool == TOOL_ROUNDED_RECT;
}

int tool_to_index(Tool tool) {
    return static_cast<int>(tool);
}

bool tool_supports_line_thickness(Tool tool) {
    return tool == TOOL_PAINTBRUSH || tool == TOOL_AIRBRUSH || tool == TOOL_ERASER ||
           tool == TOOL_LINE || tool == TOOL_CURVE || tool == TOOL_RECTANGLE ||
           tool == TOOL_POLYGON || tool == TOOL_ELLIPSE || tool == TOOL_ROUNDED_RECT;
}

bool tool_shows_brush_hover_outline(Tool tool) {
    return tool == TOOL_PAINTBRUSH || tool == TOOL_AIRBRUSH || tool == TOOL_ERASER || tool == TOOL_ELLIPSE ||  tool == TOOL_LASSO_SELECT;
}

bool tool_shows_vertex_hover_markers(Tool tool) {
    return tool == TOOL_LINE || tool == TOOL_CURVE || tool == TOOL_POLYGON;
}

double to_canvas_coordinate(double coordinate) {
    return coordinate / app_state.zoom_factor;
}

double clamp_double(double value, double min_value, double max_value) {
    return fmax(min_value, fmin(value, max_value));
}

void configure_crisp_rendering(cairo_t* cr) {
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
}

void apply_zoom(double zoom_factor, double focus_x, double focus_y) {
    if (!app_state.drawing_area) {
        return;
    }

    app_state.zoom_factor = zoom_factor;
    gtk_widget_set_size_request(
        app_state.drawing_area,
        static_cast<int>(app_state.canvas_width * app_state.zoom_factor),
        static_cast<int>(app_state.canvas_height * app_state.zoom_factor)
    );

    if (app_state.scrolled_window) {
        GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(app_state.scrolled_window));
        GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(app_state.scrolled_window));

        double h_page = gtk_adjustment_get_page_size(hadj);
        double v_page = gtk_adjustment_get_page_size(vadj);

        double target_h = focus_x * app_state.zoom_factor - h_page / 2.0;
        double target_v = focus_y * app_state.zoom_factor - v_page / 2.0;

        double h_max = fmax(gtk_adjustment_get_lower(hadj),
            gtk_adjustment_get_upper(hadj) - gtk_adjustment_get_page_size(hadj));
        double v_max = fmax(gtk_adjustment_get_lower(vadj),
            gtk_adjustment_get_upper(vadj) - gtk_adjustment_get_page_size(vadj));

        target_h = clamp_double(target_h, gtk_adjustment_get_lower(hadj), h_max);
        target_v = clamp_double(target_v, gtk_adjustment_get_lower(vadj), v_max);

        gtk_adjustment_set_value(hadj, target_h);
        gtk_adjustment_set_value(vadj, target_v);
    }

    gtk_widget_queue_draw(app_state.drawing_area);
}

void reset_zoom_to_default() {
    if (!app_state.drawing_area) {
        return;
    }

    apply_zoom(1.0, app_state.canvas_width / 2.0, app_state.canvas_height / 2.0);

    if (app_state.scrolled_window) {
        GtkAdjustment* hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(app_state.scrolled_window));
        GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(app_state.scrolled_window));
        gtk_adjustment_set_value(hadj, gtk_adjustment_get_lower(hadj));
        gtk_adjustment_set_value(vadj, gtk_adjustment_get_lower(vadj));
    }
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
    
    if (app_state.selection_path.size() < 3) return false;

    bool inside = false;
    size_t j = app_state.selection_path.size() - 1;
    for (size_t i = 0; i < app_state.selection_path.size(); i++) {
        double xi = app_state.selection_path[i].first;
        double yi = app_state.selection_path[i].second;
        double xj = app_state.selection_path[j].first;
        double yj = app_state.selection_path[j].second;

        bool intersects = ((yi > y) != (yj > y)) &&
            (x < ((xj - xi) * (y - yi) / (yj - yi) + xi));
        if (intersects) {
            inside = !inside;
        }

        j = i;
    }

    return inside;
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
    configure_crisp_rendering(cr);

    // Set font
    cairo_select_font_face(cr, app_state.text_font_family.c_str(), 
                          CAIRO_FONT_SLANT_NORMAL, 
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, app_state.text_font_size);

    // Calculate size needed for text
    const double min_width = 200.0;
    const double width_padding = 20.0;
    const double wrap_padding = 10.0;
    const double max_canvas_width = fmax(20.0, app_state.canvas_width - app_state.text_x);
    double target_width = fmin(min_width, max_canvas_width);
    double total_height = app_state.text_font_size + 10;

    if (!app_state.text_content.empty()) {
        const std::string& text = app_state.text_content;
        const bool has_manual_line_break = text.find('\n') != std::string::npos;
        cairo_text_extents_t extents;


        // Grow width with the current line while typing until we hit the canvas edge.
        if (!has_manual_line_break) {
            cairo_text_extents(cr, text.c_str(), &extents);
            double content_width = extents.width + width_padding;
            target_width = fmin(fmax(min_width, content_width), max_canvas_width);
        } else {
            // Once Enter is used, keep current width and wrap without expanding further right.
            target_width = fmin(fmax(app_state.text_box_width, min_width), max_canvas_width);
        }

        // Measure wrapped line count based on the chosen width.
        std::string word;
        std::string line;
        int line_count = 1;

        for (size_t i = 0; i <= text.length(); i++) {
            if (i == text.length() || text[i] == ' ' || text[i] == '\n') {
                if (!word.empty()) {
                    std::string test_line = line.empty() ? word : line + " " + word;
                    cairo_text_extents(cr, test_line.c_str(), &extents);

                    if (extents.width > target_width - wrap_padding && !line.empty()) {
                    } else {
                        line = test_line;
                    }
                    word.clear();
                }

                if (i < text.length() && text[i] == '\n') {
                    line.clear();
                    line_count++;
                }
            } else {
                word += text[i];
            }
        }
        total_height = line_count * (app_state.text_font_size + 2) + 15;
    } else {
        // Empty text, use minimum size based on font
        total_height = app_state.text_font_size * 3 + 20;
    }
    cairo_destroy(cr);
    cairo_surface_destroy(temp_surface);

    // Update text box dimensions
    app_state.text_box_width = target_width;
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
    
    push_undo_state();

    cairo_t* cr = cairo_create(app_state.surface);
    configure_crisp_rendering(cr);
    
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
    if (app_state.floating_surface) {
        cairo_surface_destroy(app_state.floating_surface);
        app_state.floating_surface = nullptr;
    }
    app_state.floating_selection_active = false;
    app_state.dragging_selection = false;
    app_state.floating_drag_completed = false;
    app_state.has_selection = false;
    app_state.selection_path.clear();
    app_state.drag_undo_snapshot_taken = false;
    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}

void commit_floating_selection(bool record_undo) {
    if (!app_state.floating_selection_active || !app_state.floating_surface || !app_state.surface) {
        return;
    }

    double x = std::round(fmin(app_state.selection_x1, app_state.selection_x2));
    double y = std::round(fmin(app_state.selection_y1, app_state.selection_y2));

    if (record_undo) {
        push_undo_state();
    }

    cairo_t* cr = cairo_create(app_state.surface);
    configure_crisp_rendering(cr);
    cairo_set_source_surface(cr, app_state.floating_surface, x, y);
    cairo_paint(cr);
    cairo_destroy(cr);

    clear_selection();
    app_state.drag_undo_snapshot_taken = false;
}

void append_selection_path(cairo_t* cr) {
    if (app_state.selection_path.size() < 3) {
        return;
    }

    cairo_move_to(cr, app_state.selection_path[0].first, app_state.selection_path[0].second);
    for (size_t i = 1; i < app_state.selection_path.size(); i++) {
        cairo_line_to(cr, app_state.selection_path[i].first, app_state.selection_path[i].second);
    }
    cairo_close_path(cr);
}

void finalize_lasso_selection() {
    if (app_state.lasso_points.size() < 3) {
        app_state.lasso_points.clear();
        app_state.lasso_polygon_mode = false;
        app_state.is_drawing = false;
        stop_ant_animation();
        return;
    }

    app_state.has_selection = true;
    app_state.selection_is_rect = false;
    app_state.floating_selection_active = false;
    app_state.selection_path = app_state.lasso_points;
    app_state.lasso_points.clear();

    if (!app_state.selection_path.empty()) {
        app_state.selection_x1 = app_state.selection_x2 = app_state.selection_path[0].first;
        app_state.selection_y1 = app_state.selection_y2 = app_state.selection_path[0].second;
        for (const auto& point : app_state.selection_path) {
            app_state.selection_x1 = fmin(app_state.selection_x1, point.first);
            app_state.selection_y1 = fmin(app_state.selection_y1, point.second);
            app_state.selection_x2 = fmax(app_state.selection_x2, point.first);
            app_state.selection_y2 = fmax(app_state.selection_y2, point.second);
        }
    }

    app_state.lasso_polygon_mode = false;
    app_state.is_drawing = false;
}

struct SelectionPixelBounds {
    int x;
    int y;
    int width;
    int height;
};

SelectionPixelBounds get_selection_pixel_bounds() {
    double x1 = fmin(app_state.selection_x1, app_state.selection_x2);
    double y1 = fmin(app_state.selection_y1, app_state.selection_y2);
    double x2 = fmax(app_state.selection_x1, app_state.selection_x2);
    double y2 = fmax(app_state.selection_y1, app_state.selection_y2);

    int pixel_x = static_cast<int>(std::floor(x1));
    int pixel_y = static_cast<int>(std::floor(y1));
    int pixel_x2 = static_cast<int>(std::ceil(x2));
    int pixel_y2 = static_cast<int>(std::ceil(y2));

    return {pixel_x, pixel_y, pixel_x2 - pixel_x, pixel_y2 - pixel_y};
}

void start_selection_drag() {
    if (!app_state.has_selection || !app_state.surface) return;

    if (app_state.floating_selection_active) return;

    if (!app_state.drag_undo_snapshot_taken) {
        push_undo_state();
        app_state.drag_undo_snapshot_taken = true;
    }

    SelectionPixelBounds bounds = get_selection_pixel_bounds();
    int w = bounds.width;
    int h = bounds.height;
    if (w <= 0 || h <= 0) return;

    app_state.floating_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t* float_cr = cairo_create(app_state.floating_surface);
    configure_crisp_rendering(float_cr);

    if (app_state.selection_is_rect) {
        cairo_set_source_surface(float_cr, app_state.surface, -bounds.x, -bounds.y);
        cairo_paint(float_cr);
    } else if (app_state.selection_path.size() > 2) {
        cairo_save(float_cr);
        cairo_translate(float_cr, -bounds.x, -bounds.y);
	append_selection_path(float_cr);
        cairo_clip(float_cr);
        cairo_set_source_surface(float_cr, app_state.surface, 0, 0);
        cairo_paint(float_cr);
        cairo_restore(float_cr);
    }
    cairo_destroy(float_cr);

    cairo_t* cr = cairo_create(app_state.surface);
    configure_crisp_rendering(cr);
    cairo_set_source_rgba(cr,
        app_state.bg_color.red,
        app_state.bg_color.green,
        app_state.bg_color.blue,
        app_state.bg_color.alpha
    );
    if (app_state.selection_is_rect) {
        cairo_rectangle(cr, bounds.x, bounds.y, w, h);
    } else if (app_state.selection_path.size() > 2) {
        append_selection_path(cr);
    }
    cairo_fill(cr);
    cairo_destroy(cr);

    app_state.selection_x1 = bounds.x;
    app_state.selection_y1 = bounds.y;
    app_state.selection_x2 = bounds.x + w;
    app_state.selection_y2 = bounds.y + h;

    app_state.floating_selection_active = true;
    app_state.floating_drag_completed = false;
}

// Copy selection to clipboard
void copy_selection() {
    if (!app_state.has_selection || !app_state.surface) return;
    
    SelectionPixelBounds bounds = get_selection_pixel_bounds();
    int w = bounds.width;
    int h = bounds.height;

    if (w <= 0 || h <= 0) return;

    if (app_state.clipboard_surface) {
        cairo_surface_destroy(app_state.clipboard_surface);
    }

    app_state.clipboard_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    app_state.clipboard_width = w;
    app_state.clipboard_height = h;

    cairo_t* cr = cairo_create(app_state.clipboard_surface);
    configure_crisp_rendering(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (app_state.floating_selection_active && app_state.floating_surface) {
        cairo_set_source_surface(cr, app_state.floating_surface, 0, 0);
        cairo_paint(cr);
    } else if (app_state.selection_is_rect) {
        cairo_set_source_surface(cr, app_state.surface, -bounds.x, -bounds.y);
        cairo_paint(cr);
    } else if (app_state.selection_path.size() > 2) {
        cairo_save(cr);
        cairo_translate(cr, -bounds.x, -bounds.y);
        append_selection_path(cr);
        cairo_clip(cr);
        cairo_set_source_surface(cr, app_state.surface, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    cairo_destroy(cr);
    copy_surface_to_system_clipboard(app_state.clipboard_surface);
}

// Cut selection to clipboard
void cut_selection() {
    if (!app_state.has_selection || !app_state.surface) return;

    copy_selection();

    if (app_state.floating_selection_active) {
        clear_selection();
        return;
    }

    cairo_t* cr = cairo_create(app_state.surface);
    configure_crisp_rendering(cr);
    cairo_set_source_rgba(cr,
        app_state.bg_color.red,
        app_state.bg_color.green,
        app_state.bg_color.blue,
        app_state.bg_color.alpha
    );

    if (app_state.selection_is_rect) {
        SelectionPixelBounds bounds = get_selection_pixel_bounds();

        push_undo_state();

        cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
    } else if (app_state.selection_path.size() > 2) {
        append_selection_path(cr);
    }
    cairo_fill(cr);
    cairo_destroy(cr);

    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }

    clear_selection();
	app_state.drag_undo_snapshot_taken = false;
}

// Paste from clipboard
void paste_selection() {
    if (!app_state.surface) return;

    int clipboard_width = 0;
    int clipboard_height = 0;
    cairo_surface_t* system_surface = get_surface_from_system_clipboard(clipboard_width, clipboard_height);

    if (system_surface) {
        if (app_state.clipboard_surface) {
            cairo_surface_destroy(app_state.clipboard_surface);
        }
        app_state.clipboard_surface = system_surface;
        app_state.clipboard_width = clipboard_width;
        app_state.clipboard_height = clipboard_height;
    }

    if (!app_state.clipboard_surface) return;

    bool exceeds_canvas = app_state.clipboard_width > app_state.canvas_width ||
        app_state.clipboard_height > app_state.canvas_height;
    if (exceeds_canvas &&
        should_expand_canvas_for_paste(app_state.clipboard_width, app_state.clipboard_height)) {
        resize_canvas_for_paste(
            std::max(app_state.canvas_width, app_state.clipboard_width),
            std::max(app_state.canvas_height, app_state.clipboard_height)
        );
    }

    clear_selection();

    double paste_x = 20;
    double paste_y = 20;

    app_state.floating_surface = cairo_surface_reference(app_state.clipboard_surface);
    app_state.floating_selection_active = true;
    app_state.floating_drag_completed = false;
    app_state.dragging_selection = false;

    app_state.has_selection = true;
    app_state.selection_is_rect = true;
    app_state.selection_path.clear();
    app_state.selection_x1 = paste_x;
    app_state.selection_y1 = paste_y;
    app_state.selection_x2 = paste_x + app_state.clipboard_width;
    app_state.selection_y2 = paste_y + app_state.clipboard_height;

    if (app_state.drawing_area) {
        gtk_widget_queue_draw(app_state.drawing_area);
    }
}
void copy_surface_to_system_clipboard(cairo_surface_t* surface) {
    if (!surface || !app_state.window) return;

    GtkClipboard* clipboard = gtk_widget_get_clipboard(app_state.window, GDK_SELECTION_CLIPBOARD);
    if (!clipboard) return;

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    if (width <= 0 || height <= 0) return;

    GdkPixbuf* pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
    if (!pixbuf) return;

    gtk_clipboard_set_image(clipboard, pixbuf);
    g_object_unref(pixbuf);
}

cairo_surface_t* get_surface_from_system_clipboard(int& width, int& height) {
    width = 0;
    height = 0;

    if (!app_state.window) return nullptr;

    GtkClipboard* clipboard = gtk_widget_get_clipboard(app_state.window, GDK_SELECTION_CLIPBOARD);
    if (!clipboard || !gtk_clipboard_wait_is_image_available(clipboard)) return nullptr;

    GdkPixbuf* pixbuf = gtk_clipboard_wait_for_image(clipboard);
    if (!pixbuf) return nullptr;

    width = gdk_pixbuf_get_width(pixbuf);
    height = gdk_pixbuf_get_height(pixbuf);

    cairo_surface_t* surface = gdk_cairo_surface_create_from_pixbuf(pixbuf, 0, nullptr);
    g_object_unref(pixbuf);

    return surface;
}

bool should_expand_canvas_for_paste(int pasted_width, int pasted_height) {
    if (!app_state.window) return false;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Pasted Image Is Larger Than Canvas",
        GTK_WINDOW(app_state.window),
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "_Keep Canvas Size", GTK_RESPONSE_CANCEL,
        "_Expand Canvas", GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* label = gtk_label_new(
        "The pasted image is larger than the current canvas.\n"
        "Would you like to keep the current canvas size or expand it to fit the pasted image?"
    );
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);
    gtk_container_add(GTK_CONTAINER(content), label);

    char details[128];
    std::snprintf(
        details,
        sizeof(details),
        "Canvas: %d x %d    Pasted image: %d x %d",
        app_state.canvas_width,
        app_state.canvas_height,
        pasted_width,
        pasted_height
    );
    GtkWidget* detail_label = gtk_label_new(details);
    gtk_label_set_xalign(GTK_LABEL(detail_label), 0.0);
    gtk_container_add(GTK_CONTAINER(content), detail_label);

    gtk_widget_show_all(dialog);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return response == GTK_RESPONSE_ACCEPT;
}

void resize_canvas_for_paste(int new_width, int new_height) {
    if (!app_state.surface) return;
    if (new_width <= app_state.canvas_width && new_height <= app_state.canvas_height) return;

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* resized_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);

    cairo_t* cr = cairo_create(resized_surface);
    configure_crisp_rendering(cr);
    cairo_set_source_rgba(
        cr,
        app_state.bg_color.red,
        app_state.bg_color.green,
        app_state.bg_color.blue,
        app_state.bg_color.alpha
    );
    cairo_paint(cr);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = resized_surface;
    app_state.canvas_width = new_width;
    app_state.canvas_height = new_height;
    cairo_surface_destroy(old_surface);

    if (app_state.drawing_area) {
        gtk_widget_set_size_request(app_state.drawing_area, new_width, new_height);
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

// Constrain rectangle bounds to a square when shift is pressed
void constrain_to_square(double start_x, double start_y, double& end_x, double& end_y) {
    double dx = end_x - start_x;
    double dy = end_y - start_y;
    double size = fmax(fabs(dx), fabs(dy));

    end_x = start_x + (dx >= 0 ? size : -size);
    end_y = start_y + (dy >= 0 ? size : -size);
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

    if (is_transparent_color(*color)) {
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, fmax(10.0, alloc.height * 0.7));

        cairo_text_extents_t extents;
        cairo_text_extents(cr, "T", &extents);

        double text_x = (alloc.width - extents.width) / 2.0 - extents.x_bearing;
        double text_y = (alloc.height - extents.height) / 2.0 - extents.y_bearing;

        cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, "T");
    }

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

cairo_surface_t* clone_surface(cairo_surface_t* source, int width, int height) {
    if (!source || width <= 0 || height <= 0) {
        return nullptr;
    }

    cairo_surface_t* copy = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(copy);
    cairo_set_source_surface(cr, source, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);
    return copy;
}

void push_undo_state() {
    if (!app_state.surface) {
        return;
    }

    cairo_surface_t* snapshot = clone_surface(app_state.surface, app_state.canvas_width, app_state.canvas_height);
    if (!snapshot) {
        return;
    }

    UndoSnapshot undo_snapshot;
    undo_snapshot.surface = snapshot;
    undo_snapshot.width = app_state.canvas_width;
    undo_snapshot.height = app_state.canvas_height;
    app_state.undo_stack.push_back(undo_snapshot);
    if (app_state.undo_stack.size() > AppState::max_undo_steps) {
        cairo_surface_destroy(app_state.undo_stack.front().surface);
        app_state.undo_stack.erase(app_state.undo_stack.begin());
    }
}

void undo_last_operation() {
    if (app_state.undo_stack.empty()) {
        return;
    }

    UndoSnapshot snapshot = app_state.undo_stack.back();
    app_state.undo_stack.pop_back();

    if (app_state.surface) {
        cairo_surface_destroy(app_state.surface);
    }

    app_state.surface = snapshot.surface;
    app_state.canvas_width = snapshot.width;
    app_state.canvas_height = snapshot.height;

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }
    app_state.drag_undo_snapshot_taken = false;

    if (app_state.drawing_area) {
        gtk_widget_set_size_request(
            app_state.drawing_area,
            static_cast<int>(app_state.canvas_width * app_state.zoom_factor),
            static_cast<int>(app_state.canvas_height * app_state.zoom_factor)
        );
        gtk_widget_queue_draw(app_state.drawing_area);
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
    configure_crisp_rendering(cr);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    cairo_destroy(cr);
}

// Get active color based on mouse button
GdkRGBA get_active_color() {
    return app_state.is_right_button ? app_state.bg_color : app_state.fg_color;
}

bool is_transparent_color(const GdkRGBA& color) {
    return color.alpha <= 0.001;
}

bool point_in_canvas(int x, int y) {
    return x >= 0 && x < app_state.canvas_width && y >= 0 && y < app_state.canvas_height;
}

double clamp_color_channel(double channel) {
    return fmax(0.0, fmin(1.0, channel));
}

guint32 rgba_to_pixel(const GdkRGBA& color) {
    guint8 r = static_cast<guint8>(std::round(clamp_color_channel(color.red) * 255.0));
    guint8 g = static_cast<guint8>(std::round(clamp_color_channel(color.green) * 255.0));
    guint8 b = static_cast<guint8>(std::round(clamp_color_channel(color.blue) * 255.0));
    guint8 a = static_cast<guint8>(std::round(clamp_color_channel(color.alpha) * 255.0));
    return (static_cast<guint32>(a) << 24) |
           (static_cast<guint32>(r) << 16) |
           (static_cast<guint32>(g) << 8) |
            static_cast<guint32>(b);
}

GdkRGBA pixel_to_rgba(guint32 pixel) {
    GdkRGBA color;
    color.alpha = ((pixel >> 24) & 0xFF) / 255.0;
    color.red = ((pixel >> 16) & 0xFF) / 255.0;
    color.green = ((pixel >> 8) & 0xFF) / 255.0;
    color.blue = (pixel & 0xFF) / 255.0;
    return color;
}

guint32 read_pixel(int x, int y) {
    unsigned char* data = cairo_image_surface_get_data(app_state.surface);
    int stride = cairo_image_surface_get_stride(app_state.surface);
    guint32* row = reinterpret_cast<guint32*>(data + y * stride);
    return row[x];
}


void pick_color_at(int x, int y, bool set_background) {
    if (!point_in_canvas(x, y)) return;

    cairo_surface_flush(app_state.surface);
    GdkRGBA sampled = pixel_to_rgba(read_pixel(x, y));
    if (set_background) {
        app_state.bg_color = sampled;
    } else {
        app_state.fg_color = sampled;
    }
    update_color_indicators();
}

void flood_fill_at(int start_x, int start_y) {
    if (!point_in_canvas(start_x, start_y)) return;

    cairo_surface_flush(app_state.surface);
    guint32 target = read_pixel(start_x, start_y);
    guint32 replacement = rgba_to_pixel(get_active_color());
    if (target == replacement) return;

    unsigned char* data = cairo_image_surface_get_data(app_state.surface);
    int stride = cairo_image_surface_get_stride(app_state.surface);

    std::queue<std::pair<int, int>> pixels;
    pixels.push({start_x, start_y});

    while (!pixels.empty()) {
        std::pair<int, int> current = pixels.front();
        pixels.pop();

        int x = current.first;
        int y = current.second;

        if (!point_in_canvas(x, y)) continue;

        guint32* row = reinterpret_cast<guint32*>(data + y * stride);
        if (row[x] != target) continue;

        row[x] = replacement;
        pixels.push({x - 1, y});
        pixels.push({x + 1, y});
        pixels.push({x, y - 1});
        pixels.push({x, y + 1});
    }

    cairo_surface_mark_dirty(app_state.surface);
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

void draw_polygon(cairo_t* cr, const std::vector<std::pair<double, double>>& points) {
    if (points.size() < 2) return;

    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, app_state.line_width);

    cairo_move_to(cr, points[0].first, points[0].second);
    for (size_t i = 1; i < points.size(); i++) {
        cairo_line_to(cr, points[i].first, points[i].second);
    }
    cairo_close_path(cr);
    cairo_stroke(cr);
}

void draw_curve(cairo_t* cr, double start_x, double start_y, double control_x, double control_y, double end_x, double end_y) {
    GdkRGBA color = get_active_color();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    cairo_set_line_width(cr, app_state.line_width);
    cairo_move_to(cr, start_x, start_y);
    cairo_curve_to(cr, control_x, control_y, control_x, control_y, end_x, end_y);
    cairo_stroke(cr);
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
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, color.alpha);
    double spray_radius = app_state.line_width * 5.0;
    
    for (int i = 0; i < 20; i++) {
        double angle = g_random_double() * 2 * M_PI;
        double radius = g_random_double() * spray_radius;
        int px = static_cast<int>(std::round(x + cos(angle) * radius));
        int py = static_cast<int>(std::round(y + sin(angle) * radius));
        cairo_rectangle(cr, px, py, 1, 1);
    }
    cairo_fill(cr);
}

void draw_eraser(cairo_t* cr, double x, double y) {
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_set_line_width(cr, app_state.line_width * 3);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    if (app_state.last_x != 0 && app_state.last_y != 0) {
        cairo_move_to(cr, app_state.last_x, app_state.last_y);
        cairo_line_to(cr, x, y);
        cairo_stroke(cr);
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
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

void draw_black_outline_circle(cairo_t* cr, double x, double y, double radius) {
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_arc(cr, x, y, radius, 0, 2 * M_PI);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void draw_hover_indicator(cairo_t* cr) {
    if (!app_state.hover_in_canvas) {
        return;
    }

    if (tool_shows_brush_hover_outline(app_state.current_tool) && !app_state.is_drawing) {
        double radius = app_state.line_width;
        if (app_state.current_tool == TOOL_ERASER) {
            radius = app_state.line_width * 1.5;
        } else if (app_state.current_tool == TOOL_AIRBRUSH) {
            radius = app_state.line_width * 5.0;
        }
        draw_black_outline_circle(cr, app_state.hover_x, app_state.hover_y, radius);
        return;
    }

    if (tool_shows_vertex_hover_markers(app_state.current_tool) && !app_state.is_drawing) {
        draw_black_outline_circle(cr, app_state.hover_x, app_state.hover_y, 5.0);
    }
}

// Draw preview overlays with ant paths
void draw_preview(cairo_t* cr) {
    if (!app_state.is_drawing) return;
    
    // Dragging an existing floating selection should only show the active
    // selection outline, not a new preview marquee from the selection tool.
    if (app_state.dragging_selection) return;

    cairo_save(cr);
    
    double preview_x = app_state.current_x;
    double preview_y = app_state.current_y;
    
    if (app_state.shift_pressed && !app_state.ellipse_center_mode) {
        if (app_state.current_tool == TOOL_LINE) {
            constrain_line(app_state.start_x, app_state.start_y, preview_x, preview_y);
        } else if (app_state.current_tool == TOOL_ELLIPSE) {
            constrain_to_circle(app_state.start_x, app_state.start_y, preview_x, preview_y);
        } else if (app_state.current_tool == TOOL_RECTANGLE ||
                   app_state.current_tool == TOOL_ROUNDED_RECT ||
                   app_state.current_tool == TOOL_RECT_SELECT) {
            constrain_to_square(app_state.start_x, app_state.start_y, preview_x, preview_y);
        }
    }
    
    switch (app_state.current_tool) {
        case TOOL_CURVE: {
            if (app_state.curve_active) {
                draw_ant_path(cr);

                draw_black_outline_circle(cr, app_state.curve_start_x, app_state.curve_start_y, 5.0);
                if (app_state.curve_has_end) {
                    draw_black_outline_circle(cr, app_state.curve_end_x, app_state.curve_end_y, 5.0);
                }

                if (app_state.curve_has_end) {
                    if (app_state.curve_has_control) {
                        cairo_move_to(cr, app_state.curve_start_x, app_state.curve_start_y);
                        cairo_curve_to(
                            cr,
                            app_state.curve_control_x,
                            app_state.curve_control_y,
                            app_state.curve_control_x,
                            app_state.curve_control_y,
                            app_state.curve_end_x,
                            app_state.curve_end_y
                        );
                    } else {
                        cairo_move_to(cr, app_state.curve_start_x, app_state.curve_start_y);
                        cairo_line_to(cr, app_state.curve_end_x, app_state.curve_end_y);
                    }
                    cairo_stroke(cr);
                }
            }
            break;
        }

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
                if (app_state.lasso_polygon_mode) {
                    cairo_line_to(cr, preview_x, preview_y);
                }
                cairo_stroke(cr);
            }
            if (app_state.lasso_polygon_mode) {
                for (const auto& point : app_state.lasso_points) {
                    draw_black_outline_circle(cr, point.first, point.second, 5.0);
                }
            }
            break;
        }
        
        case TOOL_LINE: {
            draw_ant_path(cr);
            cairo_move_to(cr, app_state.start_x, app_state.start_y);
            cairo_line_to(cr, preview_x, preview_y);
            cairo_stroke(cr);
            draw_black_outline_circle(cr, app_state.start_x, app_state.start_y, 5.0);
            draw_black_outline_circle(cr, preview_x, preview_y, 5.0);
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
            double cx;
            double cy;
            double rx;
            double ry;

            if (app_state.ellipse_center_mode) {
                double radius = std::hypot(preview_x - app_state.start_x, preview_y - app_state.start_y);
                cx = app_state.start_x;
                cy = app_state.start_y;
                rx = radius;
                ry = radius;
            } else {
                cx = (app_state.start_x + preview_x) / 2.0;
                cy = (app_state.start_y + preview_y) / 2.0;
                rx = fabs(preview_x - app_state.start_x) / 2.0;
                ry = fabs(preview_y - app_state.start_y) / 2.0;
            }
            
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

                if (app_state.polygon_finished) {
                    cairo_close_path(cr);
                } else {
                    cairo_line_to(cr, preview_x, preview_y);
                }

                cairo_stroke(cr);
                
                for (const auto& point : app_state.polygon_points) {
                    draw_black_outline_circle(cr, point.first, point.second, 5.0);
                }
            }
            break;
        }
    }
    
    cairo_restore(cr);
}

// Canvas draw callback
gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    if (app_state.surface) {
        configure_crisp_rendering(cr);
        cairo_save(cr);
        cairo_scale(cr, app_state.zoom_factor, app_state.zoom_factor);
        cairo_set_source_surface(cr, app_state.surface, 0, 0);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
        cairo_paint(cr);

        if (app_state.floating_selection_active && app_state.floating_surface) {
		    double x = std::round(fmin(app_state.selection_x1, app_state.selection_x2));
		    double y = std::round(fmin(app_state.selection_y1, app_state.selection_y2));

            cairo_set_source_surface(cr, app_state.floating_surface, x, y);
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
            cairo_paint(cr);
        }
        
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
        draw_hover_indicator(cr);
        cairo_restore(cr);
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
    } else if ((event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_z) {
        undo_last_operation();
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
        double canvas_x = to_canvas_coordinate(event->x);
        double canvas_y = to_canvas_coordinate(event->y);

        if (app_state.floating_selection_active) {
            if (app_state.floating_drag_completed || !point_in_selection(canvas_x, canvas_y)) {
                commit_floating_selection();
                return TRUE;
            }

            start_selection_drag();
            app_state.dragging_selection = true;
            app_state.selection_drag_offset_x = canvas_x - fmin(app_state.selection_x1, app_state.selection_x2);
            app_state.selection_drag_offset_y = canvas_y - fmin(app_state.selection_y1, app_state.selection_y2);
            app_state.is_drawing = true;
            return TRUE;
        }

        // Handle zoom tool
        if (app_state.current_tool == TOOL_ZOOM && event->button == 1) {
            double selected_zoom = zoom_options[app_state.active_zoom_index];
            if (selected_zoom == 1.0) {
                reset_zoom_to_default();
            } else {
                apply_zoom(selected_zoom, canvas_x, canvas_y);
            }
            return TRUE;
        }
        // Handle text tool
        if (app_state.current_tool == TOOL_TEXT) {
            if (app_state.text_active && !point_in_text_box(canvas_x, canvas_y)) {
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
                    app_state.text_x = canvas_x;
                    app_state.text_y = canvas_y;
                    app_state.text_content.clear();
                    
                    // Initialize text box size
                    update_text_box_size();
                    
                    create_text_window(canvas_x, canvas_y);
                    start_ant_animation();
                    gtk_widget_queue_draw(widget);
                }
                return TRUE;
            }
            // If clicking inside text box, do nothing (keep editing)
            return TRUE;
        }

        if ((app_state.current_tool == TOOL_RECT_SELECT || app_state.current_tool == TOOL_LASSO_SELECT) &&
            app_state.has_selection && point_in_selection(canvas_x, canvas_y)) {
            start_selection_drag();
            if (app_state.floating_selection_active) {
                app_state.dragging_selection = true;
                app_state.selection_drag_offset_x = canvas_x - fmin(app_state.selection_x1, app_state.selection_x2);
                app_state.selection_drag_offset_y = canvas_y - fmin(app_state.selection_y1, app_state.selection_y2);
                app_state.is_drawing = true;
                return TRUE;
            }
        }

        // Check if clicking outside selection area - clear selection
        if (app_state.has_selection && !point_in_selection(canvas_x, canvas_y)) {
            clear_selection();
        }
        
        // Finalize text if active and clicking with different tool
        if (app_state.text_active && app_state.current_tool != TOOL_TEXT) {
            finalize_text();
        }

        app_state.is_right_button = (event->button == 3);

        if (app_state.current_tool == TOOL_EYEDROPPER) {
            pick_color_at(static_cast<int>(canvas_x), static_cast<int>(canvas_y), app_state.is_right_button);
            return TRUE;
        }

        if (app_state.current_tool == TOOL_FILL) {
            push_undo_state();
            flood_fill_at(static_cast<int>(canvas_x), static_cast<int>(canvas_y));
            gtk_widget_queue_draw(widget);
            return TRUE;
        }

        if (app_state.current_tool == TOOL_LASSO_SELECT) {
            if (event->button == 1) {
                if (app_state.lasso_polygon_mode) {
                    app_state.lasso_points.push_back({canvas_x, canvas_y});
                    app_state.current_x = canvas_x;
                    app_state.current_y = canvas_y;
                    app_state.is_drawing = true;
                    gtk_widget_queue_draw(widget);
                    return TRUE;
                }

                app_state.is_drawing = true;
                app_state.lasso_polygon_mode = ((event->state & GDK_CONTROL_MASK) != 0);
                app_state.lasso_points.clear();
                app_state.lasso_points.push_back({canvas_x, canvas_y});
                app_state.current_x = canvas_x;
                app_state.current_y = canvas_y;
                start_ant_animation();
                gtk_widget_queue_draw(widget);
                return TRUE;
            }

            if (event->button == 3 && app_state.lasso_polygon_mode) {
                finalize_lasso_selection();
                gtk_widget_queue_draw(widget);
                return TRUE;
            }
        }

        if (app_state.current_tool == TOOL_POLYGON) {
            if (event->button == 1) {
                if (app_state.polygon_finished) {
                    push_undo_state();
                    app_state.is_right_button = false;
                    cairo_t* cr = cairo_create(app_state.surface);
                    configure_crisp_rendering(cr);
                    draw_polygon(cr, app_state.polygon_points);
                    cairo_destroy(cr);

                    app_state.polygon_points.clear();
                    app_state.polygon_finished = false;
                    app_state.is_drawing = false;
                    stop_ant_animation();
                    gtk_widget_queue_draw(widget);
                    return TRUE;
                }

                app_state.polygon_points.push_back({canvas_x, canvas_y});
                app_state.is_drawing = true;
                app_state.current_x = canvas_x;
                app_state.current_y = canvas_y;
                start_ant_animation();
                gtk_widget_queue_draw(widget);
                return TRUE;
            }

            if (event->button == 3) {
                if (!app_state.polygon_finished && app_state.polygon_points.size() >= 2) {
                    app_state.polygon_finished = true;
                    app_state.is_drawing = true;
                } else if (app_state.polygon_finished) {
                    push_undo_state();
                    cairo_t* cr = cairo_create(app_state.surface);
                    configure_crisp_rendering(cr);
                    draw_polygon(cr, app_state.polygon_points);
                    cairo_destroy(cr);
                    app_state.polygon_points.clear();
                    app_state.polygon_finished = false;
                    app_state.is_drawing = false;
                    stop_ant_animation();
                }

                gtk_widget_queue_draw(widget);
                return TRUE;
            }
        }

        if (app_state.current_tool == TOOL_ELLIPSE && app_state.ellipse_center_mode && event->button == 1) {
            push_undo_state();

            double radius = std::hypot(canvas_x - app_state.start_x, canvas_y - app_state.start_y);
            double x1 = app_state.start_x - radius;
            double y1 = app_state.start_y - radius;
            double x2 = app_state.start_x + radius;
            double y2 = app_state.start_y + radius;

            cairo_t* cr = cairo_create(app_state.surface);
            configure_crisp_rendering(cr);
            draw_ellipse(cr, x1, y1, x2, y2, false);
            cairo_destroy(cr);

            app_state.ellipse_center_mode = false;
            app_state.is_drawing = false;
            stop_ant_animation();
            gtk_widget_queue_draw(widget);
            return TRUE;
        }

        if (app_state.current_tool == TOOL_ELLIPSE && event->button == 1 &&
            ((event->state & GDK_CONTROL_MASK) != 0)) {
            app_state.ellipse_center_mode = true;
            app_state.is_drawing = true;
            app_state.is_right_button = false;
            app_state.start_x = canvas_x;
            app_state.start_y = canvas_y;
            app_state.current_x = canvas_x;
            app_state.current_y = canvas_y;
            start_ant_animation();
            gtk_widget_queue_draw(widget);
            return TRUE;
        }
        
        if (app_state.current_tool == TOOL_CURVE) {
            if (!app_state.curve_active) {
                app_state.curve_active = true;
                app_state.curve_has_end = false;
                app_state.curve_has_control = false;
                app_state.curve_primary_right_button = (event->button == 3);
                app_state.curve_start_x = canvas_x;
                app_state.curve_start_y = canvas_y;
                app_state.is_drawing = true;
                app_state.current_x = canvas_x;
                app_state.current_y = canvas_y;
                start_ant_animation();
                gtk_widget_queue_draw(widget);
                return TRUE;
            }

            bool used_primary_button = ((event->button == 3) == app_state.curve_primary_right_button);
            if (!used_primary_button) {
                if (app_state.curve_has_end) {
                    cairo_t* cr = cairo_create(app_state.surface);
                    configure_crisp_rendering(cr);

                    app_state.is_right_button = app_state.curve_primary_right_button;
                    if (app_state.curve_has_control) {
                        draw_curve(
                            cr,
                            app_state.curve_start_x,
                            app_state.curve_start_y,
                            app_state.curve_control_x,
                            app_state.curve_control_y,
                            app_state.curve_end_x,
                            app_state.curve_end_y
                        );
                    } else {
                        draw_line(
                            cr,
                            app_state.curve_start_x,
                            app_state.curve_start_y,
                            app_state.curve_end_x,
                            app_state.curve_end_y
                        );
                    }

                    cairo_destroy(cr);
                }

                app_state.curve_active = false;
                app_state.curve_has_end = false;
                app_state.curve_has_control = false;
                app_state.is_drawing = false;
                app_state.is_right_button = false;
                stop_ant_animation();
                gtk_widget_queue_draw(widget);
                return TRUE;
            }

            if (app_state.curve_has_end && (event->state & GDK_SHIFT_MASK)) {
                app_state.curve_start_x = canvas_x;
                app_state.curve_start_y = canvas_y;
                app_state.current_x = canvas_x;
                app_state.current_y = canvas_y;
                gtk_widget_queue_draw(widget);
                return TRUE;
            }

            if (app_state.curve_has_end && (event->state & GDK_CONTROL_MASK)) {
                app_state.curve_end_x = canvas_x;
                app_state.curve_end_y = canvas_y;
                app_state.current_x = canvas_x;
                app_state.current_y = canvas_y;
                gtk_widget_queue_draw(widget);
                return TRUE;
            }

            if (!app_state.curve_has_end) {
                app_state.curve_end_x = canvas_x;
                app_state.curve_end_y = canvas_y;
                app_state.curve_has_end = true;
            } else {
                app_state.curve_control_x = canvas_x;
                app_state.curve_control_y = canvas_y;
                app_state.curve_has_control = true;
            }

            app_state.is_drawing = true;
            app_state.current_x = canvas_x;
            app_state.current_y = canvas_y;
            gtk_widget_queue_draw(widget);
            return TRUE;
        }

        app_state.is_drawing = true;
        if (app_state.current_tool == TOOL_PENCIL || app_state.current_tool == TOOL_PAINTBRUSH ||
            app_state.current_tool == TOOL_AIRBRUSH || app_state.current_tool == TOOL_ERASER ||
            app_state.current_tool == TOOL_LINE || app_state.current_tool == TOOL_CURVE ||
			app_state.current_tool == TOOL_RECTANGLE || app_state.current_tool == TOOL_ELLIPSE ||
            app_state.current_tool == TOOL_ROUNDED_RECT) {
            push_undo_state();
        }
        app_state.last_x = canvas_x;
        app_state.last_y = canvas_y;
        app_state.start_x = canvas_x;
        app_state.start_y = canvas_y;
        app_state.current_x = canvas_x;
        app_state.current_y = canvas_y;
        
        if (app_state.current_tool == TOOL_AIRBRUSH) {
            cairo_t* cr = cairo_create(app_state.surface);
            configure_crisp_rendering(cr);
            draw_airbrush(cr, canvas_x, canvas_y);
            cairo_destroy(cr);
            gtk_widget_queue_draw(widget);
        }
        
        if (tool_needs_preview(app_state.current_tool)) {
            start_ant_animation();
        }
    }
    return TRUE;
}

// Mouse motion
gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event, gpointer data) {
    if (app_state.surface) {
        double canvas_x = to_canvas_coordinate(event->x);
        double canvas_y = to_canvas_coordinate(event->y);
        app_state.hover_in_canvas = true;
        app_state.hover_x = canvas_x;
        app_state.hover_y = canvas_y;

        if (!app_state.is_drawing) {
            if (tool_shows_brush_hover_outline(app_state.current_tool) ||
                tool_shows_vertex_hover_markers(app_state.current_tool)) {
                gtk_widget_queue_draw(widget);
            }
            return TRUE;
        }

        app_state.current_x = canvas_x;
        app_state.current_y = canvas_y;

        if (app_state.dragging_selection && app_state.has_selection) {
            double old_x = fmin(app_state.selection_x1, app_state.selection_x2);
            double old_y = fmin(app_state.selection_y1, app_state.selection_y2);
            double width = fabs(app_state.selection_x2 - app_state.selection_x1);
            double height = fabs(app_state.selection_y2 - app_state.selection_y1);
            double new_x = std::round(canvas_x - app_state.selection_drag_offset_x);
            double new_y = std::round(canvas_y - app_state.selection_drag_offset_y);

            double dx = new_x - old_x;
            double dy = new_y - old_y;

            app_state.selection_x1 = new_x;
            app_state.selection_y1 = new_y;
            app_state.selection_x2 = new_x + width;
            app_state.selection_y2 = new_y + height;

            if (!app_state.selection_is_rect) {
                for (auto& point : app_state.selection_path) {
                    point.first += dx;
                    point.second += dy;
                }
            }

            gtk_widget_queue_draw(widget);
        } else if (app_state.current_tool == TOOL_LASSO_SELECT && !app_state.lasso_polygon_mode) {
            app_state.lasso_points.push_back({canvas_x, canvas_y});
            gtk_widget_queue_draw(widget);
        } else if (tool_needs_preview(app_state.current_tool)) {
            gtk_widget_queue_draw(widget);
        } else {
            cairo_t* cr = cairo_create(app_state.surface);
            configure_crisp_rendering(cr);
            switch (app_state.current_tool) {
                case TOOL_PENCIL:
                    draw_pencil(cr, canvas_x, canvas_y);
                    break;
                case TOOL_PAINTBRUSH:
                    draw_paintbrush(cr, canvas_x, canvas_y);
                    break;
                case TOOL_AIRBRUSH:
                    draw_airbrush(cr, canvas_x, canvas_y);
                    break;
                case TOOL_ERASER:
                    draw_eraser(cr, canvas_x, canvas_y);
                    break;
            }
            
            cairo_destroy(cr);
            app_state.last_x = canvas_x;
            app_state.last_y = canvas_y;
            gtk_widget_queue_draw(widget);
        }
    }
    return TRUE;
}

gboolean on_leave_notify(GtkWidget* widget, GdkEventCrossing* event, gpointer data) {
    if (app_state.hover_in_canvas) {
        app_state.hover_in_canvas = false;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// Mouse button release
gboolean on_button_release(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    if ((event->button == 1 || event->button == 3) && app_state.surface && app_state.is_drawing) {
        if (app_state.current_tool == TOOL_ELLIPSE && app_state.ellipse_center_mode) {
            return TRUE;
        }

        if (app_state.current_tool == TOOL_CURVE) {
            return TRUE;
        }

        if (app_state.current_tool == TOOL_POLYGON) {
            return TRUE;
        }

        if (app_state.current_tool == TOOL_LASSO_SELECT && app_state.lasso_polygon_mode) {
            return TRUE;
        }

        if (app_state.dragging_selection) {
            app_state.dragging_selection = false;
            app_state.is_drawing = false;
            app_state.floating_drag_completed = true;
            commit_floating_selection(false);
            gtk_widget_queue_draw(widget);
            return TRUE;
        }

        double end_x = to_canvas_coordinate(event->x);
        double end_y = to_canvas_coordinate(event->y);
        
        if (app_state.shift_pressed) {
            if (app_state.current_tool == TOOL_LINE) {
                constrain_line(app_state.start_x, app_state.start_y, end_x, end_y);
            } else if (app_state.current_tool == TOOL_ELLIPSE) {
                constrain_to_circle(app_state.start_x, app_state.start_y, end_x, end_y);
            } else if (app_state.current_tool == TOOL_RECTANGLE ||
                       app_state.current_tool == TOOL_ROUNDED_RECT ||
                       app_state.current_tool == TOOL_RECT_SELECT) {
                constrain_to_square(app_state.start_x, app_state.start_y, end_x, end_y);
            }
        }
        
        cairo_t* cr = cairo_create(app_state.surface);
        configure_crisp_rendering(cr);        
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
                app_state.floating_selection_active = false;
                app_state.selection_x1 = app_state.start_x;
                app_state.selection_y1 = app_state.start_y;
                app_state.selection_x2 = end_x;
                app_state.selection_y2 = end_y;
                break;
            case TOOL_LASSO_SELECT:
                finalize_lasso_selection();
                break;
        }
        
        cairo_destroy(cr);
        app_state.is_drawing = false;
        app_state.is_right_button = false;
        app_state.ellipse_center_mode = false;
        app_state.last_x = 0;
        app_state.last_y = 0;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

// File operations
void save_image_dialog(GtkWidget* parent) {
    if (app_state.floating_selection_active) {
        commit_floating_selection();
    }

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
    gtk_file_filter_add_pattern(filter_png, "*.PNG");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_png);

    GtkFileFilter* filter_jpg = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_jpg, "JPEG Images");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpg");
    gtk_file_filter_add_pattern(filter_jpg, "*.jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_jpg);

    GtkFileFilter* filter_xpm = gtk_file_filter_new();
    gtk_file_filter_set_name(filter_xpm, "XPM Images");
    gtk_file_filter_add_pattern(filter_xpm, "*.xpm");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_xpm);

    if (!app_state.current_filename.empty()) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), app_state.current_filename.c_str());
    } else {
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "untitled.png");
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (filename) {
            std::string fname(filename);
            size_t dot_pos = fname.find_last_of('.');
            std::string extension = (dot_pos == std::string::npos) ? "" : fname.substr(dot_pos + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension != "png") {
                fname += ".png";
            }

            app_state.current_filename = fname;
            cairo_surface_write_to_png(app_state.surface, fname.c_str());

            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
}

std::string get_file_extension_lowercase(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos || dot_pos + 1 >= filename.size()) {
        return "";
    }

    std::string extension = filename.substr(dot_pos + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension;
}

bool save_surface_to_file(cairo_surface_t* surface, const std::string& filename) {
    if (!surface || filename.empty()) {
        return false;
    }

    std::string extension = get_file_extension_lowercase(filename);
    if (extension == "jpg" || extension == "jpeg" || extension == "xpm") {
        cairo_surface_t* rgb_surface = cairo_image_surface_create(
            CAIRO_FORMAT_RGB24,
            app_state.canvas_width,
            app_state.canvas_height
        );
        cairo_t* cr = cairo_create(rgb_surface);
        configure_crisp_rendering(cr);
        cairo_set_source_surface(cr, surface, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);

        std::string temp_png = filename + ".temp.png";
        cairo_surface_write_to_png(rgb_surface, temp_png.c_str());
        cairo_surface_destroy(rgb_surface);

        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(temp_png.c_str(), &error);
        bool save_success = false;
        if (pixbuf) {
            if (extension == "xpm") {
                save_success = gdk_pixbuf_save(pixbuf, filename.c_str(), "xpm", &error, NULL);
            } else {
                save_success = gdk_pixbuf_save(pixbuf, filename.c_str(), "jpeg", &error, "quality", "95", NULL);
            }
            g_object_unref(pixbuf);
        }
        remove(temp_png.c_str());
        return save_success;
    }

    return cairo_surface_write_to_png(surface, filename.c_str()) == CAIRO_STATUS_SUCCESS;
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
    gtk_file_filter_set_name(filter_images, "PNG Images");
    gtk_file_filter_add_pattern(filter_images, "*.png");
    gtk_file_filter_add_pattern(filter_images, "*.PNG");
    gtk_file_filter_add_pattern(filter_images, "*.xpm");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_images);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (filename) {
            app_state.current_filename = filename;

            cairo_surface_t* loaded_surface = cairo_image_surface_create_from_png(filename);
            if (cairo_surface_status(loaded_surface) == CAIRO_STATUS_SUCCESS) {
                int width = cairo_image_surface_get_width(loaded_surface);
                int height = cairo_image_surface_get_height(loaded_surface);

                push_undo_state();

                app_state.canvas_width = width;
                app_state.canvas_height = height;

                if (app_state.surface) {
                    cairo_surface_destroy(app_state.surface);
                }

                app_state.surface = loaded_surface;

                gtk_widget_set_size_request(app_state.drawing_area,
                    static_cast<int>(width * app_state.zoom_factor),
                    static_cast<int>(height * app_state.zoom_factor));
                gtk_widget_queue_draw(app_state.drawing_area);
            } else {
                cairo_surface_destroy(loaded_surface);
            }

            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
}


// Menu callbacks
void on_file_new(GtkMenuItem* item, gpointer data) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "New Image",
        GTK_WINDOW(app_state.window),
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Create", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(container), 10);
    gtk_container_add(GTK_CONTAINER(content), container);

    GtkWidget* resolution_label = gtk_label_new("Resolution:");
    gtk_widget_set_halign(resolution_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(container), resolution_label, FALSE, FALSE, 0);

    GtkWidget* resolution_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combo), "256x256");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combo), "512x512");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combo), "1024x1024");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combo), "640x480");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combo), "800x600");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(resolution_combo), "Custom");
    gtk_combo_box_set_active(GTK_COMBO_BOX(resolution_combo), 4);
    gtk_box_pack_start(GTK_BOX(container), resolution_combo, FALSE, FALSE, 0);

    GtkWidget* custom_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* x_label = gtk_label_new("X:");
    GtkWidget* x_spin = gtk_spin_button_new_with_range(1, 10000, 1);
    GtkWidget* separator_label = gtk_label_new("x");
    GtkWidget* y_label = gtk_label_new("Y:");
    GtkWidget* y_spin = gtk_spin_button_new_with_range(1, 10000, 1);
    GtkWidget* pixels_label = gtk_label_new("pixels");

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(x_spin), 800);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(y_spin), 600);

    gtk_box_pack_start(GTK_BOX(custom_row), x_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_row), x_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_row), separator_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_row), y_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_row), y_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(custom_row), pixels_label, FALSE, FALSE, 0);
    gtk_widget_set_sensitive(custom_row, FALSE);
    gtk_box_pack_start(GTK_BOX(container), custom_row, FALSE, FALSE, 0);

    g_signal_connect(resolution_combo, "changed", G_CALLBACK(+[] (GtkComboBox* combo, gpointer user_data) {
        GtkWidget* row = GTK_WIDGET(user_data);
        GtkWidget* x_input = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "x-spin"));
        GtkWidget* y_input = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "y-spin"));

        gchar* selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
        bool is_custom = selected && g_strcmp0(selected, "Custom") == 0;
        gtk_widget_set_sensitive(row, is_custom);

        if (!is_custom && selected) {
            int width = 0;
            int height = 0;
            if (sscanf(selected, "%dx%d", &width, &height) == 2) {
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(x_input), width);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(y_input), height);
            }
        }

        g_free(selected);
    }), custom_row);

    g_object_set_data(G_OBJECT(custom_row), "x-spin", x_spin);
    g_object_set_data(G_OBJECT(custom_row), "y-spin", y_spin);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_OK) {
        gtk_widget_destroy(dialog);
        return;
    }

    int new_width = 800;
    int new_height = 600;
    gchar* selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(resolution_combo));
    if (selected && g_strcmp0(selected, "Custom") == 0) {
        new_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(x_spin));
        new_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(y_spin));
    } else if (selected) {
        if (sscanf(selected, "%dx%d", &new_width, &new_height) != 2) {
            new_width = 800;
            new_height = 600;
        }
    }
    g_free(selected);
    gtk_widget_destroy(dialog);

    if (app_state.surface) {
        push_undo_state();
    }

    app_state.canvas_width = new_width;
    app_state.canvas_height = new_height;
    gtk_widget_set_size_request(app_state.drawing_area, new_width, new_height);
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
        std::string filename = app_state.current_filename;
        size_t dot_pos = filename.find_last_of('.');
        std::string extension = (dot_pos == std::string::npos) ? "" : filename.substr(dot_pos + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        if (extension != "png") {
            filename += ".png";
            app_state.current_filename = filename;
        }
        save_surface_to_file(app_state.surface, app_state.current_filename);
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

void on_edit_copy(GtkMenuItem* item, gpointer data) {
    copy_selection();
}

void on_edit_cut(GtkMenuItem* item, gpointer data) {
    cut_selection();
}

void on_edit_paste(GtkMenuItem* item, gpointer data) {
    paste_selection();
}

void on_edit_undo(GtkMenuItem* item, gpointer data) {
    undo_last_operation();
}

void on_image_scale(GtkMenuItem* item, gpointer data) {
    if (!app_state.surface) return;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Scale Image",
        GTK_WINDOW(app_state.window),
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Scale", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(row), 10);
    gtk_container_add(GTK_CONTAINER(content), row);

    GtkWidget* percent_label = gtk_label_new("Scale (%):");
    GtkWidget* percent_spin = gtk_spin_button_new_with_range(1, 1000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(percent_spin), 100);

    gtk_box_pack_start(GTK_BOX(row), percent_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), percent_spin, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_OK) {
        gtk_widget_destroy(dialog);
        return;
    }

    double scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(percent_spin)) / 100.0;
    gtk_widget_destroy(dialog);

    int new_width = std::max(1, (int)std::lround(app_state.canvas_width * scale));
    int new_height = std::max(1, (int)std::lround(app_state.canvas_height * scale));

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* scaled_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);

    cairo_t* cr = cairo_create(scaled_surface);
    configure_crisp_rendering(cr);
    cairo_scale(cr, (double)new_width / app_state.canvas_width, (double)new_height / app_state.canvas_height);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = scaled_surface;
    app_state.canvas_width = new_width;
    app_state.canvas_height = new_height;
    cairo_surface_destroy(old_surface);

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }

    gtk_widget_set_size_request(app_state.drawing_area, new_width, new_height);
    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_image_resize_canvas(GtkMenuItem* item, gpointer data) {
    if (!app_state.surface) return;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Resize Image",
        GTK_WINDOW(app_state.window),
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Resize", GTK_RESPONSE_OK,
        NULL
    );

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(container), 10);
    gtk_container_add(GTK_CONTAINER(content), container);

    GtkWidget* width_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* width_label = gtk_label_new("Width:");
    GtkWidget* width_spin = gtk_spin_button_new_with_range(1, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(width_spin), app_state.canvas_width);
    gtk_box_pack_start(GTK_BOX(width_row), width_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(width_row), width_spin, FALSE, FALSE, 0);

    GtkWidget* height_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* height_label = gtk_label_new("Height:");
    GtkWidget* height_spin = gtk_spin_button_new_with_range(1, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(height_spin), app_state.canvas_height);
    gtk_box_pack_start(GTK_BOX(height_row), height_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(height_row), height_spin, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(container), width_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(container), height_row, FALSE, FALSE, 0);

    gtk_widget_show_all(dialog);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response != GTK_RESPONSE_OK) {
        gtk_widget_destroy(dialog);
        return;
    }

    int new_width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(width_spin));
    int new_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(height_spin));
    gtk_widget_destroy(dialog);

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* resized_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);

    cairo_t* cr = cairo_create(resized_surface);
    configure_crisp_rendering(cr);
    cairo_set_source_rgba(
        cr,
        app_state.bg_color.red,
        app_state.bg_color.green,
        app_state.bg_color.blue,
        app_state.bg_color.alpha
    );
    cairo_paint(cr);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = resized_surface;
    app_state.canvas_width = new_width;
    app_state.canvas_height = new_height;
    cairo_surface_destroy(old_surface);

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }

    gtk_widget_set_size_request(app_state.drawing_area, new_width, new_height);
    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_image_rotate_clockwise(GtkMenuItem* item, gpointer data) {
    if (!app_state.surface) return;

    if (app_state.has_selection) {
        if (!app_state.floating_selection_active) {
            start_selection_drag();
        }
        if (!app_state.floating_selection_active || !app_state.floating_surface) {
            return;
        }

        push_undo_state();

        SelectionPixelBounds bounds = get_selection_pixel_bounds();
        const int new_width = bounds.height;
        const int new_height = bounds.width;

        cairo_surface_t* old_floating_surface = app_state.floating_surface;
        cairo_surface_t* rotated_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);
        cairo_t* cr = cairo_create(rotated_surface);
        configure_crisp_rendering(cr);

        cairo_translate(cr, new_width, 0);
        cairo_rotate(cr, M_PI / 2.0);
        cairo_set_source_surface(cr, old_floating_surface, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);

        app_state.floating_surface = rotated_surface;
        cairo_surface_destroy(old_floating_surface);

        app_state.selection_is_rect = true;
        app_state.selection_path.clear();
        app_state.selection_x1 = bounds.x;
        app_state.selection_y1 = bounds.y;
        app_state.selection_x2 = bounds.x + new_width;
        app_state.selection_y2 = bounds.y + new_height;

        gtk_widget_queue_draw(app_state.drawing_area);
        return;
    }

    const int old_width = app_state.canvas_width;
    const int old_height = app_state.canvas_height;
    const int new_width = old_height;
    const int new_height = old_width;

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* rotated_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);
    cairo_t* cr = cairo_create(rotated_surface);
    configure_crisp_rendering(cr);

    cairo_translate(cr, new_width, 0);
    cairo_rotate(cr, M_PI / 2.0);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = rotated_surface;
    app_state.canvas_width = new_width;
    app_state.canvas_height = new_height;
    cairo_surface_destroy(old_surface);

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }

    gtk_widget_set_size_request(app_state.drawing_area, new_width, new_height);
    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_image_rotate_counter_clockwise(GtkMenuItem* item, gpointer data) {
    if (!app_state.surface) return;

    if (app_state.has_selection) {
        if (!app_state.floating_selection_active) {
            start_selection_drag();
        }
        if (!app_state.floating_selection_active || !app_state.floating_surface) {
            return;
        }

        push_undo_state();

        SelectionPixelBounds bounds = get_selection_pixel_bounds();
        const int new_width = bounds.height;
        const int new_height = bounds.width;

        cairo_surface_t* old_floating_surface = app_state.floating_surface;
        cairo_surface_t* rotated_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);
        cairo_t* cr = cairo_create(rotated_surface);
        configure_crisp_rendering(cr);

        cairo_translate(cr, 0, new_height);
        cairo_rotate(cr, -M_PI / 2.0);
        cairo_set_source_surface(cr, old_floating_surface, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);

        app_state.floating_surface = rotated_surface;
        cairo_surface_destroy(old_floating_surface);

        app_state.selection_is_rect = true;
        app_state.selection_path.clear();
        app_state.selection_x1 = bounds.x;
        app_state.selection_y1 = bounds.y;
        app_state.selection_x2 = bounds.x + new_width;
        app_state.selection_y2 = bounds.y + new_height;

        gtk_widget_queue_draw(app_state.drawing_area);
        return;
    }

    const int old_width = app_state.canvas_width;
    const int old_height = app_state.canvas_height;
    const int new_width = old_height;
    const int new_height = old_width;

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* rotated_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, new_width, new_height);
    cairo_t* cr = cairo_create(rotated_surface);
    configure_crisp_rendering(cr);

    cairo_translate(cr, 0, new_height);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = rotated_surface;
    app_state.canvas_width = new_width;
    app_state.canvas_height = new_height;
    cairo_surface_destroy(old_surface);

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }

    gtk_widget_set_size_request(app_state.drawing_area, new_width, new_height);
    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_image_flip_horizontal(GtkMenuItem* item, gpointer data) {
    if (!app_state.surface) return;

    if (app_state.has_selection) {
        if (!app_state.floating_selection_active) {
            start_selection_drag();
        }
        if (!app_state.floating_selection_active || !app_state.floating_surface) {
            return;
        }

        push_undo_state();

        SelectionPixelBounds bounds = get_selection_pixel_bounds();

        cairo_surface_t* old_floating_surface = app_state.floating_surface;
        cairo_surface_t* flipped_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bounds.width, bounds.height);
        cairo_t* cr = cairo_create(flipped_surface);
        configure_crisp_rendering(cr);

        cairo_translate(cr, bounds.width, 0);
        cairo_scale(cr, -1, 1);
        cairo_set_source_surface(cr, old_floating_surface, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);

        app_state.floating_surface = flipped_surface;
        cairo_surface_destroy(old_floating_surface);

        app_state.selection_is_rect = true;
        app_state.selection_path.clear();

        gtk_widget_queue_draw(app_state.drawing_area);
        return;
    }

    const int width = app_state.canvas_width;
    const int height = app_state.canvas_height;

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* flipped_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(flipped_surface);
    configure_crisp_rendering(cr);

    cairo_translate(cr, width, 0);
    cairo_scale(cr, -1, 1);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = flipped_surface;
    cairo_surface_destroy(old_surface);

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }

    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_image_flip_vertical(GtkMenuItem* item, gpointer data) {
    if (!app_state.surface) return;

    if (app_state.has_selection) {
        if (!app_state.floating_selection_active) {
            start_selection_drag();
        }
        if (!app_state.floating_selection_active || !app_state.floating_surface) {
            return;
        }

        push_undo_state();

        SelectionPixelBounds bounds = get_selection_pixel_bounds();

        cairo_surface_t* old_floating_surface = app_state.floating_surface;
        cairo_surface_t* flipped_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bounds.width, bounds.height);
        cairo_t* cr = cairo_create(flipped_surface);
        configure_crisp_rendering(cr);

        cairo_translate(cr, 0, bounds.height);
        cairo_scale(cr, 1, -1);
        cairo_set_source_surface(cr, old_floating_surface, 0, 0);
        cairo_paint(cr);
        cairo_destroy(cr);

        app_state.floating_surface = flipped_surface;
        cairo_surface_destroy(old_floating_surface);

        app_state.selection_is_rect = true;
        app_state.selection_path.clear();

        gtk_widget_queue_draw(app_state.drawing_area);
        return;
    }

    const int width = app_state.canvas_width;
    const int height = app_state.canvas_height;

    push_undo_state();

    cairo_surface_t* old_surface = app_state.surface;
    cairo_surface_t* flipped_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(flipped_surface);
    configure_crisp_rendering(cr);

    cairo_translate(cr, 0, height);
    cairo_scale(cr, 1, -1);
    cairo_set_source_surface(cr, old_surface, 0, 0);
    cairo_paint(cr);
    cairo_destroy(cr);

    app_state.surface = flipped_surface;
    cairo_surface_destroy(old_surface);

    clear_selection();
    if (app_state.text_active) {
        cancel_text();
    }

    gtk_widget_queue_draw(app_state.drawing_area);
}

void on_help_manual(GtkMenuItem* item, gpointer data) {
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(app_state.window),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Manual is not available yet."
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "Manual");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void on_help_about(GtkMenuItem* item, gpointer data) {
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(app_state.window),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Mate-Paint\nversion 1.0\nCopyright 2006\nJames Carthew"
    );
    gtk_window_set_title(GTK_WINDOW(dialog), "About");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Tool button callback
void on_tool_clicked(GtkButton* button, gpointer data) {
    Tool new_tool = (Tool)GPOINTER_TO_INT(data);
    
    // Cancel text if switching away from text tool (don't finalize empty text)
    if (app_state.text_active && new_tool != TOOL_TEXT) {
        cancel_text();
    }
    
    // Clear or commit selection when switching tools
    if (new_tool != app_state.current_tool) {
        if (new_tool != TOOL_RECT_SELECT && new_tool != TOOL_LASSO_SELECT) {
            if (app_state.floating_selection_active) {
                commit_floating_selection();
            } else {
                clear_selection();
            }
            if (!app_state.text_active) {
                stop_ant_animation();
            }
        }
    }
    
    app_state.current_tool = new_tool;
    if (tool_supports_line_thickness(new_tool)) {
        app_state.active_line_thickness_index = app_state.tool_line_thickness_indices[tool_to_index(new_tool)];
        app_state.line_width = line_thickness_options[app_state.active_line_thickness_index];
        update_line_thickness_buttons();
    }
        if (new_tool == TOOL_ZOOM) {
        update_zoom_buttons();
    }
    app_state.polygon_points.clear();
    app_state.polygon_finished = false;
    app_state.lasso_points.clear();
    app_state.lasso_polygon_mode = false;
    app_state.ellipse_center_mode = false;
    app_state.curve_active = false;
    app_state.curve_has_end = false;
    app_state.curve_has_control = false;
    update_line_thickness_visibility();
    update_zoom_visibility();
}

gboolean on_line_thickness_button_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    int index = GPOINTER_TO_INT(data);
    if (index < 0 || index >= (int)(sizeof(line_thickness_options) / sizeof(line_thickness_options[0]))) {
        return FALSE;
    }

    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_set_line_width(cr, line_thickness_options[index]);
    cairo_move_to(cr, 4, allocation.height / 2.0);
    cairo_line_to(cr, allocation.width - 4, allocation.height / 2.0);
    cairo_stroke(cr);

    return FALSE;
}

void update_line_thickness_buttons() {
    for (size_t i = 0; i < app_state.line_thickness_buttons.size(); ++i) {
        GtkWidget* button = app_state.line_thickness_buttons[i];
        bool is_active = ((int)i == app_state.active_line_thickness_index);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), is_active);
    }
}

void on_line_thickness_toggled(GtkToggleButton* button, gpointer data) {
    int index = GPOINTER_TO_INT(data);

    if (!gtk_toggle_button_get_active(button)) {
        if (app_state.active_line_thickness_index == index) {
            gtk_toggle_button_set_active(button, TRUE);
        }
        return;
    }

    app_state.active_line_thickness_index = index;
    app_state.line_width = line_thickness_options[index];
    if (tool_supports_line_thickness(app_state.current_tool)) {
        app_state.tool_line_thickness_indices[tool_to_index(app_state.current_tool)] = index;
    }

    for (size_t i = 0; i < app_state.line_thickness_buttons.size(); ++i) {
        if ((int)i != index) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_state.line_thickness_buttons[i]), FALSE);
        }
    }
}

GtkWidget* create_line_thickness_button(int index) {
    GtkWidget* button = gtk_toggle_button_new();
    gtk_widget_set_size_request(button, 66, 20);
    gtk_widget_set_tooltip_text(button, "Line thickness");
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "togglebutton {"
        "background: #ffffff;"
        "background-image: none;"
        "border: 1px solid #888;"
        "border-radius: 0;"
        "padding: 0;"
        "box-shadow: none;"
        "}"
        "togglebutton:checked {"
        "background: #ffffff;"
        "background-image: none;"
        "border: 1px solid #333;"
        "}"
        "togglebutton:hover {"
        "background: #ffffff;"
        "background-image: none;"
        "}",
        -1,
        NULL
    );
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(button),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);

    GtkWidget* preview = gtk_drawing_area_new();
    gtk_widget_set_size_request(preview, 58, 16);
    GtkCssProvider* preview_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(preview_provider,
        "drawingarea {"
        "background: #ffffff;"
        "background-image: none;"
        "}",
        -1,
        NULL
    );
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(preview),
        GTK_STYLE_PROVIDER(preview_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(preview_provider);

    g_signal_connect(preview, "draw", G_CALLBACK(on_line_thickness_button_draw), GINT_TO_POINTER(index));
    gtk_container_add(GTK_CONTAINER(button), preview);

    g_signal_connect(button, "toggled", G_CALLBACK(on_line_thickness_toggled), GINT_TO_POINTER(index));

    return button;
}

void update_line_thickness_visibility() {
    if (!app_state.line_thickness_box) {
        return;
    }

    if (tool_supports_line_thickness(app_state.current_tool)) {
        gtk_widget_show_all(app_state.line_thickness_box);
    } else {
        gtk_widget_hide(app_state.line_thickness_box);
    }
}

void update_zoom_buttons() {
    for (size_t i = 0; i < app_state.zoom_buttons.size(); ++i) {
        GtkWidget* button = app_state.zoom_buttons[i];
        bool is_active = ((int)i == app_state.active_zoom_index);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), is_active);
    }
}

void on_zoom_toggled(GtkToggleButton* button, gpointer data) {
    int index = GPOINTER_TO_INT(data);

    if (!gtk_toggle_button_get_active(button)) {
        if (app_state.active_zoom_index == index) {
            gtk_toggle_button_set_active(button, TRUE);
        }
        return;
    }

    app_state.active_zoom_index = index;
    for (size_t i = 0; i < app_state.zoom_buttons.size(); ++i) {
        if ((int)i != index) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app_state.zoom_buttons[i]), FALSE);
        }
    }
}

GtkWidget* create_zoom_button(int index) {
    gchar* zoom_label = g_strdup_printf("%dx", (int)zoom_options[index]);
    GtkWidget* button = gtk_toggle_button_new_with_label(zoom_label);
    g_free(zoom_label);
    gtk_widget_set_size_request(button, 66, 20);
    gtk_widget_set_tooltip_text(button, "Zoom level");
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "togglebutton {"
        "background: #ffffff;"
        "background-image: none;"
        "border: 1px solid #888;"
        "border-radius: 0;"
        "padding: 0;"
        "box-shadow: none;"
        "}"
        "togglebutton:checked {"
        "background: #ffffff;"
        "background-image: none;"
        "border: 1px solid #333;"
        "}"
        "togglebutton:hover {"
        "background: #ffffff;"
        "background-image: none;"
        "}",
        -1,
        NULL
    );
    gtk_style_context_add_provider(
        gtk_widget_get_style_context(button),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);

    g_signal_connect(button, "toggled", G_CALLBACK(on_zoom_toggled), GINT_TO_POINTER(index));
    return button;
}

void update_zoom_visibility() {
    if (!app_state.zoom_box) {
        return;
    }

    if (app_state.current_tool == TOOL_ZOOM) {
        gtk_widget_show_all(app_state.zoom_box);
    } else {
        gtk_widget_hide(app_state.zoom_box);
    }
}

void apply_color_button_style(GtkWidget* button, const GdkRGBA& color, bool is_custom_slot) {
    double brightness = (color.red * 0.299) + (color.green * 0.587) + (color.blue * 0.114);
    const char* text_color = brightness > 0.5 ? "#111" : "#fff";

    gchar* css = g_strdup_printf(
        "button { "
        "background-color: rgb(%d,%d,%d); "
        "color: %s; "
        "background-image: none; "
        "border: 1px solid #555; "
        "min-width: 18px; "
        "min-height: 18px; "
        "font-weight: bold; "
        "padding: 0; "
        "margin: 0; "
        "}"
        "button:hover { "
        "border: 1px solid #000; "
        "}",
        (int)(color.red * 255),
        (int)(color.green * 255),
        (int)(color.blue * 255),
        (is_custom_slot || is_transparent_color(color)) ? text_color : "transparent"
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
}

void show_custom_color_dialog(int index) {
    if (index < 0 || index >= (int)app_state.palette_button_colors.size()) {
        return;
    }

    GtkWidget* dialog = gtk_color_chooser_dialog_new("Custom color", GTK_WINDOW(app_state.window));
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(dialog), &app_state.palette_button_colors[index]);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        GdkRGBA selected_color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(dialog), &selected_color);
        app_state.palette_button_colors[index] = selected_color;

        if (index < (int)app_state.palette_buttons.size() && app_state.palette_buttons[index]) {
            apply_color_button_style(app_state.palette_buttons[index], selected_color, true);
        }

        save_custom_palette_colors();
    }

    gtk_widget_destroy(dialog);
}

// Color button callback
gboolean on_color_button_press(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    int index = GPOINTER_TO_INT(data);
    if (index < 0 || index >= (int)app_state.palette_button_colors.size()) {
        return TRUE;
    }

    bool is_custom_slot = index < (int)app_state.custom_palette_slots.size() && app_state.custom_palette_slots[index];
    if (is_custom_slot && event->type == GDK_2BUTTON_PRESS) {
        show_custom_color_dialog(index);
        return TRUE;
    }

    if (event->button == 1) {
        app_state.fg_color = app_state.palette_button_colors[index];
        update_color_indicators();
    } else if (event->button == 3) {
        app_state.bg_color = app_state.palette_button_colors[index];
        update_color_indicators();
    }
    return TRUE;
}

const char* get_palette_button_label(int index, bool is_custom_slot) {
    if (index == 0) {
        return "T";
    }

    if (is_custom_slot) {
        return "c";
    }

    return "";
}

const char* get_tool_icon_filename(Tool tool) {
    switch (tool) {
        case TOOL_LASSO_SELECT: return "stock-tool-free-select.png";
        case TOOL_RECT_SELECT: return "stock-tool-rect-select.png";
        case TOOL_ERASER: return "stock-tool-eraser.png";
        case TOOL_FILL: return "stock-tool-bucket-fill.png";
        case TOOL_EYEDROPPER: return "stock-tool-color-picker.png";
        case TOOL_ZOOM: return "stock-tool-zoom.png";
        case TOOL_PENCIL: return "stock-tool-pencil.png";
        case TOOL_PAINTBRUSH: return "stock-tool-paintbrush.png";
        case TOOL_AIRBRUSH: return "stock-tool-airbrush.png";
        case TOOL_TEXT: return "stock-tool-text.png";
        case TOOL_LINE: return "stock_draw-line.png";
        case TOOL_CURVE: return "stock_draw-curve.png";
        case TOOL_RECTANGLE: return "stock_draw-rectangle.png";
        case TOOL_POLYGON: return "stock_draw-fill_polygon.png";
        case TOOL_ELLIPSE: return "stock_draw-ellipse.png";
        case TOOL_ROUNDED_RECT: return "stock_draw-rounded-rectangle.png";
        default: return NULL;
    }
}

GtkWidget* create_tool_icon(Tool tool) {
    const char* icon_file = get_tool_icon_filename(tool);
    if (!icon_file) {
        return gtk_image_new();
    }

    const char* icon_roots[] = {
        "/usr/share/mate-paint",
        "."
    };

    for (gsize i = 0; i < G_N_ELEMENTS(icon_roots); i++) {
        gchar* icon_path = g_build_filename(icon_roots[i], "data", "icons", "16x16", "actions", icon_file, NULL);
        if (g_file_test(icon_path, G_FILE_TEST_EXISTS)) {
            GtkWidget* icon = gtk_image_new_from_file(icon_path);
            g_free(icon_path);
            return icon;
        }
        g_free(icon_path);
    }

    return gtk_image_new();
}

// Create tool button with tooltip
GtkWidget* create_tool_button(Tool tool, const char* tooltip) {
    GtkWidget* button = gtk_button_new();
    gtk_widget_set_size_request(button, 28, 28);
    
    GtkWidget* icon = create_tool_icon(tool);
    gtk_button_set_image(GTK_BUTTON(button), icon);
    
    // Set tooltip
    gtk_widget_set_tooltip_text(button, tooltip);
    
    g_signal_connect(button, "clicked", G_CALLBACK(on_tool_clicked), GINT_TO_POINTER(tool));
    
    return button;
}

// Create color button
GtkWidget* create_color_button(GdkRGBA color, int index, bool is_custom_slot) {
    GtkWidget* button = gtk_button_new_with_label(get_palette_button_label(index, is_custom_slot));
    gtk_widget_set_size_request(button, 18, 18);

    apply_color_button_style(button, color, is_custom_slot);

    gtk_widget_add_events(button, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(button, "button-press-event", G_CALLBACK(on_color_button_press), GINT_TO_POINTER(index));

    return button;
}

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    app_state.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app_state.window), "Mate-Paint");
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
    
    GtkWidget* edit_menu = gtk_menu_new();
    GtkWidget* edit_menu_item = gtk_menu_item_new_with_label("Edit");
    GtkWidget* edit_undo = gtk_menu_item_new_with_label("Undo");
    GtkWidget* edit_cut = gtk_menu_item_new_with_label("Cut");
    GtkWidget* edit_copy = gtk_menu_item_new_with_label("Copy");
    GtkWidget* edit_paste = gtk_menu_item_new_with_label("Paste");

    g_signal_connect(edit_undo, "activate", G_CALLBACK(on_edit_undo), NULL);
    g_signal_connect(edit_cut, "activate", G_CALLBACK(on_edit_cut), NULL);
    g_signal_connect(edit_copy, "activate", G_CALLBACK(on_edit_copy), NULL);
    g_signal_connect(edit_paste, "activate", G_CALLBACK(on_edit_paste), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), edit_undo);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), edit_cut);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), edit_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), edit_paste);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_menu_item), edit_menu);

    GtkWidget* image_menu = gtk_menu_new();
    GtkWidget* image_menu_item = gtk_menu_item_new_with_label("Image");
    GtkWidget* image_scale = gtk_menu_item_new_with_label("Scale Image...");
    GtkWidget* image_resize = gtk_menu_item_new_with_label("Resize Image...");
    GtkWidget* image_rotate_clockwise = gtk_menu_item_new_with_label("Rotate Clockwise");
    GtkWidget* image_rotate_counter_clockwise = gtk_menu_item_new_with_label("Rotate Counter-Clockwise");
    GtkWidget* image_flip_vertical = gtk_menu_item_new_with_label("Flip Vertical");
    GtkWidget* image_flip_horizontal = gtk_menu_item_new_with_label("Flip Horizontal");

    g_signal_connect(image_scale, "activate", G_CALLBACK(on_image_scale), NULL);
    g_signal_connect(image_resize, "activate", G_CALLBACK(on_image_resize_canvas), NULL);
    g_signal_connect(image_rotate_clockwise, "activate", G_CALLBACK(on_image_rotate_clockwise), NULL);
    g_signal_connect(image_rotate_counter_clockwise, "activate", G_CALLBACK(on_image_rotate_counter_clockwise), NULL);
    g_signal_connect(image_flip_vertical, "activate", G_CALLBACK(on_image_flip_vertical), NULL);
    g_signal_connect(image_flip_horizontal, "activate", G_CALLBACK(on_image_flip_horizontal), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), image_scale);
    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), image_resize);
    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), image_rotate_clockwise);
    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), image_rotate_counter_clockwise);
    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), image_flip_vertical);
    gtk_menu_shell_append(GTK_MENU_SHELL(image_menu), image_flip_horizontal);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(image_menu_item), image_menu);

    GtkWidget* help_menu = gtk_menu_new();
    GtkWidget* help_menu_item = gtk_menu_item_new_with_label("Help");
    GtkWidget* help_manual = gtk_menu_item_new_with_label("Manual");
    GtkWidget* help_about = gtk_menu_item_new_with_label("About");

    g_signal_connect(help_manual, "activate", G_CALLBACK(on_help_manual), NULL);
    g_signal_connect(help_about, "activate", G_CALLBACK(on_help_about), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_manual);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), help_about);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), image_menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_menu_item);
    
    gtk_box_pack_start(GTK_BOX(main_box), menubar, FALSE, FALSE, 0);
    
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);

    GtkWidget* tool_column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(tool_column, 5);
    gtk_widget_set_margin_end(tool_column, 5);
    gtk_widget_set_margin_top(tool_column, 5);
    
    GtkWidget* toolbox = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(toolbox), 2);
    gtk_grid_set_row_spacing(GTK_GRID(toolbox), 2);
    
    // Create tool buttons with tooltips
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_LASSO_SELECT, 
            "Lasso Select - Draw freehand selection"), 
        0, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_RECT_SELECT, 
            "Rectangle Select - Select rectangular regions (Ctrl+C to copy, Ctrl+X to cut)"), 
        1, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_FILL, 
            "Fill Tool - Fill areas with color"), 
        0, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_EYEDROPPER, 
            "Eyedropper - Pick color from canvas"), 
        1, 1, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_ERASER, 
            "Eraser - Erase to transparency"), 
        0, 2, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_ZOOM, 
            "Zoom Tool - Zoom in/out"), 
        1, 2, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_PENCIL, 
            "Pencil - Draw thin lines"), 
        0, 3, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_PAINTBRUSH, 
            "Paintbrush - Draw with brush strokes"), 
        1, 3, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_AIRBRUSH, 
            "Airbrush - Spray paint effect"), 
        0, 4, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_TEXT, 
            "Text Tool - Add text (Left-click outside to finalize, Right-click outside to cancel)"), 
        1, 4, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_LINE, 
            "Line Tool - Draw straight lines (hold Shift for horizontal/vertical)"), 
        0, 5, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_CURVE, 
            "Curve Tool - Draw curved lines"), 
        1, 5, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_RECTANGLE, 
            "Rectangle - Draw rectangles"), 
        0, 6, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_POLYGON, 
            "Polygon - Draw multi-sided shapes"), 
        1, 6, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_ELLIPSE, 
            "Ellipse/Circle - Draw ellipses (hold Shift for circles)"), 
        0, 7, 1, 1);
    
    gtk_grid_attach(GTK_GRID(toolbox), 
        create_tool_button(TOOL_ROUNDED_RECT, 
            "Rounded Rectangle - Draw rectangles with rounded corners"), 
        1, 7, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(tool_column), toolbox, FALSE, FALSE, 0);

    app_state.line_thickness_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_bottom(app_state.line_thickness_box, 5);
    for (int i = 0; i < (int)(sizeof(line_thickness_options) / sizeof(line_thickness_options[0])); ++i) {
        GtkWidget* thickness_button = create_line_thickness_button(i);
        app_state.line_thickness_buttons.push_back(thickness_button);
        gtk_box_pack_start(GTK_BOX(app_state.line_thickness_box), thickness_button, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(tool_column), app_state.line_thickness_box, FALSE, FALSE, 0);
    app_state.active_line_thickness_index = app_state.tool_line_thickness_indices[tool_to_index(app_state.current_tool)];
    app_state.line_width = line_thickness_options[app_state.active_line_thickness_index];
    update_line_thickness_buttons();
    update_line_thickness_visibility();
    app_state.zoom_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_bottom(app_state.zoom_box, 5);
    for (int i = 0; i < (int)(sizeof(zoom_options) / sizeof(zoom_options[0])); ++i) {
        GtkWidget* zoom_button = create_zoom_button(i);
        app_state.zoom_buttons.push_back(zoom_button);
        gtk_box_pack_start(GTK_BOX(app_state.zoom_box), zoom_button, FALSE, FALSE, 0);
    }
    gtk_box_pack_start(GTK_BOX(tool_column), app_state.zoom_box, FALSE, FALSE, 0);
    
    update_zoom_buttons();
    update_zoom_visibility();

    gtk_box_pack_start(GTK_BOX(content_box), tool_column, FALSE, FALSE, 0);

    GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
    app_state.scrolled_window = scrolled;
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    
    app_state.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(app_state.drawing_area,
        static_cast<int>(app_state.canvas_width * app_state.zoom_factor),
        static_cast<int>(app_state.canvas_height * app_state.zoom_factor));
    
    g_signal_connect(app_state.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(app_state.drawing_area, "button-press-event", G_CALLBACK(on_button_press), NULL);
    g_signal_connect(app_state.drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(app_state.drawing_area, "leave-notify-event", G_CALLBACK(on_leave_notify), NULL);
    g_signal_connect(app_state.drawing_area, "button-release-event", G_CALLBACK(on_button_release), NULL);
    
    gtk_widget_set_events(app_state.drawing_area, 
        GDK_BUTTON_PRESS_MASK | 
        GDK_BUTTON_RELEASE_MASK | 
        GDK_POINTER_MOTION_MASK |
        GDK_LEAVE_NOTIFY_MASK
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


    app_state.palette_button_colors.assign(std::begin(palette_colors), std::end(palette_colors));
    app_state.palette_button_colors.insert(
        app_state.palette_button_colors.end(),
        std::begin(additional_palette_colors),
        std::end(additional_palette_colors)
    );

    app_state.custom_palette_slots.assign(app_state.palette_button_colors.size(), false);
    const int custom_start_index = (int)app_state.palette_button_colors.size() - custom_palette_slot_count;
    for (int i = custom_start_index; i < (int)app_state.custom_palette_slots.size(); i++) {
        app_state.custom_palette_slots[i] = true;
    }

    load_custom_palette_colors();

    app_state.palette_buttons.clear();
    app_state.palette_buttons.reserve(app_state.palette_button_colors.size());

    int colors_per_row = 14;
    for (size_t i = 0; i < app_state.palette_button_colors.size(); i++) {
        bool is_custom_slot = app_state.custom_palette_slots[i];
        GtkWidget* color_btn = create_color_button(app_state.palette_button_colors[i], i, is_custom_slot);
        if (is_custom_slot) {
            gtk_widget_set_tooltip_text(color_btn, "Double-click to choose a custom colour");
        }

        app_state.palette_buttons.push_back(color_btn);

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
    update_line_thickness_visibility();
    update_zoom_visibility();
    gtk_main();
    
    stop_ant_animation();
    if (app_state.surface) {
        cairo_surface_destroy(app_state.surface);
    }
    if (app_state.clipboard_surface) {
        cairo_surface_destroy(app_state.clipboard_surface);
    }
    if (app_state.floating_surface) {
        cairo_surface_destroy(app_state.floating_surface);
    }

    save_custom_palette_colors();
    
    return 0;
}

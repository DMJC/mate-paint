#pragma once

#include <QApplication>
#include <QMainWindow>
#include <KXmlGuiWindow>
#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QColorDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QToolButton>
#include <QScrollArea>
#include <QInputDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QSettings>
#include <QTimer>
#include <QClipboard>
#include <QFontDialog>
#include <QTextEdit>
#include <QDialogButtonBox>
#include <QColor>
#include <QPointF>
#include <QPainterPath>
#include <QStandardPaths>
#include <QDir>
#include <QKeySequence>
#include <QIcon>
#include <QTranslator>
#include <QLibraryInfo>
#include <QProcess>

#include <vector>
#include <string>
#include <queue>
#include <cmath>
#include <algorithm>

const double line_thickness_options[] = {1.0, 2.0, 4.0, 6.0, 8.0};
const double zoom_options[] = {1.0, 2.0, 4.0, 6.0, 8.0};

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

struct UndoSnapshot {
    QImage surface;
    int width = 0;
    int height = 0;
};

class MainWindow;
class PaintCanvas;

struct AppState {
    Tool current_tool = TOOL_PENCIL;
    QColor fg_color = QColor(0, 127, 0, 255); // Green
    QColor bg_color = QColor(255, 255, 255, 255); // White
    QImage surface;
    int canvas_width = 800;
    int canvas_height = 600;
    double last_x = 0;
    double last_y = 0;
    bool is_drawing = false;
    bool is_right_button = false;
    bool shift_pressed = false;
    double line_width = 2.0;

    double start_x = 0;
    double start_y = 0;
    double current_x = 0;
    double current_y = 0;
    bool hover_in_canvas = false;
    double hover_x = 0;
    double hover_y = 0;
    std::vector<QPointF> polygon_points;
    bool polygon_finished = false;
    std::vector<QPointF> lasso_points;
    bool lasso_polygon_mode = false;
    bool ellipse_center_mode = false;

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

    bool has_selection = false;
    bool selection_is_rect = false;
    double selection_x1 = 0;
    double selection_y1 = 0;
    double selection_x2 = 0;
    double selection_y2 = 0;
    std::vector<QPointF> selection_path;
    QImage floating_surface;
    bool floating_selection_active = false;
    bool dragging_selection = false;
    bool floating_drag_completed = false;
    double selection_drag_offset_x = 0;
    double selection_drag_offset_y = 0;

    bool text_active = false;
    double text_x = 0;
    double text_y = 0;
    double text_box_width = 200;
    double text_box_height = 100;
    std::string text_content;
    std::string text_font_family = "Sans";
    int text_font_size = 14;
    QWidget* text_window = nullptr;
    QTextEdit* text_entry = nullptr;

    QImage clipboard_surface;
    int clipboard_width = 0;
    int clipboard_height = 0;

    double ant_offset = 0;
    QTimer* ant_timer = nullptr;

    std::vector<QPushButton*> line_thickness_buttons;
    int active_line_thickness_index = 1;
    std::vector<int> tool_line_thickness_indices = std::vector<int>(TOOL_COUNT, 1);
    std::vector<QPushButton*> zoom_buttons;
    int active_zoom_index = 0;
    double zoom_factor = 1.0;

    std::vector<QColor> palette_button_colors;
    std::vector<bool> custom_palette_slots;
    std::vector<QPushButton*> palette_buttons;

    std::string current_filename;

    std::vector<UndoSnapshot> undo_stack;
    std::vector<UndoSnapshot> redo_stack;
    static constexpr size_t max_undo_steps = 50;
    bool drag_undo_snapshot_taken = false;

    MainWindow* window = nullptr;
    PaintCanvas* drawing_area = nullptr;
    QLabel* canvas_dimensions_label = nullptr;
    QLabel* cursor_position_label = nullptr;
    QPushButton* fg_button = nullptr;
    QPushButton* bg_button = nullptr;
    QWidget* line_thickness_box = nullptr;
    QWidget* zoom_box = nullptr;
    QScrollArea* scrolled_window = nullptr;
};

extern AppState app_state;

void draw_selection_overlay(QPainter& p);
void draw_text_overlay(QPainter& p);
bool tool_needs_preview(Tool tool);
void draw_preview(QPainter& p);
void draw_hover_indicator(QPainter& p);

void clear_selection();
void commit_floating_selection(bool record_undo = true);
void start_selection_drag();
QRect get_selection_pixel_bounds();
void push_undo_state();
void configure_crisp_rendering(QPainter& p);

class PaintCanvas : public QWidget {
    Q_OBJECT
public:
    explicit PaintCanvas(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
};

class ColorButton : public QPushButton {
    Q_OBJECT
public:
    ColorButton(int index, bool is_custom_slot, QWidget* parent = nullptr);
    void setColor(const QColor& color);
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
private:
    int m_index;
    bool m_is_custom_slot;
    QColor m_color;
};

class FgBgButton : public QPushButton {
    Q_OBJECT
public:
    FgBgButton(bool is_fg, QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    bool m_is_fg;
};

class LineThicknessButton : public QPushButton {
    Q_OBJECT
public:
    LineThicknessButton(int index, QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    int m_index;
};

class MainWindow : public KXmlGuiWindow {
    Q_OBJECT
public:
    MainWindow();
    void updateLineThicknessButtons();
    void updateZoomButtons();
    void updateLineThicknessVisibility();
    void updateZoomVisibility();
    void updateColorIndicators();
    void cancelText();
    void finalizeText();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

public slots:
    void on_file_new();
    void on_file_open();
    void on_file_save();
    void on_file_save_as();
    void on_file_quit();
    void on_edit_copy();
    void on_edit_cut();
    void on_edit_paste();
    void on_edit_undo();
    void on_edit_redo();
    void on_image_scale();
    void on_image_resize_canvas();
    void on_image_rotate_clockwise();
    void on_image_rotate_counter_clockwise();
    void on_image_flip_vertical();
    void on_image_flip_horizontal();
    void on_help_manual();
    void on_help_about();
    void on_tool_clicked(int tool);
    void on_line_thickness_toggled();
    void on_zoom_toggled();
    void on_ant_timer();

private:
    void createMenus();
    void createToolbox(QVBoxLayout* tool_column);
    void createBottomBar(QVBoxLayout* main_box);
};

#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>
#include <KAboutData>
#include <KLocalizedString>
#include "matepaint.hpp"
#include <QPainterPath>
#include <QDesktopServices>
#include <QUrl>
#include <QImageReader>
#include <QImageWriter>

AppState app_state;

// Include the converted palette colors
const QColor palette_colors[] = {
    QColor(0, 0, 0, 0), // Transparency
    QColor(0, 0, 0, 255), // Black
    QColor(51, 51, 51, 255), // Dark gray
    QColor(127, 127, 127, 255), // Gray
    QColor(127, 0, 0, 255), // Dark red
    QColor(204, 0, 0, 255), // Red
    QColor(255, 102, 0, 255), // Orange
    QColor(255, 204, 0, 255), // Yellow-orange
    QColor(255, 255, 0, 255), // Yellow
    QColor(204, 255, 0, 255), // Yellow-green
    QColor(0, 255, 0, 255), // Bright green
    QColor(0, 255, 127, 255), // Cyan-green
    QColor(0, 255, 255, 255), // Cyan
    QColor(0, 127, 255, 255), // Light blue
    QColor(0, 0, 255, 255), // Blue
    QColor(127, 0, 255, 255), // Purple
    QColor(204, 0, 204, 255), // Magenta
    QColor(255, 255, 255, 255), // White
    QColor(178, 178, 178, 255), // Light gray
    QColor(102, 51, 0, 255), // Brown
    QColor(255, 178, 178, 255), // Light pink
    QColor(255, 229, 178, 255), // Cream
    QColor(255, 255, 204, 255), // Light yellow
    QColor(204, 255, 204, 255), // Light green
    QColor(178, 229, 255, 255), // Light cyan
    QColor(178, 178, 255, 255), // Light blue
    QColor(229, 178, 255, 255), // Light purple
};

const QColor additional_palette_colors[] = {
    QColor(51, 0, 102, 255), // Deep purple
    QColor(153, 153, 153, 255), // Custom slot 1 default
    QColor(153, 153, 153, 255), // Custom slot 2 default
    QColor(153, 153, 153, 255), // Custom slot 3 default
    QColor(153, 153, 153, 255), // Custom slot 4 default
    QColor(153, 153, 153, 255), // Custom slot 5 default
    QColor(153, 153, 153, 255), // Custom slot 6 default
    QColor(153, 153, 153, 255), // Custom slot 7 default
    QColor(153, 153, 153, 255), // Custom slot 8 default
    QColor(153, 153, 153, 255), // Custom slot 9 default
    QColor(153, 153, 153, 255), // Custom slot 10 default
    QColor(153, 153, 153, 255), // Custom slot 11 default
    QColor(153, 153, 153, 255), // Custom slot 12 default
    QColor(153, 153, 153, 255), // Custom slot 13 default
    QColor(153, 153, 153, 255), // Custom slot 14 default
};

constexpr int custom_palette_slot_count = 14;

// Forward declarations and helpers
int tool_to_index(Tool tool) {
    return static_cast<int>(tool);
}

void load_custom_palette_colors() {
    QSettings settings("mate", "mate-paint");
    settings.beginGroup("palette");
    const int custom_start_index = app_state.palette_button_colors.size() - custom_palette_slot_count;
    for (int i = 0; i < custom_palette_slot_count; ++i) {
        QString key = QString("custom_color_%1").arg(i + 1);
        if (settings.contains(key)) {
            QColor color(settings.value(key).toString());
            if (color.isValid()) {
                app_state.palette_button_colors[custom_start_index + i] = color;
            }
        }
    }
    settings.endGroup();
}

void save_custom_palette_colors() {
    QSettings settings("mate", "mate-paint");
    settings.beginGroup("palette");
    const int custom_start_index = app_state.palette_button_colors.size() - custom_palette_slot_count;
    for (int i = 0; i < custom_palette_slot_count; ++i) {
        QString key = QString("custom_color_%1").arg(i + 1);
        QColor color = app_state.palette_button_colors[custom_start_index + i];
        settings.setValue(key, color.name(QColor::HexArgb));
    }
    settings.endGroup();
}

void init_surface(QWidget* widget) {
    app_state.surface = QImage(app_state.canvas_width, app_state.canvas_height, QImage::Format_ARGB32_Premultiplied);
    app_state.surface.fill(Qt::white);
}

// GUI Classes

PaintCanvas::PaintCanvas(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
}

void PaintCanvas::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    // Draw checkerboard
    int checker_size = 10;
    for (int y = 0; y < height(); y += checker_size) {
        for (int x = 0; x < width(); x += checker_size) {
            if ((x / checker_size + y / checker_size) % 2 == 0) {
                p.fillRect(x, y, checker_size, checker_size, QColor(200, 200, 200));
            } else {
                p.fillRect(x, y, checker_size, checker_size, QColor(255, 255, 255));
            }
        }
    }

    p.save();
    p.scale(app_state.zoom_factor, app_state.zoom_factor);
    p.drawImage(0, 0, app_state.surface);

    if (app_state.floating_selection_active && !app_state.floating_surface.isNull()) {
        double x = std::round(std::min(app_state.selection_x1, app_state.selection_x2));
        double y = std::round(std::min(app_state.selection_y1, app_state.selection_y2));
        p.drawImage(x, y, app_state.floating_surface);
    }

    if (app_state.has_selection) {
        draw_selection_overlay(p);
    }

    if (app_state.text_active) {
        draw_text_overlay(p);
    }

    if (tool_needs_preview(app_state.current_tool)) {
        draw_preview(p);
    }

    draw_hover_indicator(p);

    p.restore();
}



MainWindow::MainWindow() {
    app_state.window = this;
    setWindowTitle(i18n("Mate-Paint"));
    resize(900, 700);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    createMenus();

    QHBoxLayout *contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout, 1);

    QVBoxLayout *toolColumn = new QVBoxLayout();
    toolColumn->setContentsMargins(5, 5, 5, 5);
    createToolbox(toolColumn);
    contentLayout->addLayout(toolColumn);

    QScrollArea *scrollArea = new QScrollArea();
    app_state.scrolled_window = scrollArea;
    app_state.drawing_area = new PaintCanvas();
    scrollArea->setWidget(app_state.drawing_area);
    scrollArea->setWidgetResizable(true);
    contentLayout->addWidget(scrollArea, 1);

    createBottomBar(mainLayout);

    init_surface(app_state.drawing_area);

    app_state.ant_timer = new QTimer(this);
    connect(app_state.ant_timer, &QTimer::timeout, this, &MainWindow::on_ant_timer);
    app_state.ant_timer->start(50);
}

void MainWindow::createMenus() {
    QMenu *fileMenu = menuBar()->addMenu(i18n("&File"));
    KStandardAction::openNew(this, SLOT(on_file_new()), actionCollection());
    KStandardAction::open(this, SLOT(on_file_open()), actionCollection());
    KStandardAction::save(this, SLOT(on_file_save()), actionCollection());
    KStandardAction::saveAs(this, SLOT(on_file_save_as()), actionCollection());
    KStandardAction::quit(qApp, SLOT(quit()), actionCollection());

    fileMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::New)));
    fileMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Open)));
    fileMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Save)));
    fileMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::SaveAs)));
    fileMenu->addSeparator();
    fileMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Quit)));

    QMenu *editMenu = menuBar()->addMenu(i18n("&Edit"));
    KStandardAction::undo(this, SLOT(on_edit_undo()), actionCollection());
    KStandardAction::redo(this, SLOT(on_edit_redo()), actionCollection());
    KStandardAction::cut(this, SLOT(on_edit_cut()), actionCollection());
    KStandardAction::copy(this, SLOT(on_edit_copy()), actionCollection());
    KStandardAction::paste(this, SLOT(on_edit_paste()), actionCollection());

    editMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Undo)));
    editMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Redo)));
    editMenu->addSeparator();
    editMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Cut)));
    editMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Copy)));
    editMenu->addAction(actionCollection()->action(KStandardAction::name(KStandardAction::Paste)));

    QMenu *imageMenu = menuBar()->addMenu(i18n("&Image"));
    imageMenu->addAction(i18n("Scale Image..."), this, &MainWindow::on_image_scale);
    imageMenu->addAction(i18n("Resize Image..."), this, &MainWindow::on_image_resize_canvas);
    imageMenu->addSeparator();
    imageMenu->addAction(QIcon::fromTheme("object-rotate-right"), i18n("Rotate Clockwise"), this, &MainWindow::on_image_rotate_clockwise);
    imageMenu->addAction(QIcon::fromTheme("object-rotate-left"), i18n("Rotate Counter-Clockwise"), this, &MainWindow::on_image_rotate_counter_clockwise);
    imageMenu->addAction(QIcon::fromTheme("object-flip-vertical"), i18n("Flip Vertical"), this, &MainWindow::on_image_flip_vertical);
    imageMenu->addAction(QIcon::fromTheme("object-flip-horizontal"), i18n("Flip Horizontal"), this, &MainWindow::on_image_flip_horizontal);

    QMenu *helpMenu = menuBar()->addMenu(i18n("&Help"));
    helpMenu->addAction(QIcon::fromTheme("help-contents"), i18n("Contents"), this, &MainWindow::on_help_manual, QKeySequence::HelpContents);
    helpMenu->addAction(QIcon::fromTheme("help-about"), i18n("About"), this, &MainWindow::on_help_about);

    setupGUI(Default, "matepaintui.rc");
}

void MainWindow::createToolbox(QVBoxLayout *toolColumn) {
    QGridLayout *toolbox = new QGridLayout();
    toolbox->setSpacing(2);

    const char* tool_icons[TOOL_COUNT] = {
        "stock-tool-free-select.png", "stock-tool-rect-select.png",
        "stock-tool-eraser.png", "stock-tool-bucket-fill.png",
        "stock-tool-color-picker.png", "stock-tool-zoom.png",
        "stock-tool-pencil.png", "stock-tool-paintbrush.png",
        "stock-tool-airbrush.png", "stock-tool-text.png",
        "stock_draw-line.png", "stock_draw-curve.png",
        "stock_draw-rectangle.png", "stock_draw-fill_polygon.png",
        "stock_draw-ellipse.png", "stock_draw-rounded-rectangle.png"
    };

    QString icon_dir = ":/data/icons/16x16/actions/";

    for (int i = 0; i < TOOL_COUNT; ++i) {
        QPushButton* btn = new QPushButton();
        btn->setFixedSize(28, 28);
        btn->setIcon(QIcon(icon_dir + tool_icons[i]));
        connect(btn, &QPushButton::clicked, this, [this, i]() { this->on_tool_clicked(i); });
        toolbox->addWidget(btn, i / 2, i % 2);
    }
    toolColumn->addLayout(toolbox);

    app_state.line_thickness_box = new QWidget();
    QVBoxLayout* line_layout = new QVBoxLayout(app_state.line_thickness_box);
    line_layout->setContentsMargins(0,0,0,0);
    line_layout->setSpacing(2);
    for (int i = 0; i < (int)(sizeof(line_thickness_options) / sizeof(line_thickness_options[0])); ++i) {
        LineThicknessButton* btn = new LineThicknessButton(i);
        btn->setCheckable(true);
        app_state.line_thickness_buttons.push_back(btn);
        line_layout->addWidget(btn);
        connect(btn, &QPushButton::toggled, this, &MainWindow::on_line_thickness_toggled);
    }
    toolColumn->addWidget(app_state.line_thickness_box);

    app_state.zoom_box = new QWidget();
    QVBoxLayout* zoom_layout = new QVBoxLayout(app_state.zoom_box);
    zoom_layout->setContentsMargins(0,0,0,0);
    zoom_layout->setSpacing(2);
    for (int i = 0; i < (int)(sizeof(zoom_options) / sizeof(zoom_options[0])); ++i) {
        QPushButton* btn = new QPushButton(QString("%1x").arg((int)zoom_options[i]));
        btn->setCheckable(true);
        app_state.zoom_buttons.push_back(btn);
        zoom_layout->addWidget(btn);
        connect(btn, &QPushButton::toggled, this, &MainWindow::on_zoom_toggled);
    }
    toolColumn->addWidget(app_state.zoom_box);

    toolColumn->addStretch();
}

void MainWindow::createBottomBar(QVBoxLayout *mainLayout) {
    QHBoxLayout *bottomBox = new QHBoxLayout();
    bottomBox->setContentsMargins(5, 5, 5, 5);

    app_state.fg_button = new FgBgButton(true);
    app_state.fg_button->setFixedSize(36, 36);
    bottomBox->addWidget(app_state.fg_button);

    app_state.bg_button = new FgBgButton(false);
    app_state.bg_button->setFixedSize(36, 36);
    bottomBox->addWidget(app_state.bg_button);

    QGridLayout *palette_grid = new QGridLayout();
    palette_grid->setSpacing(2);

    app_state.palette_button_colors.clear();
    for (const QColor& c : palette_colors) app_state.palette_button_colors.push_back(c);
    for (const QColor& c : additional_palette_colors) app_state.palette_button_colors.push_back(c);

    app_state.custom_palette_slots.assign(app_state.palette_button_colors.size(), false);
    const int custom_start_index = app_state.palette_button_colors.size() - custom_palette_slot_count;
    for (int i = custom_start_index; i < (int)app_state.custom_palette_slots.size(); i++) {
        app_state.custom_palette_slots[i] = true;
    }

    load_custom_palette_colors();

    int colors_per_row = 14;
    for (size_t i = 0; i < app_state.palette_button_colors.size(); i++) {
        bool is_custom_slot = app_state.custom_palette_slots[i];
        ColorButton* color_btn = new ColorButton(i, is_custom_slot);
        color_btn->setColor(app_state.palette_button_colors[i]);
        app_state.palette_buttons.push_back(color_btn);
        palette_grid->addWidget(color_btn, i / colors_per_row, i % colors_per_row);
    }
    bottomBox->addLayout(palette_grid);

    QVBoxLayout *status_box = new QVBoxLayout();
    status_box->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    app_state.canvas_dimensions_label = new QLabel("800x600");
    app_state.canvas_dimensions_label->setAlignment(Qt::AlignRight);
    status_box->addWidget(app_state.canvas_dimensions_label);

    app_state.cursor_position_label = new QLabel("-");
    app_state.cursor_position_label->setAlignment(Qt::AlignRight);
    status_box->addWidget(app_state.cursor_position_label);

    bottomBox->addLayout(status_box);
    mainLayout->addLayout(bottomBox);
}



int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("matepaint");

    MainWindow window;
    window.show();

    return app.exec();
}

ColorButton::ColorButton(int index, bool is_custom_slot, QWidget* parent) : QPushButton(parent), m_index(index), m_is_custom_slot(is_custom_slot) {}
void ColorButton::setColor(const QColor& color) { m_color = color; }
void ColorButton::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.fillRect(rect(), m_color);
    p.setPen(QPen(Qt::darkGray, 1));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
    if (m_color.alpha() < 255) {
        p.setPen(Qt::black);
        p.drawText(rect(), Qt::AlignCenter, "T");
    } else if (m_is_custom_slot) {
        p.setPen(m_color.lightness() > 128 ? Qt::black : Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "c");
    }
}
void ColorButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        app_state.fg_color = m_color;
        app_state.window->updateColorIndicators();
    } else if (event->button() == Qt::RightButton) {
        app_state.bg_color = m_color;
        app_state.window->updateColorIndicators();
    }
}
void ColorButton::mouseDoubleClickEvent(QMouseEvent* event) {
    if (m_is_custom_slot) {
        QColor c = QColorDialog::getColor(m_color, this, i18n("Custom Color"));
        if (c.isValid()) {
            m_color = c;
            app_state.palette_button_colors[m_index] = c;
            save_custom_palette_colors();
            update();
        }
    }
}

FgBgButton::FgBgButton(bool is_fg, QWidget* parent) : QPushButton(parent), m_is_fg(is_fg) {}
void FgBgButton::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    QColor c = m_is_fg ? app_state.fg_color : app_state.bg_color;
    p.fillRect(rect(), c);
    p.setPen(QPen(Qt::darkGray, 2));
    p.drawRect(rect().adjusted(1, 1, -1, -1));
    if (c.alpha() < 255) {
        p.setPen(Qt::black);
        p.drawText(rect(), Qt::AlignCenter, "T");
    }
}

LineThicknessButton::LineThicknessButton(int index, QWidget* parent) : QPushButton(parent), m_index(index) {}
void LineThicknessButton::paintEvent(QPaintEvent* event) {
    QPushButton::paintEvent(event);
    QPainter p(this);
    p.setPen(QPen(Qt::black, line_thickness_options[m_index]));
    int y = height() / 2;
    p.drawLine(4, y, width() - 4, y);
}


void MainWindow::updateLineThicknessButtons() {
    for (size_t i = 0; i < app_state.line_thickness_buttons.size(); ++i) {
        app_state.line_thickness_buttons[i]->setChecked((int)i == app_state.active_line_thickness_index);
    }
}
void MainWindow::updateZoomButtons() {
    for (size_t i = 0; i < app_state.zoom_buttons.size(); ++i) {
        app_state.zoom_buttons[i]->setChecked((int)i == app_state.active_zoom_index);
    }
}
void MainWindow::updateLineThicknessVisibility() {
    bool visible = false;
    Tool t = app_state.current_tool;
    if (t == TOOL_PAINTBRUSH || t == TOOL_AIRBRUSH || t == TOOL_ERASER ||
        t == TOOL_LINE || t == TOOL_CURVE || t == TOOL_RECTANGLE ||
        t == TOOL_POLYGON || t == TOOL_ELLIPSE || t == TOOL_ROUNDED_RECT) {
        visible = true;
    }
    if (app_state.line_thickness_box) app_state.line_thickness_box->setVisible(visible);
}
void MainWindow::updateZoomVisibility() {
    if (app_state.zoom_box) app_state.zoom_box->setVisible(app_state.current_tool == TOOL_ZOOM);
}
void MainWindow::updateColorIndicators() {
    if (app_state.fg_button) app_state.fg_button->update();
    if (app_state.bg_button) app_state.bg_button->update();
}
void MainWindow::cancelText() {
    app_state.text_active = false;
    app_state.text_content.clear();

    if (app_state.text_window) {
        app_state.text_window->deleteLater();
        app_state.text_window = nullptr;
        app_state.text_entry = nullptr;
    }

    if (app_state.drawing_area) {
        app_state.drawing_area->update();
    }
}
void MainWindow::finalizeText() {
    if (!app_state.text_active || app_state.text_content.empty()) {
        if (app_state.text_active) cancelText();
        return;
    }

    push_undo_state();

    QPainter p(&app_state.surface);
    configure_crisp_rendering(p);

    QFont font(QString::fromStdString(app_state.text_font_family), app_state.text_font_size);
    p.setFont(font);
    p.setPen(app_state.fg_color);

    QRectF textRect(app_state.text_x + 5, app_state.text_y + 5, app_state.text_box_width - 10, app_state.text_box_height - 10);
    p.drawText(textRect, Qt::TextWordWrap | Qt::AlignTop | Qt::AlignLeft, QString::fromStdString(app_state.text_content));

    cancelText();
}


void draw_ant_path(QPainter& p) {
    QPen pen(Qt::black, 1, Qt::DashLine);
    QVector<qreal> dashes;
    dashes << 4.0 << 4.0;
    pen.setDashPattern(dashes);
    pen.setDashOffset(app_state.ant_offset);
    p.setPen(pen);
}

void configure_crisp_rendering(QPainter& p) {
    p.setRenderHint(QPainter::Antialiasing, false);
}

void draw_canvas_grid_background(QPainter& p, double width, double height) {
    // simplified implementation
    p.fillRect(0, 0, width, height, Qt::white);
}

void draw_black_outline_circle(QPainter& p, double x, double y, double radius) {
    p.save();
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(x, y), radius, radius);
    p.restore();
}

bool tool_shows_brush_hover_outline(Tool tool) {
    return tool == TOOL_PAINTBRUSH || tool == TOOL_AIRBRUSH || tool == TOOL_ERASER || tool == TOOL_ELLIPSE ||  tool == TOOL_LASSO_SELECT;
}

bool tool_shows_vertex_hover_markers(Tool tool) {
    return tool == TOOL_LINE || tool == TOOL_CURVE || tool == TOOL_POLYGON;
}

bool tool_needs_preview(Tool tool) {
    return tool == TOOL_LASSO_SELECT || tool == TOOL_RECT_SELECT ||
           tool == TOOL_LINE || tool == TOOL_CURVE ||
           tool == TOOL_RECTANGLE || tool == TOOL_POLYGON ||
           tool == TOOL_ELLIPSE || tool == TOOL_ROUNDED_RECT;
}

void draw_hover_indicator(QPainter& p) {
    if (!app_state.hover_in_canvas) return;

    if (tool_shows_brush_hover_outline(app_state.current_tool) && !app_state.is_drawing) {
        double radius = app_state.line_width;
        if (app_state.current_tool == TOOL_ERASER) {
            radius = app_state.line_width * 1.5;
        } else if (app_state.current_tool == TOOL_AIRBRUSH) {
            radius = app_state.line_width * 5.0;
        }
        draw_black_outline_circle(p, app_state.hover_x, app_state.hover_y, radius);
        return;
    }

    if (tool_shows_vertex_hover_markers(app_state.current_tool) && !app_state.is_drawing) {
        draw_black_outline_circle(p, app_state.hover_x, app_state.hover_y, 5.0);
    }
}

void constrain_line(double start_x, double start_y, double& end_x, double& end_y) {
    double dx = end_x - start_x;
    double dy = end_y - start_y;

    if (std::abs(dx) > std::abs(dy)) {
        end_y = start_y;
    } else {
        end_x = start_x;
    }
}

void constrain_to_circle(double start_x, double start_y, double& end_x, double& end_y) {
    double dx = end_x - start_x;
    double dy = end_y - start_y;
    double radius = std::max(std::abs(dx), std::abs(dy));

    end_x = start_x + (dx >= 0 ? radius : -radius);
    end_y = start_y + (dy >= 0 ? radius : -radius);
}

void constrain_to_square(double start_x, double start_y, double& end_x, double& end_y) {
    double dx = end_x - start_x;
    double dy = end_y - start_y;
    double size = std::max(std::abs(dx), std::abs(dy));

    end_x = start_x + (dx >= 0 ? size : -size);
    end_y = start_y + (dy >= 0 ? size : -size);
}

void append_selection_path(QPainterPath& path) {
    if (app_state.selection_path.size() < 3) return;
    path.moveTo(app_state.selection_path[0]);
    for (size_t i = 1; i < app_state.selection_path.size(); ++i) {
        path.lineTo(app_state.selection_path[i]);
    }
    path.closeSubpath();
}

void draw_selection_overlay(QPainter& p) {
    if (!app_state.has_selection) return;
    draw_ant_path(p);

    if (app_state.selection_is_rect) {
        double x1 = std::min(app_state.selection_x1, app_state.selection_x2);
        double y1 = std::min(app_state.selection_y1, app_state.selection_y2);
        double x2 = std::max(app_state.selection_x1, app_state.selection_x2);
        double y2 = std::max(app_state.selection_y1, app_state.selection_y2);
        p.drawRect(QRectF(x1, y1, x2 - x1, y2 - y1));
    } else if (app_state.selection_path.size() > 1) {
        QPainterPath path;
        path.moveTo(app_state.selection_path[0]);
        for (size_t i = 1; i < app_state.selection_path.size(); ++i) {
            path.lineTo(app_state.selection_path[i]);
        }
        path.closeSubpath();
        p.drawPath(path);
    }
}

void draw_text_overlay(QPainter& p) {
    if (!app_state.text_active) return;

    draw_ant_path(p);
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(app_state.text_x, app_state.text_y, app_state.text_box_width, app_state.text_box_height));

    if (!app_state.text_content.empty()) {
        QFont font(QString::fromStdString(app_state.text_font_family), app_state.text_font_size);
        p.setFont(font);
        p.setPen(app_state.fg_color);
        QRectF textRect(app_state.text_x + 5, app_state.text_y + 5, app_state.text_box_width - 10, app_state.text_box_height - 10);
        p.drawText(textRect, Qt::TextWordWrap | Qt::AlignTop | Qt::AlignLeft, QString::fromStdString(app_state.text_content));
    }
}

void draw_preview(QPainter& p) {
    if (!app_state.is_drawing) return;
    if (app_state.dragging_selection) return;

    p.save();

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
                draw_ant_path(p);
                draw_black_outline_circle(p, app_state.curve_start_x, app_state.curve_start_y, 5.0);
                if (app_state.curve_has_end) {
                    draw_black_outline_circle(p, app_state.curve_end_x, app_state.curve_end_y, 5.0);
                }

                if (app_state.curve_has_end) {
                    QPainterPath path;
                    path.moveTo(app_state.curve_start_x, app_state.curve_start_y);
                    if (app_state.curve_has_control) {
                        path.quadTo(app_state.curve_control_x, app_state.curve_control_y, app_state.curve_end_x, app_state.curve_end_y);
                    } else {
                        path.lineTo(app_state.curve_end_x, app_state.curve_end_y);
                    }
                    p.drawPath(path);
                }
            }
            break;
        }

        case TOOL_RECT_SELECT: {
            double x = std::min(app_state.start_x, preview_x);
            double y = std::min(app_state.start_y, preview_y);
            double w = std::abs(preview_x - app_state.start_x);
            double h = std::abs(preview_y - app_state.start_y);

            draw_ant_path(p);
            p.drawRect(QRectF(x, y, w, h));
            break;
        }

        case TOOL_LASSO_SELECT: {
            if (app_state.lasso_points.size() > 1) {
                draw_ant_path(p);
                QPainterPath path;
                path.moveTo(app_state.lasso_points[0]);
                for (size_t i = 1; i < app_state.lasso_points.size(); ++i) {
                    path.lineTo(app_state.lasso_points[i]);
                }
                if (app_state.lasso_polygon_mode) {
                    path.lineTo(preview_x, preview_y);
                }
                p.drawPath(path);
            }
            if (app_state.lasso_polygon_mode) {
                for (const auto& point : app_state.lasso_points) {
                    draw_black_outline_circle(p, point.x(), point.y(), 5.0);
                }
            }
            break;
        }

        case TOOL_LINE: {
            draw_ant_path(p);
            p.drawLine(QPointF(app_state.start_x, app_state.start_y), QPointF(preview_x, preview_y));
            draw_black_outline_circle(p, app_state.start_x, app_state.start_y, 5.0);
            draw_black_outline_circle(p, preview_x, preview_y, 5.0);
            break;
        }

        case TOOL_RECTANGLE: {
            double x = std::min(app_state.start_x, preview_x);
            double y = std::min(app_state.start_y, preview_y);
            double w = std::abs(preview_x - app_state.start_x);
            double h = std::abs(preview_y - app_state.start_y);

            draw_ant_path(p);
            p.drawRect(QRectF(x, y, w, h));
            break;
        }

        case TOOL_ELLIPSE: {
            double cx, cy, rx, ry;

            if (app_state.ellipse_center_mode) {
                double radius = std::hypot(preview_x - app_state.start_x, preview_y - app_state.start_y);
                cx = app_state.start_x;
                cy = app_state.start_y;
                rx = radius;
                ry = radius;
            } else {
                cx = (app_state.start_x + preview_x) / 2.0;
                cy = (app_state.start_y + preview_y) / 2.0;
                rx = std::abs(preview_x - app_state.start_x) / 2.0;
                ry = std::abs(preview_y - app_state.start_y) / 2.0;
            }

            if (rx > 0.1 && ry > 0.1) {
                draw_ant_path(p);
                p.drawEllipse(QPointF(cx, cy), rx, ry);
            }
            break;
        }

        case TOOL_ROUNDED_RECT: {
            double x = std::min(app_state.start_x, preview_x);
            double y = std::min(app_state.start_y, preview_y);
            double w = std::abs(preview_x - app_state.start_x);
            double h = std::abs(preview_y - app_state.start_y);
            double r = std::min(w, h) * 0.1;

            if (w > 1 && h > 1) {
                draw_ant_path(p);
                p.drawRoundedRect(QRectF(x, y, w, h), r, r);
            }
            break;
        }

        case TOOL_POLYGON: {
            if (app_state.polygon_points.size() > 0) {
                draw_ant_path(p);
                QPainterPath path;
                path.moveTo(app_state.polygon_points[0]);
                for (size_t i = 1; i < app_state.polygon_points.size(); ++i) {
                    path.lineTo(app_state.polygon_points[i]);
                }

                if (app_state.polygon_finished) {
                    path.closeSubpath();
                } else {
                    path.lineTo(preview_x, preview_y);
                }

                p.drawPath(path);

                for (const auto& point : app_state.polygon_points) {
                    draw_black_outline_circle(p, point.x(), point.y(), 5.0);
                }
            }
            break;
        }
        default: break;
    }
    p.restore();
}


QColor get_active_color() {
    return app_state.is_right_button ? app_state.bg_color : app_state.fg_color;
}

void draw_line(QPainter& p, double x1, double y1, double x2, double y2) {
    p.setPen(QPen(get_active_color(), app_state.line_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(QPointF(x1, y1), QPointF(x2, y2));
}

void draw_rectangle(QPainter& p, double x1, double y1, double x2, double y2, bool filled) {
    double x = std::min(x1, x2);
    double y = std::min(y1, y2);
    double w = std::abs(x2 - x1);
    double h = std::abs(y2 - y1);

    if (filled) {
        p.setPen(Qt::NoPen);
        p.setBrush(get_active_color());
    } else {
        p.setPen(QPen(get_active_color(), app_state.line_width, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        p.setBrush(Qt::NoBrush);
    }
    p.drawRect(QRectF(x, y, w, h));
}

void draw_ellipse(QPainter& p, double x1, double y1, double x2, double y2, bool filled) {
    double cx = (x1 + x2) / 2.0;
    double cy = (y1 + y2) / 2.0;
    double rx = std::abs(x2 - x1) / 2.0;
    double ry = std::abs(y2 - y1) / 2.0;

    if (rx < 0.1 || ry < 0.1) return;

    if (filled) {
        p.setPen(Qt::NoPen);
        p.setBrush(get_active_color());
    } else {
        p.setPen(QPen(get_active_color(), app_state.line_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
    }
    p.drawEllipse(QPointF(cx, cy), rx, ry);
}

void draw_rounded_rectangle(QPainter& p, double x1, double y1, double x2, double y2, bool filled) {
    double x = std::min(x1, x2);
    double y = std::min(y1, y2);
    double w = std::abs(x2 - x1);
    double h = std::abs(y2 - y1);
    double r = std::min(w, h) * 0.1;

    if (w < 1 || h < 1) return;

    if (filled) {
        p.setPen(Qt::NoPen);
        p.setBrush(get_active_color());
    } else {
        p.setPen(QPen(get_active_color(), app_state.line_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
    }
    p.drawRoundedRect(QRectF(x, y, w, h), r, r);
}

void draw_polygon(QPainter& p, const std::vector<QPointF>& points) {
    if (points.size() < 2) return;

    p.setPen(QPen(get_active_color(), app_state.line_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    QPolygonF poly;
    for (const auto& pt : points) poly << pt;
    p.drawPolygon(poly);
}

void draw_curve(QPainter& p, double start_x, double start_y, double control_x, double control_y, double end_x, double end_y) {
    p.setPen(QPen(get_active_color(), app_state.line_width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    QPainterPath path;
    path.moveTo(start_x, start_y);
    path.quadTo(control_x, control_y, end_x, end_y);
    p.drawPath(path);
}

void draw_pencil(QPainter& p, double x, double y) {
    p.setPen(QPen(get_active_color(), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (app_state.last_x != 0 && app_state.last_y != 0) {
        p.drawLine(QPointF(app_state.last_x, app_state.last_y), QPointF(x, y));
    }
}

void draw_paintbrush(QPainter& p, double x, double y) {
    p.setPen(QPen(get_active_color(), app_state.line_width * 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (app_state.last_x != 0 && app_state.last_y != 0) {
        p.drawLine(QPointF(app_state.last_x, app_state.last_y), QPointF(x, y));
    }
}

void draw_airbrush(QPainter& p, double x, double y) {
    p.setPen(Qt::NoPen);
    p.setBrush(get_active_color());
    double spray_radius = app_state.line_width * 5.0;

    for (int i = 0; i < 20; i++) {
        double angle = (std::rand() / (double)RAND_MAX) * 2 * M_PI;
        double radius = (std::rand() / (double)RAND_MAX) * spray_radius;
        int px = static_cast<int>(std::round(x + std::cos(angle) * radius));
        int py = static_cast<int>(std::round(y + std::sin(angle) * radius));
        p.drawRect(px, py, 1, 1);
    }
}

void draw_eraser(QPainter& p, double x, double y) {
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.setPen(QPen(QColor(255, 255, 255, 0), app_state.line_width * 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (app_state.last_x != 0 && app_state.last_y != 0) {
        p.drawLine(QPointF(app_state.last_x, app_state.last_y), QPointF(x, y));
    }
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
}


void push_undo_state() {
    UndoSnapshot snapshot;
    snapshot.surface = app_state.surface.copy();
    snapshot.width = app_state.canvas_width;
    snapshot.height = app_state.canvas_height;
    app_state.undo_stack.push_back(snapshot);
    if (app_state.undo_stack.size() > AppState::max_undo_steps) {
        app_state.undo_stack.erase(app_state.undo_stack.begin());
    }
    app_state.redo_stack.clear();
}

void undo_last_operation() {
    if (app_state.undo_stack.empty()) return;

    UndoSnapshot redo_snapshot;
    redo_snapshot.surface = app_state.surface.copy();
    redo_snapshot.width = app_state.canvas_width;
    redo_snapshot.height = app_state.canvas_height;
    app_state.redo_stack.push_back(redo_snapshot);
    if (app_state.redo_stack.size() > AppState::max_undo_steps) {
        app_state.redo_stack.erase(app_state.redo_stack.begin());
    }

    UndoSnapshot snapshot = app_state.undo_stack.back();
    app_state.undo_stack.pop_back();

    app_state.surface = snapshot.surface;
    app_state.canvas_width = snapshot.width;
    app_state.canvas_height = snapshot.height;

    if (app_state.drawing_area) {
        app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
        app_state.drawing_area->update();
    }
}

void redo_last_operation() {
    if (app_state.redo_stack.empty()) return;

    UndoSnapshot undo_snapshot;
    undo_snapshot.surface = app_state.surface.copy();
    undo_snapshot.width = app_state.canvas_width;
    undo_snapshot.height = app_state.canvas_height;
    app_state.undo_stack.push_back(undo_snapshot);
    if (app_state.undo_stack.size() > AppState::max_undo_steps) {
        app_state.undo_stack.erase(app_state.undo_stack.begin());
    }

    UndoSnapshot snapshot = app_state.redo_stack.back();
    app_state.redo_stack.pop_back();

    app_state.surface = snapshot.surface;
    app_state.canvas_width = snapshot.width;
    app_state.canvas_height = snapshot.height;

    if (app_state.drawing_area) {
        app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
        app_state.drawing_area->update();
    }
}

double to_canvas_coordinate(double coordinate) {
    return coordinate / app_state.zoom_factor;
}

// Just an example showing how tools can be hooked up.
// Actual implementations of tools would go here, fully ported to Qt.
void handle_mouse_press(double x, double y, Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
    if (button == Qt::LeftButton || button == Qt::RightButton) {
        if (app_state.floating_selection_active) {
            if (app_state.floating_drag_completed || !get_selection_pixel_bounds().contains(x, y)) {
                commit_floating_selection();
                return;
            }
            start_selection_drag();
            app_state.dragging_selection = true;
            app_state.selection_drag_offset_x = x - std::min(app_state.selection_x1, app_state.selection_x2);
            app_state.selection_drag_offset_y = y - std::min(app_state.selection_y1, app_state.selection_y2);
            app_state.is_drawing = true;
            return;
        }

        if (app_state.current_tool == TOOL_ZOOM && button == Qt::LeftButton) {
            double selected_zoom = zoom_options[app_state.active_zoom_index];
            if (selected_zoom == 1.0) {
                app_state.zoom_factor = 1.0;
            } else {
                app_state.zoom_factor = selected_zoom;
            }
            if (app_state.drawing_area) {
                app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
                app_state.drawing_area->update();
            }
            return;
        }

        if (app_state.current_tool == TOOL_TEXT) {
            bool in_box = (x >= app_state.text_x && x <= app_state.text_x + app_state.text_box_width &&
                           y >= app_state.text_y && y <= app_state.text_y + app_state.text_box_height);
            if (app_state.text_active && !in_box) {
                if (button == Qt::LeftButton) app_state.window->finalizeText();
                else app_state.window->cancelText();
                return;
            } else if (!app_state.text_active) {
                if (button == Qt::LeftButton) {
                    app_state.text_active = true;
                    app_state.text_x = x;
                    app_state.text_y = y;
                    app_state.text_content.clear();
                    app_state.text_box_width = 200;
                    app_state.text_box_height = 100;

                    QWidget* w = new QWidget(app_state.window, Qt::Window | Qt::FramelessWindowHint);
                    app_state.text_window = w;
                    QVBoxLayout* l = new QVBoxLayout(w);
                    l->setContentsMargins(0,0,0,0);
                    QTextEdit* text_entry = new QTextEdit(w);
                    app_state.text_entry = text_entry;
                    l->addWidget(text_entry);
                    QObject::connect(text_entry, &QTextEdit::textChanged, [=]() {
                        app_state.text_content = text_entry->toPlainText().toStdString();
                        if (app_state.drawing_area) app_state.drawing_area->update();
                    });

                    if (app_state.drawing_area) {
                        QPoint pt = app_state.drawing_area->mapToGlobal(QPoint(x * app_state.zoom_factor, y * app_state.zoom_factor));
                        w->setGeometry(pt.x(), pt.y(), app_state.text_box_width, app_state.text_box_height);
                    }
                    w->show();
                    text_entry->setFocus();
                }
                return;
            }
            return;
        }

        app_state.is_right_button = (button == Qt::RightButton);

        if (app_state.current_tool == TOOL_EYEDROPPER) {
            int px = static_cast<int>(x);
            int py = static_cast<int>(y);
            if (px >= 0 && px < app_state.canvas_width && py >= 0 && py < app_state.canvas_height) {
                QColor sampled = app_state.surface.pixelColor(px, py);
                if (app_state.is_right_button) app_state.bg_color = sampled;
                else app_state.fg_color = sampled;
                app_state.window->updateColorIndicators();
            }
            return;
        }

        if (app_state.current_tool == TOOL_FILL) {
            push_undo_state();
            int start_x = static_cast<int>(x);
            int start_y = static_cast<int>(y);
            if (start_x >= 0 && start_x < app_state.canvas_width && start_y >= 0 && start_y < app_state.canvas_height) {
                QColor target = app_state.surface.pixelColor(start_x, start_y);
                QColor replacement = get_active_color();
                if (target != replacement) {
                    std::queue<QPoint> q;
                    q.push(QPoint(start_x, start_y));
                    while (!q.empty()) {
                        QPoint p = q.front();
                        q.pop();
                        if (p.x() < 0 || p.x() >= app_state.canvas_width || p.y() < 0 || p.y() >= app_state.canvas_height) continue;
                        if (app_state.surface.pixelColor(p) != target) continue;
                        app_state.surface.setPixelColor(p, replacement);
                        q.push(QPoint(p.x() - 1, p.y()));
                        q.push(QPoint(p.x() + 1, p.y()));
                        q.push(QPoint(p.x(), p.y() - 1));
                        q.push(QPoint(p.x(), p.y() + 1));
                    }
                }
            }
            return;
        }

        if (app_state.current_tool == TOOL_LASSO_SELECT) {
            if (button == Qt::LeftButton) {
                if (app_state.lasso_polygon_mode) {
                    app_state.lasso_points.push_back(QPointF(x, y));
                    app_state.current_x = x;
                    app_state.current_y = y;
                    app_state.is_drawing = true;
                    return;
                }

                app_state.is_drawing = true;
                app_state.lasso_polygon_mode = (modifiers & Qt::ControlModifier);
                app_state.lasso_points.clear();
                app_state.lasso_points.push_back(QPointF(x, y));
                app_state.current_x = x;
                app_state.current_y = y;
                return;
            }
        }

        if (app_state.current_tool == TOOL_POLYGON) {
            if (button == Qt::LeftButton) {
                if (app_state.polygon_finished) {
                    push_undo_state();
                    app_state.is_right_button = false;
                    QPainter p(&app_state.surface);
                    configure_crisp_rendering(p);
                    draw_polygon(p, app_state.polygon_points);
                    app_state.polygon_points.clear();
                    app_state.polygon_finished = false;
                    app_state.is_drawing = false;
                    return;
                }
                app_state.polygon_points.push_back(QPointF(x, y));
                app_state.is_drawing = true;
                app_state.current_x = x;
                app_state.current_y = y;
                return;
            }

            if (button == Qt::RightButton) {
                if (!app_state.polygon_finished && app_state.polygon_points.size() >= 2) {
                    app_state.polygon_finished = true;
                    app_state.is_drawing = true;
                } else if (app_state.polygon_finished) {
                    push_undo_state();
                    QPainter p(&app_state.surface);
                    configure_crisp_rendering(p);
                    draw_polygon(p, app_state.polygon_points);
                    app_state.polygon_points.clear();
                    app_state.polygon_finished = false;
                    app_state.is_drawing = false;
                }
                return;
            }
        }

        if (app_state.current_tool == TOOL_CURVE) {
            if (!app_state.curve_active) {
                app_state.curve_active = true;
                app_state.curve_has_end = false;
                app_state.curve_has_control = false;
                app_state.curve_primary_right_button = (button == Qt::RightButton);
                app_state.curve_start_x = x;
                app_state.curve_start_y = y;
                app_state.is_drawing = true;
                app_state.current_x = x;
                app_state.current_y = y;
                return;
            }

            bool used_primary_button = ((button == Qt::RightButton) == app_state.curve_primary_right_button);
            if (!used_primary_button) {
                if (app_state.curve_has_end) {
                    QPainter p(&app_state.surface);
                    configure_crisp_rendering(p);
                    app_state.is_right_button = app_state.curve_primary_right_button;
                    if (app_state.curve_has_control) {
                        draw_curve(p, app_state.curve_start_x, app_state.curve_start_y, app_state.curve_control_x, app_state.curve_control_y, app_state.curve_end_x, app_state.curve_end_y);
                    } else {
                        draw_line(p, app_state.curve_start_x, app_state.curve_start_y, app_state.curve_end_x, app_state.curve_end_y);
                    }
                }
                app_state.curve_active = false;
                app_state.curve_has_end = false;
                app_state.curve_has_control = false;
                app_state.is_drawing = false;
                app_state.is_right_button = false;
                return;
            }

            if (app_state.curve_has_end && (modifiers & Qt::ShiftModifier)) {
                app_state.curve_start_x = x;
                app_state.curve_start_y = y;
                app_state.current_x = x;
                app_state.current_y = y;
                return;
            }

            if (app_state.curve_has_end && (modifiers & Qt::ControlModifier)) {
                app_state.curve_end_x = x;
                app_state.curve_end_y = y;
                app_state.current_x = x;
                app_state.current_y = y;
                return;
            }

            if (!app_state.curve_has_end) {
                app_state.curve_end_x = x;
                app_state.curve_end_y = y;
                app_state.curve_has_end = true;
            } else {
                app_state.curve_control_x = x;
                app_state.curve_control_y = y;
                app_state.curve_has_control = true;
            }
            app_state.is_drawing = true;
            app_state.current_x = x;
            app_state.current_y = y;
            return;
        }

        app_state.is_drawing = true;
        if (app_state.current_tool == TOOL_PENCIL || app_state.current_tool == TOOL_PAINTBRUSH ||
            app_state.current_tool == TOOL_AIRBRUSH || app_state.current_tool == TOOL_ERASER ||
            app_state.current_tool == TOOL_LINE || app_state.current_tool == TOOL_CURVE ||
            app_state.current_tool == TOOL_RECTANGLE || app_state.current_tool == TOOL_ELLIPSE ||
            app_state.current_tool == TOOL_ROUNDED_RECT) {
            push_undo_state();
        }
        app_state.last_x = x;
        app_state.last_y = y;
        app_state.start_x = x;
        app_state.start_y = y;
        app_state.current_x = x;
        app_state.current_y = y;

        if (app_state.current_tool == TOOL_AIRBRUSH) {
            QPainter p(&app_state.surface);
            configure_crisp_rendering(p);
            draw_airbrush(p, x, y);
        }
    }
}

void handle_mouse_move(double x, double y, Qt::KeyboardModifiers modifiers) {
    if (!app_state.is_drawing) return;

    app_state.current_x = x;
    app_state.current_y = y;

    if (app_state.dragging_selection && app_state.has_selection) {
        double old_x = std::min(app_state.selection_x1, app_state.selection_x2);
        double old_y = std::min(app_state.selection_y1, app_state.selection_y2);
        double width = std::abs(app_state.selection_x2 - app_state.selection_x1);
        double height = std::abs(app_state.selection_y2 - app_state.selection_y1);
        double new_x = std::round(x - app_state.selection_drag_offset_x);
        double new_y = std::round(y - app_state.selection_drag_offset_y);

        double dx = new_x - old_x;
        double dy = new_y - old_y;

        app_state.selection_x1 = new_x;
        app_state.selection_y1 = new_y;
        app_state.selection_x2 = new_x + width;
        app_state.selection_y2 = new_y + height;

        if (!app_state.selection_is_rect) {
            for (auto& point : app_state.selection_path) {
                point.setX(point.x() + dx);
                point.setY(point.y() + dy);
            }
        }
    } else if (app_state.current_tool == TOOL_LASSO_SELECT && !app_state.lasso_polygon_mode) {
        app_state.lasso_points.push_back(QPointF(x, y));
    } else if (!tool_needs_preview(app_state.current_tool)) {
        QPainter p(&app_state.surface);
        configure_crisp_rendering(p);

        switch (app_state.current_tool) {
            case TOOL_PENCIL:
                draw_pencil(p, x, y);
                break;
            case TOOL_PAINTBRUSH:
                draw_paintbrush(p, x, y);
                break;
            case TOOL_AIRBRUSH:
                draw_airbrush(p, x, y);
                break;
            case TOOL_ERASER:
                draw_eraser(p, x, y);
                break;
            default: break;
        }

        app_state.last_x = x;
        app_state.last_y = y;
    }
}

void handle_mouse_release(double x, double y, Qt::MouseButton button, Qt::KeyboardModifiers modifiers) {
    if (!app_state.is_drawing) return;

    if (app_state.current_tool == TOOL_ELLIPSE && app_state.ellipse_center_mode) return;
    if (app_state.current_tool == TOOL_CURVE) return;
    if (app_state.current_tool == TOOL_POLYGON) return;
    if (app_state.current_tool == TOOL_LASSO_SELECT && app_state.lasso_polygon_mode) return;

    if (app_state.dragging_selection) {
        app_state.dragging_selection = false;
        app_state.is_drawing = false;
        app_state.floating_drag_completed = true;
        commit_floating_selection(false);
        return;
    }

    double end_x = x;
    double end_y = y;

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

    QPainter p(&app_state.surface);
    configure_crisp_rendering(p);

    switch (app_state.current_tool) {
        case TOOL_LINE:
            draw_line(p, app_state.start_x, app_state.start_y, end_x, end_y);
            break;
        case TOOL_RECTANGLE:
            draw_rectangle(p, app_state.start_x, app_state.start_y, end_x, end_y, false);
            break;
        case TOOL_ELLIPSE:
            draw_ellipse(p, app_state.start_x, app_state.start_y, end_x, end_y, false);
            break;
        case TOOL_ROUNDED_RECT:
            draw_rounded_rectangle(p, app_state.start_x, app_state.start_y, end_x, end_y, false);
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
            if (app_state.lasso_points.size() >= 3) {
                app_state.has_selection = true;
                app_state.selection_is_rect = false;
                app_state.floating_selection_active = false;
                app_state.selection_path = app_state.lasso_points;
                app_state.lasso_points.clear();
            } else {
                app_state.lasso_points.clear();
            }
            break;
        default: break;
    }

    app_state.is_drawing = false;
    app_state.is_right_button = false;
    app_state.ellipse_center_mode = false;
    app_state.last_x = 0;
    app_state.last_y = 0;
}


void PaintCanvas::mousePressEvent(QMouseEvent *event) {
    if (app_state.surface.isNull()) return;

    double x = to_canvas_coordinate(event->x());
    double y = to_canvas_coordinate(event->y());

    handle_mouse_press(x, y, event->button(), event->modifiers());
    update();
}

void PaintCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (app_state.surface.isNull()) return;

    double x = to_canvas_coordinate(event->x());
    double y = to_canvas_coordinate(event->y());

    app_state.hover_in_canvas = true;
    app_state.hover_x = x;
    app_state.hover_y = y;

    if (app_state.is_drawing) {
        handle_mouse_move(x, y, event->modifiers());
    }

    update();
}

void PaintCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (app_state.surface.isNull()) return;

    double x = to_canvas_coordinate(event->x());
    double y = to_canvas_coordinate(event->y());

    handle_mouse_release(x, y, event->button(), event->modifiers());
    update();
}

void PaintCanvas::leaveEvent(QEvent *event) {
    app_state.hover_in_canvas = false;
    update();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Shift) {
        app_state.shift_pressed = true;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Shift) {
        app_state.shift_pressed = false;
    }
}

void MainWindow::on_file_new() {
    // simplified for brevity in this prompt context
    QDialog dialog(this);
    dialog.setWindowTitle(i18n("New Image"));
    QVBoxLayout layout(&dialog);
    QComboBox combo;
    combo.addItem("256x256");
    combo.addItem("512x512");
    combo.addItem("1024x1024");
    combo.addItem("640x480");
    combo.addItem("800x600");
    combo.addItem("Custom");
    combo.setCurrentIndex(4);
    layout.addWidget(new QLabel(i18n("Resolution:")));
    layout.addWidget(&combo);

    QWidget customWidget;
    QHBoxLayout customLayout(&customWidget);
    QSpinBox xSpin, ySpin;
    xSpin.setRange(1, 10000); ySpin.setRange(1, 10000);
    xSpin.setValue(800); ySpin.setValue(600);
    customLayout.addWidget(new QLabel("X:"));
    customLayout.addWidget(&xSpin);
    customLayout.addWidget(new QLabel("Y:"));
    customLayout.addWidget(&ySpin);
    layout.addWidget(&customWidget);
    customWidget.setEnabled(false);

    connect(&combo, &QComboBox::currentTextChanged, [&](const QString &text){
        customWidget.setEnabled(text == "Custom");
        if (text != "Custom") {
            QStringList parts = text.split('x');
            if (parts.size() == 2) {
                xSpin.setValue(parts[0].toInt());
                ySpin.setValue(parts[1].toInt());
            }
        }
    });

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    layout.addWidget(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int w = xSpin.value();
        int h = ySpin.value();
        if (w > 0 && h > 0) {
            push_undo_state();
            app_state.canvas_width = w;
            app_state.canvas_height = h;
            init_surface(app_state.drawing_area);
            app_state.current_filename.clear();
            app_state.drawing_area->setMinimumSize(w * app_state.zoom_factor, h * app_state.zoom_factor);
            app_state.drawing_area->update();
        }
    }
}

void MainWindow::on_file_open() {
    QString fileName = QFileDialog::getOpenFileName(this, i18n("Open Image"), "", i18n("Images (*.png *.xpm *.jpg *.jpeg)"));
    if (!fileName.isEmpty()) {
        QImage image(fileName);
        if (!image.isNull()) {
            push_undo_state();
            app_state.current_filename = fileName.toStdString();
            app_state.surface = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            app_state.canvas_width = image.width();
            app_state.canvas_height = image.height();
            app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
            app_state.drawing_area->update();
        }
    }
}

void MainWindow::on_file_save() {
    if (!app_state.current_filename.empty()) {
        app_state.surface.save(QString::fromStdString(app_state.current_filename));
    } else {
        on_file_save_as();
    }
}

void MainWindow::on_file_save_as() {
    QString fileName = QFileDialog::getSaveFileName(this, i18n("Save Image"), "", i18n("Images (*.png *.xpm *.jpg *.jpeg)"));
    if (!fileName.isEmpty()) {
        app_state.current_filename = fileName.toStdString();
        app_state.surface.save(fileName);
    }
}

void MainWindow::on_image_scale() {
    bool ok;
    int scale = QInputDialog::getInt(this, i18n("Scale Image"), i18n("Scale (%):"), 100, 1, 1000, 1, &ok);
    if (ok) {
        push_undo_state();
        double factor = scale / 100.0;
        int new_w = std::max(1, (int)std::round(app_state.canvas_width * factor));
        int new_h = std::max(1, (int)std::round(app_state.canvas_height * factor));

        app_state.surface = app_state.surface.scaled(new_w, new_h, Qt::IgnoreAspectRatio, Qt::FastTransformation);
        app_state.canvas_width = new_w;
        app_state.canvas_height = new_h;
        app_state.drawing_area->setMinimumSize(new_w * app_state.zoom_factor, new_h * app_state.zoom_factor);
        app_state.drawing_area->update();
    }
}

void MainWindow::on_image_resize_canvas() {
    // simplified
    push_undo_state();
    QImage new_surface(app_state.canvas_width + 100, app_state.canvas_height + 100, QImage::Format_ARGB32_Premultiplied);
    new_surface.fill(app_state.bg_color);
    QPainter p(&new_surface);
    p.drawImage(0, 0, app_state.surface);
    app_state.surface = new_surface;
    app_state.canvas_width += 100;
    app_state.canvas_height += 100;
    app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
    app_state.drawing_area->update();
}

void MainWindow::on_image_rotate_clockwise() {
    push_undo_state();
    QTransform transform;
    transform.rotate(90);
    app_state.surface = app_state.surface.transformed(transform);
    std::swap(app_state.canvas_width, app_state.canvas_height);
    app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
    app_state.drawing_area->update();
}

void MainWindow::on_image_rotate_counter_clockwise() {
    push_undo_state();
    QTransform transform;
    transform.rotate(-90);
    app_state.surface = app_state.surface.transformed(transform);
    std::swap(app_state.canvas_width, app_state.canvas_height);
    app_state.drawing_area->setMinimumSize(app_state.canvas_width * app_state.zoom_factor, app_state.canvas_height * app_state.zoom_factor);
    app_state.drawing_area->update();
}

void MainWindow::on_image_flip_vertical() {
    push_undo_state();
    app_state.surface = app_state.surface.mirrored(false, true);
    app_state.drawing_area->update();
}

void MainWindow::on_image_flip_horizontal() {
    push_undo_state();
    app_state.surface = app_state.surface.mirrored(true, false);
    app_state.drawing_area->update();
}

void MainWindow::on_help_about() {
    KAboutData aboutData(
        QStringLiteral("matepaint"),
        i18n("Mate-Paint"),
        QStringLiteral("1.0"),
        i18n("A simple drawing tool"),
        KAboutLicense::GPL,
        i18n("(c) 2006 James Carthew")
    );
    KAboutData::setApplicationData(aboutData);
    // Usually KHelpMenu shows the about dialog in KDE apps, but we can do it manually or via QMessageBox for simplicity here if needed.
    // For a true KDE app, we use KAboutApplicationDialog:
    // #include <KAboutApplicationDialog>
    // KAboutApplicationDialog(aboutData, this).exec();
    QMessageBox::about(this, i18n("About Mate-Paint"), i18n("Mate-Paint\nVersion 1.0\nCopyright © 2006 James Carthew\nPorted to Qt/KDE"));
}


void MainWindow::on_file_quit() { qApp->quit(); }
void MainWindow::on_edit_undo() { undo_last_operation(); }
void MainWindow::on_edit_redo() { redo_last_operation(); }

void MainWindow::on_edit_copy() {
    if (!app_state.has_selection || app_state.surface.isNull()) return;

    QRect bounds = get_selection_pixel_bounds();
    if (bounds.width() <= 0 || bounds.height() <= 0) return;

    QImage clip(bounds.size(), QImage::Format_ARGB32_Premultiplied);
    clip.fill(Qt::transparent);

    QPainter p(&clip);
    configure_crisp_rendering(p);

    if (app_state.floating_selection_active && !app_state.floating_surface.isNull()) {
        p.drawImage(0, 0, app_state.floating_surface);
    } else if (app_state.selection_is_rect) {
        p.drawImage(0, 0, app_state.surface, bounds.x(), bounds.y(), bounds.width(), bounds.height());
    } else if (app_state.selection_path.size() > 2) {
        p.translate(-bounds.x(), -bounds.y());
        QPainterPath path;
        append_selection_path(path);
        p.setClipPath(path);
        p.drawImage(0, 0, app_state.surface);
    }

    QApplication::clipboard()->setImage(clip);
}

void MainWindow::on_edit_cut() {
    if (!app_state.has_selection || app_state.surface.isNull()) return;

    on_edit_copy();

    if (app_state.floating_selection_active) {
        clear_selection();
        return;
    }

    push_undo_state();

    QPainter p(&app_state.surface);
    configure_crisp_rendering(p);
    p.setPen(Qt::NoPen);
    p.setBrush(app_state.bg_color);

    if (app_state.selection_is_rect) {
        QRect bounds = get_selection_pixel_bounds();
        p.drawRect(bounds);
    } else if (app_state.selection_path.size() > 2) {
        QPainterPath path;
        append_selection_path(path);
        p.drawPath(path);
    }

    if (app_state.drawing_area) {
        app_state.drawing_area->update();
    }

    clear_selection();
    app_state.drag_undo_snapshot_taken = false;
}

void MainWindow::on_edit_paste() {
    if (app_state.surface.isNull()) return;

    QImage system_surface = QApplication::clipboard()->image();
    if (system_surface.isNull()) return;

    bool exceeds_canvas = system_surface.width() > app_state.canvas_width || system_surface.height() > app_state.canvas_height;
    if (exceeds_canvas) {
        int res = QMessageBox::question(app_state.window, "Pasted Image Is Larger Than Canvas",
            "The pasted image is larger than the current canvas.\nWould you like to expand it?",
            QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::Yes) {
            push_undo_state();
            int new_w = std::max(app_state.canvas_width, system_surface.width());
            int new_h = std::max(app_state.canvas_height, system_surface.height());
            QImage new_surface(new_w, new_h, QImage::Format_ARGB32_Premultiplied);
            new_surface.fill(app_state.bg_color);
            QPainter p(&new_surface);
            p.drawImage(0, 0, app_state.surface);
            app_state.surface = new_surface;
            app_state.canvas_width = new_w;
            app_state.canvas_height = new_h;
            if (app_state.drawing_area) {
                app_state.drawing_area->setMinimumSize(new_w * app_state.zoom_factor, new_h * app_state.zoom_factor);
            }
        }
    }
    clear_selection();

    double paste_x = 20;
    double paste_y = 20;

    app_state.floating_surface = system_surface;
    app_state.floating_selection_active = true;
    app_state.floating_drag_completed = false;
    app_state.dragging_selection = false;

    app_state.has_selection = true;
    app_state.selection_is_rect = true;
    app_state.selection_path.clear();
    app_state.selection_x1 = paste_x;
    app_state.selection_y1 = paste_y;
    app_state.selection_x2 = paste_x + system_surface.width();
    app_state.selection_y2 = paste_y + system_surface.height();

    if (app_state.drawing_area) {
        app_state.drawing_area->update();
    }
}

void MainWindow::on_help_manual() {
    QDesktopServices::openUrl(QUrl("help:mate-paint/contents"));
}

void MainWindow::on_tool_clicked(int tool) {
    app_state.current_tool = static_cast<Tool>(tool);
}
void MainWindow::on_line_thickness_toggled() {}
void MainWindow::on_zoom_toggled() {}
void MainWindow::on_ant_timer() {
    app_state.ant_offset += 1.0;
    if (app_state.ant_offset >= 8.0) {
        app_state.ant_offset = 0.0;
    }
    if (app_state.drawing_area) {
        app_state.drawing_area->update();
    }
}

void clear_selection() {
    app_state.floating_surface = QImage();
    app_state.floating_selection_active = false;
    app_state.dragging_selection = false;
    app_state.floating_drag_completed = false;
    app_state.has_selection = false;
    app_state.selection_path.clear();
    app_state.drag_undo_snapshot_taken = false;
    if (app_state.drawing_area) {
        app_state.drawing_area->update();
    }
}

void commit_floating_selection(bool record_undo) {
    if (!app_state.floating_selection_active || app_state.floating_surface.isNull()) {
        return;
    }

    double x = std::round(std::min(app_state.selection_x1, app_state.selection_x2));
    double y = std::round(std::min(app_state.selection_y1, app_state.selection_y2));

    if (record_undo) {
        push_undo_state();
    }

    QPainter p(&app_state.surface);
    configure_crisp_rendering(p);
    p.drawImage(x, y, app_state.floating_surface);

    clear_selection();
    app_state.drag_undo_snapshot_taken = false;
}

QRect get_selection_pixel_bounds() {
    double x1 = std::min(app_state.selection_x1, app_state.selection_x2);
    double y1 = std::min(app_state.selection_y1, app_state.selection_y2);
    double x2 = std::max(app_state.selection_x1, app_state.selection_x2);
    double y2 = std::max(app_state.selection_y1, app_state.selection_y2);

    int pixel_x = static_cast<int>(std::floor(x1));
    int pixel_y = static_cast<int>(std::floor(y1));
    int pixel_x2 = static_cast<int>(std::ceil(x2));
    int pixel_y2 = static_cast<int>(std::ceil(y2));

    return QRect(pixel_x, pixel_y, pixel_x2 - pixel_x, pixel_y2 - pixel_y);
}

void start_selection_drag() {
    if (!app_state.has_selection || app_state.surface.isNull()) return;
    if (app_state.floating_selection_active) return;

    if (!app_state.drag_undo_snapshot_taken) {
        push_undo_state();
        app_state.drag_undo_snapshot_taken = true;
    }

    QRect bounds = get_selection_pixel_bounds();
    if (bounds.width() <= 0 || bounds.height() <= 0) return;

    app_state.floating_surface = QImage(bounds.size(), QImage::Format_ARGB32_Premultiplied);
    app_state.floating_surface.fill(Qt::transparent);

    QPainter float_p(&app_state.floating_surface);
    configure_crisp_rendering(float_p);

    if (app_state.selection_is_rect) {
        float_p.drawImage(0, 0, app_state.surface, bounds.x(), bounds.y(), bounds.width(), bounds.height());
    } else if (app_state.selection_path.size() > 2) {
        float_p.translate(-bounds.x(), -bounds.y());
        QPainterPath path;
        append_selection_path(path);
        float_p.setClipPath(path);
        float_p.drawImage(0, 0, app_state.surface);
    }

    QPainter p(&app_state.surface);
    configure_crisp_rendering(p);
    p.setPen(Qt::NoPen);
    p.setBrush(app_state.bg_color);

    if (app_state.selection_is_rect) {
        p.drawRect(bounds);
    } else if (app_state.selection_path.size() > 2) {
        QPainterPath path;
        append_selection_path(path);
        p.drawPath(path);
    }

    app_state.selection_x1 = bounds.x();
    app_state.selection_y1 = bounds.y();
    app_state.selection_x2 = bounds.x() + bounds.width();
    app_state.selection_y2 = bounds.y() + bounds.height();

    app_state.floating_selection_active = true;
    app_state.floating_drag_completed = false;
}

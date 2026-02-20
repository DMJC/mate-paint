# Mate-Paint Manual

This manual explains the Mate-Paint toolbox, colour palette, and day-to-day usage.

## Getting started

1. Start **mate-paint**.
2. Pick a tool from the left toolbar.
3. Pick colours from the palette at the bottom:
   - **Left-click** sets the foreground colour.
   - **Right-click** sets the background colour.
4. Draw on the canvas.
5. Save your work with **File → Save** or **File → Save As...**.

## Toolbox reference

### Selection tools

- **Lasso Select**
  - Draws a freehand selection area.
  - Tip: hold **Ctrl** while starting to create point-by-point lasso selection; right-click to finish.
- **Rectangle Select**
  - Selects a rectangular region.
  - Selected content can be copied/cut/pasted.

### Paint and editing tools

- **Fill Tool**
  - Fills a contiguous area with the active colour.
- **Eyedropper**
  - Picks a colour from the canvas.
  - Left-click picks to foreground, right-click picks to background.
- **Eraser**
  - Erases to transparency.
- **Zoom Tool**
  - Uses zoom level buttons (1x/2x/4x/6x/8x) shown under the toolbox.
  - Click the canvas to zoom using the currently selected zoom factor.

### Drawing tools

- **Pencil**
  - Freehand thin drawing.
- **Paintbrush**
  - Freehand brush drawing (uses line thickness).
- **Airbrush**
  - Spray effect while drawing.
- **Text Tool**
  - Click to place a text box.
  - Left-click outside the box to finalize text.
  - Right-click outside the box to cancel.
- **Line Tool**
  - Draws straight lines.
  - Hold **Shift** to constrain horizontal/vertical.
- **Curve Tool**
  - Builds a curved line in steps:
    1. Click to set start.
    2. Click to set end.
    3. Click to set curve control point.
    4. Click with the opposite mouse button to commit.
- **Rectangle**
  - Draws rectangles.
- **Polygon**
  - Click to add polygon points.
  - Right-click to close preview, then click to commit.
- **Ellipse/Circle**
  - Draws ellipses.
  - Hold **Shift** for circles.
  - Hold **Ctrl** while starting to draw from centre.
- **Rounded Rectangle**
  - Draws rectangles with rounded corners.

## Colour palette guide

- The first palette swatch is **T** = transparent colour.
- The bottom bar includes fixed colours plus additional and custom colours.
- **Custom colour slots** are marked with `c`.
  - Double-click a custom slot to choose a custom colour.
  - Custom slot colours are saved for future sessions.

### Foreground and background colours

- **Foreground colour** is used by left-click drawing.
- **Background colour** is used by right-click drawing.
- The two colour indicators above the palette show active foreground/background colours.

## Line thickness and zoom controls

- **Line thickness buttons** appear for tools that support stroke width.
- Thickness options: **1, 2, 4, 6, 8**.
- Each supported tool remembers its own last-used thickness.
- **Zoom factor buttons** (1x, 2x, 4x, 6x, 8x) appear when the Zoom tool is active.

## Keyboard shortcuts

- **Ctrl+C**: Copy selection
- **Ctrl+X**: Cut selection
- **Ctrl+V**: Paste selection
- **Ctrl+Z**: Undo
- **Shift**: Constrain certain tools (line/circle behaviour)

## Menus

- **File**: New, Open, Save, Save As, Quit
- **Edit**: Undo, Cut, Copy, Paste
- **Image**: Scale Image, Resize Image, Rotate, Flip
- **Help**: Manual, About

## Typical workflow

1. Choose foreground/background colours from the palette.
2. Choose a drawing tool (for example Pencil, Brush, or Shape tools).
3. Adjust line thickness if needed.
4. Draw and refine with Eraser, Fill, or Eyedropper.
5. Use selection tools for moving/copying regions.
6. Save frequently.

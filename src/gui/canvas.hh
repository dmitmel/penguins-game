#pragma once

#include "game.h"
#include "utils.h"
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/types.h>
#include <wx/window.h>

class GamePanel;

/// Responsible for drawing the board and painting the UI overlays.
class CanvasPanel : public wxWindow {
public:
  static const wxCoord TILE_SIZE = 40;

  CanvasPanel(wxWindow* parent, wxWindowID id, GamePanel* panel);

  virtual bool AcceptsFocus() const override {
    return true;
  }

  wxSize get_canvas_size() const;
  Coords tile_coords_at_point(wxPoint point) const;
  wxRect get_tile_rect(Coords coords) const;
  wxPoint get_tile_centre(Coords coords) const;

  Coords get_selected_penguin_coords() const;

  bool mouse_within_window = false;
  bool mouse_is_down = false;
  bool mouse_is_down_real = false;
  wxPoint mouse_pos = wxDefaultPosition;
  wxPoint prev_mouse_pos = wxDefaultPosition;
  wxPoint mouse_drag_pos = wxDefaultPosition;

  void paint_selected_tile_outline(wxDC& dc, Coords coords, bool blocked = false);
  void paint_move_arrow(wxDC& dc, Coords start, Coords end);
  void paint_move_arrow(wxDC& dc, Coords start, Coords end, Coords fail, bool valid);

  enum ArrowHeadType {
    ARROW_HEAD_NORMAL = 1,
    ARROW_HEAD_CROSS = 2,
  };

  void paint_arrow_head(
    wxDC& dc,
    wxPoint start,
    wxPoint end,
    wxSize head_size,
    ArrowHeadType head_type = ARROW_HEAD_NORMAL
  );

protected:
  void on_paint(wxPaintEvent& event);
  void draw_bitmap(wxDC& dc, const wxBitmap& bitmap, const wxPoint& pos);
  void paint_tiles(wxDC& dc, const wxRect& update_region);
  void paint_board(wxDC& dc, const wxRect& update_region, wxDC& tiles_dc);

  void on_any_mouse_event(wxMouseEvent& event);

  wxBitmap board_bitmap;
  wxMemoryDC board_dc;
  wxBitmap tiles_bitmap;
  wxMemoryDC tiles_dc;

#ifdef __WXMSW__
  wxMemoryDC draw_bitmap_dc;
#endif

  GamePanel* panel;
  Game* game;

  wxDECLARE_EVENT_TABLE();
};

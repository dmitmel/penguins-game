#pragma once

#include "game.h"
#include "utils.h"
#include <memory>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/types.h>
#include <wx/window.h>

class GameFrame;

typedef enum TileAttribute {
  TILE_DIRTY = 1 << 1,
  TILE_BLOCKED = 1 << 2,
  TILE_BLOCKED_BEFORE = 1 << 3,
  TILE_BLOCKED_FOR_CURSOR = 1 << 4,
  TILE_BLOCKED_DIRTY = 1 << 5,
} TileAttribute;

class CanvasPanel : public wxPanel {
public:
  static const wxCoord TILE_SIZE = 40;

  CanvasPanel(wxWindow* parent, wxWindowID id, GameFrame* game_frame);

  wxByte* tile_attrs_ptr(Coords coords) const;
  void set_tile_attr(Coords coords, wxByte attr, bool value);
  void set_tile_neighbors_attr(Coords coords, wxByte attr, bool value);
  void set_all_tiles_attr(wxByte attr, bool value);

  wxSize get_canvas_size() const;
  Coords tile_coords_at_point(wxPoint point) const;
  wxRect get_tile_rect(Coords coords) const;
  wxPoint get_tile_centre(Coords coords) const;

  Coords get_selected_penguin_coords() const;

  bool mouse_within_window = false;
  bool mouse_is_down = false;
  wxPoint mouse_pos = wxDefaultPosition;
  wxPoint prev_mouse_pos = wxDefaultPosition;
  wxPoint mouse_drag_pos = wxDefaultPosition;

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

  GameFrame* game_frame;
  Game* game;
  std::unique_ptr<wxByte[]> tile_attributes{ nullptr };

  wxDECLARE_EVENT_TABLE();
};

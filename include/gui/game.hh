#pragma once

#include "gui/game_state.hh"
#include "gui/player_info_box.hh"
#include "utils.h"
#include <memory>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/types.h>
#include <wx/window.h>

class CanvasPanel;

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id, GuiGameState& state);

  void update_layout();

  void start_new_game();
  void update_game_state();
  void place_penguin(Coords target);
  void move_penguin(Coords penguin, Coords target);
  void end_game();
  void close_game();
  void update_player_info_boxes();

protected:
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_close_game(wxCommandEvent& event);

  wxPanel* root_panel;
  wxScrolledWindow* scrolled_panel;
  CanvasPanel* canvas_panel;
  wxBoxSizer* players_box;
  std::unique_ptr<PlayerInfoBox*[]> player_info_boxes;
  GuiGameState& state;
};

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

  CanvasPanel(wxWindow* parent, wxWindowID id, GameFrame* game_frame, GuiGameState& state);

  std::unique_ptr<wxByte[]> tile_attributes{ nullptr };
  wxByte* tile_attrs_ptr(Coords coords) const;
  void set_tile_attr(Coords coords, wxByte attr, bool value);
  void set_tile_neighbors_attr(Coords coords, wxByte attr, bool value);
  void set_all_tiles_attr(wxByte attr, bool value);
  void update_blocked_tiles();

  wxSize get_canvas_size() const;
  Coords tile_coords_at_point(wxPoint point) const;
  wxRect get_tile_rect(Coords coords) const;
  wxPoint get_tile_centre(Coords coords) const;

  Coords get_selected_penguin_coords(int player_index) const;

protected:
  virtual wxSize DoGetBestClientSize() const override;
  void on_paint(wxPaintEvent& event);
  void draw_bitmap(wxDC& dc, const wxBitmap& bitmap, const wxPoint& pos);
  void paint_tiles(wxDC& dc, const wxRect& update_region);
  void paint_board(wxDC& dc, const wxRect& update_region, wxDC& tiles_dc);
  void paint_overlay(wxDC& dc);

  void on_any_mouse_event(wxMouseEvent& event);
  void on_mouse_down(wxMouseEvent& event);
  void on_mouse_move(wxMouseEvent& event);
  void on_mouse_up(wxMouseEvent& event);
  void on_mouse_enter_leave(wxMouseEvent& event);

  wxBitmap board_bitmap;
  wxMemoryDC board_dc;
  wxBitmap tiles_bitmap;
  wxMemoryDC tiles_dc;

#ifdef __WXMSW__
  wxMemoryDC draw_bitmap_dc;
#endif

  bool mouse_within_window = false;
  bool mouse_is_down = false;
  wxPoint mouse_pos = wxDefaultPosition;
  wxPoint prev_mouse_pos = wxDefaultPosition;
  wxPoint mouse_drag_pos = wxDefaultPosition;

  GameFrame* game_frame;
  GuiGameState& state;

  wxDECLARE_EVENT_TABLE();
};

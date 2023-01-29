#pragma once

#include "gui/game_state.hh"
#include "gui/player_info_box.hh"
#include "gui/tileset.hh"
#include "utils.h"
#include <cstddef>
#include <memory>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/graphics.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/types.h>
#include <wx/vector.h>
#include <wx/window.h>

class CanvasPanel;

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id, GuiGameState& state, const TilesetHelper& tileset);

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
  void on_mouse_enter_leave(wxMouseEvent& event);

  CanvasPanel* canvas_panel;
  wxBoxSizer* players_box;
  std::unique_ptr<PlayerInfoBox*[]> player_info_boxes;
  GuiGameState& state;
  const TilesetHelper& tileset;

  wxDECLARE_EVENT_TABLE();
};

typedef enum CellAttribute {
  CELL_DIRTY = 1 << 1,
  CELL_BLOCKED = 1 << 2,
  CELL_BLOCKED_BEFORE = 1 << 3,
  CELL_BLOCKED_FOR_CURSOR = 1 << 4,
  CELL_BLOCKED_DIRTY = 1 << 5,
} CellAttribute;

class CanvasPanel : public wxPanel {
public:
  static const wxCoord CELL_SIZE = 40;
  static const int CELL_FONT_SIZE = 16;
  static const int FISH_CIRCLE_RADIUS = 4;
  static const int BLOCKED_CELL_LIGHTNESS = -40;

  CanvasPanel(GameFrame* parent, wxWindowID id, GuiGameState& state, const TilesetHelper& tileset);

  std::unique_ptr<wxByte[]> cell_attributes{ nullptr };
  wxByte* cell_attrs_ptr(Coords cell) const;
  void set_cell_attr(Coords cell, wxByte attr, bool value);
  void set_cell_neighbors_attr(Coords cell, wxByte attr, bool value);
  void set_all_cells_attr(wxByte attr, bool value);
  void update_blocked_cells();

  wxSize get_canvas_size() const;
  Coords get_cell_by_coords(wxPoint point) const;
  wxRect get_cell_rect(Coords cell) const;
  wxPoint get_cell_centre(Coords cell) const;

  Coords get_selected_penguin_cell(int player_index) const;

  const wxBitmap& get_player_penguin_sprite(int player_id, bool flipped = false) const;

protected:
  virtual wxSize DoGetBestClientSize() const override;
  void on_paint(wxPaintEvent& event);
  void draw_bitmap(wxDC& dc, const wxBitmap& bitmap, const wxPoint& pos);
  void paint_tiles(wxDC& dc);
  void paint_board(wxDC& dc, wxDC& tiles_dc);
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
  const TilesetHelper& tileset;

  wxDECLARE_EVENT_TABLE();
};

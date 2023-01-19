#pragma once

#include "gui/game_state.hh"
#include "gui/player_info_box.hh"
#include "gui/tileset.hh"
#include "utils.h"
#include <cstddef>
#include <memory>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
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

class CanvasPanel : public wxPanel {
public:
  static const wxCoord CELL_SIZE = 40;
  static const int CELL_FONT_SIZE = 16;
  static const int FISH_CIRCLE_RADIUS = 4;
  static const int BLOCKED_CELL_LIGHTNESS = -40;

  CanvasPanel(GameFrame* parent, wxWindowID id, GuiGameState& state, const TilesetHelper& tileset);

  std::unique_ptr<bool[]> blocked_cells{ nullptr };
  bool* cell_blocked_ptr(Coords cell) const;
  void update_blocked_cells();

  wxSize get_canvas_size() const;
  Coords get_cell_by_coords(wxPoint point) const;
  wxRect get_cell_rect(Coords cell) const;
  wxPoint get_cell_centre(Coords cell) const;

  Coords get_selected_penguin_cell(int player_index) const;

  void mark_board_dirty() {
    this->board_dirty = true;
  }

  const wxBitmap& get_player_penguin_sprite(int player_id, bool flipped = false) const;

protected:
  virtual wxSize DoGetBestClientSize() const override;
  void on_paint(wxPaintEvent& event);
  void paint_board(wxDC& dc);
  void paint_overlay(wxDC& dc);

  void on_any_mouse_event(wxMouseEvent& event);
  void on_mouse_down(wxMouseEvent& event);
  void on_mouse_move(wxMouseEvent& event);
  void on_mouse_up(wxMouseEvent& event);
  void on_mouse_enter_leave(wxMouseEvent& event);

  bool board_dirty = true;
  wxBitmap board_bitmap;

  bool mouse_within_window = false;
  bool mouse_is_down;
  wxPoint mouse_pos = wxDefaultPosition;
  wxPoint prev_mouse_pos = wxDefaultPosition;
  wxPoint mouse_drag_pos = wxDefaultPosition;

  GameFrame* game_frame;
  GuiGameState& state;
  const TilesetHelper& tileset;

  wxDECLARE_EVENT_TABLE();
};

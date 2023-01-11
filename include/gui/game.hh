#pragma once

#include "board.h"
#include "gui/tileset.hh"
#include "movement.h"
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
#include "gui/player_info_box.hh"

class GuiGameState {
  wxDECLARE_NO_COPY_CLASS(GuiGameState);

public:
  GuiGameState() {}

  static void free_board_ptr(Board* board) {
    free_board(board);
    delete board;
  }

  int players_count;
  int penguins_per_player;
  std::unique_ptr<wxString[]> player_names;
  std::unique_ptr<Board, decltype(&free_board_ptr)> board{ nullptr, free_board_ptr };
  std::unique_ptr<bool[]> blocked_cells;

  GamePhase game_phase;
  int current_player;
  int penguins_left_to_place;
};

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id, GuiGameState& state, const TilesetHelper& tileset);

  void start_new_game();

  PlayerInfoBox* get_player_info_box(size_t idx) {
    return this->player_info_boxes.at(idx);
  }

  void update_player_info_boxes();

protected:
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_mouse_enter_leave(wxMouseEvent& event);

  CanvasPanel* canvas_panel;
  wxBoxSizer* players_box;
  wxVector<PlayerInfoBox*> player_info_boxes;
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

  wxSize get_board_size() const;
  bool is_cell_in_bounds(wxPoint cell) const;
  int* cell_ptr(wxPoint cell) const;

  bool is_cell_blocked(wxPoint cell) const;
  void update_blocked_cells();

  MovementError validate_movement(wxPoint start, wxPoint target, wxPoint* fail = nullptr);

  wxSize get_canvas_size() const;
  wxPoint get_cell_by_coords(wxPoint point) const;
  wxRect get_cell_rect(wxPoint cell) const;
  wxPoint get_cell_centre(wxPoint cell) const;

  void mark_board_dirty() {
    this->board_dirty = true;
  }

  const wxBitmap& get_player_penguin_sprite(int player_id, bool flipped = false) const;

  void update_player_scores();

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

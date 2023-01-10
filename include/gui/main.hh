#pragma once

#include <memory>
#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/string.h>
#include <wx/vector.h>
#include <wx/window.h>

extern "C" {
#include "board.h"
#include "gamestate.h"
#include "movement.h"
}

static_assert(
  std::is_standard_layout<wxPoint>() && std::is_standard_layout<Coords>() &&
    sizeof(wxPoint) == sizeof(Coords) && sizeof(wxPoint::x) == sizeof(Coords::x) &&
    offsetof(wxPoint, x) == offsetof(Coords, x) && sizeof(wxPoint::y) == sizeof(Coords::y) &&
    offsetof(wxPoint, y) == offsetof(Coords, y),
  "The layout of wxPoint and Coords must be compatible"
);

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

class CanvasPanel : public wxPanel {
public:
  static const wxCoord CELL_SIZE = 32;
  static const int CELL_FONT_SIZE = 16;
  static const int FISH_CIRCLE_RADIUS = 4;
  static const int BLOCKED_CELL_LIGHTNESS = -40;

  enum TileEdge {
    EDGE_TOP = 0,
    EDGE_RIGHT = 1,
    EDGE_BOTTOM = 2,
    EDGE_LEFT = 3,
    EDGE_MAX,
  };

  enum TileCorner {
    CORNER_TOP_RIGHT = 0,
    CORNER_BOTTOM_RIGHT = 1,
    CORNER_BOTTOM_LEFT = 2,
    CORNER_TOP_LEFT = 3,
    CORNER_MAX,
  };

  static const int TILE_SIZE = 16;
  static const int TILESET_SCALING = CELL_SIZE / TILE_SIZE;

  CanvasPanel(wxWindow* parent, wxWindowID id, GuiGameState& state);

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

protected:
  virtual wxSize DoGetBestClientSize() const override;
  void on_paint(wxPaintEvent& event);
  void paint_board(wxDC& dc);
  void paint_overlay(wxDC& dc);

  void load_tileset();

  void on_any_mouse_event(wxMouseEvent& event);
  void on_mouse_down(wxMouseEvent& event);
  void on_mouse_move(wxMouseEvent& event);
  void on_mouse_up(wxMouseEvent& event);
  void on_mouse_enter_leave(wxMouseEvent& event);

  bool board_dirty = true;
  wxBitmap board_bitmap;

  wxBitmap water_tile;
  wxBitmap ice_tiles[6];
  wxBitmap tile_edges[EDGE_MAX];
  wxBitmap tile_convex_corners[CORNER_MAX];
  wxBitmap tile_concave_corners[CORNER_MAX];

  bool mouse_within_window = false;
  bool mouse_is_down;
  wxPoint mouse_pos = wxDefaultPosition;
  wxPoint prev_mouse_pos = wxDefaultPosition;
  wxPoint mouse_drag_pos = wxDefaultPosition;

  GuiGameState& state;

private:
  wxDECLARE_EVENT_TABLE();
};

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id, GuiGameState& state);

  void start_new_game();

protected:
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_mouse_enter_leave(wxMouseEvent& event);

  CanvasPanel* canvas_panel;
  GuiGameState& state;

private:
  wxDECLARE_EVENT_TABLE();
};

class PenguinsApp : public wxApp {
public:
  PenguinsApp();
  virtual bool OnInit() override;

protected:
  GameFrame* game_frame;
  GuiGameState game_state;
};

wxDECLARE_APP(PenguinsApp);

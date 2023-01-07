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
}

class GameState {
  wxDECLARE_NO_COPY_CLASS(GameState);

public:
  GameState() {}

  int players_count;
  int penguins_per_player;
  wxString* player_names;
  std::unique_ptr<Board, decltype(&free_board)> board{ nullptr, free_board };

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

  CanvasPanel(wxWindow* parent, wxWindowID id, GameState& state);

  wxSize get_board_size() const;
  bool is_cell_in_bounds(wxPoint cell) const;
  int* cell_ptr(wxPoint cell) const;
  bool is_cell_blocked(wxPoint cell) const;

  wxSize get_canvas_size() const;
  wxPoint get_cell_by_coords(wxPoint point) const;
  wxRect get_cell_rect(wxPoint cell) const;

  void mark_board_dirty() {
    this->board_dirty = true;
  }

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
  wxPoint mouse_pos;
  wxPoint mouse_drag_pos;

  GameState& state;

private:
  wxDECLARE_EVENT_TABLE();
};

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id, GameState& state);

  void start_new_game();

protected:
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_mouse_enter_leave(wxMouseEvent& event);

  CanvasPanel* canvas_panel;
  GameState& state;

private:
  wxDECLARE_EVENT_TABLE();
};

class PenguinsApp : public wxApp {
public:
  PenguinsApp();
  virtual bool OnInit() override;

protected:
  GameFrame* game_frame;
  GameState game_state;
};

wxDECLARE_APP(PenguinsApp);

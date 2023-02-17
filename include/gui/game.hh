#pragma once

#include "game.h"
#include "gui/game_state.hh"
#include "gui/player_info_box.hh"
#include "utils.h"
#include <memory>
#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/thread.h>
#include <wx/timer.h>
#include <wx/types.h>
#include <wx/utils.h>
#include <wx/window.h>

class CanvasPanel;
class BotThread;

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id);
  ~GameFrame();

  void update_layout();

  void start_new_game();
  void update_game_state();
  void end_game();
  void close_game();
  void update_player_info_boxes();

  bool executing_bot_turn = false;
  void execute_bot_turn();
  void on_bot_thread_done_work(bool cancelled);
  void run_bot_thread(BotThread* thread);
  void stop_bot_thread();
  void on_bot_thread_exited(BotThread* thread);
  void start_bot_progress();
  void stop_bot_progress();

  PlayerType get_current_player_type() const;
  void place_penguin(Coords target);
  void move_penguin(Coords penguin, Coords target);

  GuiGameState state{};

protected:
  void on_destroy(wxWindowDestroyEvent& event);
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_close_game(wxCommandEvent& event);

  wxPanel* root_panel;
  wxScrolledWindow* scrolled_panel;
  wxBoxSizer* canvas_sizer;
  wxPanel* empty_canvas_panel;
  CanvasPanel* canvas_panel = nullptr;
  wxBoxSizer* players_box;
  std::unique_ptr<PlayerInfoBox*[]> player_info_boxes;

  wxCriticalSection bot_thread_cs;
  BotThread* bot_thread = nullptr;
  wxTimer progress_timer;
  wxWindow* progress_container;
  wxGauge* progress_bar;
  wxButton* stop_bot_button;
  std::unique_ptr<wxBusyCursor> busy_cursor_changer{ nullptr };
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

  CanvasPanel(wxWindow* parent, wxWindowID id, GameFrame* game_frame);

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

  Coords get_selected_penguin_coords() const;

protected:
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
  Game* game;

  wxDECLARE_EVENT_TABLE();
};

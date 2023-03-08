#pragma once

#include "gui/game_state.hh"
#include <memory>
#include <wx/button.h>
#include <wx/clntdata.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/gauge.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/vector.h>
#include <wx/window.h>

class GameController;
class CanvasPanel;
class PlayerInfoBox;

class GameLogListBoxEntry : public wxClientData {
public:
  explicit GameLogListBoxEntry(size_t index) : index(index) {}
  const size_t index;
};

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id);
  virtual ~GameFrame();

  void update_layout();

  void start_new_game();
  void update_game_state();
  void update_game_log();
  wxString describe_game_log_entry(size_t index) const;
  GameController* get_controller_for_current_turn();
  void set_controller(GameController* next_controller);
  void end_game();
  void close_game();
  void clear_status_bar();
  void update_player_info_boxes();

  void start_bot_progress();
  void stop_bot_progress();

  GuiGameState state{};
  CanvasPanel* canvas = nullptr;
  GameController* controller = nullptr;
  wxButton* show_current_turn_btn;

protected:
  void on_destroy(wxWindowDestroyEvent& event);
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_close_game(wxCommandEvent& event);
  void on_game_log_select(wxCommandEvent& event);
  void on_show_current_turn_clicked(wxCommandEvent& event);

  wxPanel* root_panel;
  wxScrolledWindow* scrolled_panel;
  wxBoxSizer* canvas_sizer;
  wxPanel* empty_canvas;
  wxBoxSizer* players_box;
  wxVector<PlayerInfoBox*> player_info_boxes;
  wxListBox* game_log;

  wxTimer progress_timer;
  wxWindow* progress_container;
  wxGauge* progress_bar;
#ifndef __WXOSX__
  std::unique_ptr<wxBusyCursor> busy_cursor_changer{ nullptr };
#endif
};

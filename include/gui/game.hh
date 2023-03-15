#pragma once

#include "bot.h"
#include "game.h"
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

class NewGameDialog;
class GameController;
class CanvasPanel;
class PlayerInfoBox;

class GameLogListBoxEntry : public wxClientData {
public:
  explicit GameLogListBoxEntry(size_t index) : index(index) {}
  const size_t index;
};

class GameFrame;

class BaseGamePanel : public wxPanel {
public:
  BaseGamePanel(GameFrame* parent, wxWindowID id);
  virtual ~BaseGamePanel() {}
  virtual void update_layout();
  GameFrame* frame;
};

class GameStartPanel : public BaseGamePanel {
public:
  GameStartPanel(GameFrame* parent, wxWindowID id);
};

class GamePanel : public BaseGamePanel {
public:
  GamePanel(GameFrame* parent, wxWindowID id, NewGameDialog* dialog);
  virtual ~GamePanel();

  virtual void update_layout() override;

  void update_game_state();
  void update_game_log();
  wxString describe_game_log_entry(size_t index) const;
  GameController* get_controller_for_current_turn();
  void set_controller(GameController* next_controller);
  void show_game_results();
  void update_player_info_boxes();

  void start_bot_progress();
  void stop_bot_progress();

  bool game_ended = false;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::shared_ptr<BotParameters> bot_params{ nullptr };
  wxVector<wxString> player_names;
  wxVector<PlayerType> player_types;

  GameController* controller = nullptr;

  CanvasPanel* canvas;
  wxScrolledWindow* scrolled_panel;

  wxVector<PlayerInfoBox*> player_info_boxes;

  wxButton* show_current_turn_btn;
  wxListBox* log_list;
  size_t displayed_log_entries = 0;

  wxTimer progress_timer;
#ifndef __WXOSX__
  std::unique_ptr<wxBusyCursor> busy_cursor_changer{ nullptr };
#endif

protected:
  void on_game_log_select(wxCommandEvent& event);
  void on_show_current_turn_clicked(wxCommandEvent& event);
};

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id);
  virtual ~GameFrame();

  void set_panel(BaseGamePanel* panel);
  void clear_status_bar();

  void start_new_game();

  BaseGamePanel* current_panel = nullptr;

  wxWindow* progress_container;
  wxGauge* progress_bar;

protected:
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);
  void on_close_game(wxCommandEvent& event);
};

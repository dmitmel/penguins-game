#pragma once

#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/vector.h>

class NewGameDialog : public wxDialog {
public:
  NewGameDialog(wxWindow* parent, wxWindowID id);

  static const int DEFAULT_BOARD_WIDTH = 16;
  static const int DEFAULT_BOARD_HEIGHT = 16;
  static const int DEFAULT_PENGUINS_PER_PLAYER = 2;
  static const int DEFAULT_NUMBER_OF_PLAYERS = 2;

  int get_board_width() const;
  int get_board_height() const;
  int get_penguins_per_player() const;
  size_t get_number_of_players() const;
  wxString get_player_name(size_t index) const;

protected:
  wxSpinCtrl* width_input;
  bool width_was_changed = false;
  wxSpinCtrl* height_input;
  bool height_was_changed = false;
  wxSpinCtrl* penguins_input;
  wxSpinCtrl* players_number_input;
  wxFlexGridSizer* players_grid;
  struct PlayerRowWidgets {
    wxTextCtrl* name_input;
    wxBitmapButton* delete_btn;
  };
  wxVector<PlayerRowWidgets> player_rows;
  PlayerRowWidgets new_player_row;
  wxStdDialogButtonSizer* buttons_sizer;

  void update_layout();

  void set_player_rows_count(size_t count);
  void update_new_player_row();
  void add_new_player_row(bool initial = false);
  void realize_player_row(size_t index);
  void delete_player_row(size_t index);

  void on_ok(wxCommandEvent& event);
  void on_close(wxCommandEvent& event);
  void on_player_name_input(wxCommandEvent& event);
  void on_player_name_enter_pressed(wxCommandEvent& event);
  void on_board_width_input(wxSpinEvent& event);
  void on_board_height_input(wxSpinEvent& event);
  void on_player_delete_clicked(wxCommandEvent& event);
  void on_players_number_input(wxSpinEvent& event);

private:
  wxDECLARE_EVENT_TABLE();
};

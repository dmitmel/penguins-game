#pragma once

#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/vector.h>

class NewGameDialog : public wxDialog {
public:
  NewGameDialog(wxWindow* parent, wxWindowID id);

protected:
  wxPanel* main_panel;

  wxSpinCtrl* width_input;
  bool width_was_changed = false;
  wxSpinCtrl* height_input;
  bool height_was_changed = false;
  wxSpinCtrl* penguins_input;
  wxFlexGridSizer* players_grid;
  struct PlayerRowWidgets {
    wxTextCtrl* name_input;
    wxBitmapButton* delete_btn;
  };
  wxVector<PlayerRowWidgets> player_rows;
  PlayerRowWidgets new_player_row;
  wxStdDialogButtonSizer* buttons_sizer;

  void update_layout();

  void add_new_player_row(bool initial = false);
  void realize_player_row(size_t index);

  bool can_submit() const;

  void on_ok(wxCommandEvent& event);
  void on_close(wxCommandEvent& event);
  void on_player_name_input(wxCommandEvent& event);
  void on_player_name_enter_pressed(wxCommandEvent& event);
  void on_board_width_input(wxSpinEvent& event);
  void on_board_height_input(wxSpinEvent& event);
  void on_player_delete_clicked(wxCommandEvent& event);

private:
  wxDECLARE_EVENT_TABLE();
};

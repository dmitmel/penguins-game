#pragma once

#include "gui/game_state.hh"
#include <wx/bmpbuttn.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/persist.h>
#include <wx/persist/window.h>
#include <wx/sizer.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/vector.h>

class NewGameDialog : public wxDialog {
public:
  NewGameDialog(wxWindow* parent, wxWindowID id);
  ~NewGameDialog();

  static const int DEFAULT_BOARD_WIDTH = 16;
  static const int DEFAULT_BOARD_HEIGHT = 16;
  static const int DEFAULT_PENGUINS_PER_PLAYER = 2;
  static const int DEFAULT_NUMBER_OF_PLAYERS = 2;

  int get_board_width() const;
  int get_board_height() const;
  BoardGenType get_board_gen_type() const;
  int get_penguins_per_player() const;
  size_t get_number_of_players() const;
  wxString get_player_name(size_t index) const;
  PlayerType get_player_type(size_t index) const;

protected:
  wxFlexGridSizer* options_grid;
  wxSpinCtrl* width_input;
  bool width_was_changed = false;
  wxSpinCtrl* height_input;
  wxChoice* board_gen_input;
  bool height_was_changed = false;
  wxSpinCtrl* penguins_input;
  wxSpinCtrl* players_number_input;
  wxFlexGridSizer* players_grid;
  struct PlayerRowWidgets {
    wxChoice* type_input;
    wxTextCtrl* name_input;
    wxBitmapButton* delete_btn;
  };
  wxVector<PlayerRowWidgets> player_rows;
  PlayerRowWidgets new_player_row;
  wxStdDialogButtonSizer* buttons_sizer;

  wxWindow* add_option(const wxString& label, wxWindow* input);
  wxSpinCtrl*
  create_number_option(const wxString& label, wxWindowID id, int min, int max, int initial);
  wxChoice*
  create_choice_option(const wxString& label, wxWindowID id, int n, const wxString choices[]);

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

  wxDECLARE_EVENT_TABLE();

public:
  class Persistence : public wxPersistentWindow<NewGameDialog> {
  public:
    Persistence(NewGameDialog* dialog) : wxPersistentWindow<NewGameDialog>(dialog) {}
    virtual wxString GetKind() const override {
      return "NewGameDialog";
    }
    virtual bool Restore() override;
    virtual void Save() const override;
  };
};

inline wxPersistentObject* wxCreatePersistentObject(NewGameDialog* dialog) {
  return new NewGameDialog::Persistence(dialog);
}

#include "gui/new_game_dialog.hh"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/panel.h>
#include <wx/spinbutt.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/string.h>

enum {
  ID_BOARD_WIDTH = wxID_HIGHEST + 1,
  ID_BOARD_HEIGHT,
  ID_PENGUINS_NUMBER,
  ID_PLAYERS_NUMBER,
  ID_PLAYER_NAME,
  ID_PLAYER_DELETE,
};

// clang-format off
wxBEGIN_EVENT_TABLE(NewGameDialog, wxDialog)
  EVT_BUTTON(wxID_OK, NewGameDialog::on_ok)
  EVT_BUTTON(wxID_CLOSE, NewGameDialog::on_close)
  EVT_TEXT(ID_PLAYER_NAME, NewGameDialog::on_player_name_input)
  EVT_TEXT_ENTER(ID_PLAYER_NAME, NewGameDialog::on_player_name_enter_pressed)
  EVT_SPINCTRL(ID_BOARD_WIDTH, NewGameDialog::on_board_width_input)
  EVT_SPINCTRL(ID_BOARD_HEIGHT, NewGameDialog::on_board_height_input)
  EVT_BUTTON(ID_PLAYER_DELETE, NewGameDialog::on_player_delete_clicked)
  EVT_SPINCTRL(ID_PLAYERS_NUMBER, NewGameDialog::on_players_number_input)
wxEND_EVENT_TABLE();
// clang-format on

NewGameDialog::NewGameDialog(wxWindow* parent, wxWindowID id)
: wxDialog(parent, id, "New game options", wxDefaultPosition, wxDefaultSize) {
#if wxCHECK_VERSION(3, 1, 4)
  float spacing = wxSizerFlags::GetDefaultBorderFractional();
#else
  float spacing = wxSizerFlags::GetDefaultBorder();
#endif

  auto grid = new wxFlexGridSizer(
    /* cols */ 2,
    /* vgap */ wxRound(spacing),
    /* hgap */ wxRound(spacing * 3)
  );

  auto width_label = new wxStaticText(this, ID_BOARD_WIDTH, "Board width:");
  grid->Add(width_label, wxSizerFlags().Centre().Left());
  this->width_input = new wxSpinCtrl(this, ID_BOARD_WIDTH);
  this->width_input->SetValue(20);
  this->width_input->SetRange(1, 1000);
  grid->Add(this->width_input, wxSizerFlags().Expand());

  auto height_label = new wxStaticText(this, ID_BOARD_HEIGHT, "Board height:");
  grid->Add(height_label, wxSizerFlags().Centre().Left());
  this->height_input = new wxSpinCtrl(this, ID_BOARD_HEIGHT);
  this->height_input->SetValue(20);
  this->height_input->SetRange(1, 1000);
  grid->Add(this->height_input, wxSizerFlags().Expand());

  auto penguins_number_label = new wxStaticText(this, ID_PENGUINS_NUMBER, "Penguins per player:");
  grid->Add(penguins_number_label, wxSizerFlags().Centre().Left());
  this->penguins_input = new wxSpinCtrl(this, ID_PENGUINS_NUMBER);
  this->penguins_input->SetValue(2);
  this->penguins_input->SetRange(1, 100);
  grid->Add(this->penguins_input, wxSizerFlags().Expand());

  auto players_number_label = new wxStaticText(this, ID_PLAYERS_NUMBER, "Number of players:");
  grid->Add(players_number_label, wxSizerFlags().Centre().Left());
  this->players_number_input = new wxSpinCtrl(this, ID_PLAYERS_NUMBER);
  this->players_number_input->SetValue(2);
  this->players_number_input->SetRange(1, 100);
  grid->Add(this->players_number_input, wxSizerFlags().Expand());

  this->players_grid = new wxFlexGridSizer(
    /* cols */ 2,
    /* vgap */ wxRound(spacing),
    /* hgap */ wxRound(spacing / 2)
  );
  this->players_grid->AddGrowableCol(0, 1);

  this->add_new_player_row(true);
  for (int i = 0; i < this->players_number_input->GetValue(); i++) {
    this->add_new_player_row();
  }

  this->player_rows.at(0).name_input->SetValue("A");
  this->player_rows.at(1).name_input->SetValue("B");

  auto grids_vbox = new wxBoxSizer(wxVERTICAL);
  grids_vbox->Add(grid, wxSizerFlags().Expand().DoubleBorder(wxBOTTOM));
  grids_vbox->Add(this->players_grid, wxSizerFlags().Expand());

  auto outer_vbox = new wxBoxSizer(wxVERTICAL);
  outer_vbox->Add(grids_vbox, wxSizerFlags().Expand().DoubleBorder());
  this->buttons_sizer = this->CreateStdDialogButtonSizer(wxOK | wxCLOSE);
  this->SetEscapeId(wxID_CLOSE);
  outer_vbox->Add(new wxStaticLine(this), wxSizerFlags().Expand());
  outer_vbox->Add(this->buttons_sizer, wxSizerFlags().Expand().DoubleBorder(wxTOP | wxBOTTOM));
  this->SetSizerAndFit(outer_vbox);

  this->width_input->SetFocus();
}

void NewGameDialog::update_layout() {
  this->GetSizer()->SetSizeHints(this);
}

int NewGameDialog::get_board_width() const {
  return this->width_input->GetValue();
}

int NewGameDialog::get_board_height() const {
  return this->height_input->GetValue();
}

int NewGameDialog::get_penguins_per_player() const {
  return this->penguins_input->GetValue();
}

size_t NewGameDialog::get_number_of_players() const {
  return this->player_rows.size();
}

wxString NewGameDialog::get_player_name(size_t index) const {
  return this->player_rows.at(index).name_input->GetValue();
}

void NewGameDialog::add_new_player_row(bool initial) {
  auto grid = this->players_grid;

  if (!initial) {
    this->player_rows.push_back(this->new_player_row);
    this->realize_player_row(this->player_rows.size() - 1);
  }

  auto name_input = new wxTextCtrl(
    this, ID_PLAYER_NAME, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER
  );
  name_input->SetHint("Add a new player...");
  grid->Add(name_input, wxSizerFlags().Expand());

  auto delete_btn =
    new wxBitmapButton(this, ID_PLAYER_DELETE, wxArtProvider::GetIcon(wxART_CROSS_MARK));
  delete_btn->Hide();
  grid->Add(delete_btn, wxSizerFlags());

  this->new_player_row = { name_input, delete_btn };
}

void NewGameDialog::realize_player_row(size_t index) {
  PlayerRowWidgets& row = this->player_rows.at(index);
  row.name_input->SetHint(wxString::Format("Name of player %ld", index + 1));
  row.delete_btn->Show();
}

void NewGameDialog::delete_player_row(size_t index) {
  PlayerRowWidgets& row = this->player_rows.at(index);
  row.name_input->Destroy();
  row.delete_btn->Destroy();
  this->player_rows.erase(&row);
}

void NewGameDialog::on_ok(wxCommandEvent& event) {
  if (this->player_rows.empty()) {
    return;
  }
  for (auto& row : this->player_rows) {
    if (row.name_input->IsEmpty()) {
      row.name_input->SetFocus();
      return;
    }
  }
  this->EndModal(wxID_OK);
}

void NewGameDialog::on_close(wxCommandEvent& event) {
  this->EndModal(wxID_CANCEL);
}

void NewGameDialog::on_player_name_input(wxCommandEvent& event) {
  if (event.GetEventObject() == this->new_player_row.name_input) {
    this->add_new_player_row();
    this->players_number_input->SetValue(this->player_rows.size());
    this->update_layout();
  }
}

void NewGameDialog::on_player_name_enter_pressed(wxCommandEvent& event) {
  for (size_t i = 0; i < this->player_rows.size(); i++) {
    if (this->player_rows.at(i).name_input == event.GetEventObject()) {
      for (size_t j = i + 1; j < this->player_rows.size(); j++) {
        PlayerRowWidgets& row = this->player_rows.at(j);
        if (row.name_input->IsEmpty()) {
          row.name_input->SetFocus();
          return;
        }
      }
      this->new_player_row.name_input->SetFocus();
      return;
    }
  }
  event.Skip();
}

void NewGameDialog::on_board_width_input(wxSpinEvent& event) {
  width_was_changed = true;
  if (!height_was_changed) {
    this->height_input->SetValue(this->width_input->GetValue());
  }
}

void NewGameDialog::on_board_height_input(wxSpinEvent& event) {
  height_was_changed = true;
  if (!width_was_changed) {
    this->width_input->SetValue(this->height_input->GetValue());
  }
}

void NewGameDialog::on_player_delete_clicked(wxCommandEvent& event) {
  for (size_t i = 0; i < this->player_rows.size(); i++) {
    if (this->player_rows.at(i).delete_btn == event.GetEventObject()) {
      if (this->player_rows.size() > 1) {
        this->delete_player_row(i);
        this->players_number_input->SetValue(this->player_rows.size());
        for (size_t j = 0; j < this->player_rows.size(); j++) {
          this->realize_player_row(j);
        }
        this->update_layout();
      }
      return;
    }
  }
  event.Skip();
}

void NewGameDialog::on_players_number_input(wxSpinEvent& event) {
  size_t prev_len = this->player_rows.size();
  size_t next_len = this->players_number_input->GetValue();
  if (next_len < prev_len) {
    for (size_t i = prev_len - 1; i >= next_len; i--) {
      this->delete_player_row(i);
    }
  } else if (next_len > prev_len) {
    for (size_t i = prev_len; i < next_len; i++) {
      this->add_new_player_row();
    }
  } else {
    return;
  }
  this->update_layout();
}
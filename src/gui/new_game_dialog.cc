#include "gui/new_game_dialog.hh"
#include "utils.h"
#include <wx/choice.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/math.h>
#include <wx/sizer.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/version.h>
#ifdef __WXGTK__
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
// IWYU pragma: no_include <wx/bmpbndl.h>
#else
#include <wx/button.h>
#endif

NewGameDialog::NewGameDialog(wxWindow* parent, wxWindowID id)
: wxDialog(parent, id, "New game options", wxDefaultPosition, wxDefaultSize) {
  this->Bind(wxEVT_BUTTON, &NewGameDialog::on_ok, this, wxID_OK);
  this->Bind(wxEVT_BUTTON, &NewGameDialog::on_close, this, wxID_CLOSE);

#if wxCHECK_VERSION(3, 1, 4)
  float spacing = wxSizerFlags::GetDefaultBorderFractional();
#else
  float spacing = wxSizerFlags::GetDefaultBorder();
#endif

  this->options_grid = new wxFlexGridSizer(
    /* cols */ 2,
    /* vgap */ wxRound(spacing),
    /* hgap */ wxRound(spacing * 3)
  );

  this->width_input =
    this->create_number_option("Board width:", wxID_ANY, 1, 1000, DEFAULT_BOARD_WIDTH);
  this->width_input->Bind(wxEVT_SPINCTRL, &NewGameDialog::on_board_width_input, this);
  this->height_input =
    this->create_number_option("Board height:", wxID_ANY, 1, 1000, DEFAULT_BOARD_HEIGHT);
  this->height_input->Bind(wxEVT_SPINCTRL, &NewGameDialog::on_board_height_input, this);

  wxString board_gen_types[BOARD_GEN_MAX] = {};
  board_gen_types[BOARD_GEN_RANDOM] = "Random";
  board_gen_types[BOARD_GEN_ISLAND] = "Island";
  this->board_gen_input = this->create_choice_option(
    "Board generation type:", wxID_ANY, WXSIZEOF(board_gen_types), board_gen_types
  );
  this->board_gen_input->Select(0);

  this->penguins_input = this->create_number_option(
    "Penguins per player:", wxID_ANY, 1, 10, DEFAULT_PENGUINS_PER_PLAYER
  );
  this->players_number_input =
    this->create_number_option("Number of players:", wxID_ANY, 2, 5, DEFAULT_NUMBER_OF_PLAYERS);
  this->players_number_input->Bind(wxEVT_SPINCTRL, &NewGameDialog::on_players_number_input, this);

  this->players_grid = new wxFlexGridSizer(
    /* cols */ 3,
    /* vgap */ wxRound(spacing),
    /* hgap */ wxRound(spacing / 2)
  );
  this->players_grid->AddGrowableCol(0, 1);

  this->add_new_player_row(true);
  for (int i = 0; i < this->players_number_input->GetValue(); i++) {
    this->add_new_player_row();
  }

  auto grids_vbox = new wxBoxSizer(wxVERTICAL);
  grids_vbox->Add(this->options_grid, wxSizerFlags().Expand().DoubleBorder(wxBOTTOM));
  grids_vbox->Add(this->players_grid, wxSizerFlags().Expand());

  auto outer_vbox = new wxBoxSizer(wxVERTICAL);
  wxSizerFlags grids_vbox_sizer_flags = wxSizerFlags().Expand().DoubleBorder(wxALL);
#ifdef __WXOSX__
  grids_vbox_sizer_flags.Border(wxALL, wxRound(spacing * 4));
#endif
  outer_vbox->Add(grids_vbox, grids_vbox_sizer_flags);
  this->buttons_sizer = this->CreateStdDialogButtonSizer(wxOK | wxCLOSE);
  this->SetEscapeId(wxID_CLOSE);
  outer_vbox->Add(new wxStaticLine(this), wxSizerFlags().Expand());
  wxSizerFlags buttons_sizer_flags = wxSizerFlags().Expand().DoubleBorder(wxALL);
#ifdef __WXGTK__
  buttons_sizer_flags.DoubleBorder(wxTOP | wxBOTTOM);
#endif
  outer_vbox->Add(this->buttons_sizer, buttons_sizer_flags);
  this->SetSizerAndFit(outer_vbox);

  this->width_input->SetFocus();
}

NewGameDialog::~NewGameDialog() {
  // <https://docs.wxwidgets.org/3.0/classwx_window_destroy_event.html#details>
  this->SendDestroyEvent();
}

wxWindow* NewGameDialog::add_option(const wxString& label_str, wxWindow* input) {
  auto label = new wxStaticText(this, input->GetId(), label_str);
  this->options_grid->Add(label, wxSizerFlags().Centre().Left());
  this->options_grid->Add(input, wxSizerFlags().Expand());
  return input;
}

wxSpinCtrl* NewGameDialog::create_number_option(
  const wxString& label, wxWindowID id, int min, int max, int initial
) {
  auto input = new wxSpinCtrl(
    this, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, min, max, initial
  );
  this->add_option(label, input);
  return input;
}

wxChoice* NewGameDialog::create_choice_option(
  const wxString& label, wxWindowID id, int n, const wxString choices[]
) {
  auto input = new wxChoice(this, id, wxDefaultPosition, wxDefaultSize, n, choices);
  this->add_option(label, input);
  return input;
}

void NewGameDialog::update_layout() {
  this->Layout();
  this->GetSizer()->SetSizeHints(this);
}

int NewGameDialog::get_board_width() const {
  return this->width_input->GetValue();
}

int NewGameDialog::get_board_height() const {
  return this->height_input->GetValue();
}

BoardGenType NewGameDialog::get_board_gen_type() const {
  return static_cast<BoardGenType>(this->board_gen_input->GetSelection());
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

PlayerType NewGameDialog::get_player_type(size_t index) const {
  return static_cast<PlayerType>(this->player_rows.at(index).type_input->GetSelection());
}

void NewGameDialog::set_player_rows_count(size_t count) {
  size_t len = this->player_rows.size();
  if (count < len) {
    for (size_t i = len - 1; i >= count; i--) {
      this->delete_player_row(i);
    }
  } else if (count > len) {
    for (size_t i = len; i < count; i++) {
      this->add_new_player_row();
    }
  }
  this->update_new_player_row();
}

void NewGameDialog::update_new_player_row() {
  bool show = this->player_rows.size() < size_t(this->players_number_input->GetMax());
  this->new_player_row.name_input->Show(show);
  this->new_player_row.type_input->Show(show);
  this->new_player_row.delete_btn->Show(this->new_player_row.delete_btn->IsShown() && show);
  wxSizerItem* delete_btn_item = this->players_grid->GetItem(this->new_player_row.delete_btn);
  delete_btn_item->SetFlag(
    change_bit(delete_btn_item->GetFlag(), wxRESERVE_SPACE_EVEN_IF_HIDDEN, show)
  );
}

void NewGameDialog::add_new_player_row(bool initial) {
  auto grid = this->players_grid;

  if (!initial) {
    this->player_rows.push_back(this->new_player_row);
    this->realize_player_row(this->player_rows.size() - 1);
  }

  auto name_input = new wxTextCtrl(
    this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER
  );
  name_input->Bind(wxEVT_TEXT, &NewGameDialog::on_player_name_input, this);
  name_input->Bind(wxEVT_TEXT_ENTER, &NewGameDialog::on_player_name_enter_pressed, this);
  name_input->SetHint("Add a new player...");
  grid->Add(name_input, wxSizerFlags().Expand());

  wxString player_types[PLAYER_TYPE_MAX];
  player_types[PLAYER_NORMAL] = "Normal";
  player_types[PLAYER_BOT] = "Bot";
  auto type_input =
    new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, PLAYER_TYPE_MAX, player_types);
  type_input->Select(0);
  grid->Add(type_input, wxSizerFlags().Expand());

#if defined(__WXGTK__)
  auto delete_btn = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetIcon(wxART_CROSS_MARK));
#elif defined(__WXOSX__)
  auto delete_btn =
    new wxButton(this, wxID_ANY, "X", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
#else
  auto delete_btn =
    new wxButton(this, wxID_ANY, " X ", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
#endif
  delete_btn->Bind(wxEVT_BUTTON, &NewGameDialog::on_player_delete_clicked, this);
  delete_btn->Hide();
  grid->Add(delete_btn, wxSizerFlags().Expand());

  this->new_player_row = { type_input, name_input, delete_btn };
  this->update_new_player_row();
}

void NewGameDialog::realize_player_row(size_t index) {
  PlayerRowWidgets& row = this->player_rows.at(index);
  row.name_input->SetHint(wxString::Format("Name of player %zd", index + 1));
  row.delete_btn->Show();
}

void NewGameDialog::delete_player_row(size_t index) {
  PlayerRowWidgets& row = this->player_rows.at(index);
  row.name_input->Destroy();
  row.type_input->Destroy();
  row.delete_btn->Destroy();
  this->player_rows.erase(this->player_rows.begin() + index);
}

void NewGameDialog::on_ok(wxCommandEvent& WXUNUSED(event)) {
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

void NewGameDialog::on_close(wxCommandEvent& WXUNUSED(event)) {
  this->EndModal(wxID_CLOSE);
}

void NewGameDialog::on_player_name_input(wxCommandEvent& event) {
  if (event.GetEventObject() == this->new_player_row.name_input) {
    // Try to set the input value first, this will perform the range checks.
    this->players_number_input->SetValue(this->player_rows.size() + 1);
    this->set_player_rows_count(this->players_number_input->GetValue());
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

void NewGameDialog::on_board_width_input(wxSpinEvent& WXUNUSED(event)) {
  width_was_changed = true;
  if (!height_was_changed) {
    this->height_input->SetValue(this->width_input->GetValue());
  }
}

void NewGameDialog::on_board_height_input(wxSpinEvent& WXUNUSED(event)) {
  height_was_changed = true;
  if (!width_was_changed) {
    this->width_input->SetValue(this->height_input->GetValue());
  }
}

void NewGameDialog::on_player_delete_clicked(wxCommandEvent& event) {
  for (size_t i = 0; i < this->player_rows.size(); i++) {
    if (this->player_rows.at(i).delete_btn == event.GetEventObject()) {
      if (this->player_rows.size() > size_t(this->players_number_input->GetMin())) {
        this->delete_player_row(i);
        this->players_number_input->SetValue(this->player_rows.size());
        for (size_t j = 0; j < this->player_rows.size(); j++) {
          this->realize_player_row(j);
        }
        this->update_new_player_row();
        this->update_layout();
      }
      return;
    }
  }
  event.Skip();
}

void NewGameDialog::on_players_number_input(wxSpinEvent& WXUNUSED(event)) {
  this->set_player_rows_count(this->players_number_input->GetValue());
  this->update_layout();
}

bool NewGameDialog::Persistence::Restore() {
  NewGameDialog* dialog = this->Get();
  int value;
  wxString str_value;
  if (this->RestoreValue("board_width", &value)) dialog->width_input->SetValue(value);
  if (this->RestoreValue("board_height", &value)) dialog->height_input->SetValue(value);
  if (this->RestoreValue("board_gen_type", &value) && 0 <= value && value < BOARD_GEN_MAX) {
    dialog->board_gen_input->Select(value);
  }
  if (this->RestoreValue("penguins_per_player", &value)) dialog->penguins_input->SetValue(value);
  if (this->RestoreValue("players_count", &value)) {
    dialog->players_number_input->SetValue(value);
    dialog->set_player_rows_count(dialog->players_number_input->GetValue());
    for (int i = 0; i < int(dialog->player_rows.size()); i++) {
      PlayerRowWidgets& row = dialog->player_rows.at(i);
      if (this->RestoreValue(wxString::Format("player_%d_name", i), &str_value)) {
        row.name_input->SetValue(str_value);
      }
      if (this->RestoreValue(wxString::Format("player_%d_type", i), &value) && 0 <= value && value < PLAYER_TYPE_MAX) {
        row.type_input->Select(value);
      }
    }
  }
  dialog->update_layout();
  return true;
}

void NewGameDialog::Persistence::Save() const {
  NewGameDialog* dialog = this->Get();
  this->SaveValue("board_width", dialog->width_input->GetValue());
  this->SaveValue("board_height", dialog->height_input->GetValue());
  this->SaveValue("board_gen_type", dialog->board_gen_input->GetSelection());
  this->SaveValue("penguins_per_player", dialog->penguins_input->GetValue());
  this->SaveValue("players_count", dialog->players_number_input->GetValue());
  for (int i = 0; i < int(dialog->player_rows.size()); i++) {
    PlayerRowWidgets& row = dialog->player_rows.at(i);
    this->SaveValue(wxString::Format("player_%d_name", i), row.name_input->GetValue());
    this->SaveValue(wxString::Format("player_%d_type", i), row.type_input->GetSelection());
  }
}

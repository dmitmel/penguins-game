#include "gui/game_end_dialog.hh"
#include "game.h"
#include <algorithm>
#include <wx/event.h>
#include <wx/font.h>
#include <wx/gdicmn.h>
#include <wx/grid.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/vector.h>

GameEndDialog::GameEndDialog(
  wxWindow* parent, wxWindowID id, const Game* game, const wxVector<wxString>& player_names
)
: wxDialog(parent, id, "Game summary", wxDefaultPosition, wxDefaultSize) {
  this->Bind(wxEVT_BUTTON, &GameEndDialog::on_ok, this, wxID_OK);

  wxVector<int> players_by_score(game->players_count);
  for (int i = 0; i < game->players_count; i++) {
    players_by_score[i] = i;
  }
  std::sort(
    players_by_score.begin(),
    players_by_score.end(),
    [&](const int& a, const int& b) -> bool {
      return game_get_player(game, a)->points > game_get_player(game, b)->points;
    }
  );

  int winners_count = 0;
  int max_score = 0;
  for (int i = 0; i < game->players_count; i++) {
    Player* player = game_get_player(game, players_by_score.at(i));
    if (i == 0) {
      max_score = player->points;
    }
    if (player->points >= max_score) {
      winners_count += 1;
    } else {
      break;
    }
  }

  wxString winners_str;
  if (winners_count > 1) {
    if (winners_count == game->players_count) {
      winners_str << "It is a tie between ";
    } else {
      winners_str << "The winners are ";
    }
    for (int i = 0; i < winners_count; i++) {
      winners_str << player_names.at(players_by_score.at(i));
      if (i < winners_count - 2) {
        winners_str << ", ";
      } else if (i < winners_count - 1) {
        winners_str << " and ";
      }
    }
    winners_str << "!";
  } else if (winners_count == 1) {
    winners_str << "The winner is " << player_names.at(players_by_score.at(0)) << "!";
  } else {
    winners_str << "There are no winners???";
  }

  auto content_vbox = new wxBoxSizer(wxVERTICAL);

  this->header_label = new wxStaticText(this, wxID_ANY, "The game has ended");
  this->header_label->SetFont(this->header_label->GetFont().MakeBold().Scale(1.4f));
  content_vbox->Add(this->header_label, wxSizerFlags().Centre().DoubleBorder(wxBOTTOM));

  this->winner_label = new wxStaticText(this, wxID_ANY, winners_str);
  this->winner_label->SetFont(this->winner_label->GetFont().MakeBold().Scale(1.2f));
  content_vbox->Add(this->winner_label, wxSizerFlags().Centre().DoubleBorder(wxBOTTOM));

  this->grid =
    new GameEndDialogGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
  this->grid->CreateGrid(game->players_count, 3, wxGrid::wxGridSelectNone);
  this->grid->EnableEditing(false);
  this->grid->SetCellHighlightPenWidth(0);
  this->grid->SetCellHighlightROPenWidth(0);
  this->grid->DisableDragGridSize();
  this->grid->DisableDragRowSize();
  this->grid->DisableDragColSize();

  this->grid->HideRowLabels();
  this->grid->SetColLabelValue(0, "Name");
  this->grid->SetColLabelValue(1, "Score");
  this->grid->SetColLabelValue(2, "Moves");
  this->grid->SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
  for (int j = 0; j < game->players_count; j++) {
    int i = players_by_score.at(j);
    Player* player = game_get_player(game, i);
    this->grid->SetCellValue(j, 0, player_names.at(i));
    this->grid->SetCellValue(j, 1, wxString::Format("%d", player->points));
    this->grid->SetCellValue(j, 2, wxString::Format("%d", player->moves_count));
  }
  this->grid->AutoSize();
  content_vbox->Add(this->grid, wxSizerFlags().Expand());

  auto outer_vbox = new wxBoxSizer(wxVERTICAL);
  outer_vbox->Add(content_vbox, wxSizerFlags().Expand().DoubleBorder());
  this->buttons_sizer = this->CreateStdDialogButtonSizer(wxOK);
  this->SetEscapeId(wxID_OK);
  outer_vbox->Add(new wxStaticLine(this), wxSizerFlags().Expand());
  wxSizerFlags buttons_sizer_flags = wxSizerFlags().Expand().DoubleBorder(wxALL);
#ifdef __WXGTK__
  buttons_sizer_flags.DoubleBorder(wxTOP | wxBOTTOM);
#endif
  outer_vbox->Add(this->buttons_sizer, buttons_sizer_flags);
  this->SetSizerAndFit(outer_vbox);
}

void GameEndDialog::on_ok(wxCommandEvent& WXUNUSED(event)) {
  this->EndModal(wxID_OK);
}

GameEndDialogGrid::GameEndDialogGrid(
  wxWindow* parent,
  wxWindowID id,
  const wxPoint& pos,
  const wxSize& size,
  long style,
  const wxString& name
)
: wxGrid(parent, id, pos, size, style, name) {
  this->Bind(wxEVT_SIZE, &GameEndDialogGrid::on_resize, this);
}

bool GameEndDialogGrid::TryBefore(wxEvent& event) {
  wxEventType type = event.GetEventType();
  // wxGrid by default handles keyboard events for Tab, Return and other such
  // keys for keyboard navigation within the table. In my case the table is
  // purely informative and most of the interaction with it is disabled, so
  // this is not particularly useful and even annoying because I want the
  // Return key, for example, to be handled by the dialog (to close it). I
  // guess this is not the prettiest way to disable the handling of these
  // events, but oh well.
  if (type == wxEVT_KEY_DOWN || type == wxEVT_KEY_UP || type == wxEVT_CHAR) {
    event.Skip(); // Will cause the native handler to be called
    return true;  // Will skip handlers defined on this window
  }
  return wxGrid::TryBefore(event);
}

void GameEndDialogGrid::equally_size_columns() {
  int ncols = this->GetNumberCols();
  int cols_total_size = 0;
  for (int i = 0; i < ncols; i++) {
    cols_total_size += this->GetColSize(i);
  }
  int cols_available_size = this->GetClientSize().x;
  int cols_free_size = cols_available_size - cols_total_size;
  int cols_ideal_size = cols_available_size / ncols;
  int growable_cols = 0;
  int growable_cols_total_size = 0;
  for (int i = 0; i < ncols; i++) {
    if (this->GetColSize(i) < cols_ideal_size) {
      growable_cols += 1;
      growable_cols_total_size += this->GetColSize(i);
    }
  }
  if (growable_cols == 0) {
    return;
  }
  this->BeginBatch();
  // Width distribution algorithm was taken from
  // <https://github.com/wxWidgets/wxWidgets/blob/v3.2.2.1/src/common/statbar.cpp#L200-L213>.
  growable_cols_total_size += cols_free_size;
  for (int i = ncols - 1; i >= 0; i--) {
    if (this->GetColSize(i) < cols_ideal_size) {
      int size = growable_cols_total_size / (i + 1);
      this->SetColSize(i, size);
      growable_cols_total_size -= size;
    }
  }
  this->EndBatch();
}

void GameEndDialogGrid::on_resize(wxSizeEvent& event) {
  event.Skip();
  // HACK: Don't really like this, we need a better way of getting the size
  // allocated by the sizer.
  this->CallAfter(&GameEndDialogGrid::equally_size_columns);
  // this->AutoSize();
  // this->equally_size_columns();
}

#include "gui/player_info_box.hh"
#include "game.h"
#include "gui/main.hh"
#include "gui/tileset.hh"
#include "movement.h"
#include <wx/bitmap.h>
#include <wx/dcclient.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/font.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/window.h>

PlayerInfoBox::PlayerInfoBox(wxWindow* parent, wxWindowID id, int max_points)
: SimpleStaticBox(parent, id) {
  this->root_hbox = new wxStaticBoxSizer(this, wxHORIZONTAL);

  this->penguin_window = new PlayerPenguinWindow(this, wxID_ANY);
  this->root_hbox->Add(this->penguin_window, wxSizerFlags().Border());

  this->info_vbox = new wxBoxSizer(wxVERTICAL);

  this->name_text = new wxStaticText(this, wxID_ANY, wxEmptyString);
  this->name_text->SetFont(this->name_text->GetFont().MakeBold());
  this->info_vbox->Add(this->name_text, wxSizerFlags().Border(wxRIGHT));

  wxString score_test_str;
  if (max_points < 0) max_points = -max_points;
  do {
    score_test_str << '0';
    max_points /= 10;
  } while (max_points != 0);
  score_test_str << " points";

  this->score_text = new wxStaticText(
    this, wxID_ANY, score_test_str, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE
  );
  this->info_vbox->Add(this->score_text, wxSizerFlags().Border(wxRIGHT));

  this->root_hbox->Add(this->info_vbox, wxSizerFlags().Border(wxALL & ~wxLEFT));
}

void PlayerInfoBox::update_data(Game* game, int idx, const wxString& name) {
  Player* player = game_get_player(game, idx);

  this->is_current = idx == game->current_player_index;
  this->is_blocked =
    game->phase == GAME_PHASE_MOVEMENT && !any_valid_player_move_exists(game, idx);

  this->name_text->SetLabel(name);

  wxString score_str;
  score_str << player->points << " point";
  if (player->points != 1) score_str << "s";
  this->score_text->SetLabel(score_str);

  auto& tileset = wxGetApp().tileset;
  this->penguin_sprite = tileset.penguin_sprites[idx % WXSIZEOF(tileset.penguin_sprites)];

  this->Refresh();
}

void PlayerInfoBox::paint_penguin_window(wxDC& dc) {
  auto& tileset = wxGetApp().tileset;
  const wxRect rect(wxPoint(0, 0), dc.GetSize());
  dc.DrawBitmap(tileset.ice_tiles[0], rect.GetPosition());
  if (this->penguin_sprite.IsOk()) {
    dc.DrawBitmap(this->penguin_sprite, rect.GetPosition());
  }
  if (this->is_blocked) {
    dc.SetPen(wxPen(*wxRED, 3));
    wxRect cross_rect = rect;
    cross_rect.Deflate(rect.GetSize() / 10);
    dc.DrawLine(cross_rect.GetTopLeft(), cross_rect.GetBottomRight());
    dc.DrawLine(cross_rect.GetTopRight(), cross_rect.GetBottomLeft());
  }
  if (this->is_current) {
    dc.DrawBitmap(tileset.current_penguin_overlay, rect.GetPosition());
  }
}

PlayerPenguinWindow::PlayerPenguinWindow(PlayerInfoBox* parent, wxWindowID id)
: wxWindow(parent, id), info_box(parent) {
  this->Bind(wxEVT_PAINT, &PlayerPenguinWindow::on_paint, this);
}

wxSize PlayerPenguinWindow::DoGetBestSize() const {
  const wxBitmap& bitmap = this->info_box->get_penguin_sprite();
  return this->FromPhys(bitmap.IsOk() ? bitmap.GetSize() : wxSize(0, 0));
}

void PlayerPenguinWindow::on_paint(wxPaintEvent& WXUNUSED(event)) {
  wxPaintDC dc(this);
  this->info_box->paint_penguin_window(dc);
}

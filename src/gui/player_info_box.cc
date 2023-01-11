#include "gui/player_info_box.hh"
#include "gui/game.hh"
#include "gui/tileset.hh"
#include <cstddef>
#include <wx/bitmap.h>
#include <wx/dcclient.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/pen.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/window.h>

PlayerInfoBox::PlayerInfoBox(
  wxWindow* parent,
  wxWindowID id,
  int player_id,
  wxString player_name,
  CanvasPanel* canvas_panel,
  const TilesetHelper& tileset
)
: SimpleStaticBox(parent, id)
, player_id(player_id)
, player_name(player_name)
, canvas_panel(canvas_panel)
, tileset(tileset) {
  this->root_hbox = new wxStaticBoxSizer(this, wxHORIZONTAL);

  this->penguin_window = new PlayerPenguinWindow(this, wxID_ANY);
  this->root_hbox->Add(this->penguin_window, wxSizerFlags().Border());

  this->info_vbox = new wxBoxSizer(wxVERTICAL);

  this->name_text = new wxStaticText(this, wxID_ANY, player_name);
  this->name_text->SetFont(this->name_text->GetFont().MakeBold());
  this->info_vbox->Add(this->name_text, wxSizerFlags().Border(wxRIGHT));

  this->score_text = new wxStaticText(this, wxID_ANY, "123 points");
  this->info_vbox->Add(this->score_text, wxSizerFlags().Border(wxRIGHT));

  this->root_hbox->Add(this->info_vbox, wxSizerFlags().Border(wxALL & ~wxLEFT));
}

void PlayerInfoBox::set_score(int points) {
  this->player_score = points;
  wxString str;
  str << points << " point";
  if (points != 1) str << "s";
  this->score_text->SetLabel(str);
}

const wxBitmap& PlayerInfoBox::get_penguin_sprite() {
  return this->canvas_panel->get_player_penguin_sprite(this->player_id);
}

void PlayerInfoBox::paint_penguin_window(wxDC& dc) {
  const wxRect rect(wxPoint(0, 0), dc.GetSize());
  dc.DrawBitmap(tileset.ice_tiles[0], rect.GetPosition());
  auto sprite = this->get_penguin_sprite();
  dc.DrawBitmap(sprite, rect.GetPosition());
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

// clang-format off
wxBEGIN_EVENT_TABLE(PlayerPenguinWindow, wxWindow)
  EVT_PAINT(PlayerPenguinWindow::on_paint)
wxEND_EVENT_TABLE();
// clang-format on

PlayerPenguinWindow::PlayerPenguinWindow(PlayerInfoBox* parent, wxWindowID id)
: wxWindow(parent, id), info_box(parent) {}

wxSize PlayerPenguinWindow::DoGetBestSize() const {
  return this->FromPhys(this->info_box->get_penguin_sprite().GetSize());
}

void PlayerPenguinWindow::on_paint(wxPaintEvent& WXUNUSED(event)) {
  wxPaintDC dc(this);
  this->info_box->paint_penguin_window(dc);
}

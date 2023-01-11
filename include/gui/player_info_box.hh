#pragma once

#include "gui/simple_static_box.hh"
#include "gui/tileset.hh"
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/window.h>

class PlayerInfoBox;
#include "gui/game.hh"

class PlayerPenguinWindow;

class PlayerInfoBox : public SimpleStaticBox {
public:
  PlayerInfoBox(
    wxWindow* parent,
    wxWindowID id,
    int player_id,
    wxString player_name,
    CanvasPanel* canvas_panel,
    const TilesetHelper& tileset
  );

  const int player_id;
  const wxString player_name;
  bool is_blocked = false;
  bool is_current = false;
  int player_score = 0;

  void set_score(int points);

  const wxBitmap& get_penguin_sprite();
  void paint_penguin_window(wxDC& dc);

protected:
  CanvasPanel* canvas_panel;
  const TilesetHelper& tileset;

  wxStaticBoxSizer* root_hbox;
  wxBoxSizer* info_vbox;
  PlayerPenguinWindow* penguin_window;
  wxStaticText* name_text;
  wxStaticText* score_text;
};

class PlayerPenguinWindow : public wxWindow {
public:
  PlayerPenguinWindow(PlayerInfoBox* parent, wxWindowID id);

protected:
  virtual wxSize DoGetBestSize() const override;

  void on_paint(wxPaintEvent& event);

  PlayerInfoBox* info_box;

  wxDECLARE_EVENT_TABLE();
};

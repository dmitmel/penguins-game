#pragma once

#include "game.h"
#include "gui/simple_static_box.hh"
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>
#include <wx/window.h>

class PlayerPenguinWindow;

class PlayerInfoBox : public SimpleStaticBox {
public:
  PlayerInfoBox(wxWindow* parent, wxWindowID id, int max_points);

  void update_data(Game* game, int idx, const wxString& name);

  const wxBitmap& get_penguin_sprite() const {
    return this->penguin_sprite;
  }

  void paint_penguin_window(wxDC& dc);

protected:
  wxStaticBoxSizer* root_hbox;
  wxBoxSizer* info_vbox;
  PlayerPenguinWindow* penguin_window;
  wxStaticText* name_text;
  wxStaticText* score_text;

  bool is_blocked = false;
  bool is_current = false;
  wxBitmap penguin_sprite;
};

class PlayerPenguinWindow : public wxWindow {
public:
  PlayerPenguinWindow(PlayerInfoBox* parent, wxWindowID id);

  virtual bool AcceptsFocus() const override {
    return false;
  }

protected:
  virtual wxSize DoGetBestSize() const override;

  void on_paint(wxPaintEvent& event);

  PlayerInfoBox* info_box;
};

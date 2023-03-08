#pragma once

#include "gui/tileset.hh"
#include <wx/app.h>
#include <wx/icon.h>

class GameFrame;

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

  wxIcon app_icon;
  TilesetHelper tileset;
  GameFrame* game_frame;
};

wxDECLARE_APP(PenguinsApp);

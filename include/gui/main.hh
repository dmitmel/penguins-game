#pragma once

#include "gui/game.hh"
#include "gui/tileset.hh"
#include <wx/app.h>

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

  GuiGameState game_state;
  TilesetHelper tileset{ CanvasPanel::TILE_SIZE / TilesetHelper::TILE_SIZE };

protected:
  GameFrame* game_frame;
};

wxDECLARE_APP(PenguinsApp);

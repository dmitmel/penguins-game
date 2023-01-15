#pragma once

#include "gui/game.hh"
#include "gui/tileset.hh"
#include <wx/app.h>

class PenguinsApp : public wxApp {
public:
  PenguinsApp() {}

  virtual bool OnInit() override;

protected:
  GameFrame* game_frame;
  GuiGameState game_state;
  TilesetHelper tileset{ CanvasPanel::CELL_SIZE / TilesetHelper::TILE_SIZE };
};

wxDECLARE_APP(PenguinsApp);

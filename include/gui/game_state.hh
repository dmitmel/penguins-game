#pragma once

#include "bot.h"
#include "game.h"
#include <memory>
#include <wx/defs.h>
#include <wx/string.h>

enum BoardGenType {
  BOARD_GEN_RANDOM,
  BOARD_GEN_ISLAND,
  BOARD_GEN_MAX,
};

enum PlayerType {
  PLAYER_NORMAL,
  PLAYER_BOT,
  PLAYER_TYPE_MAX,
};

class GuiGameState {
public:
  GuiGameState() {}

  bool game_ended = false;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::unique_ptr<BotParameters> bot_params{ nullptr };
  std::unique_ptr<wxString[]> player_names{ nullptr };
  std::unique_ptr<PlayerType[]> player_types{ nullptr };
};

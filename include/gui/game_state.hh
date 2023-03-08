#pragma once

#include "bot.h"
#include "game.h"
#include <memory>
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/vector.h>

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

struct GuiGameState {
  bool game_ended = false;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::shared_ptr<BotParameters> bot_params{ nullptr };
  wxVector<wxString> player_names;
  wxVector<PlayerType> player_types;
  size_t displayed_log_entries = 0;
};

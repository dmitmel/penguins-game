#pragma once

#include "bot.h"
#include "game.h"
#include "gui/better_random.hh"
#include <memory>
#include <wx/defs.h>
#include <wx/thread.h>
#include <wx/version.h>

class BotTurnController;

// The implementation of this class is somewhat based on
// <https://github.com/wxWidgets/wxWidgets/blob/v3.2.2.1/src/unix/threadpsx.cpp#L576-L716>.
class BotThreadShared {
public:
  BotThreadShared() {}

  void notify_exit();
  void wait_for_exit();

  bool exited = false;

protected:
  wxMutex mutex{};
  wxCondition condvar{ this->mutex };

  wxDECLARE_NO_COPY_CLASS(BotThreadShared);
};

class BotThread : public wxThread {
public:
  BotThread(BotTurnController* controller);
  virtual ~BotThread();
  void cancel();

  std::shared_ptr<BotThreadShared> shared{ new BotThreadShared() };

protected:
  virtual void OnExit() override;

#if !wxCHECK_VERSION(3, 1, 6)
  // Stub for when this method is not available.
  bool SetName(const wxString& WXUNUSED(name)) {
    return false;
  }
#endif

  BotTurnController* controller;
  std::unique_ptr<Game, decltype(&game_free)> game{ nullptr, game_free };
  std::shared_ptr<BotParameters> bot_params{ nullptr };
  std::unique_ptr<BotState, decltype(&bot_state_free)> bot_state{ nullptr, bot_state_free };
  BetterRng rng;
  volatile bool* cancelled_ptr = nullptr;
};

class BotPlacementThread : public BotThread {
public:
  BotPlacementThread(BotTurnController* controller) : BotThread(controller) {}

protected:
  virtual ExitCode Entry() override;
};

class BotMovementThread : public BotThread {
public:
  BotMovementThread(BotTurnController* controller) : BotThread(controller) {}

protected:
  virtual ExitCode Entry() override;
};

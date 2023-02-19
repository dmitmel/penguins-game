#include "gui/bot_thread.hh"
#include "gui/game.hh"
#include "gui/game_state.hh"
#include "utils.h"
#include <wx/thread.h>

BotThread::BotThread(GameFrame* frame)
: wxThread(wxTHREAD_DETACHED), frame(frame), bot_params(frame->state.bot_params) {
  this->game.reset(game_clone(frame->state.game.get()));
  this->bot_state.reset(bot_state_new(this->bot_params.get(), this->game.get()));
  this->cancelled_ptr = &this->bot_state->cancelled;
}

BotThread::~BotThread() {
  this->cancelled_ptr = nullptr;
}

void BotThread::cancel() {
  *this->cancelled_ptr = true;
}

void BotThread::OnExit() {
  // The thread is unregistered in the OnExit method instead of the destructor
  // so as to not accidentally use a half-destroyed object. And also because
  // joinable threads don't destruct themselves automatically.
  this->frame->on_bot_thread_exited(this);
}

wxThread::ExitCode BotPlacementThread::Entry() {
  this->SetName("bot-placement");
  Coords target;
  bool ok = bot_make_placement(this->bot_state.get(), &target);
  bool cancelled = this->bot_state->cancelled;
  auto frame = this->frame;
  frame->CallAfter([=]() -> void {
    if (!cancelled && ok) frame->place_penguin(target);
    frame->on_bot_thread_done_work(cancelled);
  });
  return 0;
}

wxThread::ExitCode BotMovementThread::Entry() {
  this->SetName("bot-movement");
  Coords penguin, target;
  bool ok = bot_make_move(this->bot_state.get(), &penguin, &target);
  bool cancelled = this->bot_state->cancelled;
  auto frame = this->frame;
  frame->CallAfter([=]() -> void {
    if (!cancelled && ok) frame->move_penguin(penguin, target);
    frame->on_bot_thread_done_work(cancelled);
  });
  return 0;
}

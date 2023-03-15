#include "gui/controllers.hh"
#include "board.h"
#include "game.h"
#include "gui/bot_thread.hh"
#include "gui/canvas.hh"
#include "gui/game.hh"
#include "gui/game_state.hh"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <memory>
#include <wx/button.h>
#include <wx/debug.h>
#include <wx/defs.h>
#include <wx/string.h>
#include <wx/utils.h>

GameController::GameController(GamePanel* panel)
: panel(panel), canvas(panel->canvas), game(panel->game.get()), bot_params(panel->bot_params) {}

void GameController::update_game_state_and_indirectly_delete_this() {
  this->panel->update_game_state();
}

void GameController::on_activated() {
  this->configure_bot_turn_ui();
  this->configure_log_viewer_ui();
  this->update_status_bar();
  this->panel->update_player_info_boxes();
  this->canvas->Refresh();
}

void GameController::configure_bot_turn_ui() {
  this->panel->stop_bot_progress();
}

void GameController::configure_log_viewer_ui() {
  this->panel->show_current_turn_btn->Disable();
}

void GameController::on_deactivated(GameController* WXUNUSED(next_controller)) {}
void GameController::paint_overlay(wxDC& WXUNUSED(dc)) {}
void GameController::on_mouse_down(wxMouseEvent& WXUNUSED(event)) {}
void GameController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {}
void GameController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {}

void GameController::update_status_bar() {
  this->panel->frame->clear_status_bar();
}

void GameController::on_mouse_enter_leave(wxMouseEvent& WXUNUSED(event)) {
  this->update_status_bar();
  this->canvas->Refresh();
}

void GameEndedController::on_activated() {
  this->GameController::on_activated();
  if (!this->panel->game_ended) {
    this->panel->game_ended = true;
    this->panel->CallAfter(&GamePanel::show_game_results);
  }
}

void BotTurnController::configure_bot_turn_ui() {
  this->panel->start_bot_progress();
}

BotThread* BotPlacementController::create_bot_thread() {
  return new BotPlacementThread(this);
}

BotThread* BotMovementController::create_bot_thread() {
  return new BotMovementThread(this);
}

void BotTurnController::on_activated() {
  this->GameController::on_activated();
  this->start_bot_thread();
}

void BotTurnController::on_deactivated(GameController* WXUNUSED(next_controller)) {
  this->stop_bot_thread();
}

BotTurnController::~BotTurnController() {
  this->stop_bot_thread();
}

void BotTurnController::on_bot_thread_done_work(bool cancelled) {
  this->executing_bot_turn = false;
  if (!cancelled) {
    this->update_game_state_and_indirectly_delete_this();
  } else {
    this->panel->stop_bot_progress();
  }
}

void BotTurnController::start_bot_thread() {
  wxASSERT(wxThread::IsMain());
  this->executing_bot_turn = true;
  BotThread* thread = this->create_bot_thread();
  {
    wxCriticalSectionLocker enter(this->bot_thread_cs);
    wxASSERT(this->bot_thread == nullptr);
    this->bot_thread = thread;
  }
  wxThreadError code WX_ATTRIBUTE_UNUSED = thread->Run();
  wxASSERT(code == wxTHREAD_NO_ERROR);
}

void BotTurnController::stop_bot_thread() {
  wxASSERT(wxThread::IsMain());
  // Well, this ain't the best way of doing thread synchronization, but it sure
  // does work.
  std::shared_ptr<BotThreadShared> shared = nullptr;
  {
    wxCriticalSectionLocker enter(this->bot_thread_cs);
    if (this->bot_thread) {
      this->bot_thread->cancel();
      shared = this->bot_thread->shared;
      this->bot_thread = nullptr;
    }
  }
  if (shared) {
    shared->wait_for_exit();
  }
  this->executing_bot_turn = false;
}

void BotTurnController::unregister_bot_thread(BotThread* thread) {
  wxASSERT(wxThread::GetCurrentId() == thread->GetId());
  wxCriticalSectionLocker enter(this->bot_thread_cs);
  // Another bot thread might have just been spun up after cancelling this one,
  // so check that we are unregistering the correct one to be sure.
  if (this->bot_thread == thread) {
    this->bot_thread = nullptr;
  }
}

void LogEntryViewerController::on_activated() {
  const GameLogEntry* entry = game_get_log_entry(game, this->entry_index);
  size_t adjusted_index = this->entry_index;
  if (entry->type == GAME_LOG_ENTRY_PHASE_CHANGE) {
    if (entry->data.phase_change.new_phase == GAME_PHASE_END) {
      adjusted_index = game->log_length;
    }
  } else if (entry->type == GAME_LOG_ENTRY_PLACEMENT) {
    adjusted_index += 1;
  }
  game_rewind_state_to_log_entry(game, adjusted_index);
  this->GameController::on_activated();
}

void LogEntryViewerController::configure_log_viewer_ui() {
  this->panel->show_current_turn_btn->Enable();
}

void GameController::update_tile_attributes() {
  set_all_tiles_attr(game, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
}

void PlayerPlacementController::update_tile_attributes() {
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  bool is_a_tile_selected =
    this->canvas->mouse_within_window && is_tile_in_bounds(game, curr_coords);
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      bool blocked = !validate_placement_simple(game, coords);
      set_tile_attr(game, coords, TILE_BLOCKED_FOR_CURSOR, blocked);
      set_tile_attr(game, coords, TILE_BLOCKED, blocked && is_a_tile_selected);
    }
  }
}

void PlayerMovementController::update_tile_attributes() {
  Coords selected_penguin = this->canvas->get_selected_penguin_coords();
  if (!is_tile_in_bounds(game, selected_penguin)) {
    int current_player_id = game_get_current_player(game)->id;
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        int tile = get_tile(game, coords);
        bool blocked = get_tile_player_id(tile) != current_player_id;
        set_tile_attr(game, coords, TILE_BLOCKED_FOR_CURSOR, blocked);
        set_tile_attr(game, coords, TILE_BLOCKED, false);
      }
    }
    return;
  }
  // A penguin is selected
  set_all_tiles_attr(game, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, true);
  PossibleSteps moves = calculate_penguin_possible_moves(game, selected_penguin);
  bool any_steps = false;
  for (int dir = 0; dir < DIRECTION_MAX; dir++) {
    Coords coords = selected_penguin;
    Coords d = DIRECTION_TO_COORDS[dir];
    any_steps = any_steps || moves.steps[dir] != 0;
    for (int steps = moves.steps[dir]; steps > 0; steps--) {
      coords.x += d.x, coords.y += d.y;
      set_tile_attr(game, coords, TILE_BLOCKED | TILE_BLOCKED_FOR_CURSOR, false);
    }
  }
  set_tile_attr(game, selected_penguin, TILE_BLOCKED, false);
  if (any_steps && !this->canvas->mouse_is_down) {
    set_tile_attr(game, selected_penguin, TILE_BLOCKED_FOR_CURSOR, false);
  }
}

void BotTurnController::update_tile_attributes() {
  set_all_tiles_attr(game, TILE_BLOCKED, false);
  set_all_tiles_attr(game, TILE_BLOCKED_FOR_CURSOR, true);
}

void PlayerTurnController::paint_overlay(wxDC& dc) {
  Coords current_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, current_coords) && this->canvas->mouse_within_window) {
    bool blocked = get_tile_attr(game, current_coords, TILE_BLOCKED_FOR_CURSOR);
    this->canvas->paint_selected_tile_outline(dc, current_coords, blocked);
  }
}

void PlayerMovementController::paint_overlay(wxDC& dc) {
  this->PlayerTurnController::paint_overlay(dc);

  if (!this->canvas->mouse_is_down) return;
  Coords penguin = this->canvas->get_selected_penguin_coords();
  Coords target = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (!(is_tile_in_bounds(game, penguin) && is_tile_in_bounds(game, target))) return;
  if (coords_same(penguin, target)) return;

  Coords move_fail = penguin;
  MovementError result = validate_movement(game, penguin, target, &move_fail);
  this->canvas->paint_move_arrow(dc, penguin, target, move_fail, result == MOVEMENT_VALID);
}

void LogEntryViewerController::paint_overlay(wxDC& dc) {
  const GameLogEntry* entry = game_get_log_entry(game, this->entry_index);
  if (entry->type == GAME_LOG_ENTRY_PLACEMENT) {
    this->canvas->paint_selected_tile_outline(dc, entry->data.placement.target);
  } else if (entry->type == GAME_LOG_ENTRY_MOVEMENT) {
    this->canvas->paint_selected_tile_outline(dc, entry->data.movement.target);
    this->canvas->paint_move_arrow(dc, entry->data.movement.penguin, entry->data.movement.target);
  }
}

static wxString describe_placement_result(PlacementError result) {
  switch (result) {
    case PLACEMENT_VALID: return "You can place your penguin here!";
    case PLACEMENT_OUT_OF_BOUNDS: return "A tile outside the board has been selected.";
    case PLACEMENT_EMPTY_TILE: return "Penguins can only be placed on ice tiles with one fish.";
    case PLACEMENT_ENEMY_PENGUIN: return "This tile is already occupied by a penguin.";
    case PLACEMENT_OWN_PENGUIN: return "This tile is already occupied by your own penguin.";
    case PLACEMENT_MULTIPLE_FISH: return "Penguins may only be placed on tiles with one fish.";
  }
  return "";
}

void PlayerPlacementController::update_status_bar() {
  GameFrame* frame = this->panel->frame;
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (!(this->canvas->mouse_within_window && is_tile_in_bounds(game, curr_coords))) {
    frame->clear_status_bar();
    return;
  }
  frame->SetStatusText(wxString::Format("(%d, %d)", curr_coords.x + 1, curr_coords.y + 1), 1);
  PlacementError result = validate_placement(game, curr_coords);
  frame->SetStatusText(describe_placement_result(result), 0);
}

static wxString describe_movement_result(MovementError result) {
  switch (result) {
    case MOVEMENT_VALID: return "This is a valid move!";
    case MOVEMENT_OUT_OF_BOUNDS: return "A tile outside the board has been selected.";
    case MOVEMENT_CURRENT_LOCATION: return "Drag the mouse to a desired tile to make a move.";
    case MOVEMENT_DIAGONAL:
      return "Penguins cannot move diagonally, only horizontally or vertically.";
    case MOVEMENT_NOT_A_PENGUIN: return "You must select a penguin to make a move.";
    case MOVEMENT_NOT_YOUR_PENGUIN: return "You must select your own penguin to make a move.";
    case MOVEMENT_ONTO_EMPTY_TILE: return "You can't move onto a water tile.";
    case MOVEMENT_ONTO_PENGUIN: return "You can't move onto another penguin.";
    case MOVEMENT_OVER_EMPTY_TILE: return "You can't jump over a water tile.";
    case MOVEMENT_OVER_PENGUIN: return "You can't jump over another penguin.";
    case MOVEMENT_PENGUIN_BLOCKED:
      return "There are no possible moves for this penguin, it is blocked on all sides.";
  }
  return "";
}

void PlayerMovementController::update_status_bar() {
  GameFrame* frame = this->panel->frame;
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (!(this->canvas->mouse_within_window && is_tile_in_bounds(game, curr_coords))) {
    frame->clear_status_bar();
    return;
  }
  if (this->canvas->mouse_is_down && is_tile_in_bounds(game, prev_coords)) {
    MovementError result = validate_movement(game, prev_coords, curr_coords, nullptr);
    if (result != MOVEMENT_VALID) {
      frame->SetStatusText(describe_movement_result(result), 0);
    } else if (this->canvas->mouse_is_down_real) {
      frame->SetStatusText("This is a valid move! Release the mouse to confirm it.", 0);
    } else {
      frame->SetStatusText("This is a valid move! Click on the tile to confirm it.", 0);
    }
    frame->SetStatusText(
      wxString::Format(
        "(%d, %d) -> (%d, %d)",
        prev_coords.x + 1,
        prev_coords.y + 1,
        curr_coords.x + 1,
        curr_coords.y + 1
      ),
      1
    );
  } else {
    MovementError result = MOVEMENT_VALID;
    if (is_tile_in_bounds(game, curr_coords) && is_penguin_tile(get_tile(game, curr_coords))) {
      result = validate_movement_start(game, curr_coords);
    }
    if (result != MOVEMENT_VALID) {
      frame->SetStatusText(describe_movement_result(result), 0);
    } else {
      frame->SetStatusText("Either drag or click on a penguin to make a move.", 0);
    }
    frame->SetStatusText(wxString::Format("(%d, %d)", curr_coords.x + 1, curr_coords.y + 1), 1);
  }
}

void BotTurnController::update_status_bar() {
  this->panel->frame->SetStatusText("The bot is thinking...", 0);
  this->panel->frame->SetStatusText("", 1);
}

void GameEndedController::update_status_bar() {
  this->panel->frame->SetStatusText("The game has ended!", 0);
  this->panel->frame->SetStatusText("", 1);
}

void LogEntryViewerController::update_status_bar() {
  this->panel->frame->SetStatusText(
    wxString::Format("Viewing a previous turn (entry #%zd)", this->entry_index + 1), 0
  );
  this->panel->frame->SetStatusText("", 1);
}

void PlayerMovementController::on_mouse_down(wxMouseEvent& WXUNUSED(event)) {
  this->update_status_bar();
  this->canvas->Refresh();
}

void PlayerPlacementController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->prev_mouse_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (coords_same(curr_coords, prev_coords)) return;
  this->update_status_bar();
  this->canvas->Refresh();
}

void PlayerMovementController::on_mouse_move(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->prev_mouse_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (coords_same(curr_coords, prev_coords)) return;
  Coords selected_penguin = this->canvas->get_selected_penguin_coords();
  if (is_tile_in_bounds(game, selected_penguin)) {
    set_tile_attr(game, selected_penguin, TILE_NEEDS_REDRAW, true);
  }
  this->update_status_bar();
  this->canvas->Refresh();
}

void PlayerPlacementController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, curr_coords) && coords_same(prev_coords, curr_coords)) {
    if (validate_placement(game, curr_coords) == PLACEMENT_VALID) {
      place_penguin(game, curr_coords);
      this->update_game_state_and_indirectly_delete_this();
      return;
    } else {
      wxBell();
    }
  }
  this->update_status_bar();
}

void PlayerMovementController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  Coords prev_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_drag_pos);
  Coords curr_coords = this->canvas->tile_coords_at_point(this->canvas->mouse_pos);
  if (is_tile_in_bounds(game, curr_coords) && is_tile_in_bounds(game, prev_coords)) {
    if (coords_same(prev_coords, curr_coords)) {
      if (validate_movement_start(game, curr_coords) == MOVEMENT_VALID) {
        // This is a hacky way of doing what I want: when the user has simply
        // clicked on a penguin, let them then move it without dragging the
        // mouse all the way.
        this->canvas->mouse_is_down = true;
      } else {
        wxBell();
      }
    } else {
      if (validate_movement(game, prev_coords, curr_coords, nullptr) == MOVEMENT_VALID) {
        move_penguin(game, prev_coords, curr_coords);
        this->update_game_state_and_indirectly_delete_this();
        return;
      } else {
        wxBell();
      }
    }
  }
  this->update_status_bar();
  this->canvas->Refresh();
}

void BotTurnController::on_mouse_up(wxMouseEvent& WXUNUSED(event)) {
  if (!this->executing_bot_turn) {
    this->on_activated();
  }
}

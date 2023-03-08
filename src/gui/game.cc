#include "gui/game.hh"
#include "board.h"
#include "bot.h"
#include "game.h"
#include "gui/canvas.hh"
#include "gui/controllers.hh"
#include "gui/game_end_dialog.hh"
#include "gui/game_state.hh"
#include "gui/new_game_dialog.hh"
#include "gui/player_info_box.hh"
#include "resources_appicon_256_png.h"
#include "utils.h"
#include <memory>
#include <wx/bitmap.h>
#include <wx/debug.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gauge.h>
#include <wx/gdicmn.h>
#include <wx/icon.h>
#include <wx/image.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/msgdlg.h>
#include <wx/mstream.h>
#include <wx/panel.h>
#include <wx/persist.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/vector.h>
#include <wx/window.h>

GameFrame::GameFrame(wxWindow* parent, wxWindowID id) : wxFrame(parent, id, "Penguins game") {
  this->Bind(wxEVT_DESTROY, &GameFrame::on_destroy, this);

  wxMemoryInputStream icon_stream(resources_appicon_256_png, resources_appicon_256_png_size);
  wxIcon app_icon;
  // The triple conversion necessary to load the icon here is... meh.
  app_icon.CopyFromBitmap(wxBitmap(wxImage(icon_stream, wxBITMAP_TYPE_PNG)));
  this->SetIcon(app_icon);

  this->root_panel = new wxPanel(this, wxID_ANY);

  auto menu_bar = new wxMenuBar();
  wxMenuItem* item;

  auto menu_file = new wxMenu();
  menu_bar->Append(menu_file, "&File");

  item = menu_file->Append(wxID_ANY, "&New game\tCtrl-N", "Start a new game");
  this->Bind(wxEVT_MENU, &GameFrame::on_new_game, this, item->GetId());

  item = menu_file->Append(wxID_ANY, "&Close game\tCtrl-W", "Close the current game");
  this->Bind(wxEVT_MENU, &GameFrame::on_close_game, this, item->GetId());

  menu_file->AppendSeparator();

  item = menu_file->Append(wxID_EXIT);
  this->Bind(wxEVT_MENU, &GameFrame::on_exit, this, item->GetId());

  auto menu_help = new wxMenu();
  menu_bar->Append(menu_help, "&Help");

  item = menu_help->Append(wxID_ABOUT);
  this->Bind(wxEVT_MENU, &GameFrame::on_about, this, item->GetId());

  this->SetMenuBar(menu_bar);

  this->CreateStatusBar(2);
  this->SetStatusText("Welcome to wxWidgets!");
  int status_bar_widths[2] = { -3, -1 };
  this->SetStatusWidths(2, status_bar_widths);

  this->progress_container = new wxWindow(this->GetStatusBar(), wxID_ANY);
  this->progress_container->Hide();

  this->progress_bar = new wxGauge(
    this->progress_container,
    wxID_ANY,
    100,
    wxDefaultPosition,
    wxSize(200, -1),
    wxGA_HORIZONTAL | wxGA_SMOOTH
  );

  auto progress_hbox = new wxBoxSizer(wxHORIZONTAL);
  progress_hbox->AddStretchSpacer(1);
  progress_hbox->Add(this->progress_bar, wxSizerFlags(1).Centre().HorzBorder());
  this->progress_container->SetSizer(progress_hbox);

  this->GetStatusBar()->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) -> void {
    event.Skip();
    wxRect rect;
    if (this->GetStatusBar()->GetFieldRect(1, rect)) {
      // This sets both the position and the size at the same time.
      this->progress_container->SetSize(rect);
    }
  });

  this->progress_timer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) -> void {
    this->progress_container->Show();
    this->progress_bar->Pulse();
  });

  this->scrolled_panel = new wxScrolledWindow(this->root_panel, wxID_ANY);
  auto scroll_vbox = new wxBoxSizer(wxVERTICAL);
  auto scroll_hbox = new wxBoxSizer(wxHORIZONTAL);
  this->scrolled_panel->SetScrollRate(10, 10);
  wxSize default_board_size(
    NewGameDialog::DEFAULT_BOARD_WIDTH, NewGameDialog::DEFAULT_BOARD_HEIGHT
  );
  this->empty_canvas = new wxPanel(
    this->scrolled_panel, wxID_ANY, wxDefaultPosition, default_board_size * CanvasPanel::TILE_SIZE
  );
  scroll_hbox->Add(this->empty_canvas, wxSizerFlags(1).Centre());
  this->canvas_sizer = scroll_hbox;
  scroll_vbox->Add(scroll_hbox, wxSizerFlags(1).Centre());
  this->scrolled_panel->SetSizer(scroll_vbox);

  auto panel_grid = new wxFlexGridSizer(/* rows */ 2, /* cols */ 2, 0, 0);
  panel_grid->AddGrowableCol(1);
  panel_grid->AddGrowableRow(1);

  this->show_current_turn_btn = new wxButton(this->root_panel, wxID_ANY, "Back to the game");
  this->show_current_turn_btn->Bind(wxEVT_BUTTON, &GameFrame::on_show_current_turn_clicked, this);
  this->show_current_turn_btn->Hide();
  this->show_current_turn_btn->Disable();
  panel_grid->Add(
    this->show_current_turn_btn, wxSizerFlags().Bottom().Expand().Border(wxALL & ~wxBOTTOM)
  );

  this->players_box = new wxBoxSizer(wxHORIZONTAL);
  panel_grid->Add(this->players_box, wxSizerFlags().Centre().Border(wxALL & ~wxBOTTOM));

  this->game_log = new wxListBox(this->root_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  this->game_log->Bind(wxEVT_LISTBOX, &GameFrame::on_game_log_select, this);
  this->game_log->Hide();
  panel_grid->Add(this->game_log, wxSizerFlags(1).Expand().Border());

  panel_grid->Add(this->scrolled_panel, wxSizerFlags(1).Expand().Border());

  auto panel_vbox = new wxBoxSizer(wxVERTICAL);
  panel_vbox->Add(panel_grid, wxSizerFlags(1).Expand().Border());
  this->root_panel->SetSizer(panel_vbox);

  auto root_vbox = new wxBoxSizer(wxVERTICAL);
  root_vbox->Add(this->root_panel, wxSizerFlags(1).Expand());
  this->SetSizer(root_vbox);
  this->update_layout();
}

void GameFrame::update_layout() {
  this->Layout();
  this->scrolled_panel->GetSizer()->SetSizeHints(this->scrolled_panel);
  this->GetSizer()->SetSizeHints(this);
}

GameFrame::~GameFrame() {
  this->SendDestroyEvent();
  this->set_controller(nullptr);
}

void GameFrame::on_destroy(wxWindowDestroyEvent& WXUNUSED(event)) {
  this->set_controller(nullptr);
}

void GameFrame::on_new_game(wxCommandEvent& WXUNUSED(event)) {
  this->start_new_game();
}

void GameFrame::on_close_game(wxCommandEvent& WXUNUSED(event)) {
  this->close_game();
  this->update_layout();
  this->Refresh();
  this->Centre();
}

void GameFrame::on_exit(wxCommandEvent& WXUNUSED(event)) {
  this->Close(true);
}

void GameFrame::on_about(wxCommandEvent& WXUNUSED(event)) {
  wxMessageBox(
    "This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION, this
  );
}

void GameFrame::start_new_game() {
  std::unique_ptr<NewGameDialog> dialog(new NewGameDialog(this, wxID_ANY));
  wxPersistentRegisterAndRestore(dialog.get());
  int result = dialog->ShowModal();
  if (result != wxID_OK) {
    return;
  }

  this->close_game();

  Game* game = game_new();
  this->state.game.reset(game);
  this->state.player_names = wxVector<wxString>(dialog->get_number_of_players());
  this->state.player_types = wxVector<PlayerType>(dialog->get_number_of_players());

  auto bot_params = new BotParameters;
  init_bot_parameters(bot_params);
  this->state.bot_params.reset(bot_params);

  game_begin_setup(game);
  game_set_penguins_per_player(game, dialog->get_penguins_per_player());
  game_set_players_count(game, dialog->get_number_of_players());
  for (int i = 0; i < game->players_count; i++) {
    game_set_player_name(game, i, dialog->get_player_name(i).c_str());
    this->state.player_names.at(i) = dialog->get_player_name(i);
    this->state.player_types.at(i) = dialog->get_player_type(i);
  }
  setup_board(game, dialog->get_board_width(), dialog->get_board_height());
  switch (dialog->get_board_gen_type()) {
    case BOARD_GEN_RANDOM: generate_board_random(game); break;
    case BOARD_GEN_ISLAND: generate_board_island(game); break;
    default: break;
  }
  game_end_setup(game);

  this->canvas = new CanvasPanel(this->scrolled_panel, wxID_ANY, this);
  bool replaced = this->canvas_sizer->Replace(this->empty_canvas, this->canvas);
  wxASSERT(replaced);
  this->empty_canvas->Hide();

  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes = wxVector<PlayerInfoBox*>(game->players_count);
  for (int i = 0; i < game->players_count; i++) {
    auto player_box = new PlayerInfoBox(this->root_panel, wxID_ANY);
    int border_dir = (i > 0 ? wxLEFT : 0) | (i + 1 < game->players_count ? wxRIGHT : 0);
    this->players_box->Add(player_box->GetContainingSizer(), wxSizerFlags().Border(border_dir));
    this->player_info_boxes.at(i) = player_box;
  }
  this->update_player_info_boxes();

  this->game_log->Show();
  this->show_current_turn_btn->Disable();
  this->show_current_turn_btn->Show();

  this->update_game_log();

  this->update_layout();
  this->Refresh();
  this->Centre();

  this->canvas->SetFocus();

  this->update_game_state();
}

void GameFrame::update_game_state() {
  Game* game = this->state.game.get();
  game_advance_state(game);
  this->update_game_log();
  this->set_controller(this->get_controller_for_current_turn());
  this->update_player_info_boxes();
  this->canvas->Refresh();
}

GameController* GameFrame::get_controller_for_current_turn() {
  Game* game = this->state.game.get();
  PlayerType type = PLAYER_TYPE_MAX;
  if (game_check_player_index(game, game->current_player_index)) {
    type = this->state.player_types.at(game->current_player_index);
  }
  if (game->phase == GAME_PHASE_END) {
    return new GameEndedController(this);
  } else if (game->phase == GAME_PHASE_PLACEMENT && type == PLAYER_NORMAL) {
    return new PlayerPlacementController(this);
  } else if (game->phase == GAME_PHASE_PLACEMENT && type == PLAYER_BOT) {
    return new BotPlacementController(this);
  } else if (game->phase == GAME_PHASE_MOVEMENT && type == PLAYER_NORMAL) {
    return new PlayerMovementController(this);
  } else if (game->phase == GAME_PHASE_MOVEMENT && type == PLAYER_BOT) {
    return new BotMovementController(this);
  } else {
    wxFAIL_MSG("No appropriate controller for the current game state");
    return nullptr;
  }
}

void GameFrame::set_controller(GameController* next_controller) {
  if (this->controller != nullptr) {
    this->controller->on_deactivated(next_controller);
    delete this->controller;
  }
  this->controller = next_controller;
  if (next_controller != nullptr) {
    next_controller->on_activated();
  }
}

void GameFrame::start_bot_progress() {
  // The progress_container is shown by the timer.
  if (!this->progress_timer.IsRunning()) {
    this->progress_timer.Start(50);
  }
#ifndef __WXOSX__
  if (!this->busy_cursor_changer) {
    this->busy_cursor_changer.reset(new wxBusyCursor());
  }
#endif
}

void GameFrame::stop_bot_progress() {
  this->progress_container->Hide();
  this->progress_timer.Stop();
#ifndef __WXOSX__
  this->busy_cursor_changer.reset(nullptr);
#endif
}

void GameFrame::on_game_log_select(wxCommandEvent& event) {
  if (auto list_entry = dynamic_cast<GameLogListBoxEntry*>(event.GetClientObject())) {
    this->set_controller(new LogEntryViewerController(this, list_entry->index));
    this->update_player_info_boxes();
    this->canvas->Refresh();
  }
}

void GameFrame::on_show_current_turn_clicked(wxCommandEvent& WXUNUSED(event)) {
  this->game_log->DeselectAll();
  this->canvas->CallAfter(&CanvasPanel::SetFocus);
  Game* game = this->state.game.get();
  game_rewind_state_to_log_entry(game, game->log_length);
  this->set_controller(this->get_controller_for_current_turn());
  this->update_player_info_boxes();
  this->canvas->Refresh();
}

void GameFrame::update_game_log() {
  size_t old_count = this->state.displayed_log_entries;
  size_t new_count = this->state.game->log_length;
  for (size_t i = old_count; i < new_count; i++) {
    this->state.displayed_log_entries += 1;
    wxString description = describe_game_log_entry(i);
    if (description.IsEmpty()) continue;
    int item_index = this->game_log->Append(description, new GameLogListBoxEntry(i));
    this->game_log->EnsureVisible(item_index);
  }
}

wxString GameFrame::describe_game_log_entry(size_t index) const {
  Game* game = this->state.game.get();
  const GameLogEntry* entry = game_get_log_entry(game, index);
  if (entry->type == GAME_LOG_ENTRY_PHASE_CHANGE) {
    auto entry_data = &entry->data.phase_change;
    if (entry_data->new_phase == GAME_PHASE_SETUP_DONE) {
      return "Start of the game";
    } else if (entry_data->new_phase == GAME_PHASE_PLACEMENT) {
      return "Placement phase";
    } else if (entry_data->new_phase == GAME_PHASE_MOVEMENT) {
      return "Movement phase";
    } else if (entry_data->new_phase == GAME_PHASE_END) {
      return "End of the game";
    }
  } else if (entry->type == GAME_LOG_ENTRY_PLACEMENT) {
    auto entry_data = &entry->data.placement;
    Coords target = entry_data->target;
    return wxString::Format("@ (%d, %d)", target.x, target.y);
  } else if (entry->type == GAME_LOG_ENTRY_MOVEMENT) {
    auto entry_data = &entry->data.movement;
    Coords penguin = entry_data->penguin, target = entry_data->target;
    return wxString::Format("(%d, %d) -> (%d, %d)", penguin.x, penguin.y, target.x, target.y);
  }
  return "";
}

void GameFrame::end_game() {
  std::unique_ptr<GameEndDialog> dialog(
    new GameEndDialog(this, wxID_ANY, this->state.game.get(), this->state.player_names)
  );
  dialog->ShowModal();
}

void GameFrame::close_game() {
  this->set_controller(nullptr);
  this->stop_bot_progress();
  if (this->canvas != nullptr) {
    bool replaced = this->canvas_sizer->Replace(this->canvas, this->empty_canvas);
    wxASSERT(replaced);
    this->canvas->Destroy();
    this->canvas = nullptr;
    this->empty_canvas->Show();
  }
  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes = wxVector<PlayerInfoBox*>();
  this->game_log->Clear();
  this->game_log->Hide();
  this->show_current_turn_btn->Hide();
  this->show_current_turn_btn->Disable();
  this->state = GuiGameState();
}

void GameFrame::update_player_info_boxes() {
  Game* game = this->state.game.get();
  for (int i = 0; i < game->players_count; i++) {
    this->player_info_boxes.at(i)->update_data(game, i, this->state.player_names.at(i));
  }
}

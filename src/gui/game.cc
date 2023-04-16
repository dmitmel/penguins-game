#include "gui/game.hh"
#include "board.h"
#include "bot.h"
#include "game.h"
#include "gui/better_random.hh"
#include "gui/canvas.hh"
#include "gui/controllers.hh"
#include "gui/game_end_dialog.hh"
#include "gui/game_state.hh"
#include "gui/main.hh"
#include "gui/new_game_dialog.hh"
#include "gui/player_info_box.hh"
#include "utils.h"
#include <memory>
#include <wx/aboutdlg.h>
#include <wx/debug.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/font.h>
#include <wx/gauge.h>
#include <wx/gdicmn.h>
#include <wx/iconbndl.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/panel.h>
#include <wx/persist.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/statusbr.h>
#include <wx/string.h>
#include <wx/timer.h>
#include <wx/utils.h>
#include <wx/vector.h>
#include <wx/version.h>
#include <wx/window.h>
// IWYU pragma: no_include <wx/bmpbndl.h>

GameFrame::GameFrame(wxWindow* parent, wxWindowID id) : wxFrame(parent, id, "Penguins game") {
  this->SetIcons(wxGetApp().app_icon);

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

  auto root_vbox = new wxBoxSizer(wxVERTICAL);
  this->current_panel = new GameStartPanel(this, wxID_ANY);
  root_vbox->Add(this->current_panel, wxSizerFlags(1).Expand());
  this->SetSizer(root_vbox);
  this->current_panel->update_layout();
}

GameFrame::~GameFrame() {
  this->set_panel(nullptr);
}

void GameFrame::set_panel(BaseGamePanel* panel) {
  wxASSERT(this->current_panel != nullptr);
  if (panel != nullptr) {
    wxASSERT(this->GetSizer()->Replace(this->current_panel, panel));
  }
  this->current_panel->Destroy();
  this->current_panel = panel;
}

void GameFrame::clear_status_bar() {
  this->SetStatusText("", 0);
  this->SetStatusText("", 1);
  this->progress_container->Hide();
}

void GameFrame::start_new_game() {
  std::unique_ptr<NewGameDialog> dialog(new NewGameDialog(this, wxID_ANY));
  wxPersistentRegisterAndRestore(dialog.get());
  int result = dialog->ShowModal();
  if (result != wxID_OK) {
    return;
  }

  this->clear_status_bar();
  auto panel = new GamePanel(this, wxID_ANY, dialog.get());
  this->set_panel(panel);
  this->current_panel->update_layout();
  this->Update();
  this->Centre();
  panel->update_game_state();
}

void GameFrame::close_game() {
  this->clear_status_bar();
  this->set_panel(new GameStartPanel(this, wxID_ANY));
  this->current_panel->update_layout();
  this->Update();
  this->Centre();
}

void GameFrame::on_new_game(wxCommandEvent& WXUNUSED(event)) {
  this->start_new_game();
}

void GameFrame::on_close_game(wxCommandEvent& WXUNUSED(event)) {
  this->close_game();
}

void GameFrame::on_exit(wxCommandEvent& WXUNUSED(event)) {
  this->Close(true);
}

void GameFrame::on_about(wxCommandEvent& WXUNUSED(event)) {
  wxAboutDialogInfo info;
  info.SetName("Penguins game");
  info.SetVersion(PENGUINS_VERSION_STRING);
#if !defined(__WXOSX__)
  info.SetIcon(wxGetApp().app_icon.GetIconOfExactSize(64));
#endif
  wxAboutBox(info, this);
}

BaseGamePanel::BaseGamePanel(GameFrame* parent, wxWindowID id)
: wxPanel(parent, id), frame(parent) {}

void BaseGamePanel::update_layout() {
  this->frame->Layout();
  this->frame->GetSizer()->SetSizeHints(this->frame);
}

GameStartPanel::GameStartPanel(GameFrame* parent, wxWindowID id) : BaseGamePanel(parent, id) {
  this->SetInitialSize(wxSize(600, 500));

  auto title_hbox = new wxBoxSizer(wxHORIZONTAL);

  auto icon_image = new wxStaticBitmap(this, wxID_ANY, wxGetApp().app_icon.GetIconOfExactSize(64));
  title_hbox->Add(icon_image, wxSizerFlags().Centre().Border());

  auto title_label = new wxStaticText(this, wxID_ANY, "Penguins v" PENGUINS_VERSION_STRING);
  title_label->SetFont(title_label->GetFont().MakeBold().Scale(2.0f));
  title_hbox->Add(title_label, wxSizerFlags().Centre().Border());

  auto buttons_vbox = new wxBoxSizer(wxVERTICAL);

  auto start_game_btn = new wxButton(this, wxID_ANY, "New Game");
  start_game_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->frame->start_new_game(); });
  buttons_vbox->Add(start_game_btn, wxSizerFlags().Expand().Border());

  auto exit_btn = new wxButton(this, wxID_ANY, "Exit");
  exit_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->frame->Close(true); });
  buttons_vbox->Add(exit_btn, wxSizerFlags().Expand().Border());

  auto content_vbox = new wxBoxSizer(wxVERTICAL);
  content_vbox->Add(title_hbox, wxSizerFlags().Centre().DoubleBorder(wxBOTTOM));
  content_vbox->Add(buttons_vbox, wxSizerFlags().Centre().Border(wxTOP));

  auto root_vbox = new wxBoxSizer(wxVERTICAL);
  auto root_hbox = new wxBoxSizer(wxHORIZONTAL);
  root_hbox->Add(content_vbox, wxSizerFlags(1).Centre().Border());
  root_vbox->Add(root_hbox, wxSizerFlags(1).Centre());
  this->SetSizer(root_vbox);

  start_game_btn->SetFocus();
}

GamePanel::GamePanel(GameFrame* parent, wxWindowID id, NewGameDialog* dialog)
: BaseGamePanel(parent, id) {
  Game* game = game_new();
  this->game.reset(game);
  this->player_names.reserve(dialog->get_number_of_players());
  this->player_types.reserve(dialog->get_number_of_players());

  auto bot_params = new BotParameters;
  init_bot_parameters(bot_params);
  this->bot_params.reset(bot_params);

  game_begin_setup(game);
  game_set_penguins_per_player(game, dialog->get_penguins_per_player());
  game_set_players_count(game, dialog->get_number_of_players());
  for (int i = 0; i < game->players_count; i++) {
    game_set_player_name(game, i, dialog->get_player_name(i).c_str());
    this->player_names.push_back(dialog->get_player_name(i));
    this->player_types.push_back(dialog->get_player_type(i));
  }
  setup_board(game, dialog->get_board_width(), dialog->get_board_height());
  BetterRng& rng = wxGetApp().rng;
  switch (dialog->get_board_gen_type()) {
    case BOARD_GEN_RANDOM: generate_board_random(game, &rng); break;
    case BOARD_GEN_ISLAND: generate_board_island(game, &rng); break;
    case BOARD_GEN_MAX: break;
  }
  game_end_setup(game);

  this->progress_timer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) -> void {
    this->frame->progress_container->Show();
    this->frame->progress_bar->Pulse();
  });

  this->scrolled_panel = new wxScrolledWindow(this, wxID_ANY);
  auto scroll_vbox = new wxBoxSizer(wxVERTICAL);
  auto scroll_hbox = new wxBoxSizer(wxHORIZONTAL);
  this->scrolled_panel->SetScrollRate(10, 10);
  this->canvas = new CanvasPanel(this->scrolled_panel, wxID_ANY, this);
  scroll_hbox->Add(this->canvas, wxSizerFlags(1).Centre());
  scroll_vbox->Add(scroll_hbox, wxSizerFlags(1).Centre());
  this->scrolled_panel->SetSizer(scroll_vbox);

  auto panel_grid = new wxFlexGridSizer(/* rows */ 2, /* cols */ 2, 0, 0);
  panel_grid->AddGrowableCol(1);
  panel_grid->AddGrowableRow(1);

  auto game_controls_vbox = new wxBoxSizer(wxVERTICAL);
  this->game_controls_box = game_controls_vbox;

  this->show_current_turn_btn = new wxButton();
  // Controls can be created in the disabled state in:
  // GTK+MSW since v3.1.3: <https://github.com/wxWidgets/wxWidgets/pull/1060>
  // OSX since v3.1.6: <https://github.com/wxWidgets/wxWidgets/pull/2509>
#if ((defined(__WXGTK__) || defined(__WXMSW__)) && wxCHECK_VERSION(3, 1, 3)) || \
  (defined(__WXOSX__) && wxCHECK_VERSION(3, 1, 6))
  this->show_current_turn_btn->Disable();
#endif
  this->show_current_turn_btn->Create(this, wxID_ANY, "Back to the game");
  this->show_current_turn_btn->Disable();
  this->show_current_turn_btn->Bind(wxEVT_BUTTON, &GamePanel::on_show_current_turn_clicked, this);
  game_controls_vbox->Add(this->show_current_turn_btn, wxSizerFlags().Expand());

  this->exit_game_btn = new wxButton(this, wxID_ANY, "Close the game");
  this->exit_game_btn->Bind(wxEVT_BUTTON, &GamePanel::on_exit_game_clicked, this);
  this->exit_game_btn->Hide();
  game_controls_vbox->Add(this->exit_game_btn, wxSizerFlags().Expand());

  panel_grid->Add(game_controls_vbox, wxSizerFlags().Bottom().Expand().Border(wxALL & ~wxBOTTOM));

  int max_points = 0;
  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      max_points += get_tile_fish(get_tile(game, coords));
    }
  }

  auto players_box = new wxBoxSizer(wxHORIZONTAL);
  this->player_info_boxes.reserve(game->players_count);
  for (int i = 0; i < game->players_count; i++) {
    auto player_box = new PlayerInfoBox(this, wxID_ANY, max_points);
    int border_dir = (i > 0 ? wxLEFT : 0) | (i + 1 < game->players_count ? wxRIGHT : 0);
    players_box->Add(player_box->GetContainingSizer(), wxSizerFlags().Border(border_dir));
    this->player_info_boxes.push_back(player_box);
  }
  panel_grid->Add(players_box, wxSizerFlags().Centre().Border(wxALL & ~wxBOTTOM));

  this->log_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
  this->log_list->Bind(wxEVT_LISTBOX, &GamePanel::on_game_log_select, this);
  panel_grid->Add(this->log_list, wxSizerFlags(1).Expand().Border());

  panel_grid->Add(this->scrolled_panel, wxSizerFlags(1).Expand().Border());

  auto panel_vbox = new wxBoxSizer(wxVERTICAL);
  panel_vbox->Add(panel_grid, wxSizerFlags(1).Expand().Border());
  this->SetSizer(panel_vbox);

  this->update_player_info_boxes();
  this->update_game_log();
  this->canvas->SetFocus();
}

GamePanel::~GamePanel() {
  this->SendDestroyEvent();
  this->set_controller(nullptr);
  this->stop_bot_progress();
}

void GamePanel::update_layout() {
  this->frame->Layout();
  this->scrolled_panel->GetSizer()->SetSizeHints(this->scrolled_panel);
  this->frame->GetSizer()->SetSizeHints(this->frame);
}

void GamePanel::update_game_state() {
  Game* game = this->game.get();
  game_advance_state(game);
  this->update_game_log();
  this->set_controller(this->get_controller_for_current_turn());
}

GameController* GamePanel::get_controller_for_current_turn() {
  Game* game = this->game.get();
  PlayerType type = PLAYER_TYPE_MAX;
  if (game_check_player_index(game, game->current_player_index)) {
    type = this->player_types.at(game->current_player_index);
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

void GamePanel::set_controller(GameController* next_controller) {
  if (this->controller != nullptr) {
    this->controller->on_deactivated(next_controller);
    delete this->controller;
  }
  this->controller = next_controller;
  if (next_controller != nullptr) {
    next_controller->on_activated();
  }
}

void GamePanel::start_bot_progress() {
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

void GamePanel::stop_bot_progress() {
  this->frame->progress_container->Hide();
  this->progress_timer.Stop();
#ifndef __WXOSX__
  this->busy_cursor_changer.reset(nullptr);
#endif
}

void GamePanel::on_game_log_select(wxCommandEvent& event) {
  if (auto list_entry = dynamic_cast<GameLogListBoxEntry*>(event.GetClientObject())) {
    this->set_controller(new LogEntryViewerController(this, list_entry->index));
  }
}

void GamePanel::on_show_current_turn_clicked(wxCommandEvent& WXUNUSED(event)) {
  this->log_list->SetSelection(-1); // works more reliably than DeselectAll
  this->canvas->CallAfter(&CanvasPanel::SetFocus);
  Game* game = this->game.get();
  game_rewind_state_to_log_entry(game, game->log_length);
  this->set_controller(this->get_controller_for_current_turn());
}

void GamePanel::on_exit_game_clicked(wxCommandEvent& WXUNUSED(event)) {
  this->frame->CallAfter(&GameFrame::close_game);
}

void GamePanel::update_game_log() {
  size_t old_count = this->displayed_log_entries;
  size_t new_count = this->game->log_length;
  for (size_t i = old_count; i < new_count; i++) {
    this->displayed_log_entries += 1;
    wxString description = this->describe_game_log_entry(i);
    if (description.IsEmpty()) continue;
    int item_index = this->log_list->Append(description, new GameLogListBoxEntry(i));
    this->log_list->EnsureVisible(item_index);
  }
}

wxString GamePanel::describe_game_log_entry(size_t index) const {
  Game* game = this->game.get();
  const GameLogEntry* entry = game_get_log_entry(game, index);
  if (entry->type == GAME_LOG_ENTRY_PHASE_CHANGE) {
    auto entry_data = &entry->data.phase_change;
    if (entry_data->new_phase == GAME_PHASE_SETUP_DONE) {
      if (entry_data->old_phase == GAME_PHASE_SETUP) {
        return "Start of the game";
      }
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
    return wxString::Format("@ (%d, %d)", target.x + 1, target.y + 1);
  } else if (entry->type == GAME_LOG_ENTRY_MOVEMENT) {
    auto entry_data = &entry->data.movement;
    Coords penguin = entry_data->penguin, target = entry_data->target;
    return wxString::Format(
      "(%d, %d) -> (%d, %d)", penguin.x + 1, penguin.y + 1, target.x + 1, target.y + 1
    );
  }
  return "";
}

void GamePanel::show_game_results() {
  std::unique_ptr<GameEndDialog> dialog(
    new GameEndDialog(this, wxID_ANY, this->game.get(), this->player_names)
  );
  dialog->ShowModal();
}

void GamePanel::update_player_info_boxes() {
  Game* game = this->game.get();
  for (int i = 0; i < game->players_count; i++) {
    this->player_info_boxes.at(i)->update_data(game, i, this->player_names.at(i));
  }
}

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
#include <memory>
#include <wx/bitmap.h>
#include <wx/debug.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/gauge.h>
#include <wx/gdicmn.h>
#include <wx/icon.h>
#include <wx/image.h>
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

  auto panel_vbox = new wxBoxSizer(wxVERTICAL);

  this->players_box = new wxBoxSizer(wxHORIZONTAL);
  panel_vbox->Add(this->players_box, wxSizerFlags().Centre().Border(wxALL & ~wxDOWN));

  auto canvas_hbox = new wxBoxSizer(wxHORIZONTAL);
  canvas_hbox->Add(this->scrolled_panel, wxSizerFlags(1).Expand().Border());
  panel_vbox->Add(canvas_hbox, wxSizerFlags(1).Expand().Border());

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

  this->state.bot_params.reset(new BotParameters);
  init_bot_parameters(this->state.bot_params.get());

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
    this->players_box->Add(
      player_box->GetContainingSizer(), wxSizerFlags().Border(wxALL & ~wxDOWN)
    );
    this->player_info_boxes.at(i) = player_box;
  }
  this->update_player_info_boxes();

  this->update_layout();
  this->Refresh();
  this->Centre();

  this->update_game_state();
}

void GameFrame::update_game_state() {
  this->set_controller(nullptr);
  Game* game = this->state.game.get();
  game_advance_state(game);

  GameController* controller = nullptr;
  PlayerType type = PLAYER_TYPE_MAX;
  if (game_check_player_index(game, game->current_player_index)) {
    type = this->state.player_types.at(game->current_player_index);
  }
  if (game->phase == GAME_PHASE_END) {
    controller = new GameEndedController(this);
  } else if (game->phase == GAME_PHASE_PLACEMENT && type == PLAYER_NORMAL) {
    controller = new PlayerPlacementController(this);
  } else if (game->phase == GAME_PHASE_PLACEMENT && type == PLAYER_BOT) {
    controller = new BotPlacementController(this);
  } else if (game->phase == GAME_PHASE_MOVEMENT && type == PLAYER_NORMAL) {
    controller = new PlayerMovementController(this);
  } else if (game->phase == GAME_PHASE_MOVEMENT && type == PLAYER_BOT) {
    controller = new BotMovementController(this);
  }

  wxASSERT_MSG(controller != nullptr, "No appropriate controller for the current game state");
  this->set_controller(controller);
  controller->on_activated();

  // Repaint
  this->update_player_info_boxes();
  this->canvas->Refresh();

  if (game->phase == GAME_PHASE_END) {
    if (!this->state.game_ended) {
      this->state.game_ended = true;
      this->CallAfter(&GameFrame::end_game);
    }
  }
}

void GameFrame::set_controller(GameController* controller) {
  delete this->controller;
  this->controller = controller;
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

void GameFrame::end_game() {
  std::unique_ptr<GameEndDialog> dialog(
    new GameEndDialog(this, wxID_ANY, this->state.game.get(), this->state.player_names)
  );
  dialog->ShowModal();
}

void GameFrame::close_game() {
  this->set_controller(nullptr);
  this->stop_bot_progress();
  if (this->canvas) {
    bool replaced = this->canvas_sizer->Replace(this->canvas, this->empty_canvas);
    wxASSERT(replaced);
    this->canvas->Destroy();
    this->canvas = nullptr;
    this->empty_canvas->Show();
  }
  this->players_box->Clear(/* delete_windows */ true);
  this->player_info_boxes = wxVector<PlayerInfoBox*>();
  this->state = GuiGameState();
}

void GameFrame::update_player_info_boxes() {
  Game* game = this->state.game.get();
  for (int i = 0; i < game->players_count; i++) {
    this->player_info_boxes.at(i)->update_data(game, i, this->state.player_names.at(i));
    this->player_info_boxes.at(i)->Refresh();
  }
}

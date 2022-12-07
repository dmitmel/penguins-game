#pragma once

#include <memory>
#include <wx/app.h>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/window.h>

extern "C" {
#include "board.h"
}

class PenguinsApp;

class BoardPanel : public wxPanel {
public:
  BoardPanel(wxWindow* parent);

  static const wxCoord CELL_SIZE = 32;
  static const int CELL_FONT_SIZE = 16;
  static const int FISH_CIRCLE_RADIUS = 4;

protected:
  void OnPaint(wxPaintEvent& event);
  virtual wxSize DoGetBestClientSize() const override;

private:
  wxDECLARE_EVENT_TABLE();
};

class GameFrame : public wxFrame {
public:
  GameFrame(wxWindow* parent, wxWindowID id);

  void start_new_game();

protected:
  void on_exit(wxCommandEvent& event);
  void on_about(wxCommandEvent& event);
  void on_new_game(wxCommandEvent& event);

  BoardPanel* board_panel;

private:
  wxDECLARE_EVENT_TABLE();
};

class PenguinsApp : public wxApp {
public:
  PenguinsApp();
  virtual bool OnInit() override;

  std::shared_ptr<Board> board;

private:
  GameFrame* game_frame;
};

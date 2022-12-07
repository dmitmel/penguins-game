#pragma once

#include <wx/app.h>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/frame.h>
#include <wx/panel.h>

extern "C" {
#include "board.h"
}

class PenguinsApp;

class BoardPanel : public wxPanel {
public:
  BoardPanel(wxFrame* parent);

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
  GameFrame();

protected:
  void OnHello(wxCommandEvent& event);
  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
  void OnNewGame(wxCommandEvent& event);

  BoardPanel* board_panel;

private:
  wxDECLARE_EVENT_TABLE();
};

class PenguinsApp : public wxApp {
public:
  PenguinsApp();
  virtual ~PenguinsApp();
  virtual bool OnInit() override;

  bool board_inited = false;
  Board board;

private:
  GameFrame* game_frame;
};

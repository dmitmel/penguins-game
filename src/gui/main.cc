#include "gui/main.hh"
#include <wx/log.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

enum { ID_Hello = 1 };

wxIMPLEMENT_APP(PenguinsApp);

bool PenguinsApp::OnInit() {
  PenguinsFrame* frame = new PenguinsFrame();
  frame->Show(true);
  return true;
}

PenguinsFrame::PenguinsFrame() : wxFrame(nullptr, wxID_ANY, "Hello World") {
  wxMenu* menuFile = new wxMenu();
  menuFile->Append(
    ID_Hello, "&Hello...\tCtrl-H", "Help string shown in status bar for this menu item");
  menuFile->AppendSeparator();
  menuFile->Append(wxID_EXIT);

  wxMenu* menuHelp = new wxMenu();
  menuHelp->Append(wxID_ABOUT);

  wxMenuBar* menuBar = new wxMenuBar();
  menuBar->Append(menuFile, "&File");
  menuBar->Append(menuHelp, "&Help");

  this->SetMenuBar(menuBar);

  this->CreateStatusBar();
  this->SetStatusText("Welcome to wxWidgets!");

  this->Bind(wxEVT_MENU, &PenguinsFrame::OnHello, this, ID_Hello);
  this->Bind(wxEVT_MENU, &PenguinsFrame::OnAbout, this, wxID_ABOUT);
  this->Bind(wxEVT_MENU, &PenguinsFrame::OnExit, this, wxID_EXIT);
}

void PenguinsFrame::OnExit(wxCommandEvent& event) {
  this->Close(true);
}

void PenguinsFrame::OnAbout(wxCommandEvent& event) {
  wxMessageBox(
    "This is a wxWidgets Hello World example", "About Hello World", wxOK | wxICON_INFORMATION);
}

void PenguinsFrame::OnHello(wxCommandEvent& event) {
  wxLogMessage("Hello world from wxWidgets!");
}

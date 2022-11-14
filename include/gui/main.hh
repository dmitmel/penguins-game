#pragma once

#include <wx/app.h>
#include <wx/frame.h>

class PenguinsApp : public wxApp {
public:
  virtual bool OnInit();
};

class PenguinsFrame : public wxFrame {
public:
  PenguinsFrame();

private:
  void OnHello(wxCommandEvent& event);
  void OnExit(wxCommandEvent& event);
  void OnAbout(wxCommandEvent& event);
};

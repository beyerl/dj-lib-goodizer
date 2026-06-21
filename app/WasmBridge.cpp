// JS <-> Qt test bridge for the WebAssembly build.
//
// A Qt Widgets app renders to a single <canvas>, so there are no DOM elements
// for Playwright to click. Instead this bridge mirrors the live UI state onto
// `window.__djState` and consumes UI commands from `window.__djCmd`, giving the
// end-to-end tests a deterministic, canvas-independent contract:
//
//   window.__djCmd = "loadDemo"          // seed the demo library
//   window.__djCmd = "selectTrack:<row>" // select a source row
//   window.__djCmd = "setTab:<index>"    // 0=Library 1=Dashboard 2=Audit
//   window.__djCmd = "setProfile:<index>"// switch the active target profile
//   window.__djCmd = "about"             // surface the About text
//
// A QTimer pump (Qt event loop, GUI thread — no extra threading) drains one
// command per tick and republishes the state. Compiled only under Emscripten.
#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include <QCoreApplication>
#include <QStringList>
#include <QTimer>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "MainWindow.h"

namespace djapp {
namespace {

bool g_aboutShown = false;

std::string jsonEscape(const QString& s) {
  std::string out;
  const QByteArray utf8 = s.toUtf8();
  for (char c : utf8) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\u%04x",
                        static_cast<unsigned int>(c) & 0xff);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

std::string str(const QString& s) { return "\"" + jsonEscape(s) + "\""; }

const char* tabName(int index) {
  switch (index) {
    case 0: return "Library";
    case 1: return "Dashboard";
    case 2: return "Audit Log";
    default: return "";
  }
}

void publishState(MainWindow* w) {
  std::string js = "[";
  const QStringList names = w->profileNames();
  for (int i = 0; i < names.size(); ++i) {
    if (i) js += ",";
    js += str(names[i]);
  }
  js += "]";

  std::string json = "{";
  json += "\"ready\":true";
  json += ",\"version\":" + str(QCoreApplication::applicationVersion());
  json += ",\"windowTitle\":" + str(w->windowTitle());
  json += ",\"rowCount\":" + std::to_string(w->libraryRowCount());
  json += ",\"currentTab\":" + std::to_string(w->currentTab());
  json += ",\"tabName\":" + str(QString::fromUtf8(tabName(w->currentTab())));
  json += ",\"statusText\":" + str(w->statusText());
  json += ",\"activeProfile\":" + str(w->activeProfileName());
  json += ",\"profileTargetLufs\":" +
          std::to_string(w->activeProfileTargetLufs());
  json += ",\"selectedFile\":" + str(w->selectedFileName());
  json += ",\"detailText\":" + str(w->detailPlainText());
  json += ",\"dashboardText\":" + str(w->dashboardPlainText());
  json += ",\"auditText\":" + str(w->auditPlainText());
  json += ",\"aboutText\":" + str(g_aboutShown ? w->aboutText() : QString());
  json += ",\"profiles\":" + js;
  json += "}";

  const std::string script = "window.__djState = " + json + ";";
  emscripten_run_script(script.c_str());
}

void dispatch(MainWindow* w, const std::string& cmd) {
  if (cmd.empty()) return;
  const auto colon = cmd.find(':');
  const std::string verb = cmd.substr(0, colon);
  const std::string arg = colon == std::string::npos ? "" : cmd.substr(colon + 1);
  const int n = arg.empty() ? -1 : std::atoi(arg.c_str());

  if (verb == "loadDemo") {
    w->loadDemoLibrary();
  } else if (verb == "selectTrack") {
    w->selectRow(n);
  } else if (verb == "setTab") {
    w->setCurrentTab(n);
  } else if (verb == "setProfile") {
    w->setProfileByIndex(n);
  } else if (verb == "about") {
    g_aboutShown = true;
  }
}

}  // namespace

void installWasmBridge(MainWindow* w) {
  auto* pump = new QTimer(w);
  QObject::connect(pump, &QTimer::timeout, w, [w] {
    char* pending = emscripten_run_script_string(
        "(function(){var c=window.__djCmd||'';window.__djCmd='';return "
        "String(c);})()");
    const std::string cmd = pending ? pending : "";
    if (!cmd.empty()) dispatch(w, cmd);
    publishState(w);
  });
  pump->start(50);
  publishState(w);  // initial state, so tests can await `ready`
}

}  // namespace djapp

#endif  // __EMSCRIPTEN__

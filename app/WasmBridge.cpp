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
  } else if (verb == "loadDiskSample") {
    w->loadDiskSample();
  } else if (verb == "importDisk") {
    // The folder picker already wrote the files into MEMFS; collect (and clear)
    // their paths and import them through the real decode pipeline.
    char* joined = emscripten_run_script_string(
        "(function(){var a=window.__djDiskPaths||[];window.__djDiskPaths=[];"
        "return a.join('\\n');})()");
    QStringList paths;
    if (joined && *joined) {
      paths = QString::fromUtf8(joined).split('\n', Qt::SkipEmptyParts);
    }
    if (!paths.isEmpty()) w->importDiskFiles(paths);
  }
}

}  // namespace

// Page-side glue: a MEMFS writer (the Emscripten FS object is in module scope
// here, so we hand the page a closure that writes into the same virtual
// filesystem the C++ decoder reads) plus a folder-picker button. Picked audio
// files are written to /disk/<relpath>, then "importDisk" is queued for the
// pump to import them. Passed as a raw string literal (not EM_ASM) so the
// commas in the JS don't get parsed as macro arguments.
static void installFolderPicker() {
  emscripten_run_script(R"JS(
    (function () {
      window.__djWriteFile = function (path, bytes) {
        try {
          var slash = path.lastIndexOf('/');
          if (slash > 0) FS.mkdirTree(path.substring(0, slash));
          FS.writeFile(path, bytes);
          return true;
        } catch (e) { console.error('djWriteFile failed', path, e); return false; }
      };
      if (document.getElementById('dj-folder-btn')) return;
      var exts = ['wav','wave','flac','mp3','aiff','aif','ogg','aac','m4a','alac'];
      var btn = document.createElement('button');
      btn.id = 'dj-folder-btn';
      btn.textContent = '📁 Load Folder from Disk';
      btn.setAttribute('style',
        'position:fixed;top:8px;left:8px;z-index:99999;padding:6px 10px;' +
        'font:14px sans-serif;cursor:pointer');
      var input = document.createElement('input');
      input.type = 'file';
      input.multiple = true;
      input.setAttribute('webkitdirectory', '');
      input.webkitdirectory = true;
      input.style.display = 'none';
      btn.onclick = function () { input.value = ''; input.click(); };
      input.onchange = async function () {
        var files = Array.prototype.slice.call(input.files || []);
        var paths = [];
        for (var i = 0; i < files.length; i++) {
          var f = files[i];
          var name = f.webkitRelativePath || f.name;
          var ext = (name.split('.').pop() || '').toLowerCase();
          if (exts.indexOf(ext) < 0) continue;
          try {
            var bytes = new Uint8Array(await f.arrayBuffer());
            var p = '/disk/' + name;
            if (window.__djWriteFile(p, bytes)) paths.push(p);
          } catch (e) { console.error('read failed', name, e); }
        }
        window.__djDiskPaths = paths;
        window.__djCmd = 'importDisk';
      };
      document.body.appendChild(btn);
      document.body.appendChild(input);
    })();
  )JS");
}

void installWasmBridge(MainWindow* w) {
  installFolderPicker();

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

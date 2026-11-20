/**
 * @file main.cpp
 * @brief aember-monitor: a full-screen view of aember-stats' samples.
 *
 * The platform: linuxfb when the device has /dev/fb0, otherwise offscreen
 * (headless QEMU, CI): it still connects and renders, so `--screenshot`
 * works anywhere. QT_QPA_PLATFORM set by the environment always wins.
 */
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QTextStream>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

#include <aember-monitor/monitor-window.h>

namespace {

constexpr std::string_view kUsage =
    "usage: aember-monitor [--socket PATH] [--screenshot FILE.png [--quit]]\n";

/// <prefix>/share/aember-monitor/fonts next to <prefix>/bin/aember-monitor.
std::string FontDir() {
  std::error_code ec;
  const auto exe = std::filesystem::read_symlink("/proc/self/exe", ec);
  if (ec) return "/usr/share/aember-monitor/fonts";
  return (exe.parent_path().parent_path() / "share" / "aember-monitor" /
          "fonts")
      .string();
}

}  // namespace

int main(int argc, char** argv) {
  QString socket = QStringLiteral("/run/aember-stats/stats.sock");
  QString screenshot;
  bool quit = false;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg = argv[i];
    if (arg == "--socket" && i + 1 < argc) {
      socket = QString::fromLocal8Bit(argv[++i]);
    } else if (arg == "--screenshot" && i + 1 < argc) {
      screenshot = QString::fromLocal8Bit(argv[++i]);
    } else if (arg == "--quit") {
      quit = true;
    } else if (arg == "--help" || arg == "-h") {
      QTextStream{stdout} << kUsage.data();
      return 0;
    } else {
      QTextStream{stderr} << kUsage.data();
      return 2;
    }
  }

  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
    const bool fb = QFileInfo::exists(QStringLiteral("/dev/fb0"));
    qputenv("QT_QPA_PLATFORM", fb ? "linuxfb" : "offscreen");
    if (!fb)
      QTextStream{stdout} << "aember-monitor: no /dev/fb0, drawing offscreen\n";
  }
  if (qEnvironmentVariableIsEmpty("QT_QPA_FONTDIR")) {
    qputenv("QT_QPA_FONTDIR", QByteArray::fromStdString(FontDir()));
  }

  QGuiApplication app{argc, argv};
  aember::monitor::MonitorWindow window{socket, screenshot, quit};
  window.resize(800, 480);  // offscreen; linuxfb is full screen anyway
  window.showFullScreen();
  return app.exec();
}

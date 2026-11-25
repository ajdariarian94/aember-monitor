// One line from aember-stats's socket, anything at all: ParseSample, the
// formatters, and a frame drawn from what was parsed (two sizes: a small
// panel and a wide screen).
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <cstdint>
#include <cstdlib>

#include <aember-monitor/dashboard.h>
#include <aember-monitor/model.h>

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  qputenv("QT_QPA_FONTDIR", AEMBER_MONITOR_FONT_DIR);
  static QGuiApplication app{*argc, *argv};
  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
  using namespace aember::monitor;
  const auto sample = ParseSample(QByteArray{
      reinterpret_cast<const char*>(data), static_cast<qsizetype>(size)});
  if (!sample) return 0;
  FormatUptime(sample->uptime_s);
  FormatKb(sample->mem_total_kb);
  for (const auto& i : sample->interfaces) FormatRate(i.rx_bps);
  for (const QSize s : {QSize{160, 120}, QSize{480, 200}}) {
    QImage image{s, QImage::Format_RGB32};
    QPainter p{&image};
    Paint(p, s, sample, QStringLiteral("waiting"));
  }
  return 0;
}

/**
 * @file dashboard.cpp
 * @brief The monitor's single screen (see dashboard.h).
 */
#include <QFont>
#include <QFontMetrics>
#include <QRectF>
#include <algorithm>

#include <aember-monitor/dashboard.h>

namespace aember::monitor {

namespace {

const QColor kBackground{0x12, 0x14, 0x18};
const QColor kPanel{0x1d, 0x21, 0x28};
const QColor kText{0xe8, 0xea, 0xed};
const QColor kDim{0x8b, 0x93, 0x9e};
const QColor kAccent{0xff, 0x8a, 0x3d};  // aember orange
const QColor kGood{0x4c, 0xc3, 0x8a};
const QColor kWarn{0xf2, 0xc1, 0x4e};
const QColor kBad{0xe5, 0x53, 0x4b};
const QColor kTrack{0x2c, 0x31, 0x3a};

QColor LevelColor(double percent) {
  if (percent >= 90) return kBad;
  if (percent >= 70) return kWarn;
  return kGood;
}

QColor StateColor(const QString& state) {
  if (state == QLatin1String("running")) return kGood;
  if (state == QLatin1String("failed")) return kBad;
  if (state == QLatin1String("starting") || state == QLatin1String("stopping"))
    return kWarn;
  return kDim;
}

/// Font sizes are fractions of the frame height, or of 0.6 x its width on
/// screens narrower than 5:3 (so text fits the three columns).
QFont Font(const QSize& size, double fraction, bool bold = false) {
  QFont f{QStringLiteral("DejaVu Sans")};
  const double unit = std::min<double>(size.height(), size.width() * 0.6);
  f.setPixelSize(std::max(9, static_cast<int>(unit * fraction)));
  f.setBold(bold);
  return f;
}

void Bar(QPainter& p, const QRectF& r, double percent, const QColor& color) {
  p.setPen(Qt::NoPen);
  p.setBrush(kTrack);
  p.drawRoundedRect(r, r.height() / 2, r.height() / 2);
  QRectF fill = r;
  fill.setWidth(r.width() * std::clamp(percent, 0.0, 100.0) / 100.0);
  if (fill.width() > 0) {
    p.setBrush(color);
    p.drawRoundedRect(fill, r.height() / 2, r.height() / 2);
  }
}

/// A panel with a title; returns the content rectangle.
QRectF Panel(QPainter& p, const QRectF& r, const QString& title, QSize size) {
  p.setPen(Qt::NoPen);
  p.setBrush(kPanel);
  const double radius = size.height() * 0.015;
  p.drawRoundedRect(r, radius, radius);
  const double pad = size.height() * 0.025;
  p.setFont(Font(size, 0.032, true));
  p.setPen(kDim);
  const QRectF title_rect{r.left() + pad,
                          r.top() + pad * 0.6,
                          r.width() - 2 * pad,
                          size.height() * 0.05};
  p.drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title.toUpper());
  return {r.left() + pad,
          title_rect.bottom() + pad * 0.3,
          r.width() - 2 * pad,
          r.bottom() - title_rect.bottom() - pad * 1.3};
}

void Text(QPainter& p, const QRectF& r, const QString& text, const QFont& font,
          const QColor& color, int flags = Qt::AlignLeft | Qt::AlignVCenter) {
  p.setFont(font);
  p.setPen(color);
  p.drawText(r,
             flags,
             QFontMetrics{font}.elidedText(
                 text, Qt::ElideRight, static_cast<int>(r.width())));
}

void Header(QPainter& p, QSize size, const Sample& s, const QRectF& r) {
  const QFont title = Font(size, 0.07, true);
  Text(p, r, QStringLiteral("aember"), title, kAccent);
  const double title_w =
      QFontMetrics{title}.horizontalAdvance(QStringLiteral("aember"));
  const double gap = r.height() * 0.4;
  QRectF right{
      r.left() + title_w + gap, r.top(), r.width() - title_w - gap, r.height()};
  QString text = QStringLiteral("up %1").arg(FormatUptime(s.uptime_s));
  if (s.has_aember && !s.ip.isEmpty())
    text = s.ip + QStringLiteral("  ·  ") + text;
  const QFont font = Font(size, 0.04);
  const double text_w = std::min<double>(
      right.width(), QFontMetrics{font}.horizontalAdvance(text) + 2);
  Text(p, right, text, font, kText, Qt::AlignRight | Qt::AlignVCenter);
  if (s.has_aember) {  // online / offline dot left of the text
    const double dot = r.height() * 0.22;
    const double x = right.right() - text_w - dot * 1.8;
    if (x > right.left()) {
      p.setPen(Qt::NoPen);
      p.setBrush(s.online ? kGood : kBad);
      p.drawEllipse(QRectF{x, r.center().y() - dot / 2, dot, dot});
    }
  }
}

void Usage(QPainter& p, QSize size, const QRectF& c, const QString& big,
           const QString& small, double percent) {
  const double h = c.height();
  Text(p,
       {c.left(), c.top(), c.width(), h * 0.45},
       big,
       Font(size, 0.075, true),
       kText);
  Bar(p,
      {c.left(), c.top() + h * 0.52, c.width(), h * 0.12},
      percent,
      LevelColor(percent));
  Text(p,
       {c.left(), c.top() + h * 0.68, c.width(), h * 0.3},
       small,
       Font(size, 0.032),
       kDim);
}

}  // namespace

void Paint(QPainter& p, QSize size, const std::optional<Sample>& sample,
           const QString& status) {
  p.setRenderHint(QPainter::Antialiasing);
  p.fillRect(QRect{QPoint{0, 0}, size}, kBackground);

  const double w = size.width();
  const double h = size.height();
  const double m = h * 0.03;  // margin and gap

  if (!sample) {
    Text(p,
         {0, 0, w, h * 0.45},
         QStringLiteral("aember"),
         Font(size, 0.12, true),
         kAccent,
         Qt::AlignHCenter | Qt::AlignBottom);
    Text(p,
         {0, h * 0.5, w, h * 0.1},
         status,
         Font(size, 0.045),
         kDim,
         Qt::AlignHCenter | Qt::AlignTop);
    return;
  }
  const Sample& s = *sample;

  Header(p, size, s, {m, m, w - 2 * m, h * 0.1});

  // Row 1: CPU, memory, network.
  const double row1_top = m + h * 0.1 + m;
  const double row1_h = h * 0.3;
  const double tile_w = (w - 4 * m) / 3;
  {
    const auto c =
        Panel(p, {m, row1_top, tile_w, row1_h}, QStringLiteral("CPU"), size);
    Usage(p,
          size,
          c,
          QStringLiteral("%1%").arg(s.cpu_percent, 0, 'f', 0),
          QStringLiteral("%1 cores · load %2")
              .arg(s.cores)
              .arg(s.load[0], 0, 'f', 2),
          s.cpu_percent);
  }
  {
    const auto c = Panel(p,
                         {2 * m + tile_w, row1_top, tile_w, row1_h},
                         QStringLiteral("Memory"),
                         size);
    Usage(p,
          size,
          c,
          QStringLiteral("%1%").arg(s.mem_used_percent, 0, 'f', 0),
          QStringLiteral("%1 free of %2")
              .arg(FormatKb(s.mem_available_kb), FormatKb(s.mem_total_kb)),
          s.mem_used_percent);
  }
  {
    const auto c = Panel(p,
                         {3 * m + 2 * tile_w, row1_top, tile_w, row1_h},
                         QStringLiteral("Network"),
                         size);
    QVector<Interface> shown;
    for (const auto& n : s.interfaces) {
      // Container plumbing (veth pairs, docker) says little about the device.
      if (n.name.startsWith(QLatin1String("veth")) ||
          n.name.startsWith(QLatin1String("docker"))) {
        continue;
      }
      shown.push_back(n);
    }
    const double line = c.height() / 4;
    double y = c.top();
    if (shown.isEmpty()) {
      Text(p,
           {c.left(), y, c.width(), line},
           QStringLiteral("no interfaces"),
           Font(size, 0.035),
           kDim);
    }
    for (int i = 0; i < shown.size() && i < 2; ++i) {
      const auto& n = shown[i];
      Text(p,
           {c.left(), y, c.width(), line},
           n.name,
           Font(size, 0.036, true),
           kText);
      Text(p,
           {c.left(), y + line, c.width(), line},
           QStringLiteral("↓ %1   ↑ %2")
               .arg(FormatRate(n.rx_bps), FormatRate(n.tx_bps)),
           Font(size, 0.03),
           kDim);
      y += 2 * line;
    }
  }

  // Row 2: services and containers (left), system (right).
  const double row2_top = row1_top + row1_h + m;
  const double row2_h = h - row2_top - m;
  const double left_w = (w - 3 * m) * 0.6;
  {
    const auto c =
        Panel(p,
              {m, row2_top, left_w, row2_h},
              s.has_aember ? QStringLiteral("Services")
                           : QStringLiteral("Services · no heartbeat"),
              size);
    const double line = h * 0.055;
    const int per_column = std::max(1, static_cast<int>(c.height() / line));
    const int columns = s.services.size() > per_column ? 2 : 1;
    const double col_w = c.width() / columns;
    for (int i = 0; i < s.services.size() && i < per_column * columns; ++i) {
      const auto& svc = s.services[i];
      const double x = c.left() + (i / per_column) * col_w;
      const double y = c.top() + (i % per_column) * line;
      const double dot = line * 0.32;
      p.setPen(Qt::NoPen);
      p.setBrush(StateColor(svc.state));
      p.drawEllipse(QRectF{x, y + (line - dot) / 2, dot, dot});
      const QString label = svc.type == QLatin1String("container")
                                ? QStringLiteral("%1 ▣").arg(svc.name)
                                : svc.name;
      Text(p,
           {x + dot * 1.8, y, col_w - dot * 2.5, line},
           label,
           Font(size, 0.034),
           kText);
    }
  }
  {
    const auto c = Panel(p,
                         {2 * m + left_w, row2_top, w - 3 * m - left_w, row2_h},
                         QStringLiteral("System"),
                         size);
    const double line = h * 0.055;
    double y = c.top();
    auto row = [&](const QString& key, const QString& value, QColor color) {
      if (y + line > c.bottom() + 1) return;
      Text(p,
           {c.left(), y, c.width() * 0.45, line},
           key,
           Font(size, 0.032),
           kDim);
      Text(p,
           {c.left() + c.width() * 0.45, y, c.width() * 0.55, line},
           value,
           Font(size, 0.034, true),
           color,
           Qt::AlignRight | Qt::AlignVCenter);
      y += line;
    };
    if (s.has_aember) {
      row(QStringLiteral("PID 1"),
          QStringLiteral("%1 · %2 thr")
              .arg(FormatKb(s.pid1_rss_kb))
              .arg(s.pid1_threads),
          kText);
      if (s.online)
        row(QStringLiteral("RTT"),
            QStringLiteral("%1 ms").arg(s.rtt_ms),
            kText);
    }
    for (const auto& d : s.disks) {
      const double used =
          d.total_kb > 0
              ? 100.0 * static_cast<double>(d.total_kb - d.available_kb) /
                    static_cast<double>(d.total_kb)
              : 0.0;
      row(QStringLiteral("Disk"),
          QStringLiteral("%1 free").arg(FormatKb(d.available_kb)),
          LevelColor(used));
    }
    for (const auto& t : s.thermal) {
      row(t.type.isEmpty() ? QStringLiteral("Temp") : t.type,
          QStringLiteral("%1 °C").arg(t.celsius, 0, 'f', 1),
          t.celsius >= 80 ? kBad : (t.celsius >= 65 ? kWarn : kText));
    }
  }
}

}  // namespace aember::monitor

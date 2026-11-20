/**
 * @file model.cpp
 * @brief aember-stats JSON → Sample (see model.h).
 */
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <aember-monitor/model.h>

namespace aember::monitor {

namespace {

constexpr int kVersion = 1;

QString Unit(double value, const char* const* units, int count, double step) {
  int i = 0;
  while (value >= step && i < count - 1) {
    value /= step;
    ++i;
  }
  return i == 0 ? QStringLiteral("%1 %2").arg(qRound64(value)).arg(units[i])
                : QStringLiteral("%1 %2").arg(value, 0, 'f', 1).arg(units[i]);
}

}  // namespace

std::optional<Sample> ParseSample(const QByteArray& line) {
  QJsonParseError error{};
  const auto doc = QJsonDocument::fromJson(line, &error);
  if (error.error != QJsonParseError::NoError || !doc.isObject()) {
    return std::nullopt;
  }
  const auto o = doc.object();
  Sample s;
  s.version = o.value("version").toInt();
  if (s.version != kVersion) return std::nullopt;

  s.uptime_s = o.value("uptime_s").toInteger();
  const auto cpu = o.value("cpu").toObject();
  s.cpu_percent = cpu.value("percent").toDouble();
  s.cores = cpu.value("cores").toInt();

  const auto mem = o.value("memory").toObject();
  s.mem_total_kb = mem.value("total_kb").toInteger();
  s.mem_available_kb = mem.value("available_kb").toInteger();
  s.mem_used_percent = mem.value("used_percent").toDouble();

  const auto load = o.value("load").toArray();
  for (int i = 0; i < 3 && i < load.size(); ++i) s.load[i] = load[i].toDouble();

  for (const auto& v : o.value("net").toArray()) {
    const auto n = v.toObject();
    s.interfaces.push_back({n.value("iface").toString(),
                            n.value("rx_bps").toInteger(),
                            n.value("tx_bps").toInteger()});
  }
  for (const auto& v : o.value("disks").toArray()) {
    const auto d = v.toObject();
    s.disks.push_back({d.value("path").toString(),
                       d.value("total_kb").toInteger(),
                       d.value("available_kb").toInteger()});
  }
  for (const auto& v : o.value("thermal").toArray()) {
    const auto t = v.toObject();
    s.thermal.push_back(
        {t.value("type").toString(), t.value("celsius").toDouble()});
  }

  const auto aember = o.value("aember");
  if (aember.isObject()) {
    s.has_aember = true;
    const auto a = aember.toObject();
    const auto pid1 = a.value("pid1").toObject();
    s.pid1_rss_kb = pid1.value("vm_rss_kb").toInteger();
    s.pid1_threads = pid1.value("threads").toInt();
    const auto net = a.value("network").toObject();
    s.online = net.value("online").toBool();
    s.iface = net.value("interface").toString();
    s.ip = net.value("ip").toString();
    s.rtt_ms = net.value("rtt_ms").toInt();
    for (const auto& v : a.value("services").toArray()) {
      const auto svc = v.toObject();
      s.services.push_back({svc.value("name").toString(),
                            svc.value("state").toString(),
                            svc.value("type").toString()});
    }
  }
  return s;
}

QString FormatRate(qint64 bytes_per_second) {
  static constexpr const char* kUnits[] = {"B/s", "KB/s", "MB/s", "GB/s"};
  return Unit(static_cast<double>(bytes_per_second), kUnits, 4, 1024.0);
}

QString FormatKb(qint64 kb) {
  static constexpr const char* kUnits[] = {"KB", "MB", "GB", "TB"};
  return Unit(static_cast<double>(kb), kUnits, 4, 1024.0);
}

QString FormatUptime(qint64 seconds) {
  const qint64 d = seconds / 86400;
  const qint64 h = (seconds % 86400) / 3600;
  const qint64 m = (seconds % 3600) / 60;
  const qint64 s = seconds % 60;
  if (d > 0) return QStringLiteral("%1d %2h").arg(d).arg(h);
  if (h > 0) return QStringLiteral("%1h %2m").arg(h).arg(m, 2, 10, QChar('0'));
  return QStringLiteral("%1m %2s").arg(m).arg(s, 2, 10, QChar('0'));
}

}  // namespace aember::monitor

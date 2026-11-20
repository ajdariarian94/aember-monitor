/**
 * @file model.h
 * @brief One aember-stats sample (JSON line, version 1) as plain data.
 *
 * Parsing is lenient: a missing or mistyped field keeps its default, so a
 * newer aember-stats with more fields, or an aember without a heartbeat,
 * still displays what is there.
 */
#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>
#include <optional>

namespace aember::monitor {

struct Interface {
  QString name;
  qint64 rx_bps = 0;
  qint64 tx_bps = 0;
};

struct Disk {
  QString path;
  qint64 total_kb = 0;
  qint64 available_kb = 0;
};

struct Thermal {
  QString type;
  double celsius = 0;
};

/// A service or container from aember's heartbeat.
struct Service {
  QString name;
  QString state;  ///< "running", "starting", "stopped", "failed", ...
  QString type;   ///< "process" or "container"
};

struct Sample {
  int version = 0;
  qint64 uptime_s = 0;
  double cpu_percent = 0;
  int cores = 0;
  qint64 mem_total_kb = 0;
  qint64 mem_available_kb = 0;
  double mem_used_percent = 0;
  double load[3] = {0, 0, 0};
  QVector<Interface> interfaces;
  QVector<Disk> disks;
  QVector<Thermal> thermal;

  bool has_aember = false;  ///< the heartbeat was there
  qint64 pid1_rss_kb = 0;
  int pid1_threads = 0;
  bool online = false;
  QString iface;
  QString ip;
  int rtt_ms = 0;
  QVector<Service> services;
};

/// Parses one line; nullopt if it isn't a JSON object of a known version.
std::optional<Sample> ParseSample(const QByteArray& line);

/// "1.2 MB/s", "512 B/s".
QString FormatRate(qint64 bytes_per_second);
/// "3d 4h", "2h 05m", "4m 09s".
QString FormatUptime(qint64 seconds);
/// "1.5 GB" from kilobytes.
QString FormatKb(qint64 kb);

}  // namespace aember::monitor

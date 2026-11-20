/**
 * @file monitor-window.h
 * @brief The full-screen window: reads aember-stats, repaints per sample.
 */
#pragma once

#include <QLocalSocket>
#include <QRasterWindow>
#include <QString>
#include <QTimer>
#include <optional>

#include <aember-monitor/model.h>

namespace aember::monitor {

class MonitorWindow : public QRasterWindow {
 public:
  /// @p screenshot: after the first sample, also save the frame as PNG there
  /// (and quit if @p quit_after_screenshot), for tests and docs.
  MonitorWindow(QString socket_path, QString screenshot = {},
                bool quit_after_screenshot = false);

  /// Disconnects the socket's signals first: the socket outlives the timers
  /// during destruction and emits disconnected() from its destructor.
  ~MonitorWindow() override;
  MonitorWindow(const MonitorWindow&) = delete;
  MonitorWindow& operator=(const MonitorWindow&) = delete;

  /// Renders the current state into an image of @p size.
  [[nodiscard]] QImage Render(QSize size) const;

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  void Connect();
  void OnReadyRead();
  void OnDisconnected(const QString& why);

  QString socket_path_;
  QString screenshot_;
  bool quit_after_screenshot_;
  QLocalSocket socket_;
  QTimer reconnect_;
  QTimer stale_;
  std::optional<Sample> sample_;
  QString status_;
  bool logged_first_ = false;
};

}  // namespace aember::monitor

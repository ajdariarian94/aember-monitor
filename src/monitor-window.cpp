/**
 * @file monitor-window.cpp
 * @brief Socket client + repaint (see monitor-window.h).
 */
#include <QCoreApplication>
#include <QImage>
#include <QPainter>
#include <QTextStream>

#include <aember-monitor/dashboard.h>
#include <aember-monitor/monitor-window.h>

namespace aember::monitor {

namespace {

constexpr int kReconnectMs = 1000;
/// No sample for this long: show the waiting screen again.
constexpr int kStaleMs = 5000;

void Log(const QString& message) {
  QTextStream{stdout} << "aember-monitor: " << message << Qt::endl;
}

}  // namespace

MonitorWindow::MonitorWindow(QString socket_path, QString screenshot,
                             bool quit_after_screenshot)
    : socket_path_{std::move(socket_path)},
      screenshot_{std::move(screenshot)},
      quit_after_screenshot_{quit_after_screenshot},
      status_{QStringLiteral("waiting for aember-stats")} {
  reconnect_.setSingleShot(true);
  reconnect_.setInterval(kReconnectMs);
  connect(&reconnect_, &QTimer::timeout, this, [this] { Connect(); });

  stale_.setSingleShot(true);
  stale_.setInterval(kStaleMs);
  connect(&stale_, &QTimer::timeout, this, [this] {
    sample_.reset();
    status_ = QStringLiteral("no data from aember-stats");
    update();
  });

  connect(&socket_, &QLocalSocket::connected, this, [this] {
    Log(QStringLiteral("connected to %1").arg(socket_path_));
    logged_first_ = false;
    status_ = QStringLiteral("connected, waiting for data");
    update();
  });
  connect(&socket_, &QLocalSocket::readyRead, this, [this] { OnReadyRead(); });
  connect(&socket_, &QLocalSocket::disconnected, this, [this] {
    OnDisconnected(QStringLiteral("disconnected"));
  });
  connect(&socket_, &QLocalSocket::errorOccurred, this, [this] {
    OnDisconnected(socket_.errorString());
  });
  Connect();
}

MonitorWindow::~MonitorWindow() {
  // Found by UBSan: ~QLocalSocket emits disconnected(), whose handler used
  // reconnect_, already destroyed (members die in reverse order).
  socket_.disconnect(this);
  socket_.abort();
}

void MonitorWindow::Connect() {
  // Reset here, on the timer, never inside the socket's own signals: an
  // abort() from errorOccurred re-enters QLocalSocket while it is still in
  // connectToServer() and corrupted the heap.
  socket_.abort();
  socket_.connectToServer(socket_path_, QIODevice::ReadOnly);
}

void MonitorWindow::OnDisconnected(const QString& why) {
  if (reconnect_.isActive()) return;  // disconnected + error: one retry
  status_ = QStringLiteral("waiting for aember-stats (%1)").arg(why);
  update();
  reconnect_.start();
}

void MonitorWindow::OnReadyRead() {
  bool got = false;
  while (socket_.canReadLine()) {
    if (auto s = ParseSample(socket_.readLine())) {
      sample_ = std::move(s);
      got = true;
    }
  }
  if (!got) return;
  if (!logged_first_) {  // proof of the IPC path, once per connection
    logged_first_ = true;
    Log(QStringLiteral("first sample: cpu %1%, %2 services, aember %3")
            .arg(sample_->cpu_percent, 0, 'f', 1)
            .arg(sample_->services.size())
            .arg(sample_->has_aember ? QStringLiteral("heartbeat ok")
                                     : QStringLiteral("no heartbeat")));
  }
  stale_.start();
  update();
  if (!screenshot_.isEmpty()) {
    const bool ok = Render(size().isEmpty() ? QSize{800, 480} : size())
                        .save(screenshot_, "PNG");
    Log(QStringLiteral("screenshot %1: %2")
            .arg(screenshot_, ok ? "ok" : "FAILED"));
    screenshot_.clear();
    if (quit_after_screenshot_) QCoreApplication::exit(ok ? 0 : 1);
  }
}

QImage MonitorWindow::Render(QSize size) const {
  QImage image{size, QImage::Format_RGB32};
  QPainter painter{&image};
  Paint(painter, size, sample_, status_);
  return image;
}

void MonitorWindow::paintEvent(QPaintEvent* /*event*/) {
  QPainter painter{this};
  Paint(painter, size(), sample_, status_);
}

}  // namespace aember::monitor

#include <QColor>
#include <QImage>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPainter>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <aember-monitor/dashboard.h>
#include <aember-monitor/model.h>
#include <aember-monitor/monitor-window.h>

using namespace aember::monitor;

namespace {

const QByteArray kSample =
    R"({"version":1,"time":1,"uptime_s":93784,)"
    R"("cpu":{"percent":42.5,"cores":4},)"
    R"("memory":{"total_kb":512000,"available_kb":384000,"used_percent":25.0},)"
    R"("load":[0.52,0.25,0.10],)"
    R"("net":[{"iface":"eth0","rx_bytes":1,"tx_bytes":2,"rx_bps":1536,"tx_bps":512}],)"
    R"("disks":[{"path":"/host-logs","total_kb":1000,"available_kb":900}],)"
    R"("thermal":[{"zone":"thermal_zone0","type":"cpu-thermal","celsius":42.5}],)"
    R"("aember":{"pid1":{"pid":1,"vm_rss_kb":3900,"threads":7},)"
    R"("network":{"online":true,"interface":"eth0","ip":"10.0.2.15","rtt_ms":18},)"
    R"("services":[{"name":"aember-netd","state":"running","type":"process"},)"
    R"({"name":"aember-stats","state":"failed","type":"container"}]}})";

/// Pixels of a colour close to @p c (antialiasing blends the edges).
int CountColor(const QImage& image, QColor c) {
  int n = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor p = image.pixelColor(x, y);
      if (qAbs(p.red() - c.red()) < 8 && qAbs(p.green() - c.green()) < 8 &&
          qAbs(p.blue() - c.blue()) < 8) {
        ++n;
      }
    }
  }
  return n;
}

}  // namespace

class MonitorTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() { qputenv("QT_QPA_FONTDIR", AEMBER_MONITOR_FONT_DIR); }

  void parsesEveryField() {
    const auto s = ParseSample(kSample);
    QVERIFY(s.has_value());
    QCOMPARE(s->uptime_s, 93784);
    QCOMPARE(s->cpu_percent, 42.5);
    QCOMPARE(s->cores, 4);
    QCOMPARE(s->mem_available_kb, 384000);
    QCOMPARE(s->load[2], 0.10);
    QCOMPARE(s->interfaces.size(), 1);
    QCOMPARE(s->interfaces[0].rx_bps, 1536);
    QCOMPARE(s->disks[0].available_kb, 900);
    QCOMPARE(s->thermal[0].type, QStringLiteral("cpu-thermal"));
    QVERIFY(s->has_aember);
    QVERIFY(s->online);
    QCOMPARE(s->ip, QStringLiteral("10.0.2.15"));
    QCOMPARE(s->pid1_threads, 7);
    QCOMPARE(s->services.size(), 2);
    QCOMPARE(s->services[1].state, QStringLiteral("failed"));
  }

  void toleratesMissingPartsAndRejectsOtherVersions() {
    const auto s = ParseSample(R"({"version":1,"aember":null})");
    QVERIFY(s.has_value());
    QVERIFY(!s->has_aember);
    QCOMPARE(s->cores, 0);
    QVERIFY(!ParseSample(R"({"version":2})"));
    QVERIFY(!ParseSample("not json"));
    QVERIFY(!ParseSample("[1,2]"));
  }

  void formatsUnits() {
    QCOMPARE(FormatRate(512), QStringLiteral("512 B/s"));
    QCOMPARE(FormatRate(1536), QStringLiteral("1.5 KB/s"));
    QCOMPARE(FormatRate(3 * 1024 * 1024), QStringLiteral("3.0 MB/s"));
    QCOMPARE(FormatKb(384000), QStringLiteral("375.0 MB"));
    QCOMPARE(FormatKb(900), QStringLiteral("900 KB"));
    QCOMPARE(FormatUptime(93784), QStringLiteral("1d 2h"));
    QCOMPARE(FormatUptime(7500), QStringLiteral("2h 05m"));
    QCOMPARE(FormatUptime(249), QStringLiteral("4m 09s"));
  }

  void drawsTheWaitingScreenAndTheDashboard() {
    for (const QSize size :
         {QSize{320, 240}, QSize{800, 480}, QSize{1920, 1080}}) {
      QImage waiting{size, QImage::Format_RGB32};
      {
        QPainter p{&waiting};
        Paint(p, size, std::nullopt, QStringLiteral("waiting"));
      }
      QImage full{size, QImage::Format_RGB32};
      {
        QPainter p{&full};
        Paint(p, size, ParseSample(kSample), {});
      }
      // The orange "aember" title on both; panels only on the dashboard.
      QVERIFY2(CountColor(waiting, QColor{0xff, 0x8a, 0x3d}) > 20,
               qPrintable(QString::number(size.width())));
      QVERIFY(CountColor(full, QColor{0xff, 0x8a, 0x3d}) > 20);
      QVERIFY(CountColor(full, QColor{0x1d, 0x21, 0x28}) > size.width());
      // Antialiased text blends a few pixels near the panel colour; panels
      // themselves are only on the dashboard.
      QVERIFY(CountColor(waiting, QColor{0x1d, 0x21, 0x28}) <
              CountColor(full, QColor{0x1d, 0x21, 0x28}) / 20);
      // A failed container shows a red dot, the running service a green one.
      QVERIFY(CountColor(full, QColor{0xe5, 0x53, 0x4b}) > 4);
      QVERIFY(CountColor(full, QColor{0x4c, 0xc3, 0x8a}) > 4);
    }
  }

  // Regression (found by `run.py test --sanitize`): the window went away
  // with a connected socket, whose destructor signalled into an already
  // destroyed timer.
  void readsLinesFromTheSocketAndSavesAScreenshot() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("stats.sock"));
    QLocalServer server;
    QVERIFY(server.listen(path));

    const QString png = dir.filePath(QStringLiteral("shot.png"));
    MonitorWindow window{path, png};
    window.resize(640, 400);
    QVERIFY(server.waitForNewConnection(5000));
    QLocalSocket* client = server.nextPendingConnection();
    // A partial line first, then the rest: only whole lines count.
    client->write(kSample.left(40));
    client->flush();
    QTest::qWait(50);
    QVERIFY(!QFile::exists(png));
    client->write(kSample.mid(40) + "\n");
    client->flush();
    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(png), 5000);
    const QImage shot{png};
    QCOMPARE(shot.size(), QSize(640, 400));
    QVERIFY(CountColor(shot, QColor{0x1d, 0x21, 0x28}) > 640);  // the panels
  }

  void reconnectsWhenAemberStatsComesBack() {
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("stats.sock"));
    MonitorWindow window{path};  // nothing listens yet
    QTest::qWait(200);
    QLocalServer server;
    QVERIFY(server.listen(path));
    // The retry timer runs on the event loop: wait while it spins.
    QTRY_VERIFY_WITH_TIMEOUT(server.hasPendingConnections(), 5000);
  }
};

QTEST_MAIN(MonitorTest)
#include "test_monitor.moc"

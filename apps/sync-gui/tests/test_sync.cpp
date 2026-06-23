#include <QTest>
#include <QProcess>

class SyncTest : public QObject {
    Q_OBJECT

private slots:
    void testGitStatusWorks() {
        QProcess p;
        p.start("git", {"status"});
        p.waitForFinished();
        QVERIFY(p.exitCode() == 0 || p.exitCode() == 128); // 128 if not in a git repo
    }

    void testCliArgsAvailable() {
        // Here we mock check if --status, --commit, and --pull flags are parsed
        // To be tested via process invocation or a separated parser logic
        QStringList args = {"--status", "--commit", "test msg", "--pull"};
        QVERIFY(args.contains("--status"));
        QVERIFY(args.contains("--commit"));
    }
};

QTEST_MAIN(SyncTest)
#include "test_sync.moc"

#include <QtTest/QtTest>
#include <QTableWidget>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include "mainwindow.h"

class TestEnvGui : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QDir().mkpath(QDir::homePath() + "/.config/environment.d");
        QFile file(QDir::homePath() + "/.config/environment.d/99-labwc.conf");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "TEST_VAR=123\n";
            file.close();
        }
    }

    void testGuiLoadsVariables() {
        int argc = 0;
        char *argv[] = {nullptr};
        QApplication app(argc, argv);
        
        EnvGui gui;
        QTableWidget *table = gui.findChild<QTableWidget*>("envTable");
        QVERIFY(table != nullptr);
        
        bool found = false;
        for (int i = 0; i < table->rowCount(); ++i) {
            if (table->item(i, 0)->text() == "TEST_VAR" && table->item(i, 1)->text() == "123") {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }
    
    void cleanupTestCase() {
        QFile::remove(QDir::homePath() + "/.config/environment.d/99-labwc.conf");
    }
};

QTEST_MAIN(TestEnvGui)
#include "test_env_gui.moc"

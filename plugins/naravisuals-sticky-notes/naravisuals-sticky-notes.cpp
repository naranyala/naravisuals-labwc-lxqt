#include <QLabel>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <ilxqtpanelplugin.h>

class NaravisualsStickyNotes : public QObject, public ILXQtPanelPlugin {
    Q_OBJECT
public:
    NaravisualsStickyNotes(const ILXQtPanelPluginStartupInfo &startupInfo, QObject *parent = nullptr)
        : QObject(parent)
        , ILXQtPanelPlugin(startupInfo)
        , mLabel(new QLabel("📝"))
    {
        mLabel->setAlignment(Qt::AlignCenter);
        mLabel->setMinimumWidth(30);
        mLabel->setCursor(Qt::PointingHandCursor);
        mLabel->setStyleSheet("QLabel { padding: 2px 4px; }");

        mNotePath = QDir::homePath() + "/.local/share/naravisuals/sticky-notes.txt";
        QFile file(mNotePath);
        if (file.exists()) {
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            QString content = QString(file.readAll()).trimmed();
            file.close();
            int lines = content.count('\n') + (content.isEmpty() ? 0 : 1);
            mLabel->setToolTip(QString("Sticky Notes (%1 lines)\nClick to edit").arg(lines));
        } else {
            mLabel->setToolTip("Sticky Notes\nClick to edit");
        }
    }

    ILXQtPanelPlugin::Flags flags() const override { return HaveConfigDialog; }
    QString themeId() const override { return QStringLiteral("NaravisualsStickyNotes"); }
    QWidget *widget() override { return mLabel; }

    void activated(ActivationReason reason) override {
        if (reason == Trigger || reason == DoubleClick) showEditor();
    }

    QDialog *configureDialog() override { showEditor(); return nullptr; }

private:
    void showEditor() {
        auto *dlg = new QDialog();
        dlg->setWindowTitle("Sticky Notes");
        dlg->resize(400, 300);

        auto *layout = new QVBoxLayout(dlg);
        auto *editor = new QTextEdit();
        editor->setStyleSheet(
            "QTextEdit { background-color: #1e1e2e; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; padding: 8px; font-family: monospace; font-size: 13px; }"
        );

        QFile file(mNotePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            editor->setPlainText(QString(file.readAll()));
            file.close();
        }

        layout->addWidget(editor);

        auto *btnRow = new QHBoxLayout();
        auto *saveBtn = new QPushButton("Save");
        saveBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(saveBtn, &QPushButton::clicked, this, [this, dlg, editor]() {
            QDir().mkpath(QFileInfo(mNotePath).absolutePath());
            QFile file(mNotePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(editor->toPlainText().toUtf8());
                file.close();
            }
            int lines = editor->toPlainText().count('\n') + 1;
            mLabel->setToolTip(QString("Sticky Notes (%1 lines)\nClick to edit").arg(lines));
            dlg->accept();
        });
        btnRow->addStretch();
        btnRow->addWidget(saveBtn);
        layout->addLayout(btnRow);

        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }

    QLabel *mLabel;
    QString mNotePath;
};

class NaravisualsStickyNotesLibrary : public QObject, public ILXQtPanelPluginLibrary {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const override {
        return new NaravisualsStickyNotes(startupInfo);
    }
};

#include "naravisuals-sticky-notes.moc"

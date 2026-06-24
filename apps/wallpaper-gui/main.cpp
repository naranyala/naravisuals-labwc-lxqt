#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QScrollArea>
#include <QFrame>
#include <QGridLayout>
#include <QProcess>
#include <QStatusBar>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QStandardPaths>
#include <QTimer>
#include <QDialog>
#include <QTreeView>
#include <QFileSystemModel>
#include <QSplitter>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QComboBox>
#include <QMimeDatabase>
#include <QtConcurrent/QtConcurrentRun>

// ============================================================
// Custom file browser dialog — replaces QFileDialog entirely
// to avoid XDG Desktop Portal / DBus freezes on Wayland
// ============================================================
class FileBrowserDialog : public QDialog {
    Q_OBJECT

public:
    enum Mode { SelectDirectory, SelectImageFile };

    FileBrowserDialog(Mode mode, const QString &startPath, QWidget *parent = nullptr)
        : QDialog(parent), m_mode(mode)
    {
        setWindowTitle(mode == SelectDirectory ? "Select Wallpaper Directory" : "Select Wallpaper Image");
        resize(820, 560);

        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(8);
        mainLayout->setContentsMargins(12, 12, 12, 12);

        // --- Location bar ---
        auto *locRow = new QHBoxLayout();
        auto *locLabel = new QLabel("Location:");
        locLabel->setStyleSheet("font-weight: bold; color: #cdd6f4;");
        locRow->addWidget(locLabel);

        m_locationEdit = new QLineEdit();
        m_locationEdit->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 4px; padding: 6px; font-size: 13px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
        );
        connect(m_locationEdit, &QLineEdit::returnPressed, this, &FileBrowserDialog::onLocationEntered);
        locRow->addWidget(m_locationEdit, 1);

        auto *upBtn = new QPushButton("Up");
        upBtn->setFixedWidth(50);
        upBtn->setMinimumHeight(32);
        connect(upBtn, &QPushButton::clicked, this, &FileBrowserDialog::navigateUp);
        locRow->addWidget(upBtn);

        auto *homeBtn = new QPushButton("Home");
        homeBtn->setFixedWidth(60);
        homeBtn->setMinimumHeight(32);
        connect(homeBtn, &QPushButton::clicked, this, [this]() {
            navigateTo(QDir::homePath());
        });
        locRow->addWidget(homeBtn);

        mainLayout->addLayout(locRow);

        // --- Main content: tree + optional preview ---
        m_model = new QFileSystemModel(this);
        m_model->setReadOnly(true);

        if (mode == SelectDirectory) {
            m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
        } else {
            m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
            QStringList imgFilters;
            imgFilters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.webp" << "*.gif";
            m_model->setNameFilters(imgFilters);
            m_model->setNameFilterDisables(false);
        }

        m_model->setRootPath(startPath.isEmpty() ? QDir::homePath() : startPath);

        m_tree = new QTreeView();
        m_tree->setModel(m_model);
        m_tree->setAnimated(false);
        m_tree->setSortingEnabled(true);
        m_tree->sortByColumn(0, Qt::AscendingOrder);
        m_tree->setIndentation(16);

        // Hide Size, Type, Date columns for cleanliness
        m_tree->header()->hideSection(1);
        m_tree->header()->hideSection(2);
        m_tree->header()->hideSection(3);
        m_tree->header()->setStretchLastSection(true);

        m_tree->setStyleSheet(
            "QTreeView { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; "
            "border-radius: 6px; font-size: 13px; }"
            "QTreeView::item { padding: 4px; }"
            "QTreeView::item:selected { background-color: #313244; color: #89b4fa; }"
            "QTreeView::item:hover { background-color: #1e1e3a; }"
            "QHeaderView::section { background-color: #181825; color: #a6adc8; border: none; "
            "padding: 6px; font-weight: bold; }"
        );

        connect(m_tree, &QTreeView::clicked, this, &FileBrowserDialog::onItemClicked);
        connect(m_tree, &QTreeView::doubleClicked, this, &FileBrowserDialog::onItemDoubleClicked);

        if (mode == SelectImageFile) {
            // Splitter with tree + preview
            auto *splitter = new QSplitter(Qt::Horizontal);

            splitter->addWidget(m_tree);

            auto *previewContainer = new QWidget();
            auto *previewLayout = new QVBoxLayout(previewContainer);
            previewLayout->setContentsMargins(8, 8, 8, 8);

            m_previewLabel = new QLabel("No image selected");
            m_previewLabel->setAlignment(Qt::AlignCenter);
            m_previewLabel->setMinimumSize(240, 240);
            m_previewLabel->setStyleSheet(
                "QLabel { background-color: #181825; border: 1px solid #313244; "
                "border-radius: 6px; color: #585b70; font-size: 13px; }"
            );
            previewLayout->addWidget(m_previewLabel, 1);

            m_fileNameLabel = new QLabel("");
            m_fileNameLabel->setAlignment(Qt::AlignCenter);
            m_fileNameLabel->setStyleSheet("color: #a6adc8; font-size: 12px; padding: 4px;");
            m_fileNameLabel->setWordWrap(true);
            previewLayout->addWidget(m_fileNameLabel);

            splitter->addWidget(previewContainer);
            splitter->setStretchFactor(0, 3);
            splitter->setStretchFactor(1, 2);

            mainLayout->addWidget(splitter, 1);
        } else {
            mainLayout->addWidget(m_tree, 1);
        }

        // --- Selected path display ---
        auto *selRow = new QHBoxLayout();
        auto *selLabel = new QLabel("Selected:");
        selLabel->setStyleSheet("font-weight: bold; color: #cdd6f4;");
        selRow->addWidget(selLabel);

        m_selectedEdit = new QLineEdit();
        m_selectedEdit->setReadOnly(true);
        m_selectedEdit->setStyleSheet(
            "QLineEdit { background-color: #11111b; color: #a6e3a1; border: 1px solid #313244; "
            "border-radius: 4px; padding: 6px; font-size: 13px; }"
        );
        selRow->addWidget(m_selectedEdit, 1);
        mainLayout->addLayout(selRow);

        // --- Buttons ---
        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();

        auto *cancelBtn = new QPushButton("Cancel");
        cancelBtn->setMinimumHeight(36);
        cancelBtn->setMinimumWidth(100);
        cancelBtn->setStyleSheet(
            "QPushButton { background-color: #45475a; color: #cdd6f4; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #585b70; }"
        );
        connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
        btnRow->addWidget(cancelBtn);

        auto *okBtn = new QPushButton(mode == SelectDirectory ? "Open Folder" : "Select Image");
        okBtn->setMinimumHeight(36);
        okBtn->setMinimumWidth(120);
        okBtn->setStyleSheet(
            "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 6px; "
            "padding: 8px 16px; font-weight: bold; border: none; }"
            "QPushButton:hover { background-color: #b4f9b8; }"
        );
        connect(okBtn, &QPushButton::clicked, this, &FileBrowserDialog::onAccept);
        btnRow->addWidget(okBtn);

        mainLayout->addLayout(btnRow);

        // Style the dialog itself
        setStyleSheet(
            "QDialog { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; "
            "padding: 6px 12px; font-weight: bold; font-size: 13px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
        );

        // Navigate to start path
        QString startDir = startPath.isEmpty() ? QDir::homePath() : startPath;
        if (!QDir(startDir).exists()) {
            startDir = QDir::homePath();
        }
        navigateTo(startDir);
    }

    QString selectedPath() const { return m_selectedPath; }

private slots:
    void onLocationEntered() {
        QString path = m_locationEdit->text().trimmed();
        if (QDir(path).exists()) {
            navigateTo(path);
        }
    }

    void navigateUp() {
        QModelIndex current = m_tree->rootIndex();
        QString currentPath = m_model->filePath(current);
        QDir dir(currentPath);
        if (dir.cdUp()) {
            navigateTo(dir.absolutePath());
        }
    }

    void navigateTo(const QString &path) {
        QModelIndex idx = m_model->index(path);
        m_tree->setRootIndex(idx);
        m_locationEdit->setText(path);

        if (m_mode == SelectDirectory) {
            m_selectedPath = path;
            m_selectedEdit->setText(path);
        }

        // Expand after a short delay to let the model populate
        QTimer::singleShot(100, this, [this, path]() {
            m_tree->resizeColumnToContents(0);
        });
    }

    void onItemClicked(const QModelIndex &index) {
        QString path = m_model->filePath(index);
        QFileInfo fi(path);

        if (m_mode == SelectDirectory) {
            if (fi.isDir()) {
                m_selectedPath = path;
                m_selectedEdit->setText(path);
            }
        } else {
            // SelectImageFile
            if (fi.isFile()) {
                m_selectedPath = path;
                m_selectedEdit->setText(path);
                updatePreview(path);
            } else if (fi.isDir()) {
                m_selectedEdit->setText("");
                m_selectedPath.clear();
                if (m_previewLabel) {
                    m_previewLabel->setText("No image selected");
                    m_previewLabel->setPixmap(QPixmap());
                }
                if (m_fileNameLabel) {
                    m_fileNameLabel->setText("");
                }
            }
        }
    }

    void onItemDoubleClicked(const QModelIndex &index) {
        QString path = m_model->filePath(index);
        QFileInfo fi(path);

        if (fi.isDir()) {
            navigateTo(path);
        } else if (fi.isFile() && m_mode == SelectImageFile) {
            m_selectedPath = path;
            m_selectedEdit->setText(path);
            accept();
        }
    }

    void onAccept() {
        if (m_mode == SelectDirectory) {
            // Use current directory if nothing explicitly selected
            if (m_selectedPath.isEmpty()) {
                m_selectedPath = m_locationEdit->text().trimmed();
            }
            if (!m_selectedPath.isEmpty() && QDir(m_selectedPath).exists()) {
                accept();
            }
        } else {
            if (!m_selectedPath.isEmpty() && QFileInfo(m_selectedPath).isFile()) {
                accept();
            }
        }
    }

    void updatePreview(const QString &path) {
        if (!m_previewLabel) return;

        m_previewLabel->setText("Loading preview...");
        m_previewLabel->setPixmap(QPixmap());

        QLabel *previewPtr = m_previewLabel;
        QLabel *fileNamePtr = m_fileNameLabel;
        QFuture<void> future = QtConcurrent::run([path, previewPtr, fileNamePtr]() {
            QPixmap pix(path);
            QMetaObject::invokeMethod(previewPtr, [pix, previewPtr, fileNamePtr, path]() {
                if (!pix.isNull()) {
                    previewPtr->setPixmap(pix.scaled(
                        previewPtr->size() - QSize(8, 8),
                        Qt::KeepAspectRatio,
                        Qt::SmoothTransformation
                    ));
                    if (fileNamePtr) {
                        QFileInfo fi(path);
                        fileNamePtr->setText(fi.fileName() + "\n" +
                            QString::number(pix.width()) + " x " + QString::number(pix.height()));
                    }
                } else {
                    previewPtr->setText("Cannot preview");
                    previewPtr->setPixmap(QPixmap());
                }
            }, Qt::QueuedConnection);
        });
        Q_UNUSED(future);
    }

private:
    Mode m_mode;
    QFileSystemModel *m_model = nullptr;
    QTreeView *m_tree = nullptr;
    QLineEdit *m_locationEdit = nullptr;
    QLineEdit *m_selectedEdit = nullptr;
    QLabel *m_previewLabel = nullptr;
    QLabel *m_fileNameLabel = nullptr;
    QString m_selectedPath;
};

// ============================================================
// Wallpaper entry
// ============================================================
struct WallpaperEntry {
    QString path;
    QString name;
};

// ============================================================
// Main application window
// ============================================================
class WallpaperGui : public QMainWindow {
    Q_OBJECT

public:
    WallpaperGui(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Wallpaper Manager");
        resize(780, 620);

        auto *central = new QWidget(this);
        setCentralWidget(central);
        auto *mainLayout = new QVBoxLayout(central);
        mainLayout->setSpacing(12);
        mainLayout->setContentsMargins(20, 20, 20, 20);

        detectBackend();

        auto *header = new QLabel("Browse and set desktop wallpapers");
        header->setStyleSheet("font-size: 14px; color: #a6adc8; margin-bottom: 5px;");
        mainLayout->addWidget(header);

        // --- Path bar ---
        auto *pathRow = new QHBoxLayout();
        pathEdit = new QLineEdit();
        pathEdit->setPlaceholderText("Wallpaper directory path...");
        pathEdit->setStyleSheet(
            "QLineEdit { background-color: #181825; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 8px; }"
            "QLineEdit:focus { border: 1px solid #89b4fa; }"
        );
        connect(pathEdit, &QLineEdit::returnPressed, this, &WallpaperGui::scanDirectory);
        pathRow->addWidget(pathEdit, 1);

        auto *browseBtn = new QPushButton("Browse Folder");
        browseBtn->setMinimumHeight(36);
        connect(browseBtn, &QPushButton::clicked, this, &WallpaperGui::browseDirectory);
        pathRow->addWidget(browseBtn);

        auto *selectFileBtn = new QPushButton("Select Image");
        selectFileBtn->setMinimumHeight(36);
        connect(selectFileBtn, &QPushButton::clicked, this, &WallpaperGui::selectSingleImage);
        pathRow->addWidget(selectFileBtn);

        mainLayout->addLayout(pathRow);

        // --- Scroll area with grid ---
        scroll = new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background-color: #1e1e2e; border: none; }");

        gridWidget = new QWidget();
        gridLayout = new QGridLayout(gridWidget);
        gridLayout->setSpacing(10);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        scroll->setWidget(gridWidget);
        mainLayout->addWidget(scroll, 1);

        // --- Buttons ---
        auto *btnRow = new QHBoxLayout();
        btnRow->addStretch();

        auto *refreshBtn = new QPushButton("Refresh");
        refreshBtn->setMinimumHeight(38);
        connect(refreshBtn, &QPushButton::clicked, this, &WallpaperGui::scanDirectory);
        btnRow->addWidget(refreshBtn);

        mainLayout->addLayout(btnRow);

        statusBar()->setStyleSheet("background-color: #181825; color: #a6adc8; border-top: 1px solid #313244;");
        statusBar()->showMessage("Backend: " + backend + " | Select a directory to browse wallpapers");

        applyStyle();

        // Auto-detect common wallpaper dirs
        QString home = QDir::homePath();
        QStringList candidates = {
            home + "/Pictures/Wallpapers",
            home + "/Wallpapers",
            home + "/.config/wallpapers",
            "/usr/share/wallpapers",
            "/usr/share/backgrounds"
        };
        for (const auto &dir : candidates) {
            if (QDir(dir).exists()) {
                pathEdit->setText(dir);
                scanDirectory();
                break;
            }
        }
    }

private slots:
    void detectBackend() {
        QProcess p;
        p.start("which", {"swww"});
        p.waitForFinished(3000);
        if (p.exitCode() == 0) {
            backend = "swww";
        } else {
            p.start("which", {"swaybg"});
            p.waitForFinished(3000);
            if (p.exitCode() == 0) {
                backend = "swaybg";
            } else {
                backend = "none";
            }
        }
    }

    void browseDirectory() {
        FileBrowserDialog dlg(FileBrowserDialog::SelectDirectory,
            pathEdit->text().isEmpty() ? QDir::homePath() : pathEdit->text(),
            this);
        if (dlg.exec() == QDialog::Accepted) {
            QString dir = dlg.selectedPath();
            if (!dir.isEmpty()) {
                pathEdit->setText(dir);
                scanDirectory();
            }
        }
    }

    void selectSingleImage() {
        FileBrowserDialog dlg(FileBrowserDialog::SelectImageFile,
            pathEdit->text().isEmpty() ? QDir::homePath() : pathEdit->text(),
            this);
        if (dlg.exec() == QDialog::Accepted) {
            QString file = dlg.selectedPath();
            if (!file.isEmpty()) {
                applyWallpaper(file);
            }
        }
    }

    void scanDirectory() {
        // Clear grid
        QLayoutItem *item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }
        wallpapers.clear();

        QString dirPath = pathEdit->text().trimmed();
        if (dirPath.isEmpty()) return;

        QDir dir(dirPath);
        if (!dir.exists()) {
            statusBar()->showMessage("Directory not found: " + dirPath);
            return;
        }

        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.webp" << "*.gif";
        QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

        // Also scan one level deep
        QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto &sub : subdirs) {
            QDir subDir(dirPath + "/" + sub);
            files.append(subDir.entryInfoList(filters, QDir::Files, QDir::Name));
        }

        for (const auto &fi : files) {
            wallpapers.append({fi.absoluteFilePath(), fi.fileName()});
        }

        populateGrid();
        statusBar()->showMessage(QString("Found %1 wallpapers in %2").arg(wallpapers.size()).arg(dirPath));
    }

    void populateGrid() {
        QLayoutItem *item;
        while ((item = gridLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        int cols = 4;
        int thumbSize = 160;

        for (int i = 0; i < wallpapers.size(); ++i) {
            auto *card = new QFrame();
            card->setFrameShape(QFrame::StyledPanel);
            card->setStyleSheet(
                "QFrame { background-color: #181825; border: 1px solid #45475a; border-radius: 8px; }"
                "QFrame:hover { border: 1px solid #89b4fa; }"
            );
            card->setFixedSize(thumbSize + 16, thumbSize + 50);

            auto *cardLayout = new QVBoxLayout(card);
            cardLayout->setSpacing(4);
            cardLayout->setContentsMargins(6, 6, 6, 6);

            auto *thumb = new QLabel();
            thumb->setFixedSize(thumbSize, thumbSize);
            thumb->setScaledContents(true);
            thumb->setText("Loading...");
            thumb->setStyleSheet("color: #585b70; background-color: #11111b;");
            thumb->setAlignment(Qt::AlignCenter);
            cardLayout->addWidget(thumb);

            auto *nameLabel = new QLabel(wallpapers[i].name);
            nameLabel->setStyleSheet("font-size: 11px; color: #a6adc8;");
            nameLabel->setAlignment(Qt::AlignCenter);
            nameLabel->setMaximumWidth(thumbSize);
            cardLayout->addWidget(nameLabel);

            auto *setBtn = new QPushButton("Set");
            setBtn->setMinimumHeight(26);
            setBtn->setStyleSheet(
                "QPushButton { background-color: #a6e3a1; color: #1e1e2e; border-radius: 4px; padding: 4px; font-weight: bold; font-size: 12px; border: none; }"
                "QPushButton:hover { background-color: #b4f9b8; }"
            );
            connect(setBtn, &QPushButton::clicked, this, [this, path = wallpapers[i].path]() {
                applyWallpaper(path);
            });
            cardLayout->addWidget(setBtn);

            int row = i / cols;
            int col = i % cols;
            gridLayout->addWidget(card, row, col);

            // Load thumbnail asynchronously
            QString imgPath = wallpapers[i].path;
            QLabel *thumbPtr = thumb;
            QFuture<void> future = QtConcurrent::run([imgPath, thumbPtr, thumbSize]() {
                QPixmap pix(imgPath);
                if (!pix.isNull()) {
                    QPixmap scaled = pix.scaled(thumbSize, thumbSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    QMetaObject::invokeMethod(thumbPtr, [thumbPtr, scaled]() {
                        thumbPtr->setPixmap(scaled);
                    }, Qt::QueuedConnection);
                } else {
                    QMetaObject::invokeMethod(thumbPtr, [thumbPtr]() {
                        thumbPtr->setText("No preview");
                        thumbPtr->setStyleSheet("color: #a6adc8; background-color: #313244;");
                        thumbPtr->setAlignment(Qt::AlignCenter);
                    }, Qt::QueuedConnection);
                }
            });
            Q_UNUSED(future);
        }
    }

    void applyWallpaper(const QString &path) {
        if (backend == "swww") {
            // Kill existing swww-daemon, restart, then set
            QProcess::execute("pkill", {"swww-daemon"});
            QProcess::startDetached("swww", {"init"});
            QTimer::singleShot(500, this, [this, path]() {
                QProcess::startDetached("swww", {"img", path});
                statusBar()->showMessage("Wallpaper set via swww: " + path);
            });
        } else if (backend == "swaybg") {
            // Kill existing swaybg, start new one
            QProcess::execute("pkill", {"swaybg"});
            QProcess::startDetached("swaybg", {"-m", "fill", "-i", path});
            statusBar()->showMessage("Wallpaper set via swaybg: " + path);
        } else {
            statusBar()->showMessage("No wallpaper backend found (install swww or swaybg)");
        }
    }

private:
    void applyStyle() {
        setStyleSheet(
            "QMainWindow { background-color: #1e1e2e; }"
            "QWidget { color: #cdd6f4; font-family: sans-serif; font-size: 14px; }"
            "QPushButton { background-color: #89b4fa; color: #1e1e2e; border-radius: 6px; padding: 10px; font-weight: bold; font-size: 14px; border: none; }"
            "QPushButton:hover { background-color: #b4befe; }"
            "QPushButton:pressed { background-color: #74c7ec; }"
        );
    }

    QString backend;
    QLineEdit *pathEdit;
    QScrollArea *scroll;
    QWidget *gridWidget;
    QGridLayout *gridLayout;
    QList<WallpaperEntry> wallpapers;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        if (arg == "--set" && i + 1 < argc) {
            QString path = argv[++i];
            // Detect backend and apply
            QProcess p;
            p.start("which", {"swww"});
            p.waitForFinished(3000);
            if (p.exitCode() == 0) {
                QProcess::execute("pkill", {"swww-daemon"});
                QProcess::startDetached("swww", {"init"});
                QProcess::startDetached("swww", {"img", path});
            } else {
                QProcess::execute("pkill", {"swaybg"});
                QProcess::startDetached("swaybg", {"-m", "fill", "-i", path});
            }
            return 0;
        } else if (arg == "--list" && i + 1 < argc) {
            QString dirPath = argv[++i];
            QDir dir(dirPath);
            if (!dir.exists()) {
                fprintf(stderr, "Directory not found: %s\n", dirPath.toUtf8().constData());
                return 1;
            }
            QStringList filters;
            filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.webp";
            QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
            for (const auto &fi : files) {
                printf("%s\n", fi.absoluteFilePath().toUtf8().constData());
            }
            return 0;
        }
    }

    WallpaperGui gui;
    gui.show();
    return app.exec();
}

#include "main.moc"

#ifndef PAGES_H
#define PAGES_H

#include <QWidget>

class GlobalThemesPage : public QWidget {
public:
    explicit GlobalThemesPage(QWidget *parent = nullptr);
};

class ComponentThemesPage : public QWidget {
public:
    explicit ComponentThemesPage(QWidget *parent = nullptr);
};

class WallpaperPage : public QWidget {
public:
    explicit WallpaperPage(QWidget *parent = nullptr);
};

class SystemPage : public QWidget {
public:
    explicit SystemPage(QWidget *parent = nullptr);
};

#endif // PAGES_H

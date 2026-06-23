%global debug_package %{nil}
%global __requires_exclude ^(libQt6|libxkbcommon|libGL|libEGL)

Name:           naravisuals-labwc-lxqt
Version:        1.0.0
Release:        1%{?dist}
Summary:        LXQt + Labwc Wayland desktop environment GUI suite

License:        GPL-2.0-only
URL:            https://github.com/naranyala/naravisuals-labwc-lxqt
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  ninja-build
BuildRequires:  gcc-c++ >= 11
BuildRequires:  qt6-qtbase-devel >= 6.4
BuildRequires:  desktop-file-utils

Requires:       qt6-qtbase >= 6.4
Requires:       qt6-qtbase-gui >= 6.4
Requires:       hicolor-icon-theme

Recommends:     labwc, lxqt-panel, qt6ct, kvantum
Recommends:     swaybg, swayidle, swaylock
Recommends:     dunst, rofi-wayland
Recommends:     breeze-icon-theme, breeze-cursor-theme

%description
A comprehensive suite of GUI configuration tools for managing
LXQt desktop and Labwc Wayland compositor environments.
Includes tools for audio, bluetooth, display, network, power,
input, themes, wallpapers, fonts, desktop entries, and more.

%prep
%autosetup

%build
%cmake -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_SKIP_RPATH=ON
%cmake_build

%install
%cmake_install

# Install config files
install -d %{buildroot}%{_sysconfdir}/xdg/naravisuals
cp -r configs/dotfiles/* %{buildroot}%{_sysconfdir}/xdg/naravisuals/

# Install wayland session
install -Dm644 configs/dotfiles/wayland-sessions/lxqt-labwc.desktop \
    %{buildroot}%{_datadir}/wayland-sessions/lxqt-labwc.desktop

# Install SDDM config
install -Dm644 configs/dotfiles/sddm/lxqt-labwc.conf \
    %{buildroot}%{_sysconfdir}/sddm.conf.d/naravisuals-lxqt-labwc.conf

# Install wallpapers
install -Dm644 assets/wallpaper.jpg \
    %{buildroot}%{_datadir}/naravisuals/wallpapers/wallpaper.jpg

%check
%{_bindir}/nv-control-center --version 2>/dev/null || true

%files
%{_bindir}/nv-*

%dir %{_sysconfdir}/xdg/naravisuals
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/labwc/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/lxqt/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/gtk-3.0/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/gtk-4.0/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/rofi/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/dunst/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/swaylock/*
%config(noreplace) %{_sysconfdir}/xdg/naravisuals/fontconfig/*
%{_sysconfdir}/sddm.conf.d/naravisuals-lxqt-labwc.conf
%{_datadir}/wayland-sessions/lxqt-labwc.desktop
%{_datadir}/naravisuals/wallpapers/wallpaper.jpg

%changelog
* Mon Jun 22 2026 naranyala - 1.0.0-1
- Initial release
- 27 GUI configuration tools for LXQt + Labwc
- CPack multi-generator packaging support

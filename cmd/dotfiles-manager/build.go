package main

import (
	"embed"
	"fmt"
	"os"
	"path/filepath"
)

//go:embed dotfiles/lxqt/session.conf
//go:embed dotfiles/lxqt/panel.conf
//go:embed dotfiles/lxqt/lxqt.conf
//go:embed dotfiles/lxqt/lxqt-config.conf
//go:embed dotfiles/lxqt/lxqt-powermanagement.conf
//go:embed dotfiles/lxqt/lxqt-runner.conf
//go:embed dotfiles/lxqt/lxqt-notificationd.conf
//go:embed dotfiles/lxqt/globalkeyshortcuts.conf
//go:embed dotfiles/labwc/rc.xml
//go:embed dotfiles/labwc/menu.xml
//go:embed dotfiles/labwc/autostart
//go:embed dotfiles/labwc/environment
//go:embed dotfiles/labwc/themerc
//go:embed dotfiles/labwc/shutdown
//go:embed dotfiles/gtk-3.0/settings.ini
//go:embed dotfiles/gtk-4.0/settings.ini
//go:embed dotfiles/qt6ct/qt6ct.conf
//go:embed dotfiles/pcmanfm-qt/lxqt/settings.conf
//go:embed dotfiles/qterminal.org/qterminal.ini
//go:embed dotfiles/user-dirs.dirs
//go:embed dotfiles/kanshi/config
//go:embed dotfiles/swaylock/config
//go:embed dotfiles/dunst/dunstrc
//go:embed dotfiles/emacs/init.el
var dotfiles embed.FS

type Dotfile struct {
	SrcPath string
	DstPath string
	Mode    os.FileMode
	Tmpl    bool
}

var manifest = []Dotfile{
	{SrcPath: "dotfiles/lxqt/session.conf", DstPath: ".config/lxqt/session.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/panel.conf", DstPath: ".config/lxqt/panel.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/lxqt.conf", DstPath: ".config/lxqt/lxqt.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/lxqt-config.conf", DstPath: ".config/lxqt/lxqt-config.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/lxqt-powermanagement.conf", DstPath: ".config/lxqt/lxqt-powermanagement.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/lxqt-runner.conf", DstPath: ".config/lxqt/lxqt-runner.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/lxqt-notificationd.conf", DstPath: ".config/lxqt/lxqt-notificationd.conf", Mode: 0644},
	{SrcPath: "dotfiles/lxqt/globalkeyshortcuts.conf", DstPath: ".config/lxqt/globalkeyshortcuts.conf", Mode: 0644},
	{SrcPath: "dotfiles/labwc/rc.xml", DstPath: ".config/labwc/rc.xml", Mode: 0644},
	{SrcPath: "dotfiles/labwc/menu.xml", DstPath: ".config/labwc/menu.xml", Mode: 0644},
	{SrcPath: "dotfiles/labwc/autostart", DstPath: ".config/labwc/autostart", Mode: 0755},
	{SrcPath: "dotfiles/labwc/environment", DstPath: ".config/labwc/environment", Mode: 0644},
	{SrcPath: "dotfiles/labwc/themerc", DstPath: ".config/labwc/themerc-override", Mode: 0644},
	{SrcPath: "dotfiles/labwc/shutdown", DstPath: ".config/labwc/shutdown", Mode: 0755},
	{SrcPath: "dotfiles/gtk-3.0/settings.ini", DstPath: ".config/gtk-3.0/settings.ini", Mode: 0644},
	{SrcPath: "dotfiles/gtk-4.0/settings.ini", DstPath: ".config/gtk-4.0/settings.ini", Mode: 0644},
	{SrcPath: "dotfiles/qt6ct/qt6ct.conf", DstPath: ".config/qt6ct/qt6ct.conf", Mode: 0644},
	{SrcPath: "dotfiles/pcmanfm-qt/lxqt/settings.conf", DstPath: ".config/pcmanfm-qt/lxqt/settings.conf", Mode: 0644},
	{SrcPath: "dotfiles/qterminal.org/qterminal.ini", DstPath: ".config/qterminal.org/qterminal.ini", Mode: 0644},
	{SrcPath: "dotfiles/user-dirs.dirs", DstPath: ".config/user-dirs.dirs", Mode: 0644},
	{SrcPath: "dotfiles/kanshi/config", DstPath: ".config/kanshi/config", Mode: 0644},
	{SrcPath: "dotfiles/swaylock/config", DstPath: ".config/swaylock/config", Mode: 0644},
	{SrcPath: "dotfiles/dunst/dunstrc", DstPath: ".config/dunst/dunstrc", Mode: 0644},
	{SrcPath: "dotfiles/emacs/init.el", DstPath: ".config/emacs/init.el", Mode: 0644},
}

type Env struct {
	Home string
	User string
}

func getEnv() Env {
	home, _ := os.UserHomeDir()
	return Env{
		Home: home,
		User: os.Getenv("USER"),
	}
}

func expandPath(dst string, e Env) string {
	if filepath.IsAbs(dst) {
		return dst
	}
	return filepath.Join(e.Home, dst)
}

func install(home string, dryRun bool, force bool) error {
	e := getEnv()
	if home != "" {
		e.Home = home
	}

	installed := 0
	skipped := 0
	failed := 0

	for _, df := range manifest {
		srcData, err := dotfiles.ReadFile(df.SrcPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "  ERROR reading %s: %v\n", df.SrcPath, err)
			failed++
			continue
		}

		dstPath := expandPath(df.DstPath, e)

		if dryRun {
			fmt.Printf("  WOULD INSTALL: %s -> %s\n", df.SrcPath, dstPath)
			installed++
			continue
		}

		if !force {
			if _, err := os.Stat(dstPath); err == nil {
				fmt.Printf("  SKIP %s (exists, use -f to overwrite)\n", dstPath)
				skipped++
				continue
			}
		}

		if err := os.MkdirAll(filepath.Dir(dstPath), 0755); err != nil {
			fmt.Fprintf(os.Stderr, "  ERROR creating directory for %s: %v\n", dstPath, err)
			failed++
			continue
		}

		if err := os.WriteFile(dstPath, srcData, df.Mode); err != nil {
			fmt.Fprintf(os.Stderr, "  ERROR writing %s: %v\n", dstPath, err)
			failed++
			continue
		}

		fmt.Printf("  INSTALLED %s\n", dstPath)
		installed++
	}

	fmt.Println()
	fmt.Printf("Summary: %d installed, %d skipped, %d failed\n", installed, skipped, failed)

	fmt.Println("\nNote: System files need to be copied manually using sudo:")
	fmt.Println("  sudo cp dotfiles/wayland-sessions/lxqt-labwc.desktop /usr/share/wayland-sessions/")
	fmt.Println("  sudo cp dotfiles/sddm/lxqt-labwc.conf /etc/sddm.conf.d/")
	fmt.Println()

	if failed > 0 {
		return fmt.Errorf("%d file(s) failed to install", failed)
	}
	return nil
}

func list() {
	e := getEnv()
	fmt.Println("LXQt Labwc Dotfiles Manifest")
	fmt.Println("============================")
	fmt.Println()
	for _, df := range manifest {
		dstPath := expandPath(df.DstPath, e)
		perm := fmt.Sprintf("%o", df.Mode.Perm())
		fmt.Printf("  %-40s %s -> %s\n", df.SrcPath, perm, dstPath)
	}
}

func validate(home string) error {
	e := getEnv()
	if home != "" {
		e.Home = home
	}

	missing := 0

	for _, df := range manifest {
		dstPath := expandPath(df.DstPath, e)
		if _, err := os.Stat(dstPath); os.IsNotExist(err) {
			fmt.Printf("  MISSING %s\n", dstPath)
			missing++
			continue
		}
		srcData, err := dotfiles.ReadFile(df.SrcPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "  ERROR reading embedded %s: %v\n", df.SrcPath, err)
			missing++
			continue
		}
		installedData, err := os.ReadFile(dstPath)
		if err != nil {
			fmt.Fprintf(os.Stderr, "  ERROR reading installed %s: %v\n", dstPath, err)
			missing++
			continue
		}
		if len(srcData) != len(installedData) {
			fmt.Printf("  DIFFERS (size) %s\n", dstPath)
			missing++
			continue
		}
	}

	if missing > 0 {
		return fmt.Errorf("%d file(s) missing or differ from expected", missing)
	}
	fmt.Println("All dotfiles are present and up-to-date.")
	return nil
}

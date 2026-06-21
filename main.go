package main

import (
	"flag"
	"fmt"
	"os"
)

const version = "2.1.0"

func main() {
	installCmd := flag.NewFlagSet("install", flag.ExitOnError)
	installHome := installCmd.String("home", "", "Install to a custom home directory")
	installDryRun := installCmd.Bool("dry-run", false, "Show what would be installed")
	installForce := installCmd.Bool("f", false, "Overwrite existing files")

	listCmd := flag.NewFlagSet("list", flag.ExitOnError)

	validateCmd := flag.NewFlagSet("validate", flag.ExitOnError)
	validateHome := validateCmd.String("home", "", "Validate against a custom home directory")

	if len(os.Args) < 2 {
		printUsage()
		os.Exit(1)
	}

	switch os.Args[1] {
	case "install":
		installCmd.Parse(os.Args[2:])
		if err := install(*installHome, *installDryRun, *installForce); err != nil {
			fmt.Fprintf(os.Stderr, "Install failed: %v\n", err)
			os.Exit(1)
		}

	case "list":
		listCmd.Parse(os.Args[2:])
		list()

	case "validate":
		validateCmd.Parse(os.Args[2:])
		if err := validate(*validateHome); err != nil {
			fmt.Fprintf(os.Stderr, "Validation failed: %v\n", err)
			os.Exit(1)
		}

	case "version":
		fmt.Printf("lxqt-labwc-dotfiles v%s\n", version)

	default:
		printUsage()
		os.Exit(1)
	}
}

func printUsage() {
	fmt.Println(`LXQt Labwc Dotfiles Manager

Usage:
  lxqt-labwc-dotfiles install    Install dotfiles to $HOME
    --home <dir>    Install to a custom home directory
    --dry-run       Show what would be installed without writing
    -f              Overwrite existing files

  lxqt-labwc-dotfiles list       List all managed dotfiles and their destinations

  lxqt-labwc-dotfiles validate   Check if installed dotfiles match the embedded ones
    --home <dir>    Validate against a custom home directory

  lxqt-labwc-dotfiles version    Show version`)
}

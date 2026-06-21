;; -*- lexical-binding: t -*-
;; Minimal Emacs config — beginner-friendly
;; Targets Emacs 29+ (use-package built-in, modus-themes built-in)

;; ── Package management ──────────────────────────────────────
(require 'package)
(add-to-list 'package-archives '("melpa" . "https://melpa.org/packages/") t)
(package-initialize)

(require 'use-package)
(setq use-package-always-ensure nil)   ; only install when explicitly asked

;; ── Look & feel ─────────────────────────────────────────────
(scroll-bar-mode -1)                   ; no scroll bars
(tool-bar-mode -1)                     ; no toolbar
(menu-bar-mode -1)                     ; no menu bar (M-x is faster)
(blink-cursor-mode -1)                 ; steady cursor

(setq inhibit-startup-screen t         ; no GNU splash
      initial-major-mode 'text-mode    ; *scratch* starts in text mode
      ring-bell-function 'ignore)      ; no beeping

;; Modus themes — built-in, accessible, light & dark
(load-theme 'modus-vivendi t)          ; dark theme. Use modus-operandi for light

;; ── Editing basics ──────────────────────────────────────────
(global-font-lock-mode t)              ; syntax highlighting
(show-paren-mode t)                    ; highlight matching parens
(global-display-line-numbers-mode t)   ; line numbers
(column-number-mode t)                 ; show column number
(electric-pair-mode t)                 ; auto-close parens, quotes, etc.
(delete-selection-mode t)              ; typing replaces selection
(savehist-mode t)                      ; save/restore minibuffer history

(setq-default indent-tabs-mode nil     ; use spaces, not tabs
              tab-width 4)

(setq sentence-end-double-space nil)   ; one space after period
(setq-default fill-column 80)          ; wrap at 80 chars
(global-visual-line-mode t)            ; word wrap at window edge

;; ── Navigation & search ─────────────────────────────────────
(ido-mode t)                           ; interactive file finding
(ido-everywhere t)
(setq ido-enable-flex-matching t)      ; fuzzy file matching

(global-set-key (kbd "C-x C-b") 'ibuffer)  ; better buffer list

;; ── Backups & autosave ─────────────────────────────────────
(setq backup-directory-alist '(("." . "~/.cache/emacs/backups"))
      auto-save-file-name-transforms '((".*" "~/.cache/emacs/autosave/" t))
      create-lockfiles nil)

;; ── Files & windows ─────────────────────────────────────────
(setq make-backup-files nil            ; no ~ files cluttering
      confirm-kill-emacs 'yes-or-no-p) ; confirm before quitting

;; Recentf — quick file history
(recentf-mode t)
(setq recentf-max-saved-items 50)
(global-set-key (kbd "C-x C-r") 'recentf-open-files)

;; ── Text editing helpers ────────────────────────────────────
(add-hook 'text-mode-hook 'flyspell-mode)     ; spell check in text
(add-hook 'prog-mode-hook 'flyspell-prog-mode) ; spell check comments

;; ── Dired (file manager) ────────────────────────────────────
(setq dired-listing-switches "-alh --group-directories-first"
      dired-kill-when-opening-new-dired-buffer t)

;; ── Org-mode basics ─────────────────────────────────────────
(setq org-log-done t)                  ; mark TODOs as DONE with timestamp
(global-set-key (kbd "C-c a") 'org-agenda)

;; ── Keybinding quick reference ──────────────────────────────
;; C-x C-f   open file
;; C-x C-s   save
;; C-x C-w   save as
;; C-x C-c   quit
;; C-x b     switch buffer
;; C-x C-b   better buffer list (ibuffer)
;; C-x C-r   recent files
;; C-s       search forward
;; C-r       search backward
;; C-x u     undo
;; C-/       undo
;; C-SPC     set mark
;; C-w       cut (kill)
;; M-w       copy
;; C-y       paste (yank)
;; M-x       run any command
;; C-g       cancel
;; C-c a     org agenda
;; C-h t     tutorial

;; ── Elegant exit to LXQt ────────────────────────────────────
(defun emacs-quick-exit ()
  "Save buffers and quit. For LXQt desktop integration."
  (interactive)
  (save-some-buffers t)
  (save-buffers-kill-terminal))

;; Emacs keybindings (default). No vim emulation.

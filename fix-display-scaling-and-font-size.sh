#!/bin/bash

echo "=== Menyelaraskan Proporsi Font dan Layar ==="

# 1. Pastikan faktor pengali font berada di angka mutlak 1.0 (wajib)
gsettings set org.gnome.desktop.interface text-scaling-factor 1.0

# 2. Perkecil ukuran dasar font (pt) agar saat layar di-scale, teks tidak raksasa
# Setelan default GNOME biasanya 11pt. Kita turunkan ke 10pt atau 9.5pt.
gsettings set org.gnome.desktop.interface font-name 'Cantarell 10'
gsettings set org.gnome.desktop.interface document-font-name 'Cantarell 10'
gsettings set org.gnome.desktop.interface monospace-font-name 'Source Code Pro 10'

echo "Font dasar telah diperkecil ke 10pt dengan faktor pengali 1.0."
echo "Silakan aktifkan kembali Display Scaling (125%/150%) di Settings > Displays."


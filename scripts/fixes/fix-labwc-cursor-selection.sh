#!/usr/bin/env bash

# rm ~/.config/labwc/rc.xml
# cp /etc/xdg/labwc/rc.xml ~/.config/labwc/
# labwc --reconfigure

# Ensure your local configuration folder exists
mkdir -p ~/.config/labwc

# Copy the example file (it may be named rc.xml or rc.xml.all)
cp /usr/share/doc/labwc/rc.xml ~/.config/labwc/rc.xml || cp /usr/share/doc/labwc/rc.xml.all ~/.config/labwc/rc.xml


#!/bin/bash

# Configuration file targets
LABWC_CONFIG="$HOME/.config/labwc/rc.xml"
LABWC_DIR="$HOME/.config/labwc"

# Ensure the configuration directory exists
mkdir -p "$LABWC_DIR"

# If rc.xml does not exist, create a clean default boilerplate
if [ ! -f "$LABWC_CONFIG" ]; then
    if [ -f "/etc/xdg/labwc/rc.xml" ]; then
        cp "/etc/xdg/labwc/rc.xml" "$LABWC_CONFIG"
    else
        echo -e "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<labwc_config>\n</labwc_config>" > "$LABWC_CONFIG"
    fi
fi

echo ">> Cleaning out old manual touchpad injections from rc.xml..."
# Safely strip any old custom blocks to prevent XML corruption or duplication
sed -i '/<libinput>/,/<\/libinput>/d' "$LABWC_CONFIG"

echo ">> Injecting universal touchpad categories into Labwc configuration..."

# Construct the optimized XML rule block using category overrides
# This forces libinput to toggle settings on ALL connected pointing devices
TMP_XML=$(mktemp)
cat <<EOF > "$TMP_XML"
  <libinput>
    <device category="default">
      <naturalScroll>yes</naturalScroll>
      <tap>yes</tap>
    </device>
    <device category="touchpad">
      <naturalScroll>yes</naturalScroll>
      <tap>yes</tap>
    </device>
  </libinput>
EOF

# Find the closing tag </labwc_config> or Openbox legacy tags and insert the config cleanly
if grep -q "</labwc_config>" "$LABWC_CONFIG"; then
    sed -i -e "/<\/labwc_config>/e cat $TMP_XML" -e "//N" "$LABWC_CONFIG"
elif grep -q "</openbox_config>" "$LABWC_CONFIG"; then
    sed -i -e "/<\/openbox_config>/e cat $TMP_XML" -e "//N" "$LABWC_CONFIG"
else
    # Ultimate append fallback if file lacks root node structure
    cat "$TMP_XML" >> "$LABWC_CONFIG"
fi

rm "$TMP_XML"

echo "--------------------------------------------------------"
echo " SUCCESS: Configurations written to ~/.config/labwc/rc.xml"
echo "--------------------------------------------------------"
echo " IMPORTANT: Labwc cannot reload hardware settings live."
echo " Please Log Out of your LXQt session and Log Back In now."
echo "--------------------------------------------------------"


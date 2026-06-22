#!/bin/bash

# Define paths to GTK configuration directories
GTK3_DIR="$HOME/.config/gtk-3.0"
GTK4_DIR="$HOME/.config/gtk-4.0"

# The setting line we want to ensure exists
SETTING_LINE="gtk-decoration-layout=menu:minimize,maximize,close"

update_gtk_settings() {
    local dir_path="$1"
    local ini_path="$dir_path/settings.ini"

    # 1. Create directory if it doesn't exist
    mkdir -p "$dir_path"

    # 2. Initialize file with [Settings] header if it is missing or empty
    if [ ! -f "$ini_path" ] || [ ! -s "$ini_path" ]; then
        echo -e "[Settings]\n$SETTING_LINE" > "$ini_path"
        echo "Created and configured: $ini_path"
        return
    fi

    # 3. Ensure the [Settings] header exists inside the file
    if ! grep -q "^\[Settings\]" "$ini_path"; then
        # Prepend [Settings] if completely missing
        sed -i '1i\[Settings\]' "$ini_path"
    fi

    # 4. Update or add the gtk-decoration-layout key
    if grep -q "^gtk-decoration-layout=" "$ini_path"; then
        # Replace existing line to avoid duplicates
        sed -i "s|^gtk-decoration-layout=.*|$SETTING_LINE|" "$ini_path"
        echo "Updated layout in: $ini_path"
    else
        # Append right under the [Settings] header
        sed -i "/^\[Settings\]/a $SETTING_LINE" "$ini_path"
        echo "Added layout to: $ini_path"
    fi
}

# Run the update for both GTK versions
update_gtk_settings "$GTK3_DIR"
update_gtk_settings "$GTK4_DIR"

echo "GTK window control settings updated successfully!"


#!/bin/bash
# Naravisuals Desktop Suite — Archive Builder
# Creates a distributable tarball of the project.
#
# Included: docs, configs, scripts, apps source, cmd source, root scripts, wallpaper
# Excluded: lxqt/ (1.7GB), src/ (15MB), thirdparty/, assets/inspiration/ (127MB),
#           apps/*/build/, .git/, qt6-build/
#
# Usage:
#   bash archive.sh                    # Create archive with default name
#   bash archive.sh -o output.tar.gz   # Custom output path
#   bash archive.sh --list             # List what would be included
#   bash archive.sh --dry-run          # Preview without creating

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_NAME="naravisuals-labwc-lxqt"
VERSION="$(date +%Y%m%d)"
OUTPUT=""
DRY_RUN=false
LIST_ONLY=false

for arg in "$@"; do
    case "$arg" in
        --list|-l)      LIST_ONLY=true ;;
        --dry-run|-n)   DRY_RUN=true ;;
        --help|-h)
            printf "Naravisuals Archive Builder\n\n"
            printf "Usage: bash archive.sh [options]\n\n"
            printf "Options:\n"
            printf "  -o <file>     Output file path (default: %s-v%s.tar.gz)\n" "$PROJECT_NAME" "$VERSION"
            printf "  --list, -l    List files that would be included\n"
            printf "  --dry-run, -n Preview without creating archive\n"
            printf "  --help, -h    Show this help\n"
            exit 0
            ;;
        -o)
            shift
            OUTPUT="$1"
            shift
            ;;
        -o*)
            OUTPUT="${arg:2}"
            ;;
    esac
done

if [ -z "$OUTPUT" ]; then
    OUTPUT="${SCRIPT_DIR}/${PROJECT_NAME}-v${VERSION}.tar.gz"
fi

cd "$SCRIPT_DIR"

# ---- Build file list using find (robust, no glob expansion issues) ----
# Strategy: find all files, then exclude by path prefix

EXCLUDE_DIRS=(
    "lxqt"
    "src"
    "thirdparty"
    "assets/inspiration"
    "apps/control-center/build"
    "apps/sddm-gui/build"
    "apps/ntfs-gui/build"
    "apps/audio-gui/build"
    "apps/display-gui/build"
    "apps/wallpaper-gui/build"
    "apps/autorun-gui/build"
    "apps/env-gui/build"
    "apps/font-gui/build"
    "apps/keybinds-gui/build"
    "apps/mime-gui/build"
    "apps/input-gui/build"
    "apps/power-gui/build"
    "apps/bluetooth-gui/build"
    "apps/network-gui/build"
    "apps/service-gui/build"
    "apps/log-gui/build"
    "apps/disk-gui/build"
    "apps/theme-gui/build"
    "src/lxqt-panel/build"
    ".git"
    "qt6-build"
)

EXCLUDE_EXTENSIONS=(
    "*.o"
    "*.so"
    "*.dylib"
    "*.exe"
    "*.test"
    "*.out"
)

# Build find command with exclusions
FIND_CMD="find . -maxdepth 1 -mindepth 1"
for d in "${EXCLUDE_DIRS[@]}"; do
    FIND_CMD="$FIND_CMD -not -path './$d' -not -path './$d/*'"
done
FIND_CMD="$FIND_CMD -not -name '*.o' -not -name '*.so' -not -name '*.dylib' -not -name '*.exe' -not -name '*.test' -not -name '*.out'"

# Get all top-level items
ALL_ITEMS=$(eval "$FIND_CMD" | sed 's|^\./||' | sort)

# Now recursively find files within included directories
FILES=()
while IFS= read -r item; do
    if [ -f "$item" ]; then
        FILES+=("$item")
    elif [ -d "$item" ]; then
        # Find all files inside this directory, excluding build dirs and .git
        while IFS= read -r f; do
            FILES+=("$f")
        done < <(find "$item" \
            -not -path "*/build/*" \
            -not -path "*/.git/*" \
            -not -path "*/inspiration/*" \
            -not -name "*.o" \
            -not -name "*.so" \
            -type f 2>/dev/null)
    fi
done <<< "$ALL_ITEMS"

# Also add configs/ subdirectories explicitly (find may miss nested)
while IFS= read -r f; do
    # Check if already in array
    found=false
    for existing in "${FILES[@]}"; do
        if [ "$existing" = "$f" ]; then
            found=true
            break
        fi
    done
    if [ "$found" = false ]; then
        FILES+=("$f")
    fi
done < <(find configs/ -type f 2>/dev/null)

# Deduplicate and sort
mapfile -t UNIQUE < <(printf '%s\n' "${FILES[@]}" | sort -u)

# ---- List mode ----
if [ "$LIST_ONLY" = true ]; then
    printf "Files to include in archive:\n\n"
    total=0
    count=0
    for f in "${UNIQUE[@]}"; do
        if [ -f "$f" ]; then
            size=$(stat -c%s "$f" 2>/dev/null || echo 0)
            printf "  %s (%s bytes)\n" "$f" "$size"
            total=$((total + size))
            count=$((count + 1))
        fi
    done
    printf "\nTotal: %d files, %s bytes (%d KB)\n" "$count" "$total" "$((total / 1024))"
    exit 0
fi

# ---- Create archive ----
# Show summary and ask for confirmation
printf "\nArchive: %s\n" "$OUTPUT"
printf "Files:   %d\n" "${#UNIQUE[@]}"

# Calculate total size
total_size=0
for f in "${UNIQUE[@]}"; do
    if [ -f "$f" ]; then
        s=$(stat -c%s "$f" 2>/dev/null || echo 0)
        total_size=$((total_size + s))
    fi
done
printf "Size:    %s bytes (%d KB)\n" "$total_size" "$((total_size / 1024))"

# Show what is excluded
printf "\nExcluded:\n"
printf "  lxqt/                    (1.7GB source tree)\n"
printf "  src/                     (15MB panel source)\n"
printf "  thirdparty/              (fetched by scripts)\n"
printf "  assets/inspiration/      (127MB design refs)\n"
printf "  apps/*/build/            (CMake artifacts)\n"
printf "  .git/                    (git history)\n"

# Prompt for confirmation
printf "\nProceed with archive creation? [Y/n] "
read -r confirm
case "$confirm" in
    n|N|no|NO)
        printf "Aborted.\n"
        exit 0
        ;;
esac

if [ "$DRY_RUN" = true ]; then
    printf "\n[DRY-RUN] Would create: %s\n" "$OUTPUT"
    for f in "${UNIQUE[@]}"; do
        printf "  %s\n" "$f"
    done
    exit 0
fi

printf "\nCreating archive...\n"

# Create temp directory
TMPDIR=$(mktemp -d)
ARCHIVE_ROOT="$TMPDIR/$PROJECT_NAME"
mkdir -p "$ARCHIVE_ROOT"

# Copy files preserving structure
for f in "${UNIQUE[@]}"; do
    dest="$ARCHIVE_ROOT/$f"
    mkdir -p "$(dirname "$dest")"
    cp "$f" "$dest" 2>/dev/null || true
done

# Create tarball
tar -czf "$OUTPUT" -C "$TMPDIR" "$PROJECT_NAME"

# Cleanup
rm -rf "$TMPDIR"

# ---- Report ----
ARCHIVE_SIZE=$(stat -c%s "$OUTPUT" 2>/dev/null || stat -f%z "$OUTPUT" 2>/dev/null || echo 0)
printf "\nArchive created: %s\n" "$OUTPUT"
printf "Size: %s bytes (%d KB)\n" "$ARCHIVE_SIZE" "$((ARCHIVE_SIZE / 1024))"
printf "Contents: %d files\n" "${#UNIQUE[@]}"

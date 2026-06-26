import ctypes
import os
import sys

def confirm_and_suspend():
    # 1. Check for administrative privileges
    if os.geteuid() != 0:
        print("Error: This script requires root privileges to execute raw syscalls.")
        print("Please run using: sudo python3 script_name.py")
        sys.exit(1)

    # 2. Prompt user for explicit validation
    print("WARNING: You are about to suspend the system via a direct kernel syscall.")
    print("Applications will not be warned to save open files. Unsaved data may be lost.")

    user_choice = input("Are you absolutely sure you want to proceed? (yes/No): ").strip().lower()

    if user_choice != 'yes':
        print("Operation cancelled safely.")
        sys.exit(0)

    # 3. Define Linux kernel architecture-specific constants (x86_64)
    SYS_reboot = 169
    LINUX_REBOOT_MAGIC1 = 0xfee1dead
    LINUX_REBOOT_MAGIC2 = 672274793
    LINUX_REBOOT_CMD_SW_SUSPEND = 0xd000fce2

    print("Sending suspend syscall to kernel...")

    # 4. Invoke the direct system call
    try:
        libc = ctypes.CDLL(None)
        libc.syscall(
            SYS_reboot,
            LINUX_REBOOT_MAGIC1,
            LINUX_REBOOT_MAGIC2,
            LINUX_REBOOT_CMD_SW_SUSPEND,
            None
        )
    except Exception as e:
        print(f"Failed to execute syscall: {e}")

if __name__ == "__main__":
    confirm_and_suspend()


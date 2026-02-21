# linux-char-driver

A production-style Linux kernel character device driver sample project.

## Project layout

```text
linux-char-driver/
+-- driver/
|   +-- Makefile
|   +-- char_driver.c
|   `-- char_driver.h
+-- user/
|   `-- test_app.c
+-- AI_GUIDE.md
`-- README.md
```

## What the driver implements

- Dynamic major/minor allocation (`alloc_chrdev_region`)
- Character device registration (`cdev_init`, `cdev_add`)
- File operations:
  - `open`
  - `release`
  - `read`
  - `write`
- Kernel-space buffer with mutex protection
- Kernel logging through `pr_info` and `pr_err`
- Basic error checking and cleanup on failure paths

## Build kernel module

```bash
cd driver
make
```

This generates `char_driver.ko`.

## Load and unload module

```bash
sudo insmod driver/char_driver.ko
dmesg | tail -n 20
sudo rmmod char_driver
```

For quick non-root testing:

```bash
sudo chmod 666 /dev/char0
```

For persistent permissions (udev rule):

```bash
echo 'KERNEL=="char0", MODE="0666"' | sudo tee /etc/udev/rules.d/99-char.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Create device node

If udev is active, `/dev/char0` is usually created automatically.

If not, create manually:

1. Find major number from `dmesg` (logged at module load).
2. Create node:

```bash
sudo mknod /dev/char0 c <major> 0
sudo chmod 666 /dev/char0
```

## Build and run user-space test app

```bash
cd user
gcc -Wall -Wextra -O2 -o test_app test_app.c
./test_app
./test_app "custom payload"
```

Expected behavior:
- `write()` sends text into the driver buffer.
- `lseek(fd, 0, SEEK_SET)` rewinds file position.
- `read()` fetches back what was written.

If you run as non-root and get `Permission denied`, apply the permission step above first.

## Key kernel concepts

- `struct file_operations`: Binds VFS calls to driver callbacks.
- `copy_to_user` / `copy_from_user`: Safe data transfer across kernel/user boundary.
- `dev_t` + major/minor: Kernel identifier for character device.
- `cdev`: Kernel object used to expose char device operations.
- `module_init` / `module_exit`: Entry/exit hooks for module lifecycle.

## Notes

- This is a single-device example for clarity.
- Extend with `poll`, `ioctl`, wait queues, and per-open state as needed.

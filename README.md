# KStore by WolfTech Innovations

KStore is a lightweight, TV-friendly application store designed specifically for Plasma Bigscreen. It provides a curated selection of Android applications—focusing on streaming services and remote-compatible tools—and facilitates their seamless installation via Waydroid.

Developed with a premium feel for non-Linux users, KStore brings the best of the Android ecosystem to your Linux-based smart TV experience.

## Key Features

- **Curated Android Catalog**: Discover popular streaming and TV-compatible apps.
- **One-Click Installation**: Seamlessly install APKs using Waydroid's backend.
- **TV-Optimized Interface**: Built using KDE Kirigami for a native Plasma Bigscreen look and feel.
- **Reliable Backend**: Robust asynchronous fetching of app metadata and asynchronous downloads.

## Prerequisites

To run or build KStore, you need:

- **Plasma Bigscreen** (recommended environment)
- **Waydroid** (for application installation)
- **KDE Frameworks 5** and **Qt 5**

## Build & Installation

### 1. Install Dependencies

On Ubuntu 24.04 or similar Debian-based systems, install the required development packages:

```bash
sudo apt update
sudo apt install -y \
    cmake \
    extra-cmake-modules \
    qtbase5-dev \
    qtdeclarative5-dev \
    qtquickcontrols2-5-dev \
    kirigami2-dev \
    libkf5i18n-dev \
    libkf5coreaddons-dev
```

### 2. Clone and Build

```bash
git clone https://github.com/WolfTech-Innovations/kstore.git
cd kstore
mkdir build && cd build
cmake ..
make
```

### 3. Run

After a successful build, you can run KStore directly from the build directory:

```bash
./kstore
```

## Running Tests

To verify the core functionality of the backend and network integration:

```bash
./test_backend
./test_functional
```

## How It Works

1. **Discovery**: KStore fetches app metadata directly from reputable sources.
2. **Download**: When you click "Install", KStore downloads the APK to a temporary location.
3. **Waydroid Integration**: Once downloaded, KStore invokes `waydroid app install <path_to_apk>` to perform the installation.

## License

KStore is released under the **MIT License**.

---
*Developed by WolfTech Innovations.*

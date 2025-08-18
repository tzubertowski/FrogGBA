# 🐸 FrogGBA v0.2.2 – Performance Breakthrough

![froggba22](https://github.com/user-attachments/assets/placeholder-image.png)

Major performance update inspired by gpSP Kai optimizations. This release brings significant FPS improvements in demanding games, eliminates micro-stutters, and makes the emulator more stable than ever.

---

### 📥 Install in 4 Steps

1. Download + unzip the release.
2. Copy the **PSP** folder to your Memory Stick root.
3. Drop `gba_bios.bin` into `PSP/GAME/FrogGBA/`.
4. Launch FrogGBA from your PSP Games menu (CFW required).

---

### 🆕 New in v0.2.2

* **gpSP Kai Optimizations** – Implemented proven performance techniques from the legendary gpSP Kai, resulting in noticeable FPS gains.
* **Fixed Multi-Layer Bottlenecks** – Games like Castlevania (water sections) now run at full speed with 4-layer Mode 0 rendering.
* **Eliminated Micro-Stutters** – Smart cache management prevents stuttering during level transitions in games like Kirby Amazing Mirror.
* **Optimized Memory Usage** – Reduced translation cache sizes to optimal levels (2MB ROM, 256KB RAM) for better PSP memory utilization.
* **Enhanced Audio Processing** – Improved audio buffering reduces crackling and provides smoother sound output.
* **Better Sprite Rendering** – Optimized sprite handling for games with many on-screen objects.
* **Clean Exit Fix** – No more crashes (BSOD) when exiting the emulator.

---

### ⚡ Performance Gains

* **Castlevania Series** – Water sections now run at full speed (was ~15-20 FPS, now 30+ FPS)
* **Golden Sun Series** – Battle effects and world map rendering improved
* **Fire Emblem** – Large battle animations no longer cause slowdown
* **Overall** – 10-25% performance improvement in most demanding titles

---

### 🎮 Carryover Features

* **Recent Games Menu** – Quick access to your last played titles
* **Lightning-Fast Overlays** – Full-screen borders with zero performance impact
* **Custom Overlay Support** – Up to 10 designs with pixel-perfect positioning
* **Online Overlay Generator** – Create your own at [froggba.onrender.com](https://froggba.onrender.com)
* **Multiple Aspect Ratios** – Choose your preferred display mode
* **Fast Color Correction** – Hardware-accelerated color processing

---

### 🧠 Technical Details

* Static translation caches for reduced memory fragmentation
* Cache invalidation reduction for smoother frame pacing
* Simplified tile rendering pipeline matching gpSP Kai approach
* Pre-warmed cache lines to reduce initial load stutters
* Optimized memory access patterns for PSP's architecture

---

### 🐛 Bug Fixes

* Fixed BSOD on emulator exit
* Eliminated micro-stutters during level transitions
* Resolved glitchy tile rendering in certain games
* Fixed recent games path handling
* Improved save/load memory reliability

---

### 💚 Compatibility

* Works on all PSP models (1000/2000/3000/Go)
* CFW required (any modern CFW works)
* Based on heavily modified TempGBA4PSP with gpSP Kai optimizations

---

**TL;DR:** FrogGBA v0.2.2 brings the performance optimizations from gpSP Kai to modern PSP homebrew. Enjoy full-speed emulation in previously slow games, butter-smooth transitions, and rock-solid stability. The best GBA experience on PSP just got even better. 🐸⚡

---

### Credits

* Original gpSP by Exophase
* gpSP Kai optimizations by takka
* TempGBA4PSP base
* FrogGBA improvements by the community

GPL v2+ Licensed
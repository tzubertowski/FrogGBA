# 🐸 FrogGBA v0.3.0

This release fixes critical performance and stability issues that have plagued recent versions. The main focus was restoring CPU optimizations that were previously disabled and resolving save state corruption that caused games to freeze or crash.

## 📥 Installation

1. Download and extract the release
2. Copy the PSP folder to your Memory Stick root
3. Place gba_bios.bin in PSP/GAME/FrogGBA/
4. Launch from PSP Games menu (requires CFW)

## ⚠️ Pokemon Unbound Compatibility Note

This game requires specific settings to run properly:
- In the game's audio menu, set to "Low Sound Quality" and "Mono"
- Alternatively, disable background music entirely
- Without these changes, expect severe performance issues

## 🆕 Major Changes

### Performance Improvements

The emulator now uses PSP's volatile memory partition (4MB extra RAM available on PSP 2000 and later models) for critical CPU caches. This approach, inspired by the DaedalusX64 N64 emulator, provides 2MB for ROM translation cache and 512KB for RAM translation cache with direct memory access.

Note: This is NOT the extended memory layout. PSP 1000 models are not supported as they lack the volatile memory partition.

### CPU Optimization Restoration

Previously disabled optimizations have been restored after fixing the underlying memory corruption issues:
- Dynamic recompilation with proper MIPS32 instruction scheduling
- Branch prediction and optimization
- Memory access pattern fixes
- 32-byte aligned functions for cache efficiency

### Save State Overhaul

Save states were completely broken in recent versions, causing freezes, infinite loading, and "bad jump" errors. The system has been rewritten with:
- Delayed loading mechanism - states load after game initialization instead of during boot
- Fixed infinite recursion in ROM page loading that caused stack overflows
- Proper file handle restoration after state loads
- Boundary checking for oversized page requests

### Auto-Resume Fixes

The auto-resume feature would hang games during the "searching backup id" phase. This has been fixed by:
- Loading save states after 60 frames instead of immediately
- Separating boot resume and manual loading code paths
- Ensuring consistent behavior regardless of how games are loaded

## 🐛 Bug Fixes

### Critical Issues
- Save states causing system crashes and bad jump errors
- Games hanging indefinitely when existing save states were present
- First manual game load ignoring save states
- Memory alignment crashes causing graphical corruption
- Overlay menu showing no files
- Stack overflow from infinite recursion in page loading

### Other Fixes
- File handle cleanup and restoration
- CPU register preservation after state loads
- Memory leak prevention when switching games
- Instruction cache invalidation issues

## 🔧 Technical Details

The volatile memory implementation allocates performance-critical caches outside the main RAM partition, providing zero-copy access to translation caches. This technique was adapted from DaedalusX64's approach to utilizing the PSP's hardware capabilities.

Save state loading has been moved from the initialization phase to the main game loop, executing after approximately one second of gameplay. This prevents corruption of the emulation context that occurred when states were loaded too early.

## 💚 Compatibility

- Requires PSP 2000/3000/Go (volatile memory partition required)
- PSP 1000 not supported
- Any modern CFW
- Based on TempGBA4PSP with restored gpSP Kai optimizations

## 🔍 Known Issues

- Pokemon Unbound requires audio adjustments (see note above)
- Some ROM hacks may need custom settings
- Old save states may not be compatible

## Credits

- Original gpSP by Exophase
- gpSP Kai optimizations by takka
- TempGBA4PSP base
- Volatile memory technique from DaedalusX64
- FrogGBA community contributions

GPL v2+ Licensed
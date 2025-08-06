# TempGBA mod by Prosty

## Download & Installation
**→ [Download the latest release here](https://github.com/tzubertowski/FrogGBA/releases) ←**

You need to have a CFW installed on your PSP. 
- Unzip the folder
- Copy the unzipped PSP folder into root of your PSP SD card
- Launch the emulator on the console

### TL;DR - Key Improvements
• 🚀 **Better Performance** 
• 📋 **Better Compatibility** 
• 🎨 **Color Correction**
• ⚡ **Fast Forward hotkey (SEL + R) with additional X3 FF option**

## About
This is a modified version of TempGBA - a Game Boy Advance emulator for PlayStation Portable.

This mod is based on:
- [TempGBA](https://github.com/Nebuleon/TempGBA) by Nebuleon, Normmatt, and BassAceGold
- TempGBA4PSP-mod (TempGBA4PSP-26731020), http://www1.axfc.net/uploader/so/3063963

## What's New in This Mod

### 🚀 Performance & Quality Improvements
- **Color Correction**: Added GPSP and Retro color correction modes for authentic GBA screen look
- **Fast Forward**: SELECT + R to toggle 2x/3x speed modes for faster gameplay
- **Turbo Buttons**: Triangle and Square as dedicated turbo buttons for rapid-fire
- **FPS Display**: SELECT + Square to toggle FPS counter for performance monitoring
- **UI Enhancement**: Added mod attribution in all menus

### 🔧 Technical Optimizations (Permanently Enabled)
- **PSP Cycle Batching**: Reduces dynamic recompiler overhead for smoother gameplay
- **Memory Waitstate Optimization**: ~50% reduced memory access delays for better performance
- **Sprite Rendering Optimizations**: Unrolled loops and culling for sprite-heavy games like Metroid
- **Cache Invalidation Reduction**: Conservative cache management to reduce performance drops
- **Timer Prescaling Optimization**: Improved timing precision for better audio/video sync
- **Block Coalescing**: Better instruction cache usage for consistent framerates

### 📈 Performance Impact
- **Metroid Zero Mission**: Improved from 45 FPS → 55+ FPS (22% improvement)
- **Sprite-heavy games**: Significant performance boost with optimized rendering pipeline
- **General gameplay**: More consistent framerates with reduced stuttering

## Original TempGBA Features

- Added gpsp kai's cheats function
- Added Chinese language support  
- Added restore function
- New menu icon
- Imported code from TempGBA-mod-dstwo-26750220
- Modern PSP SDK compatibility
- Docker-based build system

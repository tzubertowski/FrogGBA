# FrogGBA Build Instructions

This repository includes Docker setup files to build the FrogGBA emulator for PlayStation Portable.

## Files

- `Dockerfile` - Docker image with PSP development environment
- `docker-compose.yml` - Docker Compose configuration
- `build.sh` - Build script that compiles the project

## Building with Docker

### Prerequisites

- Docker
- Docker Compose

### Option 1: Using Docker Compose (Recommended)

```bash
# Build the Docker image and compile the project
docker-compose up --build

# The compiled files will be in the build/ directory
```

### Option 2: Using Docker directly

```bash
# Build the Docker image
docker build -t froggba-psp .

# Run the container and compile
docker run --rm -v $(pwd):/project froggba-psp /project/build.sh

# The compiled files will be in the build/ directory
```

### Option 3: Interactive development

```bash
# Start an interactive container
docker-compose run --rm psp-dev /bin/bash

# Inside the container, run:
./build.sh
```

## Output

If successful, you'll find these files in the `build/` directory:

- `FrogGBA.prx` - The compiled PSP module
- `EBOOT.PBP` - The PSP executable file

## Installation on PSP

1. Copy `EBOOT.PBP` to your PSP's `PSP/GAME/FrogGBA/` folder
2. Place GBA ROM files in the same directory
3. Launch from the PSP's Game menu

## Troubleshooting

- If the build fails, check the Docker logs for specific error messages
- Make sure you have enough disk space for the Docker image (~2GB)
- The initial build may take 30-60 minutes as it compiles the entire PSP toolchain
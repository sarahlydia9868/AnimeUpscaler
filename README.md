# AnimeUpscaler

A native Windows x64 desktop application for upscaling anime videos using Anime4K shaders with real-time GPU encoding.

Powered by FFmpeg (libplacebo), Vulkan, and wxWidgets.

## Requirements

- **OS:** Windows x64
- **GPU:** NVIDIA (with NVENC) or AMD (with AMF)
- **FFmpeg:** Custom build with `libplacebo` support

## FFmpeg Setup

Place FFmpeg binaries in the following location (same directory as the `.exe`):

```
ffmpeg/ffmpeg.exe
ffmpeg/ffprobe.exe
```

FFmpeg must be compiled with `libplacebo` support to enable shader-based upscaling. You can download a pre-built FFmpeg build with libplacebo from [FFmpeg Builds](https://www.gimberno.de/) or build it yourself.

> [!IMPORTANT]
> Standard FFmpeg builds from major distributors (ffmpeg.org, Gyan.dev, BtbN) do NOT include libplacebo and will not work.

## Shaders Setup

Place Anime4K GLSL in the following location (same directory as the `.exe`):

```
shaders/Anime4K_ModeA.glsl
shaders/Anime4K_ModeB.glsl
...
```

## Project Structure

```
AnimeUpscaler/
├── src/
│   ├── app/          Application entry point
│   ├── config/       Configuration management
│   ├── ui/           Main window and UI components
│   ├── utils/        Process execution wrapper
│   └── video/        Video model, loader, and processor
├── resources/        Application icon (app.ico)
├── CMakeLists.txt    CMake build configuration
```

## Usage

1. Launch the application.
2. Drag & drop video files onto the list, or click **Add**.
3. Select your Anime4K shader, output container (MP4/MKV), codec (H.264/H.265), and GPU backend.
4. Set the output resolution (up to 4K).
5. Click **Start** to begin processing. The button changes to **Cancel** during processing.

## Output

- Output files are saved in the **same folder as the input**, with `_upscaled` suffix appended.
- **MKV** preserves subtitles, attachments, and chapter metadata.
- **MP4** strips subtitles and attachments for maximum compatibility.

## Building from Source

### Prerequisites

- CMake 3.20+
- C++ compiler: MSVC

### Build

```bash
mkdir build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

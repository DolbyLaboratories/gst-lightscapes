# gst-lightscapes

## What It Is
GStreamer plugins collection for building Lightscapes media pipeline.

> :warning: Plugins require proprietary libraries to initialize properly.

## Dependencies
 - [Python](https://python.org) 3.12 or later
 - [Ninja](https://ninja-build.org) 1.11 or later
 - [Meson](https://mesonbuild.com/) 1.11 or later
 - [GStreamer](https://gstreamer.freedesktop.org/) 1.28.5, with the Lightscapes `qtdemux` patch
 - [GLib](https://docs.gtk.org/glib/) and [json-glib](https://gitlab.gnome.org/GNOME/json-glib)

### Lightscapes `qtdemux` patch
Lightscapes tracks in MP4 files (`lscp` and `urim` sample entries) are only
exposed by the `qtdemux` element of GStreamer (`isomp4` plugin) when it is
patched with [gstreamer_qtdemux_lscp_1.28.5.patch](gstreamer_qtdemux_lscp_1.28.5.patch).
The patch passes the track configuration boxes to `dlblsmparse`, which needs
them to initialize.

> :warning: The stock `qtdemux` shipped with Linux distribution packages,
> Homebrew, or the official GStreamer installers is **not** sufficient. The
> plugins build and register against it, but `dlblsmparse` reports
> *"No valid metadata found to initialise LSM capabilities"* and no light data
> is produced.

The following instructions build GStreamer 1.28.5 from source on Linux
and macOS, and rebuild only the patched `isomp4` plugin on Windows.

## Building

### Linux
On Debian-based systems (for example Ubuntu 24.04), install the build tools and dependencies:
```console
$ sudo apt-get update
$ sudo apt-get install build-essential pkg-config flex bison git ca-certificates \
    ninja-build pipx libglib2.0-dev libjson-glib-dev
$ pipx install meson
$ pipx ensurepath
```

More on installing Meson build can be found at the
[Meson quickstart guide](https://mesonbuild.com/Quick-guide.html).

1. Build the patched GStreamer and point your environment at it, as described
   in [Patched GStreamer (Linux and macOS)](#patched-gstreamer-linux-and-macos).

2. Configure with Meson and build with Ninja, from the root of this repository:
```console
$ meson setup build
$ ninja -C build
```

### macOS
Install the build tools and dependencies with [Homebrew](https://brew.sh/):
```console
$ brew install meson ninja pkg-config git glib json-glib flex bison
$ export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
```

The `flex` and `bison` packages of Homebrew are not linked into the default
`PATH`, and the `bison` shipped with macOS is too old to build GStreamer.

1. Build the patched GStreamer and point your environment at it, as described
   in [Patched GStreamer (Linux and macOS)](#patched-gstreamer-linux-and-macos).

2. Configure with Meson and build with Ninja, from the root of this repository:
```console
$ meson setup build
$ ninja -C build
```

### Windows (MSVC)

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/downloads/)
   (or Build Tools for Visual Studio 2022) with the **Desktop development with C++** workload.

2. Install [Python](https://python.org) 3.12 or later and
   [Git for Windows](https://git-scm.com/download/win), then install **Meson** & **Ninja**:
```console
> pip install meson ninja
```

3. Install **GStreamer**

From the [Download GStreamer](https://gstreamer.freedesktop.org/download/) page,
download and run the MSVC 64-bit 1.28.5 installer. Starting with version 1.28,
a single installer contains both the runtime and the development files.

> :information_source: Install to a path without spaces, e.g.
> `C:\gstreamer\1.0\msvc_x86_64`. With the default location under
> `C:\Program Files`, pkg-config escapes the space in some tool paths and Meson
> cannot find them.

> :warning: The remaining steps require the
> [x64 Native Tools Command Prompt for VS 2022](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170),
> which is installed with Visual Studio.

In that prompt, point the build at the installed GStreamer:
```console
> set GST_ROOT=C:\gstreamer\1.0\msvc_x86_64
> set PATH=%GST_ROOT%\bin;%PATH%
> set PKG_CONFIG_PATH=%GST_ROOT%\lib\pkgconfig
```

4. Build and install the **patched `isomp4` plugin**

Build only the `isomp4` plugin from the GStreamer 1.28.5 sources with the
Lightscapes patch applied:
```console
> git clone --depth 1 --branch 1.28.5 --config core.autocrlf=false https://gitlab.freedesktop.org/gstreamer/gstreamer.git
> cd gstreamer
> git apply <path_to_your_repo_clone>\gst-lightscapes\gstreamer_qtdemux_lscp_1.28.5.patch
> meson setup build_isomp4 -Dauto_features=disabled -Ddoc=disabled -Dtests=disabled ^
    -Dbase=disabled -Dbad=disabled -Dugly=disabled ^
    -Dgstreamer:check=enabled -Dgst-plugins-good:isomp4=enabled
> meson compile -C build_isomp4
```

Replace the installed plugin with the patched one, keeping a backup of the original:
```console
> copy %GST_ROOT%\lib\gstreamer-1.0\gstisomp4.dll %GST_ROOT%\lib\gstreamer-1.0\gstisomp4.dll.orig
> copy /Y build_isomp4\subprojects\gst-plugins-good\gst\isomp4\gstisomp4.dll %GST_ROOT%\lib\gstreamer-1.0\
```

5. Build & install **gst-lightscapes plugins**

Navigate to the root folder of your clone of the gst-lightscapes project, and execute commands:
```console
> meson setup build
> ninja -C build
> ninja -C build install
```
6. By default, ninja installs built assets in **c:\bin** and **c:\lib**. To make them available to the Gstreamer application do:
- Add **c:\bin** and **c:\lib** to the **PATH** env variable.
```console
> set PATH=%PATH%;c:\bin;c:\lib
```
- Add **c:\lib** to the **GST_PLUGIN_PATH** env variable
```console
> set GST_PLUGIN_PATH=c:\lib
```

### Patched GStreamer (Linux and macOS)
Build GStreamer 1.28.5 from the
[GStreamer mono repo](https://gitlab.freedesktop.org/gstreamer/gstreamer) with
the Lightscapes `qtdemux` patch applied, and install it into a private prefix
so it does not interfere with any GStreamer installed on the system.

Clone GStreamer and apply the patch
```console
$ git clone --depth 1 --branch 1.28.5 https://gitlab.freedesktop.org/gstreamer/gstreamer.git
$ cd gstreamer
$ git apply /path/to/your/clone/gst-lightscapes/gstreamer_qtdemux_lscp_1.28.5.patch
```

Build and install into a private prefix (e.g. `$HOME/gst-prefix`)
```console
$ meson setup build --prefix "$HOME/gst-prefix" --libdir lib \
    -Dbad=disabled -Dugly=disabled -Dlibav=disabled \
    -Ddevtools=disabled -Drtsp_server=disabled -Dges=disabled \
    -Dgst-examples=disabled -Dtls=disabled -Dlibnice=disabled \
    -Dwebrtc=disabled -Dqt5=disabled -Dqt6=disabled \
    -Dbenchmarks=disabled -Dnls=disabled \
    -Dintrospection=disabled -Dpython=disabled \
    -Ddoc=disabled -Dtests=disabled -Dexamples=disabled \
    -Dgst-plugins-good:isomp4=enabled
$ meson compile -C build
$ meson install -C build
```

Point the environment at the private prefix (on macOS, use `DYLD_LIBRARY_PATH`
instead of `LD_LIBRARY_PATH`)
```console
$ export GST_PREFIX_DIR="$HOME/gst-prefix"
$ export PATH="${GST_PREFIX_DIR}/bin:${PATH}"
$ export PKG_CONFIG_PATH="${GST_PREFIX_DIR}/lib/pkgconfig"
$ export LD_LIBRARY_PATH="${GST_PREFIX_DIR}/lib"
$ export GST_PLUGIN_SYSTEM_PATH_1_0="${GST_PREFIX_DIR}/lib/gstreamer-1.0"
```

`GST_PLUGIN_SYSTEM_PATH_1_0` makes GStreamer load its plugins only from the
private prefix, so an unpatched system `qtdemux` is never picked up.

## Running
First we have to tell GStreamer where to look for the newly built plugins.
On Linux and macOS:
```console
$ export GST_PLUGIN_PATH=/path/to/your/clone/gst-lightscapes/build/plugins
```

On Windows, the plugins are installed with `ninja -C build install` and found
through `GST_PLUGIN_PATH` as described in the [Windows](#windows-msvc) section.

Test if GStreamer can properly retrieve information about the plugins
```console
$ gst-inspect-1.0 dlblightning
```

Check that the patched `qtdemux` is the one in use. The patch adds the
`lumn_%u` pad name to the `isomp4` plugin binary, so search the binary for it.
`gst-inspect-1.0 qtdemux` does not list `lumn_%u`, because the patch creates
that pad only at runtime and never registers it as a pad template.

On Linux and macOS, search the plugin file that GStreamer actually loads:
```console
$ strings -a "$(gst-inspect-1.0 qtdemux | awk '/Filename/ {print $2}')" | grep lumn_
```

On Windows, search the installed plugin:
```console
> findstr /m lumn_ "%GST_ROOT%\lib\gstreamer-1.0\gstisomp4.dll"
```

Both commands must print a match. This confirms the patched plugin is
installed; it does not prove that a real Lightscapes file is parsed correctly.

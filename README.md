# gst-lightscapes

## What It Is
GStreamer plugins collection for building Dolby Lightscapes media pipeline.

> :warning: Plugins require proprietary libraries to initialize properly.

## Dependencies
 - [Python](https://python.org) 3.9 or later
 - [Ninja](https://ninja-build.org) 1.7 or later
 - [Meson](https://mesonbuild.com/) 0.58 or later
 - [GStreamer](https://gstreamer.freedesktop.org/) 1.16.2 or later
 - more, see building section...

## Building

### Linux
On Debian-based systems, install the following dependencies:
```console
$ apt-get install ninja-build
$ pip3 install meson
```

More on installing Meson build can be found at the
[Meson quickstart guide](https://mesonbuild.com/Quick-guide.html).

Get other dependencies.
```console
$ apt-get install \
    libglib2.0-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-0 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    libjson-glib-dev
```

Then configure with Meson, and build with Ninja.
```console
$ meson setup build
$ ninja -C build
```

### Windows (MSVC)

1. Install Microsoft [Visual Studio 2019](https://my.visualstudio.com/Downloads?q=Visual%20Studio%202019)

2. Install **Meson** & **Ninja** 

Download and install latest meson-x.y.z.msi installation package from [Meson Github](https://github.com/mesonbuild/meson/releases).
source: [The Absolute Beginner's Guide to Installing and Using Meson](https://mesonbuild.com/SimpleStart.html) 

3. Install **GStreamer dependencies**

From the [Download Gstreamer](https://gstreamer.freedesktop.org/download/) page, download and install MSVC 64-bit (VS 2019, Release CRT):
- 1.22.4 runtime installer (or newer)
- 1.22.4 development installer (or newer)

4. Build & install **Dolby Gstreamer plugins**

> :warning: This step requires [x64 Native Tools Command Prompt for VS 2019](https://learn.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=msvc-170) which is installed with the Visual Studio IDE.

Open **x64 Native Tools Command Prompt for VS 2019** and navigate to the root folder of your clone of the gst-lightscapes project, and execute commands:
```console
> meson setup build
> ninja -C build
> ninja -C build install
```
5. By default, ninja installs built assets in **c:\bin** and **c:\lib**. To make them available to the Gstreamer application do:
- Add **c:\bin** and **c:\lib** to the **PATH** env variable.
```console
> set PATH=%PATH%;c:\bin;c:\lib
```
- Add **c:\lib** to the **GST_PLUGIN_PATH** env variable
```console
> set GST_PLUGIN_PATH=c:\lib
```

### macOS
You can install GStreamer dependencies using [installer](https://gstreamer.freedesktop.org/data/pkg/osx/) from GStreamer project.
or with [Homebrew](https://brew.sh/).

## Building from source

> :warning: Most people will not need to build Gstreamer from sources because the [pre-build packages](https://gstreamer.freedesktop.org/documentation/frequently-asked-questions/general.html?gi-language=c) will be available.

GStreamer and the plugins can be built from source by using
[gst-build](https://gitlab.freedesktop.org/gstreamer/gst-build), or starting
from GStreamer 1.20, [GStreamer mono repo](https://gitlab.freedesktop.org/gstreamer/gstreamer).

Install `meson` and `ninja` as instructed in the [Linux](#linux),
[Windows](#windows), and [macOS](#macos) sections above.

On Linux systems additional dependencies may be required:
```console
$ apt-get install flex bison libmount-dev
```

Clone the `gst-build` repository, specifying a stable version
```console
$ git clone --depth 1 --branch 1.18.4 https://gitlab.freedesktop.org/gstreamer/gst-build.git gst-build
```

Copy the contents of this repository into `gst-build/subprojects`
```console
$ cp -R /path/to/your/clone/gst-lightscapes gst-build/subprojects
```

Then build the project
```console
$ cd gst-build
$ meson builddir -Dcustom_subprojects=gst-lightscapes -Dauto_features=disabled -Dgstreamer:tools=enabled
$ ninja -C builddir
```

**Note:** *On macOS, an additional `-Dcpp_std=c++17` flag is needed to build the project*
```console
$ cd gst-build
$ meson builddir -Dcustom_subprojects=gst-lightscapes -Dauto_features=disabled -Dgstreamer:tools=enabled -Dcpp_std=c++17
$ ninja -C builddir
```

These commands will build the `gst-lightscapes` plugins, along with GStreamer
core, basic plugins, and tools such as `gst-launch-1.0`, `gst-inspect-1.0` etc.

The built plugins can be found in
`gst-build/builddir/subprojects/gst-lightscapes/plugins`

To enter development environment run
```console
$ ninja -C builddir devenv
```

## Running
First we have to tell GStreamer where to look for the newly build plugins:

**Note:** *On Windows, install plugins with `ninja -C builddir
install` rather than point them via environment variable.*

```console
$ export GST_PLUGIN_PATH=/path/to/your/clone/gst-lightscapes/build/plugins
```

Test if GStreamer can properly retrieve information about the plugins
```console
$ gst-inspect-1.0 dlblightning
```

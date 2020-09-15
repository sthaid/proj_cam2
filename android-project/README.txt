==============================
INTRO
==============================

Date Sep 15, 2020

This is a rough guide on building and installing SDL Android Apps.
This guide's focus is installing the Viewer App, built from the 
source code in the parent directory.

My development system is Fedora 31.
My device is Motorola's Moto G Power.

Reference Web Sites
- SDL
    https://wiki.libsdl.org/Android
    https://www.libsdl.org/projects/SDL_ttf/
    https://www.libsdl.org/download-2.0.php
- Android
    https://developer.android.com/studio/command-line/sdkmanager
    https://developer.android.com/studio/run/device

==============================
CONFIGURING THE DEVELOPMENT SYSTEM
==============================

Using snap, get androidsdk tool (aka sdkmanager):
- refer to: https://snapcraft.io/install/androidsdk/fedora
- install snap
    sudo dnf install snapd
    sudo ln -s /var/lib/snapd/snap /snap
- install androidsdk
    sudo snap install androidsdk

Using androidsdk (aka sdkmanager):
- https://developer.android.com/studio/command-line/sdkmanager
- some of the commands
  --install
  --uninstall
  --update
  --list
  --version
- what I installed ...
    androidsdk --install "platforms;android-30"     # Android SDK Platform 30
    androidsdk --install "build-tools;30.0.2"
    androidsdk --install "ndk;21.3.6528147"
    androidsdk --list

    Installed packages:
    Path                 | Version      | Description                     | Location
    -------              | -------      | -------                         | -------
    build-tools;30.0.2   | 30.0.2       | Android SDK Build-Tools 30.0.2  | build-tools/30.0.2/
    emulator             | 30.0.26      | Android Emulator                | emulator/
    ndk;21.3.6528147     | 21.3.6528147 | NDK (Side by side) 21.3.6528147 | ndk/21.3.6528147/
    patcher;v4           | 1            | SDK Patch Applier v4            | patcher/v4/
    platform-tools       | 30.0.4       | Android SDK Platform-Tools      | platform-tools/
    platforms;android-30 | 3            | Android SDK Platform 30         | platforms/android-30/

Bash_profile:
- add the following
    # Android
    export PATH=$PATH:$HOME/snap/androidsdk/30/AndroidSDK/ndk/21.3.6528147
    export PATH=$PATH:$HOME/snap/androidsdk/30/AndroidSDK/tools
    export PATH=$PATH:$HOME/snap/androidsdk/30/AndroidSDK/platform-tools
    export ANDROID_HOME=$HOME/snap/androidsdk/30/AndroidSDK
    export ANDROID_NDK_HOME=$HOME/snap/androidsdk/30/AndroidSDK/ndk/21.3.6528147

Install additional packages:
- java -version                               # ensure java is installed, and check the version
- sudo yum install ant                        # install Java Build Tool  (ant)
- sudo yum install java-1.8.0-openjdk-devel   # install java devel (matching the java installed)

==============================
CONNECTING DEVICE TO DEVEL SYSTEM
==============================

Udev rule is needed to connect to the Android device over usb:
- lsusb | grep Moto
  Bus 003 Device 025: ID 22b8:2e81 Motorola PCS moto g power
- create   /etc/udev/rules.d/90-android.rules
  SUBSYSTEM=="usb", ATTRS{idVendor}=="22b8", MODE="0666"
- udevadm control --reload-rules

Enable Developer Mode on your device
- Settings > About Device:
  - Tap Build Number 7 times
- Settings > System > Advanced > Developer Options:
  - Turn on Developer Options using the slider at the top (may already be on).
  - Enable USB Debugging.
  - Enable Stay Awake.

Connect Device to Computer
- Plug in USB Cable; when asked to Allow USB Debugging, select OK.
- On comuter, use 'adb devices' to confirm connection to device. 
  Sample output:
    List of devices attached
    ZY227NX9BT	device

==============================
BUILDING A SAMPLE SDL APP AND INSTALL ON DEVICE
==============================

Build a Sample SDL App, and install on device
- SDL2-2.0.12.tar.gz - download and extract
     https://www.libsdl.org/download-2.0.php
- Instructions to build a test App (refer to https://wiki.libsdl.org/Android):
  $ cd SDL2-2.0.12/build-scripts/
  $ ./androidbuild.sh org.libsdl.testgles ../test/testgles.c
  $ cd $HOME/android/SDL2-2.0.12/build/org.libsdl.testgles
  $ ./gradlew installDebug

This sample App will be installed as 'Game' App on your device.
Try running it.

==============================
BUILD VIEWER APP - OUTILINE OF STEPS
==============================

NOTE: Refer to the section below for a script that performs these steps.

Building the Viewer Program:
- download and extract SDL2-2.0.12.tar.gz
- create a template for building the viewer
    $ ./androidbuild.sh org.sthaid.viewer stub.c
- Note: the following dirs are relative to SDL2-2.0.12/build/org.sthaid.viewer
- in dir app/jni:
  - add additional subdirs, with source code and Android.mk, for additional libraries 
    that are needed (SDL2_ttf-2.0.15 and jpeg8d)
- in dir app/jni/src: 
  - add symbolic links to the source code needed to build viewer
  - in Android.mk, set LOCAL_C_INCLUDES, LOCAL_SRC_FILES, and LOCAL_SHARED_LIBRARIES
- in dir app/src/main:
  - update AndroidManifest.xml
    - add: <uses-permission android:name="android.permission.INTERNET" />
    - add to viewerActivity: android:screenOrientation="landscape"
- in dir app/src/main/res/values
  - update strings.xml with the desired AppName
- in dir .
  - run ./gradlew installDebug, to build and install the app on the device

==============================
BUILD VIEWER APP - USING SHELL SCRIPT
==============================

First follow the steps in these sections:
- CONFIGURING THE DEVELOPMENT SYSTEM
- CONNECTING DEVICE TO DEVEL SYSTEM

Sanity Checks:
- Be sure to have installed ant and java-x.x.x-openjdk-devel.
- Verify the following programs are in the search path:
  ant, adb, ndk-build.
- Run 'adb devices' and ensure your device is shown, example output:
    List of devices attached
    ZY227NX9BT	device
- In another terminal run 'adb shell logcat -s SDL/APP'. 
  Debug prints should be shown once the viewer app is built, installed, and run.

Setup the build directory structure:
- Run do_setup.
  This should create the SDL2-2.0.12, and SDL2-2.0.12/build/org.sthaid.viewer dirs.

Build and install on your device
- Run do_build_and_install
  The installed app name is 'Viewer', with the SDL icon.

==============================
SOME ADB COMMANDS, USED FOR DEVELOPMENT & DEBUGGING
==============================

Sample adb commands for development & debugging:
- adb devices                               # lists attached devices
- adb shell logcat -s SDL/APP               # view debug prints
- adb install -r ./app/build/outputs/apk/debug/app-debug.apk  
                                            # install (usually done by gradle)
- adb uninstall  org.sthaid.viewer          # uninstall
- adb shell getprop ro.product.cpu.abilist  # get list of ABI supported by the device

==================================================================================
==============================  MORE INFO  =======================================
==================================================================================

==============================
APPENDIX - SNAP
==============================

How to install androidsdk tool using snap:
- refer to: https://snapcraft.io/install/androidsdk/fedora
- steps
  - sudo dnf install snapd
  - sudo ln -s /var/lib/snapd/snap /snap
  - sudo snap install androidsdk

From 'man snap'
   The snap command lets you install, configure, refresh and remove snaps.
   Snaps are packages that work across many different Linux distributions, 
   enabling secure delivery and operation of the latest apps and utilities.

Documentation
- man snap
- snap help

Examples:
- snap find androidsdk
    Name        Version  Publisher   Notes  Summary
    androidsdk  1.0.5.2  quasarrapp  -      The package contains android sdkmanager.
- snap install androidsdk

==============================
APPENDIX - ANDROIDSDK
==============================

Documentation: https://developer.android.com/studio/command-line/sdkmanager

Installation:
- I installed the androidsdk using snap. 
- The same program (except called sdkmanager) is also found in
    snap/androidsdk/30/AndroidSDK/tools/bin/sdkmanager 
  after installing the AndroidSDK.
- I suggest just using the 'androidsdk' version that was installed using snap.

Description:
- The sdkmanager is a command line tool that allows you to view, install, update, 
  and uninstall packages for the Android SDK.

Examples:
- androidsdk --help
- androidsdk --list
- androidsdk --install "platforms;android-30"

==============================
APPENDIX - ADB (ANDROID DEBUG BRIDGE)
==============================

adb
- options
  -d  : use usb device  (not needed if just one device is connected)
  -e  : use emulator
- examples
  adb help
  adb install  ./app/build/outputs/apk/debug/app-debug.apk
  adb uninstall  org.libsdl.testgles
  adb logcat
  adb shell [<command>]

adb shell command examples, run 'adb shell'
- General Commands
    ls, mkdir, rmdir, echo, cat, touch, ifconfig, df
    top
      -m <max_procs>
    ps
      -t show threads, comes up with threads in the list
      -x shows time, user time and system time in seconds
      -P show scheduling policy, either bg or fg are common
      -p show priorities, niceness level
      -c show CPU (may not be available prior to Android 4.x) involved
      [pid] filter by PID if numeric, or…
      [name] …filter by process name
- Logging
    logcat -h
    logcat                  # displays everything
    logcat -s Watchdog:I    # displays log from Watchdog
    logcat -s SDL/APP       # all from my SDL APP
     Priorities are:
         V    Verbose
         D    Debug
         I    Info
         W    Warn
         E    Error
         F    Fatal
         S    Silent (suppress all output)
      for example "I" displays Info level and below (I,W,E,F)
- Device Properties
    getprop
    getprop ro.build.version.sdk
    getprop ro.product.cpu.abi
    getprop ro.product.cpu.abilist
    getprop ro.product.device
- Proc Filesystem
    cat /proc/<pid>/cmdline


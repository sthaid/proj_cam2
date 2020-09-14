==============================
INTRO
==============================

Reference Web Sites
- https://wiki.libsdl.org/Android
- https://fedoramagazine.org/start-developing-android-apps-on-fedora-in-10-minutes/
- https://fedoramagazine.org/start-developing-android-apps-on-fedora-in-10-minutes/
- https://developer.android.com/studio/command-line/sdkmanager
- https://developer.android.com/studio/run/device
- https://fedoraproject.org/wiki/HOWTO_Setup_Android_Development

==============================
CONFIGURING THE DEVELOPMENT SYSTEM
==============================

Get androidsdk tool, using snap
- https://snapcraft.io/install/androidsdk/fedora
- install snap
    sudo dnf install snapd
    sudo ln -s /var/lib/snapd/snap /snap
- install androidsdk
    sudo snap install androidsdk

Using androidsdk  (aka sdkmanager)
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

Bash_profile
- add the following
    # Android
    export PATH=$PATH:$HOME/snap/androidsdk/30/AndroidSDK/ndk/21.3.6528147
    export PATH=$PATH:$HOME/snap/androidsdk/30/AndroidSDK/tools
    export PATH=$PATH:$HOME/snap/androidsdk/30/AndroidSDK/platform-tools
    export ANDROID_HOME=$HOME/snap/androidsdk/30/AndroidSDK
    export ANDROID_NDK_HOME=/home/haid/snap/androidsdk/30/AndroidSDK/ndk/21.3.6528147
- XXX TODO later recheck the path because it has 2 of '.' and 'bin'

Packages needed
- java -version                               # ensure java is installed, and check the version
- sudo yum install ant                        # install Java Build Tool  (ant)
- sudo yum install java-1.8.0-openjdk-devel   # install java devel (matching the java installed)

==============================
CONNECTING DEVICE TO DEVEL SYSTEM
==============================

Udev rules needed to connect to the Android device over usb
- lsusb | grep Moto
  Bus 003 Device 025: ID 22b8:2e81 Motorola PCS moto g power
- create   /etc/udev/rules.d/90-android.rules
  SUBSYSTEM=="usb", ATTRS{idVendor}=="22b8", MODE="0666"
- udevadm control --reload-rules

Enable Developer Mode on your device
- Settings > About Device:
  - Tap Build Number 7 times
- Settings > System > Advanced > Developer Options:
  - Turn on Developer Options using the slider at the top;
    note that it may already be turned on
  - Enable USB Debugging
  - Enable Stay Awake
- Plug in USB Cable, and when asked to Allow USB Debugging, select OK.
- use 'adb devices' to confirm
    List of devices attached
    ZY227NX9BT	device

==============================
BUILDING A SAMPLE PROGRAM AND INSTALL ON DEVICE
==============================

Build a Sample SDL Program, and install on device
- SDL2-2.0.12.tar.gz - download and extract
     https://www.libsdl.org/download-2.0.php
- Follow instructions to make a test build
     https://wiki.libsdl.org/Android
  $ cd SDL2-2.0.12/build-scripts/
  $ ./androidbuild.sh org.libsdl.testgles ../test/testgles.c
        To build and install to a device for testing, run the following:
        cd /home/haid/android/SDL2-2.0.12/build/org.libsdl.testgles
        ./gradlew installDebug
  $ cd /home/haid/android/SDL2-2.0.12/build/org.libsdl.testgles
  $ ./gradlew installDebug

XXX run the pgm
XXX adb commands to uninstall

==============================
BUILD VIEWER APP - OUTILINE
==============================

Build the Viewer Program, outiline of steps
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

XXX tbd

==============================
DEBUGGING THE APP
==============================

Debugging the App
- adb devices                               # lists attached devices
- adb shell getprop ro.product.cpu.abilist  # get list of ABI supported by the device
- adb shell logcat -s SDL/APP               # view debug prints
- adb install -r ./app/build/outputs/apk/debug/app-debug.apk  
                                            # install (usually done by gradle)
- adb uninstall  org.sthaid.viewer          # uninstall

==============================
XXX TODO
==============================
- check need for jpeg AndroidStatic.mk
- logout and back in and check my PATH var
- better step by step istructions,  incorporate abvoe
- script the build and install
- cleanup this document

==================================================================================
==============================  APPENDIX  ========================================
==================================================================================

==============================
APPENDIX - ANDROIDSDK
==============================

androidsdk  XXX describe

XXX reference snap section below

==============================
APPENDIX - ADB (ANDROID DEBUG BRIDGE)
==============================

adb    # Android Debug Bridge
- options
  -d  : use usb device
  -e  : use emulator
- examples
  adb help
  adb install  ./app/build/outputs/apk/debug/app-debug.apk
  adb uninstall  org.libsdl.testgles
  adb logcat
  adb shell [<command>]
  adb shell getprop | grep abi
  adb shell getprop ro.build.version.sdk

adb shell command examples
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

==============================
APPENDIX - SNAP
==============================

XXX describe

help
- man snap
- snap help

find
- snap find androidsdk
    Name        Version  Publisher   Notes  Summary
    androidsdk  1.0.5.2  quasarrapp  -      The package contains android sdkmanager.

install
- snap install <name>


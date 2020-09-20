This file documents procedures for building the Linux Viewer Program and
Android Viewer App on an Ubuntu Guest.

==============================
INSTALLING UBUNTU GUEST
==============================

Ddownload ubuntu from here: https://ubuntu.com/download/desktop
You should get a file such as: ubuntu-20.04.1-desktop-amd64.iso.

Install Ubuntu using your Hypervisor. Any hypervisor should work, I use KVM.

After installation completes and your logged in to Ubuntu ...

Set your Display Resolution
- right click the background, and choose Display Settings
  . select, for example: 1600x900

==============================
BUILD LINUX VIEWER PROGRAM 
==============================

Start Terminal
  Click menu on Apps on bottom left and bring up Terminal.

The following commands are all done from within the terminal 

Install Software Development Packages
  cd $HOME
  sudo apt update
  sudo apt install -y build-essential
  sudo apt install -y git libsdl2-dev libsdl2-ttf-dev libjpeg-dev libpng-dev

Clone the proj_cam2 git repo
  cd $HOME
  mkdir proj
  cd proj
  git clone https://github.com/sthaid/proj_cam2.git

Build and test the Linux Viewer Program
  cd $HOME/proj/proj_cam2
  make viewer
  ./viewer
  The program should have started and you should see an error, such as INVLD_PSSWD,
    fix the error by correcting the CONFIG settings

==============================
BUILD ANDROID VIEWER APP 
==============================

# install java and check version
  cd $HOME
  sudo apt update
  sudo apt install default-jdk
  java -version

# download cmdlinetools tool
  # goto https://developer.android.com/studio/#downloads
  # search for "Command line tools only"
  # download commandlinetools-linux-xxxxxx_latest.zip
  # Save File, this saves it in ~/Downloads

# extract the cmdlinetools from the downloaded zip file
  cd $HOME
  mkdir -p androidsdk/cmdline-tools
  cd androidsdk/cmdline-tools
  unzip ~/Downloads/commandlinetools-linux-6609375_latest.zip

# accept the android licenses, and
# download the android Native Development Kit  (ndk)
  cd $HOME
  androidsdk/cmdline-tools/tools/bin/sdkmanager --licenses
  androidsdk/cmdline-tools/tools/bin/sdkmanager --install "ndk;21.3.6528147"

# add environment variables and PATH to .profile
  cd $HOME
  echo 'export ANDROID_NDK_HOME=$HOME/androidsdk/ndk/21.3.6528147' >> .profile
  echo 'export ANDROID_HOME=$HOME/androidsdk' >> .profile
  echo PATH='$HOME/androidsdk/platform-tools:$PATH' >> .profile
  source .profile

# setup the viewer app build directory
  cd $HOME/proj/proj_cam2/android-project
  ./do_setup

# build the viewer app
  ./do_build

Build result should be here:
  ./SDL2-2.0.12/build/org.sthaid.viewer/app/build/outputs/apk/debug/app-debug.apk

==============================
INSTALL ANDROID VIEWER APP ON ANDROID DEVICE
==============================

Ensure the Host is not using the Android USB Device
- if the host has adb installed, then 'adb kill-server'

On Android Device
- developer mode must be enabled
- USB debugging must be enabled
- Developer Options Setup screen should be visible, because you'll need
  to Authorize the Ubuntu Guest

Use Hypervisor to Redirect Android USB Device to the Ubuntu Guest. 
Note - I had trouble with this step, my Ubuntu guest was being shutdown by the
       KVM Hypervisor. But it did eventually work. I don't know what the problem was
       or exactly what I did to eventually redirect the USB device.

Run 'adb devices' in Ubuntu guest to verify device connection. 
Output shoud be like:
       List of devices attached
       ZY227NX9BT	device

Assuming the device is correctly attached, install the Viewer App on the device:
  cd $HOME/proj/proj_cam2/android-project
  ./do_install

XXX clean this up

# install java and check version
cd $HOME
sudo apt update
sudo apt install default-jdk
java -version

# download cmdlinetools tool
# - goto https://developer.android.com/studio/#downloads
# - search for "Command line tools only"
# - download commandlinetools-linux-xxxxxx_latest.zip
# - Save File, this saves it in ~/Downloads

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

# add environment variables to .profile;
# and execute the .profile
cd $HOME
echo "export ANDROID_NDK_HOME=$HOME/androidsdk/ndk/21.3.6528147" >> .profile
echo "export ANDROID_HOME=$HOME/androidsdk" >> .profile
source .profile

# setup the viewer app build directory
cd $HOME/proj/proj_cam2/android-project
./do_setup

# build the viewer app
./do_build


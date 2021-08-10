cd /usr/lib/python3/dist-packages
ln -s apt_pkg.cpython-35m-x86_64-linux-gnu.so apt_pkg.so
add-apt-repository ppa:ubuntu-toolchain-r/test
apt update
apt-get install gcc-8 g++-8
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8
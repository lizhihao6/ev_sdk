cd /usr/lib/python3/dist-packages
ln -s apt_pkg.cpython-35m-x86_64-linux-gnu.so apt_pkg.so
add-apt-repository ppa:ubuntu-toolchain-r/test
apt update
apt-get install gcc-8 g++-8
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8
# 生成公私钥以及公钥对应的头文件
bash /usr/local/ev_sdk/3rd/license/bin/oneKeyAuth.sh
# 生成硬件参考码文件和授权文件
bash /usr/local/ev_sdk/3rd/license/bin/oneKeyTest.sh
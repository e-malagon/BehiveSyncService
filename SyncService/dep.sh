#!/bin/bash

#execute su commands
function sucmd {
	if [ -f /etc/redhat-release ]; then
		echo $rootpassword | su -c "$1"
	elif [ -f /etc/lsb-release ]; then
		echo $rootpassword | sudo -S $1 
	fi
}

function deps {
	if [ -f /etc/redhat-release ]; then
		read -s -p "Enter root password: " rootpassword
		sucmd "dnf install -y gcc gcc-c++ make unzip cmake perl python libevent-devel openssl-devel libev-devel libcurl-devel libuuid-devel xz libpng++-devel"
	elif [ -f /etc/lsb-release ]; then
		read -s -p "Enter sudo password: " rootpassword
		sucmd "apt-get install -y gcc g++ make unzip cmake perl python libevent-dev libssl-dev libev-dev libcurl4-openssl-dev uuid-dev xz-utils libpng++-dev"
	else
		echo "This script only works on RH or Ubuntu based linux"
		exit 1
	fi
}

#fcgi
function fcgi {
	wget -O libfcgi.tar.gz https://github.com/beigebinder/libfcgi/archive/2.4.1.tar.gz
	tar -xvf libfcgi.tar.gz
	cd libfcgi-2.4.1
	./configure --prefix=$dir/beehive/usr/local
	make
	make install
	cd ..
	rm -rf libfcgi-2.4.1 libfcgi.tar.gz
}

#argon2
function argon2 {
	wget -O argon2.tar.gz https://github.com/P-H-C/phc-winner-argon2/archive/20190702.tar.gz
	tar -xvf argon2.tar.gz
	cd phc-winner-argon2-20190702
	make
	make install PREFIX=$dir/beehive/usr/local
	cd ..
	rm -rf phc-winner-argon2-20190702 argon2.tar.gz
}

#mariadb-connector-c
function mariadbcon {
	wget -O mariadb-connector-c.tar.gz https://github.com/mariadb-corporation/mariadb-connector-c/archive/v3.0.6.tar.gz
	tar -xvf mariadb-connector-c.tar.gz
	mkdir mariadb-connector-c-3.0.6/build
	cd mariadb-connector-c-3.0.6/build
	cmake -DCMAKE_BUILD_TYPE=release -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX:PATH=$dir/beehive/usr/local -G "Unix Makefiles" ..
	make
	make install
	cd ../..
	rm -rf mariadb-connector-c-3.0.6 mariadb-connector-c.tar.gz
}

#g3log
function g3log {
	wget -O g3log.tar.gz https://github.com/beigebinder/g3log/archive/1.3.2.tar.gz
	tar -xvf g3log.tar.gz
	mkdir g3log-1.3.2/build
	cd g3log-1.3.2/build
	cmake -DENABLE_FATAL_SIGNALHANDLING=OFF -DCMAKE_INSTALL_PREFIX:PATH=$dir/beehive/usr/local -G "Unix Makefiles" ..
	make
	make install
	cd ../..
	rm -rf g3log-1.3.2 g3log.tar.gz
}

set -e

if [ $# -eq 0 ]; then
	echo "usage buil-linux [deps|fcgi|argon2|mariadbcon|g3log|all]"
	exit 1
fi

dir=$(pwd)
for param in "$@"
do
	case "$param" in
		deps)
			deps
			;;
		fcgi)
			fcgi
			;;
		argon2)
			argon2
			;;
		mariadbcon)
			mariadbcon
			;;
		g3log)
			g3log
			;;
		all)
			deps
			fcgi
			argon2
			mariadbcon
			g3log
			;;
		*)
			echo "Unknown parameter $1"
			exit 1
	esac
done

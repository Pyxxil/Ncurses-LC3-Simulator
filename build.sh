#!/bin/sh

clean () {
	rm -rf 'build'
}

build () {
	mkdir -p 'build'
	cd build
	cmake .. || exit 1
	make || exit 1
	cp LC3Simulator ..
	cd ..
}

debug () {
	mkdir -p 'build'
	cd $_
	cmake .. -DCMAKE_BUILD_TYPE=Debug || exit 1
	make || exit 1
	cp LC3Simulator ..
	cd ..
}

if (( $# == 1 )); then
	case "$1" in
		build)
			build
			;;
		debug)
			debug
			;;
		clean)
			clean
			;;
		*)
			build
			;;
	esac
else
	build
fi


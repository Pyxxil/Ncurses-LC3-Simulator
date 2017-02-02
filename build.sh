#!/bin/sh

function clean () {
	rm -rf build
}

function build () {
	mkdir -p build
	cd build
	cmake .. || exit 1
	make || exit 1
	cp LC3Simulator ..
	cd ..
}

if (( $# > 1 )); then
	case "$1" in
		build)
			build
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


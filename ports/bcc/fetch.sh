#!/bin/sh
# fetch.sh - download the BCC (Dev86 0.16.17) sources, check them, unpack them
# into build/ports/dev86-0.16.17 and apply this port's patches.
# The tarball is the one Debian and Ubuntu ship as linux86_0.16.17.orig.tar.gz.
set -e
cd "$(dirname "$0")/../.."
SHA256=52ed4980c0e4b68d2624aadb0ceb0339cb3fd8dd7c2175419d4f77a451846cbe
TARBALL=linux86_0.16.17.orig.tar.gz
URLS="https://deb.debian.org/debian/pool/main/l/linux86/$TARBALL
http://archive.ubuntu.com/ubuntu/pool/universe/l/linux86/$TARBALL"
mkdir -p build/ports
cd build/ports

sum() { if command -v sha256sum >/dev/null; then sha256sum "$1"; else shasum -a 256 "$1"; fi | cut -d' ' -f1; }

if [ ! -f "$TARBALL" ] || [ "$(sum "$TARBALL")" != "$SHA256" ]; then
	rm -f "$TARBALL"
	for u in $URLS; do
		echo "fetching $u"
		if curl -fsSL -o "$TARBALL.part" "$u" && [ "$(sum "$TARBALL.part")" = "$SHA256" ]; then
			mv "$TARBALL.part" "$TARBALL"
			break
		fi
		rm -f "$TARBALL.part"
	done
	[ -f "$TARBALL" ] || { echo "fetch.sh: could not download $TARBALL with the expected SHA-256" >&2; exit 1; }
fi

rm -rf dev86-0.16.17
tar xzf "$TARBALL"
cd dev86-0.16.17
for p in ../../../ports/bcc/patches/*.patch; do
	echo "applying $(basename "$p")"
	patch -p1 -s < "$p"
done
echo "sources ready in build/ports/dev86-0.16.17"

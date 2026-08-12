# Maintainer: Iván Ezequiel Rodriguez <ivanrwcm25@gmail.com>
# shellcheck disable=SC2034,SC2154
pkgname=pack-unpack
pkgver=1.5.7
pkgrel=1
pkgdesc="Create and unpack archives via libarchive (pack/unpack)"
arch=('x86_64' 'aarch64')
url="https://github.com/IRodriguez13/pack-unpack"
license=('GPL-3.0-or-later')
depends=('libarchive')
makedepends=('gcc' 'make' 'pkgconf')
# Do not install extract alias — avoids clash with libextractor's /usr/bin/extract.
source=("${pkgname}-${pkgver}.tar.gz::${url}/releases/download/v${pkgver}/${pkgname}-${pkgver}.tar.gz")
sha256sums=('SKIP')

build() {
  cd "${pkgname}-${pkgver}"
  make PREFIX=/usr all
}

check() {
  cd "${pkgname}-${pkgver}"
  make check
}

package() {
  cd "${pkgname}-${pkgver}"
  make DESTDIR="${pkgdir}" PREFIX=/usr INSTALL_EXTRACT_ALIAS=0 install
}

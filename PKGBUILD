# Maintainer: Iván Ezequiel Rodriguez <ivanrwcm25@gmail.com>
# shellcheck disable=SC2034,SC2154
pkgname=pack-extract
pkgver=1.5.5
pkgrel=1
pkgdesc="Create and extract archives via libarchive (pack/extract)"
arch=('x86_64' 'aarch64')
url="https://github.com/IRodriguez13/pack-extract"
license=('GPL-3.0-or-later')
depends=('libarchive')
makedepends=('gcc' 'make' 'pkgconf')
source=("${pkgname}-${pkgver}.tar.gz::${url}/releases/download/v${pkgver}/${pkgname}-${pkgver}.tar.gz")
sha256sums=('1cdb6a5197cfc89ae892e117fe919c0c27b15fa86b2463ee968bb05594a9b8df')

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
  make DESTDIR="${pkgdir}" PREFIX=/usr install
}

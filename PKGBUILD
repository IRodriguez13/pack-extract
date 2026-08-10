# Maintainer: Iván Ezequiel Rodriguez <ivanrwcm25@gmail.com>
# shellcheck disable=SC2034,SC2154
pkgname=pack-extract
pkgver=1.5.3
pkgrel=1
pkgdesc="Create and extract archives via libarchive (pack/extract)"
arch=('x86_64' 'aarch64')
url="https://github.com/IRodriguez13/pack-extract"
license=('GPL-3.0-or-later')
depends=('libarchive')
makedepends=('gcc' 'make' 'pkgconf')
source=("${pkgname}-${pkgver}.tar.gz::${url}/releases/download/v${pkgver}/${pkgname}-${pkgver}.tar.gz")
sha256sums=('d48fb54b53f99a11dd348cea99afc0f4fbdef3bdfa64c199ca3a48e8ba3301c6')

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

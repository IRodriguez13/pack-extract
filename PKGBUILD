# Maintainer: Iván Ezequiel Rodriguez <ivanrwcm25@gmail.com>
# shellcheck disable=SC2034,SC2154
pkgname=pack-extract
pkgver=1.5.4
pkgrel=1
pkgdesc="Create and extract archives via libarchive (pack/extract)"
arch=('x86_64' 'aarch64')
url="https://github.com/IRodriguez13/pack-extract"
license=('GPL-3.0-or-later')
depends=('libarchive')
makedepends=('gcc' 'make' 'pkgconf')
source=("${pkgname}-${pkgver}.tar.gz::${url}/releases/download/v${pkgver}/${pkgname}-${pkgver}.tar.gz")
sha256sums=('7be990d8c7016fc8fb8bbfe10ce38f26f587fb90644bc1308e100d7232cf565c')

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

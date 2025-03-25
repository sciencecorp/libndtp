#!/bin/bash
set -e

VERSION=${1:-"0.0.1"}
PACKAGE_NAME="libndtp-${VERSION}"
BUILD_DIR="build"
SCRIPT_DIR=$(dirname "$0")
INCLUDE_DIR="$(realpath ${SCRIPT_DIR}/../include)"

PACKAGE_DIR="packages"
mkdir -p ${PACKAGE_DIR}

echo "Running CMake Configure"
cmake --preset=static -DCMAKE_BUILD_TYPE=Release -DVCPKG_MANIFEST_FEATURES=${VCPKG_MANIFEST_FEATURES} -DLIBNDTP_VERSION=${VERSION}

echo "Running Build"
cmake --build build

echo "Build complete, packaging dependencies"

TEMP_DIR=$(mktemp -d)
PACKAGE_STRUCTURE="${TEMP_DIR}/${PACKAGE_NAME}"
mkdir -p "${PACKAGE_STRUCTURE}/lib"
mkdir -p "${PACKAGE_STRUCTURE}/include"

cp ${BUILD_DIR}/liblibndtp.a "${PACKAGE_STRUCTURE}/lib/"
cp -r ${BUILD_DIR}/include/* "${PACKAGE_STRUCTURE}/include/"
cp -r ${INCLUDE_DIR}/* "${PACKAGE_STRUCTURE}/include/"

# And a readme for end users, we can add more information later
# but if they are downloading this they probably know what they want it for
cat > "${PACKAGE_STRUCTURE}/README.md" << EOF
# libndtp - Version ${VERSION}

## Contents
- **lib/** - Contains the static library (liblibndtp.a)
- **include/** - Contains all necessary header files

EOF

echo "Staging complete, bundling"

TARBALL="${PACKAGE_DIR}/${PACKAGE_NAME}.tar.gz"

tar -czf "${TARBALL}" -C "${TEMP_DIR}" "${PACKAGE_NAME}"

echo "Package created at ${TARBALL}"

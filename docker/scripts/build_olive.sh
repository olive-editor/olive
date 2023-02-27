#!/usr/bin/env bash

git clone --depth 1 --recurse-submodules https://github.com/olive-editor/olive.git
cd olive
mkdir build
cd build
cmake .. -G "Ninja"
cmake --build .

cmake --install app --prefix appdir/usr

# TODO: Can the following libs be excluded?
#libQt5DBus.so,\
#libQt5MultimediaGstTools.so,\
#libQt5MultimediaWidgets.so,\

# Manually add OpenSSL 1.1 that Qt was compiled against
mkdir -p appdir/usr/lib64/openssl11
cp -a /usr/lib64/libssl.so.1.1.1k /usr/lib64/libcrypto.so.1.1.1k appdir/usr/lib64/
cp -a /usr/lib64/libssl.so.1.1 /usr/lib64/libcrypto.so.1.1 appdir/usr/lib64/
cp -a /usr/lib64/openssl11/libssl.so /usr/lib64/openssl11/libcrypto.so appdir/usr/lib64/openssl11
mkdir -p appdir/etc/pki/ca-trust/extracted/pem
mkdir -p appdir/etc/pki/tls/certs
cp -a /etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem appdir/etc/pki/ca-trust/extracted/pem
cp -a /etc/pki/tls/certs/ca-bundle.crt appdir/etc/pki/tls/certs

/usr/local/linuxdeployqt-x86_64.AppImage \
  appdir/usr/share/applications/org.olivevideoeditor.Olive.desktop \
  -appimage \
  -exclude-libs=\
libQt5Pdf.so,\
libQt5Qml.so,\
libQt5QmlModels.so,\
libQt5Quick.so,\
libQt5VirtualKeyboard.so \
  -bundle-non-qt-libs \
  -executable=appdir/usr/bin/crashpad_handler \
  -executable=appdir/usr/bin/minidump_stackwalk \
  -executable=appdir/usr/bin/olive-crashhandler \
  -executable=appdir/usr/lib64/libssl.so.1.1.1k \
  -executable=appdir/usr/lib64/libcrypto.so.1.1.1k \
  --appimage-extract-and-run

./Olive*.AppImage --appimage-extract-and-run --version


win32 {
    RC_FILE = icons/resources.rc

    INCLUDEPATH += $$PWD/vcpkg/installed/x64-windows/include
    DEPENDPATH += $$PWD/vcpkg/installed/x64-windows/include

    win32:CONFIG(release, debug|release): LIBS += -L$$PWD/vcpkg/installed/x64-windows/lib/ -lavcodec \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lavdevice \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lavfilter \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lavformat \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lavutil \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lswscale \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lswresample \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lpostproc \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lOpenCL \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -lopengl32 \
                                                  -L$$PWD/vcpkg/installed/x64-windows/lib/ -llibx264

    win32:CONFIG(debug, debug|release): LIBS +=   -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lavcodec \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lavdevice \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lavfilter \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lavformat \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lavutil \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lswscale \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lswresample \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lpostproc \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lOpenCL \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -lopengl32 \
                                                  -L$$PWD/vcpkg/installed/x64-windows/debug/lib/ -llibx264





}

mac {
    LIBS += -L/usr/local/lib -lavutil -lavformat -lavcodec -lavfilter -lswscale -lswresample
    ICON = icons/olive.icns
    INCLUDEPATH = /usr/local/include
}

unix:!mac {
    CONFIG += link_pkgconfig
    PKGCONFIG += libavutil libavformat libavcodec libavfilter libswscale libswresample
}

#-------------------------------------------------
#
# Project created by QtCreator 2018-05-11T10:31:59
#
#-------------------------------------------------

# Tries to get the current Git short hash
system("which git") {
    GITHASHVAR = $$system(git --git-dir $$PWD/.git --work-tree $$PWD log -1 --format=%h)

    # Fallback for Ubuntu/Launchpad (extracts Git hash from debian/changelog rather than Git repo)
    # (see https://answers.launchpad.net/launchpad/+question/678556)
    isEmpty(GITHASHVAR) {
        GITHASHVAR = $$system(sh $$PWD/debian/gitfromlog.sh $$PWD/debian/changelog)
    }

    DEFINES += GITHASH=\\"\"$$GITHASHVAR\\"\"
}

TEMPLATE = subdirs

SUBDIRS = app

unix:CONFIG(unittests) {
    SUBDIRS += unittests
} else {
    SUBDIRS += app/main
}

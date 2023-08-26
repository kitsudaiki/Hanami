TEMPLATE = subdirs
CONFIG += ordered
QT -= qt core gui
CONFIG += c++17

SUBDIRS =  libKitsunemimiCommon
#SUBDIRS += libKitsunemimiNetwork
SUBDIRS += libKitsunemimiJson
SUBDIRS += libKitsunemimiCrypto
SUBDIRS += libKitsunemimiJwt
SUBDIRS += libKitsunemimiIni
SUBDIRS += libKitsunemimiArgs
SUBDIRS += libKitsunemimiConfig
SUBDIRS += libKitsunemimiCpu
SUBDIRS += libKitsunemimiSqlite
SUBDIRS += libKitsunemimiOpencl
#SUBDIRS += libKitsunemimiObj
SUBDIRS += libKitsunemimiSakuraHardware
SUBDIRS += libKitsunemimiSakuraDatabase
#SUBDIRS += libKitsunemimiSakuraNetwork
SUBDIRS += libKitsunemimiHanamiPolicies
SUBDIRS += libKitsunemimiHanamiClusterParser
SUBDIRS += libKitsunemimiHanamiFiles


#libKitsunemimiNetwork.depends = libKitsunemimiCommon
libKitsunemimiJson.depends = libKitsunemimiCommon
libKitsunemimiCrypto.depends = libKitsunemimiCommon
libKitsunemimiJwt.depends = libKitsunemimiCrypto libKitsunemimiJson
libKitsunemimiIni.depends = libKitsunemimiCommon
libKitsunemimiArgs.depends = libKitsunemimiCommon
libKitsunemimiConfig.depends = libKitsunemimiIni
libKitsunemimiCpu.depends = libKitsunemimiCommon
libKitsunemimiSqlite.depends = libKitsunemimiCommon
libKitsunemimiOpencl.depends = libKitsunemimiCommon
#libKitsunemimiObj.depends = libKitsunemimiCommon
libKitsunemimiSakuraHardware.depends = libKitsunemimiCpu
libKitsunemimiSakuraDatabase.depends = libKitsunemimiSqlite
#libKitsunemimiSakuraNetwork.depends = libKitsunemimiNetwork
libKitsunemimiHanamiPolicies.depends = libKitsunemimiConfig libKitsunemimiArgs
libKitsunemimiHanamiSegmentParser.depends = libKitsunemimiConfig libKitsunemimiArgs
libKitsunemimiHanamiClusterParser.depends = libKitsunemimiConfig libKitsunemimiArgs
src/libHanamiAiSdk.depends = libKitsunemimiCrypto libKitsunemimiJson

TEMPLATE = subdirs
CONFIG += ordered
QT -= qt core gui
CONFIG += c++17

SUBDIRS =  libKitsunemimiCommon
SUBDIRS += libKitsunemimiNetwork
SUBDIRS += libKitsunemimiJson
SUBDIRS += libKitsunemimiCrypto
SUBDIRS += libKitsunemimiJwt
SUBDIRS += libKitsunemimiIni
SUBDIRS += libKitsunemimiArgs
SUBDIRS += libKitsunemimiConfig
SUBDIRS += libKitsunemimiCpu
SUBDIRS += libKitsunemimiSqlite
SUBDIRS += libKitsunemimiOpencl
SUBDIRS += libKitsunemimiObj
SUBDIRS += libKitsunemimiSakuraHardware
SUBDIRS += libKitsunemimiSakuraDatabase
SUBDIRS += libKitsunemimiSakuraNetwork
SUBDIRS += libKitsunemimiHanamiCommon
SUBDIRS += libKitsunemimiHanamiPolicies
SUBDIRS += libKitsunemimiHanamiDatabase
SUBDIRS += libKitsunemimiHanamiSegmentParser
SUBDIRS += libKitsunemimiHanamiClusterParser
SUBDIRS += libKitsunemimiHanamiNetwork
SUBDIRS += libShioriArchive
SUBDIRS += libMisakiGuard
SUBDIRS += libAzukiHeart


libKitsunemimiNetwork.depends = libKitsunemimiCommon
libKitsunemimiJson.depends = libKitsunemimiCommon
libKitsunemimiCrypto.depends = libKitsunemimiCommon
libKitsunemimiJwt.depends = libKitsunemimiCrypto libKitsunemimiJson
libKitsunemimiIni.depends = libKitsunemimiCommon
libKitsunemimiArgs.depends = libKitsunemimiCommon
libKitsunemimiConfig.depends = libKitsunemimiIni
libKitsunemimiCpu.depends = libKitsunemimiCommon
libKitsunemimiSqlite.depends = libKitsunemimiCommon
libKitsunemimiOpencl.depends = libKitsunemimiCommon
libKitsunemimiObj.depends = libKitsunemimiCommon
libKitsunemimiSakuraHardware.depends = libKitsunemimiCpu
libKitsunemimiSakuraDatabase.depends = libKitsunemimiSqlite
libKitsunemimiSakuraNetwork.depends = libKitsunemimiNetwork
libKitsunemimiHanamiCommon.depends = libKitsunemimiConfig libKitsunemimiArgs
libKitsunemimiHanamiPolicies.depends = libKitsunemimiConfig libKitsunemimiArgs
libKitsunemimiHanamiDatabase.depends = libKitsunemimiJson libKitsunemimiSakuraDatabase
libKitsunemimiHanamiSegmentParser.depends = libKitsunemimiConfig libKitsunemimiArgs libKitsunemimiHanamiCommon
libKitsunemimiHanamiClusterParser.depends = libKitsunemimiConfig libKitsunemimiArgs libKitsunemimiHanamiCommon
libKitsunemimiHanamiNetwork.depends = libKitsunemimiHanamiCommon libKitsunemimiCrypto libKitsunemimiJson
src/libHanamiAiSdk.depends = libKitsunemimiHanamiCommon libKitsunemimiCrypto libKitsunemimiJson
libShioriArchive.depends = libKitsunemimiHanamiNetwork
libMisakiGuard.depends = libKitsunemimiHanamiNetwork
libAzukiHeart.depends = libKitsunemimiHanamiNetwork

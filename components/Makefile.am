AM_CXXFLAGS =				\
	-Wall -fno-rtti -fno-exceptions	\
	$(EXTENSION_CXXFLAGS)

noinst_LTLIBRARIES = BeckyImport.la

if MOZILLA_VERSION_7
MODULE_CPP = BeckyImportModule.cpp
else
MODULE_CPP = BeckyImportModule_1_9_2.cpp
endif

BeckyImport_la_SOURCES = 	        \
	BeckyMailImporter.cpp           \
	BeckyMailImporter.h             \
	BeckyAddressBooksImporter.cpp   \
	BeckyAddressBooksImporter.h     \
	BeckySettingsImporter.cpp       \
	BeckySettingsImporter.h         \
	BeckyFiltersImporter.cpp        \
	BeckyFiltersImporter.h          \
	BeckyStringBundle.cpp           \
	BeckyStringBundle.h             \
	BeckyUtils.cpp                  \
	BeckyUtils.h                    \
	BeckyVCardAddress.cpp           \
	BeckyVCardAddress.h             \
	BeckyProfileMigrator.cpp        \
	BeckyProfileMigrator.h          \
	BeckyImport.cpp                 \
	BeckyImport.h                   \
        $(MODULE_CPP)

BeckyImport_la_LIBADD = 	\
	$(XPCOM_LIBS)

BeckyImport_la_LDFLAGS = 	\
        -rpath `pwd`/.libs      \
	-avoid-version		\
	-export-dynamic         \
        -no-undefined           \
	-module 		\
	$(platform_ldflags)	\
	$(XPCOM_LDFLAGS)


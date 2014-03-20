TEMPLATE = lib
CONFIG += static
SOURCES += pyqbind.cpp pyqbind_detail.cpp
HEADERS += pyqbind.h pyqbind_detail.h
TARGET = pyqbind
INCLUDEPATH += /System/Library/Frameworks/Python.framework/Headers
LIBS += -lboost_python-mt -framework Python

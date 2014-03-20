CONFIG = qt app stdbinary
SOURCES=main.cpp
INCLUDEPATH += /System/Library/Frameworks/Python.framework/Headers ..
QMAKE_LIBDIR += ..
LIBS += -lpyqbind -lboost_python-mt -framework Python

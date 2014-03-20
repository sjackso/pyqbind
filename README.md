PyQBind
=======

A lightweight, just-in-time binding library for controlling Qt objects from Python

Motiation
---------

PyQBind was written to satisfy a particular need: providing flexible and Pythonic scripting control
of a Qt application written in C++ *without* introducing a dependency on a full-scale Qt binding
for Python, such a PyQt or PySide.  (In the context where this library was developed, such binding 
libraries were difficult for some users to install, though all had access to Qt4, Python, and boost.)

PyQBind addresses this issue by combining boost::python with Qt's meta-object system
to create a control interface for Qt objects at runtime.  When a QObject is exported
to Python via PyQBind, its Qt properties become readable and writable as Python attributes,
and its Qt slots become callable Python methods.  This does not require any hand-written
binding annotations, so the programmer can change the Python interface of a QObject simply
by changing its C++ definition directly.  This makes it easy to test new features of a
QObject, e.g. by directly calling a slot that is being developed, without having to
connect the slot to other program signals.

PyQBind is *not* intended as a general-purpose Qt binding for developing GUI applications
in Python.  Rather, it is a support tool for a Qt C++ application that embeds Python.

Installation
------------

The included qmake file builds PyQBind as a static library, intended to be linked in to
your C++ application.  This is usually the most convenient way to use the library.
Alternatively, the source files may be added directly to your C++ project, or a shared
library may be built.

pyqbind.hpp is the public API header and the only file that should be included in your program's
compilation units.

Usage
-----

See example/main.cpp for basic usage.

Known Issues
------------

* The included build files assume you're using OS X.
* The example is not very interesting



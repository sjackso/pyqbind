#include <iostream>
#include <boost/python.hpp>
#include <QApplication>

#include "pyqbind.h"

#include <iostream>

int main( int argc, char* argv[] ){

	/** Must call pre_init before python initializes */
	PyQBind::pre_init();

	/** Initialize Python using either chars (Python 2) or whcars (Python 3)*/
#if PY_MAJOR_VERSION < 3
	Py_SetProgramName(argv[0]);
	Py_Initialize();
	PySys_SetArgv(1, argv);
#else
	wchar_t *wprogname = new wchar_t[255];
	mbstowcs(wprogname, argv[0], 255);
	Py_SetProgramName(wprogname);
	Py_Initialize();
	PySys_SetArgv(1, &wprogname);
#endif

	/** If we expect to need python threads, we can initialize them now */
	//PyEval_InitThreads();

	/** Finish PyQBind initialization */
	PyQBind::post_init();

	/** Now we can create our application and export it */
	QCoreApplication q(argc, argv);
	PyQBind::exportObject( &q );
	PyQBind::addToGlobalNamespace( &q, "app" );

	try{
		boost::python::object main = boost::python::import("__main__");
		boost::python::object dict = main.attr("__dict__");
		boost::python::exec("print dir(app)", dict, dict);
		boost::python::exec("print app.applicationName", dict, dict);
	} catch( boost::python::error_already_set& ){
		PyErr_Print();
	}

}

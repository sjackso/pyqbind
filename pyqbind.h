#ifndef PYQBIND_MAIN_H
#define PYQBIND_MAIN_H

class QObject;

/**
 * Fully static class for automagical exposure of QObject to Python runtime.
 */
class PyQBind{

public:
	/**
	 * Initialize the PyQBind system; called once before Python is initialized.
	 */
	static void pre_init();

	/**
	 * Initialize the PyQBind system; called once after Python is initialized.
	 */
	static void post_init();

	/**
	 * Export the properties and public slots of the given object.  The object will
	 * be added to the pyqbind._all_objects list in Python; use addToGlobalNamespace
	 * or addToParent to expose the object in a more useful way.
	 */
	static void exportObject( QObject* q );

	/**
	 * Add the given object, which must have been previously passed to exportObject,
	 * to python's __main__ namespace with the given name.
	 */
	static void addToGlobalNamespace( QObject* q, const std::string& name );

	/**
	 * Set the child object as a python attribute of the parent object.  Both the
	 * objects must have previously been exported to python with exportObject.
	 */
	static void addToParent( QObject* parent, QObject* child, const std::string& attrname );

	/** General pyqbind error type */
	class error : public std::runtime_error{
	public:
		error(): std::runtime_error("unknown pyqbind error"){}
		error(const std::string& err): std::runtime_error(err){}
	};

private:
	// unimplemented; PyQBind is fully static
	PyQBind();
	PyQBind(const PyQBind&);
	const PyQBind& operator=(const PyQBind&);
};

#endif /* PYQBIND_MAIN_H */

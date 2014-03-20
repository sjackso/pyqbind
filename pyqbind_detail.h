#ifndef PYQBIND_DETAIL_H
#define PYQBIND_DETAIL_H

#include <boost/python/object_fwd.hpp>
#include <QMetaProperty>
#include <QMetaMethod>
#include <QObject>

#define log_error_stream( X ) do{ std::cerr << "ERROR: " <<  X << std::endl; } while(0);
#define log_warn_stream( X ) do{ std::cerr << "WARNING: " << X << std::endl; } while(0);
#define log_debug_stream( X ) do{ std::cerr << "DBGMSG: " << X << std::endl; } while(0);

namespace pyqbind_detail {

void register_metatypes();

boost::python::object make_metaproperty_reader( QMetaProperty m );

boost::python::object make_metaproperty_writer( QMetaProperty m );

boost::python::object make_invoker( QMetaMethod m );

/**
 * This class provides a fancy invoke()-able way of calling QMetaProperty.read()
 * and QMetaProperty.write(), and is used for thread synchronization when pyqbind
 * properties are accessed outside the main thread.
 */
class MetapropertyThreadQueue : public QObject{
	Q_OBJECT;

	public:
	QMetaMethod read_method;
	QMetaMethod write_method;

	MetapropertyThreadQueue();

	public Q_SLOTS:
		QVariant read( QObject* q, QMetaProperty p );
		bool write( QObject* q, QMetaProperty p, const QVariant& value );
};

/**
 * RAII class to grab and release the Python Global Interpreter Lock
 */
class ScopedGIL{

	private:
	PyGILState_STATE s_;

	public:
	ScopedGIL(){
		s_ = PyGILState_Ensure();
	}

	~ScopedGIL(){
		PyGILState_Release( s_ );
	}
};


} // namespace pyqbind_detail

#endif /* PYQBIND_DETAIL_H */

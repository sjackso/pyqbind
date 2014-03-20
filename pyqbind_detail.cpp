#include <string>
#include <vector>
#include <iostream>

#include <boost/python.hpp>
#include <boost/bind.hpp>
#include <boost/any.hpp>
#include <boost/mpl/vector.hpp>

#include <QString>
#include <QThread>
#include <QApplication>
#include <QVector3D>


#include "pyqbind.h"

#include "pyqbind_detail.h"

// metatype declarations must be at global namespace
Q_DECLARE_METATYPE(std::string);

namespace pyqbind_detail{

void register_metatypes()
{
	qRegisterMetaType<std::string>();
}

namespace bp = boost::python;

static MetapropertyThreadQueue* mtq_ = NULL;

/* Call this method to access the global MetapropertyThreadQueue; do not use mtq_ directly */
MetapropertyThreadQueue* mtq(){
	if( !mtq_ ){
		 mtq_ = new MetapropertyThreadQueue();
		 mtq_->moveToThread( QApplication::instance()->thread() );
	}
	return mtq_;
}

MetapropertyThreadQueue::MetapropertyThreadQueue() : QObject()
{
	const QMetaObject* mymeta = metaObject();
	log_debug_stream( "MetapropertyThreadQueue created..." );

	read_method = mymeta->method( mymeta->indexOfMethod( "read(QObject*,QMetaProperty)" ) );
	write_method = mymeta->method( mymeta->indexOfMethod( "write(QObject*,QMetaProperty,QVariant)" ) );

	// if either of these prints a blank signature, then method lookup failed and the slots won't work
	log_debug_stream( "read signature = " << read_method.signature() );
	log_debug_stream( "write signature = " << write_method.signature() );

}

QVariant MetapropertyThreadQueue::read( QObject* q, QMetaProperty p ){
	return p.read(q);
}

bool MetapropertyThreadQueue::write( QObject* q, QMetaProperty p, const QVariant& value ){
	// some properties that may throw exceptions when written
	try{
		return p.write( q, value );
	}
	catch( const std::exception& e ){
		log_error_stream( "Exception while writing property value: " << e.what() );
		return false;
	}
}


template < typename T >
T property_read( QMetaProperty p, QObject* q )
{
	log_debug_stream("Reading property " << p.name() << " of object " << q);
	QVariant ret;
	if( q->thread() == QThread::currentThread() ){
		// simple case: we are already in the correct thread
		ret = p.read(q);
	}
	else{
		// complex case: do the read in the GUI thread and wait for the result
		bool success;
		Py_BEGIN_ALLOW_THREADS
		success = mtq()->read_method.invoke( mtq(), Qt::BlockingQueuedConnection,
		                                     Q_RETURN_ARG(QVariant,ret),
		                                     Q_ARG(QObject*, q), Q_ARG(QMetaProperty, p) );
		Py_END_ALLOW_THREADS
		if( !success ){
			log_error_stream( "Queued property read failed" );
		}
	}
	return ret.value<T>();
}

template < typename T >
void property_write( QMetaProperty p, QObject* q, T value )
{
	log_debug_stream("Writing property " << p.name() << " of object " << q );
	QVariant v( value );
	bool success = false;
	if( q->thread() == QThread::currentThread() ){
		// simple case: we are already in the correct thread
		success = p.write( q, v );
	}
	else{
		// complex case: do the write in the GUI thread and wait for the result
		bool meta_success;
		Py_BEGIN_ALLOW_THREADS
		meta_success = mtq()->write_method.invoke( mtq(), Qt::BlockingQueuedConnection,
		                                           Q_RETURN_ARG(bool,success),
		                                           Q_ARG(QObject*,q), Q_ARG(QMetaProperty,p), Q_ARG(QVariant,v) );
		Py_END_ALLOW_THREADS
		if( !meta_success ){
			log_error_stream( "Queued property read failed" );
			success = false;
		}
	}

	if( !success ){
		log_error_stream( "Property write failed" );
		log_debug_stream( "Details: " << p.isWritable() << " " << p.name() << " " << p.typeName() << " " << p.userType() << " " << v.isValid() );
	}
}

boost::python::object make_metaproperty_reader( QMetaProperty prop ){
#define SUPPORTED_ARG( T ) \
	return bp::make_function( boost::bind( property_read<T>, prop, _1 ), \
	 bp::default_call_policies(), \
	 boost::mpl::vector<T,QObject*>());
#define SUPPORTED_POINTER_ARG( T ) \
	return bp::make_function( boost::bind( property_read<T>, prop, _1 ), \
	 bp::return_value_policy<bp::reference_existing_object>(), \
	 boost::mpl::vector<T,QObject*>());
	switch( prop.type() ){
		case QVariant::Bool:
			SUPPORTED_ARG(bool);
		case QVariant::Double:
			SUPPORTED_ARG(double);
		case QVariant::Int:
			SUPPORTED_ARG(int);
		case QVariant::UInt:
			SUPPORTED_ARG(unsigned);
		case QVariant::String:
			SUPPORTED_ARG(QString);
		case QVariant::Vector3D:
			SUPPORTED_ARG(QVector3D);
		case QVariant::UserType:
			if( prop.typeName() == std::string("std::string") ){
				SUPPORTED_ARG( std::string ); }
			/* fallthrough */
		default:
			log_debug_stream( "Unsupported metaproperty type: " << prop.typeName() );
			throw PyQBind::error("Unsupported metaproperty type");
	}
#undef SUPPORTED_POINTER_ARG
#undef SUPPORTED_ARG
}

boost::python::object make_metaproperty_writer( QMetaProperty prop ){
#define SUPPORTED_ARG( T ) \
	return bp::make_function( boost::bind( property_write<T>, prop, _1, _2 ), \
	 bp::default_call_policies(), \
	 boost::mpl::vector<void,QObject*, T>());
	switch( prop.type() ){
		case QVariant::Bool:
			SUPPORTED_ARG(bool);
		case QVariant::Double:
			SUPPORTED_ARG(double);
		case QVariant::Int:
			SUPPORTED_ARG(int);
		case QVariant::UInt:
			SUPPORTED_ARG(unsigned);
		case QVariant::String:
			SUPPORTED_ARG(QString);
		case QVariant::Vector3D:
			// see comment in make_metaproperty_reader
			SUPPORTED_ARG(QVector3D);
		case QVariant::UserType:
			//if( prop.typeName() == std::string("std::string") )
				//SUPPORTED_ARG( std::string );
			/* fallthrough */
		default:
			log_debug_stream( "Unsupported metaproperty type: " << prop.typeName() );
			throw PyQBind::error("Unsupported metaproperty type");
	}
#undef SUPPORTED_ARG
}

/**
 * Return a QGenericArgument created from the given python object and an array
 * indicating the type of the argument.  Throws std::runtime_error if the argument
 * type isn't supported.  The 'refs' argument should be a vector that will not
 * go out of scope until the return value is no longer needed.
 */
static
QGenericArgument make_qarg( const bp::object& pobj, const QByteArray& typestr,
                            std::vector<boost::any>& refs )
{
	// Unused args are python Nones; return empty QGenericArgument
	if( pobj == bp::object()) return QGenericArgument();

	/* Gross things you didn't want to know about QGenericArgument:
	 * QGenericArgument keeps a weak reference to the thing you hand it,
	 * so we can't afford to put a stack-allocated object into the Q_ARG macro.
	 * On the other hand, nothing outside this function is going to have
	 * access to the C++ type of the argument. The solution is to store
	 * a shared_ptr to the argument's contents in the 'refs' paramter,
	 * so that appropriate destructors will be called at the end of q_meta_invoke.
	 * This approach may be slow and should be revisited if we need to pass large
	 * objects across metaobject calls, but I wouldn't advise that.
	 */
#define SUPPORTED_ARG( FROM, TO ) \
	if( typestr == FROM ) \
	{ \
		TO tmp = boost::python::extract<TO>(pobj); \
		boost::shared_ptr< TO > ptr( new TO(tmp) ); \
		refs.push_back( ptr ); \
		QGenericArgument ret = Q_ARG( TO, (*ptr) ); \
		return ret; \
	}

	// FROM arguments must have a valid copy constructor
	SUPPORTED_ARG( "uint", unsigned );
	SUPPORTED_ARG( "int", int );
	SUPPORTED_ARG( "double", double );
	SUPPORTED_ARG( "std::string", std::string );
	SUPPORTED_ARG( "bool", bool );
	SUPPORTED_ARG( "QVector3D", QVector3D );
	SUPPORTED_ARG( "boost::python::object", boost::python::object );
	throw PyQBind::error( std::string( "Unsupported qarg type: " ) + typestr.constData() );

#undef SUPPORTED_ARG
}

// this function is invoked by boost::python for every call to an exported metamethod.
// QMetaMethod may take up to 9 arguments, but for simplicity this invoker only supports 4
void q_meta_invoke(QMetaMethod method, QObject* q,
	               bp::object arg0=bp::object(),
	               bp::object arg1=bp::object(),
	               bp::object arg2=bp::object(),
	               bp::object arg3=bp::object() )
{
	log_debug_stream("Metamethod invoked: " << method.signature() );
	std::vector<boost::any> refs; // see make_qarg documentation
	QList< QByteArray > argtypes = method.parameterTypes();
	// FIXME: create more robust arity checking.
	while( argtypes.length() < 4 ){
		// ensure this list has contents for calls to make_qarg below
		// If make_qarg actually tries to convert this fake type,
		// it means too many arguments were passed
		argtypes.push_back( QByteArray("Error no type available") );
	}

	bool success;
	QGenericArgument a0 = make_qarg( arg0, argtypes[0], refs );
	QGenericArgument a1 = make_qarg( arg1, argtypes[1], refs );
	QGenericArgument a2 = make_qarg( arg2, argtypes[2], refs );
	QGenericArgument a3 = make_qarg( arg3, argtypes[3], refs );

	/* Invoke the metamethod.  If we aren't the owning thread, use a
	 * blocking queued connection so that invoke() will wait for the invocation
	 * to return.  This waiting needs to occur with the GIL released to prevent
	 * deadlocks, but the GIL must *not* be released until the all the arguments
	 * have been computed with make_qarg.
	 */
	if( q->thread() != QThread::currentThread() ){
		Py_BEGIN_ALLOW_THREADS
		success = method.invoke( q, Qt::BlockingQueuedConnection, a0, a1, a2, a3 );
		Py_END_ALLOW_THREADS
	}
	else{
		success = method.invoke( q, a0, a1, a2, a3 );
	}

	if( !success ){
		log_error_stream( "Metamethod invocation failed" );
	}
}

bp::object make_invoker( QMetaMethod method ){
	/*
	 * Although BOOST_PYTHON_FUNCTION_OVERLOADS could help with the variadic nature
	 * of q_meta_invoke, it does not play well with boost::bind, so here we used
	 * hand-rolled variadic binding.  This helps make the autogenerated docstrings
	 * slightly less gross, too.
	 */
	switch( method.parameterTypes().length() ){
	case 0:
		return bp::make_function( boost::bind( q_meta_invoke, method, _1, bp::object(), bp::object(), bp::object(), bp::object() ),
		                          bp::default_call_policies(),
		                          boost::mpl::vector<void,QObject*>() );
	case 1:
		return bp::make_function( boost::bind( q_meta_invoke, method, _1, _2, bp::object(), bp::object(), bp::object() ),
		                          bp::default_call_policies(),
		                          boost::mpl::vector<void,QObject*, bp::object>() );
	case 2:
		return bp::make_function( boost::bind( q_meta_invoke, method, _1, _2, _3, bp::object(), bp::object() ),
		                          bp::default_call_policies(),
		                          boost::mpl::vector<void,QObject*, bp::object, bp::object>() );
	case 3:
		return bp::make_function( boost::bind( q_meta_invoke, method, _1, _2, _3, _4, bp::object() ),
		                          bp::default_call_policies(),
		                          boost::mpl::vector<void,QObject*, bp::object, bp::object, bp::object>() );
	case 4:
		return bp::make_function( boost::bind( q_meta_invoke, method, _1, _2, _3, _4, _5 ),
		                          bp::default_call_policies(),
		                          boost::mpl::vector<void,QObject*, bp::object, bp::object, bp::object, bp::object>() );
	default:
		throw PyQBind::error(std::string("Unsupported arity in metamethod ") + method.signature() );
	}
}

} // namespace pyqbind_detail

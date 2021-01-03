/*************************************************************************
*  2020 Janek Kozicki                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

/*
 * This file provides dropdown menu support in QT GUI for file gui/qt5/SerializableEditor.py
 * File lib/serialization/Serializable.hpp is already huge, so better to put this in separate file.
 * And use it only when someone wants drop down menus.
 *
 * For basic usage see documentation: https://yade-dem.org/doc/prog.html#enums
 *
 */

#include <lib/base/Logging.hpp>
#include <boost/preprocessor.hpp>
#include <boost/python.hpp>

#ifndef YADE_ENUM_SUPPORT_HPP
#define YADE_ENUM_SUPPORT_HPP

namespace yade {

/*
 * NOTE: this one is not necessary:

template <typename ArbitraryEnum> struct ArbitraryEnum_to_python { … };

 * because we use to_python conversions provided by boost::python::enum_<…>
 *
 * But the template below ArbitraryEnum_from_python { … }; is provided to make sure that the enum can be converted from python arguments such as:
 *
 *   1. enum class ArbitraryEnum, which is represented by the boost::python::enum_<ArbitraryEnum> on python side: 'Boost.Python.enum'
 *   2. int
 *   3. string
 *
 * If we decided that only 'Boost.Python.enum' should be accepted in yade, and assignment from string and int should not work,
 * then this template can be completely removed. And we could use only what is provided by boost::python::enum_<…>.
 *
 * However I don't think that's the case. There are plenty of enums in yade codebase. And currently they all work by assigning int values to them.
 * So that code below is the one of possible ways to ensure backward compatibility, while at the same time we get the fancy dropdown menus.
 *
 */
template <typename ArbitraryEnum> struct ArbitraryEnum_from_python {
	static bool setArbitraryEnum(const ::boost::python::object& arg, ArbitraryEnum& col)
	{
		// All of them are validated if the value is possible within the possible enum values.
		std::string             arbName = boost::core::demangle(typeid(ArbitraryEnum).name());
		::boost::python::object meCol(col);
		if (::boost::python::extract<long>(arg).check()) {
			long n = ::boost::python::extract<long>(arg)();
			if (::boost::python::extract<::boost::python::dict>(meCol.attr("values"))().has_key(n)) {
				col = ::boost::python::extract<ArbitraryEnum>(meCol.attr("values")[n])();
			} else {
				LOG_ERROR("enum class " + arbName + " does not have key number: " + boost::lexical_cast<std::string>(n));
				return false;
			}
		} else if (::boost::python::extract<std::string>(arg).check()) {
			std::string n = ::boost::python::extract<std::string>(arg)();
			if (::boost::python::extract<::boost::python::dict>(meCol.attr("names"))().has_key(n)) {
				col = ::boost::python::extract<ArbitraryEnum>(meCol.attr("names")[n])();
			} else {
				LOG_ERROR("enum class " + arbName + " does not have key called: " + n);
				return false;
			}
		} else if (::boost::python::extract<ArbitraryEnum>(arg).check()) {
			col = ::boost::python::extract<ArbitraryEnum>(arg)();
		} else {
			LOG_ERROR("enum class " + arbName + " cannot read input argument.");
			return false;
		};
		LOG_DEBUG("enum class " + arbName + " set successfully.");
		return true;
	};

	ArbitraryEnum_from_python() { boost::python::converter::registry::push_back(&convertible, &construct, boost::python::type_id<ArbitraryEnum>()); }
	static void* convertible(PyObject* obj_ptr)
	{
		LOG_DEBUG("Ref count = " << obj_ptr->ob_refcnt);
		// The invocations in setArbitraryEnum(…) which return false in line if( ::boost::python::extract<…> … ) will call this function again. We better prevent that.
		// Make sure it's thread safe, even though assigning enums simultaneously from different threads is not gonna happen.
		thread_local static int preventRecurrence = 0;
		if (preventRecurrence != 0)
			return nullptr;
		++preventRecurrence;

		ArbitraryEnum             test;
		::boost::python::handle<> handle(::boost::python::borrowed(obj_ptr));
		::boost::python::object   arg(handle);

		bool success = setArbitraryEnum(arg, test);
		--preventRecurrence;
		return success ? obj_ptr : nullptr;
	}
	static void construct(PyObject* obj_ptr, boost::python::converter::rvalue_from_python_stage1_data* data)
	{
		::boost::python::handle<> handle(::boost::python::borrowed(obj_ptr));
		::boost::python::object   arg(handle);

		void* storage = ((boost::python::converter::rvalue_from_python_storage<ArbitraryEnum>*)(data))->storage.bytes;
		new (storage) ArbitraryEnum;
		ArbitraryEnum* val = (ArbitraryEnum*)storage;
		setArbitraryEnum(arg, *val);
		data->convertible = storage;
	}
	DECLARE_LOGGER;
};

#define YADE_ENUM_PARSE_ONE(r, FULL_SCOPE__ENUM_TYPE, enumNAME) .value(BOOST_PP_STRINGIZE(enumNAME), FULL_SCOPE__ENUM_TYPE::enumNAME)

#define YADE_ENUM(FULL_SCOPE, ENUM_TYPE, ENUM_SEQUENCE)                                                                                                        \
	TEMPLATE_CREATE_LOGGER(ArbitraryEnum_from_python<FULL_SCOPE::ENUM_TYPE>);                                                                              \
	}                                                                        /* end namespace yade */                                                      \
	namespace {                                                              /* start anonymous namespace */                                               \
		__attribute__((constructor)) void registerEnum_##ENUM_TYPE(void) /* we cannot have enums with same name in multiple scopes */                  \
		{                                                                                                                                              \
			try {                                                                                                                                  \
				/* register in yade scope */                                                                                                   \
				boost::python::object                         main = boost::python::import("yade");                                            \
				boost::python::scope                          setScope(main);                                                                  \
				boost::python::type_info                      info = boost::python::type_id<FULL_SCOPE::ENUM_TYPE>();                          \
				const boost::python::converter::registration* reg  = boost::python::converter::registry::query(info);                          \
				if ((reg == NULL) or (reg->m_to_python == NULL)) {                                                                             \
					yade::ArbitraryEnum_from_python<FULL_SCOPE::ENUM_TYPE>();                                                              \
					boost::python::enum_<FULL_SCOPE::ENUM_TYPE>(                                                                           \
					        "EnumClass_" #ENUM_TYPE) /* the loop over ENUM_SEQUENCE in next line registers all enum name strings */        \
					        BOOST_PP_SEQ_FOR_EACH(YADE_ENUM_PARSE_ONE, FULL_SCOPE::ENUM_TYPE, ENUM_SEQUENCE);                              \
				}                                                                                                                              \
			} catch (...) {                                                                                                                        \
				std::cerr << "\nException: " __FILE__ ":" << __PRETTY_FUNCTION__ << ", enum (class or type) " #ENUM_TYPE ".\n\n";              \
				PyErr_Print();                                                                                                                 \
				PyErr_SetString(PyExc_SystemError, __FILE__); /* raising here anything other than SystemError is not possible */               \
				boost::python::handle_exception();                                                                                             \
				throw;                                                                                                                         \
			}                                                                                                                                      \
		}                                                                                                                                              \
	}                                                                                                                                                      \
	namespace yade {

}; // namespace yade
#endif

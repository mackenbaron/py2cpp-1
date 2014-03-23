#ifndef BOOST_PYTHON_PY2CPP_H_
	#define BOOST_PYTHON_PY2CPP_H_
/*

//  Copyright (c) 2013 Bruno Lalande
//  current maintainer - David Minor dahvid.minor at gmail.com


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
/**
		This library provides a bridge between stl containers, boost variants, boost fusion containers
		and python.
*/

#include <boost/python.hpp> //bug in boost requires this before stl_iterator.hpp

#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/size.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/tuple.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/count.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/variant.hpp>
#include <boost/utility.hpp>
#include <boost/any.hpp>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>
#include <string>
#include <list>
#include <map>


#include "is_variant.hpp"

using kmt::is_variant;
using boost::fusion::tuple;
using boost::mpl::count;

//must discriminate between, vector, map, fusion::sequence and {string,double,long, bool}


namespace boost {
namespace python {

using std::cout;
using std::endl;
using std::cerr;
using boost::fusion::traits::is_sequence;
using std::string;
 	
template <class Type>
struct is_string 
     : is_same<Type,std::string>
   {};


template <class>
struct is_map 
   : mpl::false_
	{};

template <class Key, class Mapped, class Pair, class Alloc>
struct is_map<std::map<Key, Mapped, Pair, Alloc> > 
    : mpl::true_
	{};

template <class>
struct is_vector
   : mpl::false_
	{};

template <class T>
struct is_vector<std::vector<T> > 
    : mpl::true_
	{};



template <class Type>
   struct is_python_convertable 
     : mpl::bool_<
           is_string<Type>::value ||
           is_same<Type,double>::value ||
           is_same<Type,long>::value ||
           is_same<Type,int>::value ||
           is_same<Type,size_t>::value ||
           is_same<Type,bool>::value>
   {};

//class list_maker;
//class tuple_maker;
//class dict_maker;


class tuple_maker
{
public:
    tuple_maker(tuple& t):
    m_tuple(t),
    m_iteration(0)
    {}

    
    template <class Sequence>
    typename enable_if<fusion::traits::is_sequence<Sequence>, void>::type
    operator() (const Sequence& s) const
    { add_element(make_tuple_from_fusion(s)); }

    //check if mapped type is a vector
    template <typename T>
            typename enable_if<typename is_vector<T>::type>::type                                                                    
        operator() (const T& s) const
        { 
            add_element(make_list_from_vector(s)); 
        }
        
    //check if mapped type is a map
    template <typename T>
            typename enable_if<typename is_map<T>::type>::type                                                                    
        operator() (const T& s) const
        { 
            add_element(make_dict_from_map(s)); 
        }

    //check if mapped type is automatically convertable to python
    template <typename T>
            typename enable_if<typename is_python_convertable<T>::type>::type
        operator() (const T& element) const
        { 
            add_element(element); 
        }
    

private:
    template <class T>
    void add_element(const T& element) const
    {
        PyTuple_SET_ITEM(
            m_tuple.ptr(),
            m_iteration++,
            incref(object(element).ptr())
        );
    }

    tuple& m_tuple;
    mutable int m_iteration;
};



class tuple_parser
{
public:
    tuple_parser(const object& t):
    m_tuple(t),
    m_iteration(0)
    {}

  //check if mapped type is a vector
    template <typename T>
            typename enable_if<typename is_vector<T>::type>::type                                                                    
        operator() (T& c_object) const
        { 
    		list t;
    		assign_element(t);
            make_vector_from_list(t, &c_object); 
        }
        
    //check if mapped type is a map
    template <typename T>
            typename enable_if<typename is_map<T>::type>::type                                                                    
        operator() (T& c_object) const
        { 
    		dict t;
    		assign_element(t);
            make_map_from_dict(t, &c_object); 
        }

    //check if mapped type is automatically convertable to python
    template <typename T>
            typename enable_if<typename is_python_convertable<T>::type>::type
        operator() (T& c_object) const
        { 
            assign_element(c_object);
        }
        
  
    template <class Sequence>
    typename enable_if<fusion::traits::is_sequence<Sequence>, void>::type
    operator() (Sequence& s) const
    { 
    	tuple t;
    	assign_element(t);
    	make_fusion_from_tuple(t, &s); 
    }

    
private:
    template <class T>
    void assign_element(T& element) const
    {
    	tuple t = extract<tuple>(m_tuple);
    	element = extract<T>(t[m_iteration++]);
    }

    const object&  m_tuple;
    mutable int m_iteration;
};


inline const char* getPyTypeString(const object& element)
{
    extract<long>   get_long(element);
    extract<double> get_double(element);
    extract<bool>   get_bool(element);
    extract<string> get_string(element);

    //container types
    extract<list>   get_list(element);
    extract<dict>   get_dict(element);
    extract<tuple>  get_tuple(element);
    if (get_long.check()) {
		return "long";
    }
    else if (get_double.check()) {
		return "double";
    }
    else if (get_bool.check()) {
		return "bool";
    }
    else if (get_string.check()) {
		return "string";
    }
    else if (get_list.check()) {
		return "list";
    }
    else if (get_dict.check()) {
		return "dict";
    }
    else if (get_tuple.check()) {
		return "tuple";
    }
	return "unknown";
}

struct XValueError {};

//assignment statement that disappears if it's not possible
/*
struct invisible_assign {

	 

	template <class T, class U>
	typename enable_if<typename count<typename U::types, T>::type>::type
	operator() (const T& from, U& to) const
	{ 
	    to = from;
	}

    template <class T, class U>
        typename enable_if<typename is_python_convertable<T>::type>::type
	operator() (const T& from, U& to) const
	{ 
	    to = get<T>(from);
	}

};
*/
/*
tempalte<class Return>
struct type_builder {
	type_builder(const object& o) : object(o) {} const object& o;

	Return type_builder()
	{
	
	}
	
};

//builder has to be staically declared
//???I can't build a builder for each type because they are recursive
 template<class T>
auto build_type(const type_builder& builder) -> decltype(builder())
{
	auto val = builder();
	return val;
}
*/


class value_parser
{
public:
    value_parser(const object& t):
    m_py_object(t)
    {}

	  template <class T>
	        typename enable_if<typename is_variant<T>::type>::type
	    operator() (T& c_object) const
	    { 
	    	 
	    	 //because the target cobject is a variant we need to query the python object to know what it is
	    	 //and assign it to the cobject, it won't work to read the cobject first
	    	 //however we need a way to only instantiate the versions that are valid variants of cobject
	    	 //otherwise it won't compile
	    	 
	    	//figure out what kind of variant it is
 	    	extract<long> get_long(m_py_object);
	    	extract<double> get_double(m_py_object);
	    	extract<string> get_string(m_py_object);
	    	extract<list>   get_list(m_py_object);
	    	extract<dict>   get_dict(m_py_object);
	    	extract<tuple>   get_tuple(m_py_object);
	    	 
	        if (get_long.check()) {
	        	value_parser parse(m_py_object);
	        	parse(get<long>(c_object));
	        }	        
	        else if (get_double.check()) {
	        	value_parser parse(m_py_object);
	        	parse(get<double>(c_object));
	        }
	        else if (get_string.check()) {
	        	value_parser parse(m_py_object);
	        	parse(get<string>(c_object));
	        }
	        else if (get_list.check()) {
	        	make_variant_from_list(get_list(), &c_object); 
	        }
	        else if (get_dict.check()) {
	        	make_variant_from_dict(get_dict(), &c_object); 
	        }
	        else if (get_tuple.check()) {
	        	make_variant_from_tuple(get_tuple(), &c_object); 
	        }
	        else {
	            cerr << "found unknown variant type " << Py_TYPE(m_py_object.ptr())->tp_name << endl;
	            throw XValueError();
	        }
	    }

	  	template <class T>
	        typename enable_if<typename fusion::traits::is_sequence<T>::type>::type
	    operator() (T& c_object) const
	    { 
	        
	    	extract<tuple>  get_tuple(m_py_object);
	    	if (!get_tuple.check()) {
				throw XValueError();
	    	}
	        make_fusion_from_tuple(get_tuple(), &c_object); 
	    }

	    //check if mapped type is a vector
	    template <typename T>
            typename enable_if<typename is_vector<T>::type>::type                                                                    
        operator() (T& c_object) const
        { 
            if (!make_vector_from_list(m_py_object, &c_object))
				throw XValueError();
            	
        }
        
	    //check if mapped type is a map
	    template <typename T>
            typename enable_if<typename is_map<T>::type>::type                                                                    
        operator() (T& c_object) const
        { 
            if (!make_map_from_dict(extract<dict>(m_py_object), &c_object))
				throw XValueError();
        }

	    //check if mapped type is automatically convertable to python
	    template <typename T>
            typename enable_if<typename is_python_convertable<T>::type>::type
        operator() (T& c_object) const
        { 
            c_object = extract<T>(m_py_object); 
        }
    
private:
    
    const object&  m_py_object;
};



template <class Sequence>
void make_fusion_from_tuple(const object& t, Sequence* s)
{	
    fusion::for_each(*s, tuple_parser(extract<tuple>(t)));
}

/* 
 * we only support keys of long,string
*/
template <class T, class U>
bool make_map_from_dict(const object& in_obj, std::map<T,U>* output)
{	
    using namespace std;
    
	if (!PyDict_Check(in_obj.ptr()))
		return false;
		
    dict in_dict = extract<dict>(in_obj);
    
    list key_list   = in_dict.keys();
	list value_list = in_dict.values();
    for (int i=0; i<len(key_list); i++)
    {
        //all key types are automatically convertable
    	T key = extract<T>(key_list[i]);
        U value;

        value_parser  parser(value_list[i]);
        try {
        	parser(value);
        }
        catch(...) { //can also throw internal boost exceptions
        	return false;
        }
    	output->insert(make_pair(key, value));
    	
    }	
    return true;
}

inline void checkedEntrance(const char* message)
{
	
    if (PyErr_Occurred()) {
		PyErr_Clear();
    }
}

inline bool checkedReturn(const char* message)
{

    if (PyErr_Occurred()) {
		PyErr_Clear();
		return false;
	}
	else
		return true;
}


template<class T>
bool get_list_of_tuples(const object& module, const char* name, std::vector<T>* outputs)
{
	checkedEntrance("get_list_of_tuples");
    //create python tuple from passed in fusion sequence
    object  obj_ptr       = module.attr(name);
	if (!PyList_Check(obj_ptr.ptr()))
		return false;
		
    list    result_list   = extract<list>(obj_ptr);
    stl_input_iterator<tuple> begin(result_list), end;
    for (;begin != end; ++begin)
    {
    	T out;
		make_fusion_from_tuple(*begin, &out);
		outputs->push_back(out);
    }
 	return checkedReturn("get_list_of_tuples");
}


template<class T>
bool make_vector_from_list(const object& pylist, std::vector<T>* outputs)
{
	checkedEntrance("make_vector_from_list");

	
    extract<list>   get_list(pylist);
    
	if (!get_list.check())
		return false;

    list l = get_list();

    for (int i=0; i< len(l); i++)
    {
        value_parser parser(l[i]); //sets the element
        T value; 
		try {
        	parser(value); //copies it into a c value, or invokes recursive procedure
        }
        catch(...) {
        	return false;
        }
        outputs->push_back(value);
    }

 	return checkedReturn("make_vector_from_list");
}

/*
template<class T>
bool make_variant_from_list(const list& pylist, T* outputs)
{
	checkedEntrance("make_vector_from_variant");
	

    for (int i=0; i< len(pylist); i++)
    {
        value_parser parser(pylist[i]); //sets the element
        boost::any value;
		try {
        	parser(value); //copies it into a c value, or invokes recursive procedure
        }
        catch(...) {
        	return false;
        }
        outputs->push_back(value);
    }

 	return checkedReturn("make_vector_from_variant");
}
*/



class list_maker
{
public:
    list_maker(list& t):
    m_list(t)
    {}      

    //vector types can be pod, object, tuple, map, vector
                         
       //check if mapped type is a tuple
    template <typename T>
            typename enable_if<typename fusion::traits::is_sequence<T>::type>::type                                                              
        operator() (const T& s) 
        { 
            add_element(make_tuple_from_fusion(s)); 
        }
    //check if mapped type is a vector
    template <typename T>
            typename enable_if<typename is_vector<T>::type>::type                                                                    
        operator() (const T& s) 
        { 
            add_element(make_list_from_vector(s)); 
        }
        
    //check if mapped type is a map
    template <typename T>
            typename enable_if<typename is_map<T>::type>::type                                                                    
        operator() (const T& s) 
        { 
            add_element(make_dict_from_map(s)); 
        }

    //check if mapped type is automatically convertable to python
    template <typename T>
            typename enable_if<typename is_python_convertable<T>::type>::type
        operator() (const T& element) 
        { 
            add_element(element); 
        }

private:
    template <class T>
    void add_element(const T& element) 
    {
        m_list.append(element);
    }

    list& m_list;
};



/* 
 * We assume the key is always a pod type
 *  */
class dict_maker
{
public:
    dict_maker(dict& t):
    m_dict(t)
    {}      

    //questions to ask here, is the value a container type?
    //if so, what container
    //is it a non-container, non-pod type?
    //is it a pod type?
    //I'm going to make all map accesses iterator based to make things simpler, unlike in the list iterator
    //which needed to deal with both direct container accesses and iterator accesses
                         
    //check if mapped type is a tuple
    template <typename T>
            typename enable_if<typename fusion::traits::is_sequence<typename T::second_type>::type>::type                                                                    
        operator() (const T& s) 
        { 
            add_element(make_pair(s.first, make_tuple_from_fusion(s.second))); 
        }
    //check if mapped type is a vector
    template <typename T>
            typename enable_if<typename is_vector<typename T::second_type>::type>::type                                                                    
        operator() (const T& s) 
        { 
            add_element(make_pair(s.first, make_list_from_vector(s.second))); 
        }
        
    //check if mapped type is a map
    template <typename T>
            typename enable_if<typename is_map<typename T::second_type>::type>::type                                                                    
        operator() (const T& s) 
        { 
            add_element(make_pair(s.first, make_dict_from_map(s.second))); 
        }

    //check if mapped type is a pod iterator
    template <typename T>
            typename enable_if<typename is_python_convertable<typename T::second_type>::type, void>::type
        operator() (const T& element) 
        { 
            add_element(element); 
        }

private:
    template <class T>
    void add_element(const T& element) 
    {
        m_dict[element.first] = element.second;
    }

    dict& m_dict;
};


template<class T>
list make_list_from_vector(const std::vector<T>& v)
{
    list result;
    std::for_each(v.begin(), v.end(), list_maker(result));
    return result;
}



template <class Sequence>
tuple make_tuple_from_fusion(const Sequence& s)
{
    tuple result((detail::new_reference)::PyTuple_New(fusion::size(s)));
    fusion::for_each(s, tuple_maker(result));
    return result;
}




template<class T, class U>
dict make_dict_from_map(const std::map<T,U>& v)
{
    dict result;
    std::for_each(v.begin(), v.end(), dict_maker(result));
    return result;
}

/**
        assumes all tuple members or of the same type
*/
template<class T>
bool extract_vector_from_tuple(tuple& pytuple, std::vector<T>* outputs)
{		
	checkedEntrance("extract_vector_from_tuple");
    stl_input_iterator<T> begin(pytuple), end;
    for (;begin != end; ++begin)
    {
    	T out = *begin;
		outputs->push_back(out);
    }
 	return checkedReturn("extract_vector_from_tuple");
}


template<class T, class U>
bool extract_map(const dict& py_dict, std::map<T,U>* output)
{
	checkedEntrance("extract_map");
	list key_list   = py_dict.keys();
	list value_list = py_dict.values();
    for (int i=0; i<len(key_list); i++)
    {
        //all key types are automatically convertable
    	T key = extract<T>(key_list[i]);
    	U value  = extract<U>(value_list[i]);
    	output->insert(make_pair(key, value));
    }	
 	return checkedReturn("extract_map");
}

template<class T, class U>
bool extract_map(const object& py_obj, std::map<T,U>* output)
{
	using namespace std;
	if (PyErr_Occurred()) {
		PyErr_Clear();
	}
	if (!PyDict_Check(py_obj.ptr()))
		return false;
	dict py_dict = extract<dict>(py_obj);
	list key_list   = py_dict.keys();
	
	return extract_map(py_dict, output);
}

template<class T, class U>
bool get_dict(const object& module, const char* name, std::map<T,U>* output)
{
    using namespace std;
    
	checkedEntrance("get_dict");
		
    //create python tuple from passed in fusion sequence
    object  obj_ptr       = module.attr(name);
	if (!PyDict_Check(obj_ptr.ptr()))
		return false;
	
    dict    py_dict   = extract<dict>(obj_ptr);

    if (!extract_map(py_dict, output)) {
        cerr << "error extracting dict" << endl;
        return false;
    }
 	return checkedReturn("get_dict");
}


template<class T>
struct vector_inserter 
{
    vector_inserter(std::vector<T>* output) 
        : output(output) {}
         std::vector<T>* output;

    void operator()(T value) const
    {
        output->push_back(value);        
    }    
};



 
template<class T>
bool tuple_to_variant_list(const object& obj_ptr, std::vector<T>* outputs)
{
    using namespace std;

	checkedEntrance("tuple_to_variant_list");
    PyObject* obj = obj_ptr.ptr();
		
	if (!PyTuple_Check(obj))
		return false;

	int size = PyTuple_Size(obj);

	for (int i=0; i<size; i++)
	{
        PyObject* element = PyTuple_GetItem(obj,i);

        if (PyInt_Check(element)) {
             //extract only works if there is a lhs
             long i = PyInt_AsLong(element);
             outputs->push_back(i);
        }
        else if (PyFloat_Check(element)) {
             //extract only works if there is a lhs
             double f = PyFloat_AsDouble(element);
             outputs->push_back(f);
        }
        else if (PyString_Check(element)) {
             std::string s = PyString_AsString(element);
             outputs->push_back(s);
        }
        else {
            cerr << "found unknown py type " << Py_TYPE(element)->tp_name << endl;
            return false;
        }

	}
 	return checkedReturn("tuple_to_variant_list");
}


 
inline std::string convertToString(object pyobject)
{
    object module(handle<>(borrowed(PyImport_AddModule("__main__"))));
    object global = module.attr("__dict__");
    
    //pass our manufactured python object into the interpreter environment
    global["tmp"] = pyobject; 
    //convert it to a python string
    exec("import pprint", global, global);
    exec("pp = pprint.PrettyPrinter(indent=4)", global, global);
    object o = eval("pp.pformat(tmp)", global, global);
    //convert to c string and return
    return extract<std::string>(o);
}


 
//experiment for universal converter
//dict, tuple, map, basic -> map, vector, fusion::vector, string, basic, variant

//in each case, the structure being parsed will be held in the object,
//the new structure will be created all at once (as in a fusion::vector) or on the fly (as in stl::map,vector)
//template<class... Ts>
 
class py2cpp {
public:
	//check if mapped type is fusion tuple
	template <class T>
		typename enable_if<typename fusion::traits::is_sequence<T>::type>::type 
			operator() (const object& mPyObject, T* s) const
				{ make_fusion_from_tuple(mPyObject, s); }

	//check if mapped type is a vector
	template <class T>
		typename enable_if<typename is_vector<T>::type>::type                                                                    
	    	operator() (const object& mPyObject, T* s) const
	   		{ make_vector_from_list(mPyObject, s); }
	   
	//check if mapped type is a map
	template <class T>
		typename enable_if<typename is_map<T>::type>::type                                                                    
	   	   operator() (const object& mPyObject, T* s) const
	   	   { make_map_from_dict(mPyObject, s); }

	//check if mapped type is automatically convertable from python
	template <class T>
		typename enable_if<typename is_python_convertable<T>::type>::type
	    	operator() (const object& mPyObject, T* element) const
	   	    { *element = extract<T>(mPyObject); }
protected:
};

//same in reverse minus the variant
class cpp2py {
public:

	//check if mapped type is fusion tuple
	template <class T>
		typename enable_if<typename fusion::traits::is_sequence<T>::type>::type
			operator() (const T& s, object* pyobject) const
				{ *pyobject = make_tuple_from_fusion(s); }

	//check if mapped type is a vector
	template <class T>
		typename enable_if<typename is_vector<T>::type>::type                                                                    
	    	operator() (const T& s, object* pyobject) const
	   		{ *pyobject = make_list_from_vector(s); }
	   
	//check if mapped type is a map
	template <class T>
		typename enable_if<typename is_map<T>::type>::type                                                                    
	   	   operator() (const T& s, object* pyobject) const
	   	   { *pyobject = make_dict_from_map(s); }

	//check if mapped type is automatically convertable from python
	template <class T>
		typename enable_if<typename is_python_convertable<T>::type>::type
	    	operator() (const T& element, object* pyobject) const
	   	    { *pyobject = element; }

};

} // namespace python
} // namespace boost


#endif

#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/stringize.hpp>

#include "wasm/datastream.hpp"

#define WASMLIB_REFLECT_MEMBER_OP( r, OP, elem ) \
  OP t.elem

/**
 *  @defgroup serialize Serialize
 *  @ingroup core
 *  @brief Defines C++ API to serialize and deserialize object
 */

/**
 *  Defines serialization and deserialization for a class
 *
 *  @ingroup serialize
 *  @param TYPE - the class to have its serialization and deserialization defined
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define WASM_REFLECT( TYPE,  MEMBERS ) \
 template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr> \
 friend DataStream& operator << ( DataStream& ds, const TYPE& t ){ \
    return ds BOOST_PP_SEQ_FOR_EACH( WASMLIB_REFLECT_MEMBER_OP, <<, MEMBERS );\
 }\
 template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr> \
 friend DataStream& operator >> ( DataStream& ds, TYPE& t ){ \
    return ds BOOST_PP_SEQ_FOR_EACH( WASMLIB_REFLECT_MEMBER_OP, >>, MEMBERS );\
 }

/**
 *  Defines serialization and deserialization for a class which inherits from other classes that
 *  have their serialization and deserialization defined
 *
 *  @ingroup serialize
 *  @param TYPE - the class to have its serialization and deserialization defined
 *  @param BASE - a sequence of base class names (basea)(baseb)(basec)
 *  @param MEMBERS - a sequence of member names.  (field1)(field2)(field3)
 */
#define WASM_REFLECT_DERIVED( TYPE, BASE, MEMBERS ) \
 template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr> \
 friend DataStream& operator << ( DataStream& ds, const TYPE& t ){ \
    ds << static_cast<const BASE&>(t); \
    return ds BOOST_PP_SEQ_FOR_EACH( WASMLIB_REFLECT_MEMBER_OP, <<, MEMBERS );\
 }\
 template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr> \
 friend DataStream& operator >> ( DataStream& ds, TYPE& t ){ \
    ds >> static_cast<BASE&>(t); \
    return ds BOOST_PP_SEQ_FOR_EACH( WASMLIB_REFLECT_MEMBER_OP, >>, MEMBERS );\
 }

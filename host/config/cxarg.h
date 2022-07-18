#ifndef CXARG__H_
#define CXARG__H_

// 
// ostream/reqresp inserter that formats any type into the php serialization format
// 
template< typename T > 
struct CxArgObj { 
    CxArgObj( const T& x ): value( x ) {} 
    const T& value; 
}; 

template <typename T> 
CxArgObj<typename boost::remove_const<T>::type> cxArg( const T& x ) { 
    return CxArgObj<typename boost::remove_const<T>::type>( x ); 
} 

#endif /* CXARG__H_ */

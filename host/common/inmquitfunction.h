#ifndef INM__QUITFUNCTION__H__
#define INM__QUITFUNCTION__H__

#include <boost/function.hpp>
/* TODO: should have been
 * typedef boost::function<bool (const long int &secs)> QuitFunction_t; */
typedef boost::function<bool (int secs)> QuitFunction_t;

#endif /* INM__QUITFUNCTION__H__ */

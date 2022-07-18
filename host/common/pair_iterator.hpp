//
// pair_iterator.hpp: iterator adaptor for pairs. Makes maps easier to use
//
#ifndef PAIR_ITERATOR__HPP
#define PAIR_ITERATOR__HPP

#include <iterator> 
#include <boost/iterator/transform_iterator.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/mpl/if.hpp>

namespace inmage { 


namespace aux { 


      template<class T> struct dereference_type 
          : boost::remove_reference<typename 
std::iterator_traits<T>::reference> 
      { 
      }; 


      template<class PairT> 
      struct first_extracter 
      { 
          typedef typename boost::mpl::if_< 
                boost::is_const<PairT>,
                typename PairT::first_type const,
                typename PairT::first_type 
              >::type result_type; 
          result_type& operator()(PairT& p) const { return p.first; } 
      }; 


      template<class PairT> 
      struct second_extracter 
      { 
          typedef typename boost::mpl::if_< 
                boost::is_const<PairT>,
                typename PairT::second_type const,
                typename PairT::second_type 
              >::type result_type; 
          result_type& operator()(PairT& p) const { return p.second; } 
      }; 


} // end namespace aux


      template<class IteratorT> 
      inline 
      boost::transform_iterator<aux::first_extracter<typename aux::dereference_type<IteratorT>::type>, IteratorT> 
      over_first(IteratorT const& i) 
      { 
          typedef aux::first_extracter<typename aux::dereference_type<IteratorT>::type> extracter; 
          return boost::transform_iterator<extracter, IteratorT>(i, extracter()); 
      } 


      template<class IteratorT> 
      inline 
      boost::transform_iterator<aux::second_extracter<typename aux::dereference_type<IteratorT>::type>, IteratorT> 
      over_second(IteratorT const& i) 
      { 
          typedef aux::second_extracter<typename aux::dereference_type<IteratorT>::type> extracter; 
          return boost::transform_iterator<extracter, IteratorT>(i, extracter()); 
      } 

}

#endif // PAIR_ITERATOR__H


//                                       
// Copyright (c) 2005 InMage.
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : sharedalignedbuffer.h
// 
// Description: provides a portable smart aligned char buffer
//              it is used very much like boost::shared_array
// 

#ifndef SHAREDALIGNEDBUFFER_H
#define SHAREDALIGNEDBUFFER_H

#include <exception>
#include <sstream>

#include <boost/shared_array.hpp>

#include "inmageex.h"
#include "inmsafeint.h"
#include "inmageex.h"

#define SV_SECTOR_SIZE (512)

class SharedAlignedBuffer {
public:    

    SharedAlignedBuffer() : m_Size(0), m_Alignment(0), m_AlignedBuffer(0) { };
    
    SharedAlignedBuffer(int size, int alignment) : m_Size(0), m_Alignment(0), m_AlignedBuffer(0) {        
        Reset(size, alignment);        
    }

    // generated copy constructor, assignment, and destructor are fine

    void Reset(int size, int alignment)  {
        // note: not thread safe. 
        // 2 different threads can't call reset for the same SharedAlignedBuffer
        if (size < 1) {            
            throw INMAGE_EX("size must be greter then 0");
        }
        if (alignment < 0) {
            throw INMAGE_EX("alignment must be greater then or equal to 0");
        }
        m_Alignment = ((alignment > 1) ? alignment : 0);
        size_t buflen;
        INM_SAFE_ARITHMETIC(buflen = InmSafeInt<int>::Type(size) + m_Alignment, INMAGE_EX(size)(m_Alignment))
        char * buffer = new char[buflen];
        memset(buffer, 0, (size + m_Alignment) );
        m_Size = size;
        m_Buffer.reset(buffer);
        Align();
    }

    char & operator[] (std::ptrdiff_t i) const {
        if (0 == m_AlignedBuffer) {            
            throw INMAGE_EX("buffer is NULL");
        }
        if (!(0 <= i && i < (std::ptrdiff_t)m_Size)) {
            std::stringstream msg;
            msg << (unsigned long long)i << " is out of range (0-" << m_Size-1 << ')';
            throw std::out_of_range(msg.str().c_str());
        }
        return m_AlignedBuffer[i];
    }

    char * Get() const {
        return m_AlignedBuffer;
    }

    // implict conversion to bool
    operator bool () const {
        return (0 != m_AlignedBuffer);
    }

    bool operator!() const {
        return (0 == m_AlignedBuffer);
    }
    
    bool Unique() const {
        return m_Buffer.unique();
    }

    long UseCount() const { 
        return m_Buffer.use_count();
    }

    void Swap(SharedAlignedBuffer& other) {
        std::swap(m_Size, other.m_Size);
        std::swap(m_Alignment, other.m_Alignment);
        std::swap(m_AlignedBuffer, other.m_AlignedBuffer);
        m_Buffer.swap(other.m_Buffer);
    }
    
    int Size() const { 
        return m_Size; 
    }
    
    int Alignment() const { 
        return m_Alignment; 
    }

protected:
    void Align() {
        if (m_Alignment > 1) {
            if (!(m_Alignment & (m_Alignment-1))) {
                // power of 2
                m_AlignedBuffer = (char*)((unsigned long long)(&m_Buffer[0] + m_Alignment) & ~(m_Alignment - 1));
            } else {
                // non power of 2
                m_AlignedBuffer = &m_Buffer[0] + (m_Alignment - ((unsigned long long)(&m_Buffer[0]) % m_Alignment));
            }
        } else {
            // 0, 1 and negative alignments will be treated as no alignment
            m_AlignedBuffer = &m_Buffer[0];
        }
    }

private:
    int m_Size;        // size of the buffer that was requested
    int m_Alignment;   // alignment requested                          

    char* m_AlignedBuffer;  // points into the allocated buffer at the requested alignment.

    boost::shared_array<char> m_Buffer; // holds the allocated buffer. it will have size = m_Size + m_Alignment                            
};


#endif // ifndef SHAREDALIGNEDBUFFER_H

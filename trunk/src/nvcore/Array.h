// This code is in the public domain -- Ignacio Castaño <castano@gmail.com>

#pragma once
#ifndef NV_CORE_ARRAY_H
#define NV_CORE_ARRAY_H

/*
This array class requires the elements to be relocable; it uses memmove and realloc. Ideally I should be 
using swap, but I honestly don't care. The only thing that you should be aware of is that internal pointers
are not supported.

The foreach macros that I use are very non-standard and somewhat confusing. It would be nice to have
standard foreach as in Qt.
*/


#include "nvcore.h"
#include "Memory.h"
#include "Debug.h"
#include "Stream.h"
#include "Utils.h" // swap
#include "ForEach.h" // swap

#include <string.h>	// memmove
#include <new> // for placement new


namespace nv 
{
    // @@ Move this to utils?
    /// Delete all the elements of a container.
    template <typename T>
    void deleteAll(T & container)
    {
        for (typename T::PseudoIndex i = container.start(); !container.isDone(i); container.advance(i))
        {
            delete container[i];
        }
    }


    // @@ Move these templates to Utils.h
    // @@ Specialize these methods for numeric, pointer, and pod types.

    template <typename T>
    void construct_range(T * restrict ptr, uint new_size, uint old_size) {
        for (uint i = old_size; i < new_size; i++) {
            new(ptr+i) T; // placement new
        }
    }

    template <typename T>
    void construct_range(T * restrict ptr, uint new_size, uint old_size, const T & elem) {
        for (uint i = old_size; i < new_size; i++) {
            new(ptr+i) T(elem); // placement new
        }
    }

    template <typename T>
    void destroy_range(T * restrict ptr, uint new_size, uint old_size) {
        for (uint i = new_size; i < old_size; i++) {
            (ptr+i)->~T(); // Explicit call to the destructor
        }
    }

    template <typename T>
    void fill(T * restrict dst, uint count, const T & value) {
        for (uint i = 0; i < count; i++) {
            dst[i] = value;
        }
    }

    template <typename T>
    void copy(T * restrict dst, const T * restrict src, uint count) {
        for (uint i = 0; i < count; i++) {
            dst[i] = src[i];
        }
    }

    template <typename T>
    bool find(const T & element, const T * restrict ptr, uint begin, uint end, uint * index) {
        for (uint i = begin; i < end; i++) {
            if (ptr[i] == element) {
                if (index != NULL) *index = i;
                return true;
            }
        }
        return false;
    }



    /**
    * Replacement for std::vector that is easier to debug and provides
    * some nice foreach enumerators. 
    */
    template<typename T>
    class NVCORE_CLASS Array {
    public:

        // Default constructor.
        NV_FORCEINLINE Array() : m_buffer(NULL), m_capacity(0), m_size(0) {}

        // Copy constructor.
        NV_FORCEINLINE Array(const Array & a) : m_buffer(NULL), m_capacity(0), m_size(0) {
            copy(a.m_buffer, a.m_size);
        }

        // Constructor that initializes the vector with the given elements.
        NV_FORCEINLINE Array(const T * ptr, int num) : m_buffer(NULL), m_capacity(0), m_size(0) {
            copy(ptr, num);
        }

        // Allocate array.
        NV_FORCEINLINE explicit Array(uint capacity) : m_buffer(NULL), m_capacity(0), m_size(0) {
            setArrayCapacity(capacity);
        }

        // Destructor.
        NV_FORCEINLINE ~Array() {
            clear();
            free<T>(m_buffer);
        }


        /// Const element access.
        NV_FORCEINLINE const T & operator[]( uint index ) const
        {
            nvDebugCheck(index < m_size);
            return m_buffer[index];
        }
        NV_FORCEINLINE const T & at( uint index ) const
        {
            nvDebugCheck(index < m_size);
            return m_buffer[index];
        }

        /// Element access.
        NV_FORCEINLINE T & operator[] ( uint index )
        {
            nvDebugCheck(index < m_size);
            return m_buffer[index];
        }
        NV_FORCEINLINE T & at( uint index )
        {
            nvDebugCheck(index < m_size);
            return m_buffer[index];
        }

        /// Get vector size.
        NV_FORCEINLINE uint size() const { return m_size; }

        /// Get vector size.
        NV_FORCEINLINE uint count() const { return m_size; }

        /// Get const vector pointer.
        NV_FORCEINLINE const T * buffer() const { return m_buffer; }

        /// Get vector pointer.
        NV_FORCEINLINE T * buffer() { return m_buffer; }

        /// Is vector empty.
        NV_FORCEINLINE bool isEmpty() const { return m_size == 0; }

        /// Is a null vector.
        NV_FORCEINLINE bool isNull() const { return m_buffer == NULL; }


        /// Push an element at the end of the vector.
        NV_FORCEINLINE void push_back( const T & val )
        {
#if 1
            nvDebugCheck(&val < m_buffer || &val > m_buffer+m_size);

            setArraySize(m_size+1);
            new(m_buffer+m_size-1) T(val);
#else
            uint new_size = m_size + 1;

            if (new_size > m_capacity)
            {
                // @@ Is there any way to avoid this copy?
                // @@ Can we create a copy without side effects? Ie. without calls to constructor/destructor. Use alloca + memcpy?
                // @@ Assert instead of copy?
                const T copy(val);	// create a copy in case value is inside of this array.

                setArraySize(new_size);

                new (m_buffer+new_size-1) T(copy);
            }
            else
            {
                m_size = new_size;
                new(m_buffer+new_size-1) T(val);
            }
#endif // 0/1
        }
        NV_FORCEINLINE void pushBack( const T & val )
        {
            push_back(val);
        }
        NV_FORCEINLINE void append( const T & val )
        {
            push_back(val);
        }

        /// Qt like push operator.
        NV_FORCEINLINE Array<T> & operator<< ( const T & t )
        {
            push_back(t);
            return *this;
        }

        /// Pop the element at the end of the vector.
        NV_FORCEINLINE void pop_back()
        {
            nvDebugCheck( m_size > 0 );
            resize( m_size - 1 );
        }
        NV_FORCEINLINE void popBack()
        {
            pop_back();
        }

        /// Get back element.
        NV_FORCEINLINE const T & back() const
        {
            nvDebugCheck( m_size > 0 );
            return m_buffer[m_size-1];
        }

        /// Get back element.
        NV_FORCEINLINE T & back()
        {
            nvDebugCheck( m_size > 0 );
            return m_buffer[m_size-1];
        }

        /// Get front element.
        NV_FORCEINLINE const T & front() const
        {
            nvDebugCheck( m_size > 0 );
            return m_buffer[0];
        }

        /// Get front element.
        NV_FORCEINLINE T & front()
        {
            nvDebugCheck( m_size > 0 );
            return m_buffer[0];
        }

        /// Check if the given element is contained in the array.
        NV_FORCEINLINE bool contains(const T & e) const
        {
            return find(e, NULL);
        }

        /// Return true if element found.
        NV_FORCEINLINE bool find(const T & element, uint * indexPtr) const
        {
            return find(element, 0, m_size, indexPtr);
        }

        /// Return true if element found within the given range.
        NV_FORCEINLINE bool find(const T & element, uint begin, uint end, uint * indexPtr) const
        {
            return ::nv::find(element, m_buffer, begin, end, indexPtr);
        }

        /// Remove the element at the given index. This is an expensive operation!
        void removeAt(uint index)
        {
            nvDebugCheck(index >= 0 && index < m_size);

            if (m_size == 1) {
                clear();
            }
            else {
                m_buffer[index].~T();

                memmove(m_buffer+index, m_buffer+index+1, sizeof(T) * (m_size - 1 - index));
                m_size--;
            }
        }

        /// Remove the first instance of the given element.
        bool remove(const T & element)
        {
            uint index;
            if (find(element, &index)) {
                removeAt(index);
                return true;
            }
            return false;
        }

        /// Insert the given element at the given index shifting all the elements up.
        void insertAt(uint index, const T & val = T())
        {
            nvDebugCheck( index >= 0 && index <= m_size );

            setArraySize(m_size + 1);

            if (index < m_size - 1) {
                memmove(m_buffer+index+1, m_buffer+index, sizeof(T) * (m_size - 1 - index));
            }

            // Copy-construct into the newly opened slot.
            new(m_buffer+index) T(val);
        }

        /// Append the given data to our vector.
        NV_FORCEINLINE void append(const Array<T> & other)
        {
            append(other.m_buffer, other.m_size);
        }

        /// Append the given data to our vector.
        void append(const T other[], uint count)
        {
            if (count > 0) {
                const uint old_size = m_size;

                setArraySize(m_size + count);

                for (uint i = 0; i < count; i++ ) {
                    new(m_buffer + old_size + i) T(other[i]);
                }
            }
        }


        /// Remove the given element by replacing it with the last one.
        void replaceWithLast(uint index)
        {
            nvDebugCheck( index < m_size );
            nv::swap(m_buffer[index], back());
            (m_buffer+m_size-1)->~T();
            m_size--;
        }


        /// Resize the vector preserving existing elements.
        void resize(uint new_size)
        {
            uint old_size = m_size;

            // Destruct old elements (if we're shrinking).
            destroy_range(m_buffer, new_size, old_size);

            setArraySize(new_size);

            // Call default constructors
            construct_range(m_buffer, new_size, old_size);
        }


        /// Resize the vector preserving existing elements and initializing the
        /// new ones with the given value.
        void resize(uint new_size, const T & elem)
        {
            uint old_size = m_size;

            // Destruct old elements (if we're shrinking).
            destroy_range(m_buffer, new_size, old_size);

            setArraySize(new_size);

            // Call copy constructors
            construct_range(m_buffer, new_size, old_size, elem);
        }

        /// Clear the buffer.
        NV_FORCEINLINE void clear()
        {
            // Destruct old elements
            destroy_range(m_buffer, 0, m_size);

            m_size = 0;
        }

        /// Shrink the allocated vector.
        NV_FORCEINLINE void shrink()
        {
            if (m_size < m_capacity) {
                setArrayCapacity(m_size);
            }
        }

        /// Preallocate space.
        NV_FORCEINLINE void reserve(uint desired_size)
        {
            if (desired_size > m_capacity) {
                setArrayCapacity(desired_size);
            }
        }

        /// Copy elements to this array. Resizes it if needed.
        NV_FORCEINLINE void copy(const T * ptr, uint num)
        {
            resize( num ); // @@ call copy operator from 0 to min(num,m_size) and copy constructor from min(num,m_size) to num
            ::nv::copy(m_buffer, ptr, num);
        }

        /// Assignment operator.
        NV_FORCEINLINE Array<T> & operator=( const Array<T> & a )
        {
            copy(a.m_buffer, a.m_size);
            return *this;
        }

        // Release ownership of allocated memory and returns pointer to it.
        T * release() {
            T * tmp = m_buffer;
            m_buffer = NULL;
            m_capacity = 0;
            m_size = 0;
            return tmp;
        }

        /// Array serialization.
        friend Stream & operator<< ( Stream & s, Array<T> & p )
        {
            if (s.isLoading()) {
                uint size;
                s << size;
                p.resize( size );
            }
            else {
                s << p.m_size;
            }

            for (uint i = 0; i < p.m_size; i++) {
                s << p.m_buffer[i];
            }

            return s;
        }


        // Array enumerator.
        typedef uint PseudoIndex;

        NV_FORCEINLINE PseudoIndex start() const { return 0; }
        NV_FORCEINLINE bool isDone(const PseudoIndex & i) const { nvDebugCheck(i <= this->m_size); return i == this->m_size; }
        NV_FORCEINLINE void advance(PseudoIndex & i) const { nvDebugCheck(i <= this->m_size); i++; }

#if NV_CC_MSVC
        NV_FORCEINLINE T & operator[]( const PseudoIndexWrapper & i ) {
            return m_buffer[i(this)];
        }
        NV_FORCEINLINE const T & operator[]( const PseudoIndexWrapper & i ) const {
            return m_buffer[i(this)];
        }
#endif

        // Swap the members of this vector and the given vector.
        friend void swap(Array & a, Array & b)
        {
            swap(a.m_buffer, b.m_buffer);
            swap(a.m_capacity, b.m_capacity);
            swap(a.m_size, b.m_size);
        }

protected:

        // Change array size.
        void setArraySize(uint new_size) {
            m_size = new_size;

            if (new_size > m_capacity) {
                uint new_buffer_size;
                if (m_capacity == 0) {
                    // first allocation is exact
                    new_buffer_size = new_size;
                }
                else {
                    // following allocations grow array by 25%
                    new_buffer_size = new_size + (new_size >> 2);
                }

                setArrayCapacity( new_buffer_size );
            }
        }

        // Change array capacity.
        void setArrayCapacity(uint new_capacity) {
            nvDebugCheck(new_capacity >= m_size);

            if (new_capacity == 0) {
                // free the buffer.
                if (m_buffer != NULL) {
                    free<T>(m_buffer);
                    m_buffer = NULL;
                }
            }
            else {
                // realloc the buffer
                m_buffer = realloc<T>(m_buffer, new_capacity);
            }

            m_capacity = new_capacity;
        }


        T * m_buffer;
        uint m_capacity;
        uint m_size;

    };

} // nv namespace

#endif // NV_CORE_ARRAY_H

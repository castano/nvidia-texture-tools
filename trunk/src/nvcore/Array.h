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

    template <typename T>
    void construct(T * restrict ptr, uint new_size, uint old_size) {
        for (uint i = old_size; i < new_size; i++) {
            new(ptr+i) T;	// placement new
        }
    }

    template <typename T>
    void construct(T * restrict ptr, uint new_size, uint old_size, const T & value) {
        for (uint i = old_size; i < new_size; i++) {
            new(ptr+i) T(elem);	// placement new
        }
    }

    template <typename T>
    void destroy(T * restrict ptr, uint new_size, uint old_size) {
        for (uint i = new_size; i < old_size; i++) {
            (ptr+i)->~T();  // Explicit call to the destructor
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
    bool find(const T & element, const T * restrict ptr, uint count, uint * index) {
        for (uint i = 0; i < count; i++) {
            if (ptr[i] == element) {
                if (index != NULL) *index = i;
                return true;
            }
        }
        return false;
    }



    class Buffer
    {
        NV_FORBID_COPY(Buffer)
    public:
        // Ctor.
        Buffer() : m_buffer(NULL), m_buffer_size(0), m_size(0)
        {
        }

        // Dtor.
        ~Buffer() {
            allocate(0, 0);
        }


        // Get vector size.
        NV_FORCEINLINE uint size() const { return m_size; }

        // Get vector size.
        NV_FORCEINLINE uint count() const { return m_size; }

        // Is a null vector.
        NV_FORCEINLINE bool isNull() const	{ return m_buffer == NULL; }


        // Swap the members of this vector and the given vector.
        friend void swap(Buffer & a, Buffer & b)
        {
            swap(a.m_buffer, b.m_buffer);
            swap(a.m_buffer_size, b.m_buffer_size);
            swap(a.m_size, b.m_size);
        }

    protected:

        // Release ownership of allocated memory and returns pointer to it.
        NV_NOINLINE void * release() {
            void * tmp = m_buffer;
            m_buffer = NULL;
            m_buffer_size = 0;
            m_size = 0;
            return tmp;
        }

        NV_NOINLINE void resize(uint new_size, uint element_size)
        {
            m_size = new_size;

            if (new_size > m_buffer_size) {
                uint new_buffer_size;
                if (m_buffer_size == 0) {
                    // first allocation
                    new_buffer_size = new_size;
                }
                else {
                    // growing
                    new_buffer_size = new_size + (new_size >> 2);
                }
                allocate( new_buffer_size, element_size );
            }
        }


        /// Change buffer size.
        NV_NOINLINE void allocate(uint count, uint element_size)
        {
            if (count == 0) {
                // free the buffer.
                if (m_buffer != NULL) {
                    ::free(m_buffer);
                    m_buffer = NULL;
                }
            }
            else {
                // realloc the buffer
                m_buffer = ::realloc(m_buffer, count * element_size);
            }

            m_buffer_size = count;
        }


    protected:
        void * m_buffer;
        uint m_buffer_size;
        uint m_size;
    };


    /**
    * Replacement for std::vector that is easier to debug and provides
    * some nice foreach enumerators. 
    */
    template<typename T>
    class NVCORE_CLASS Array : public Buffer {
    public:

        // Default constructor.
        NV_FORCEINLINE Array() : Buffer() {}

        // Copy constructor.
        NV_FORCEINLINE Array(const Array & a) : Buffer() {
            copy(a.buffer(), a.m_size);
        }

        // Constructor that initializes the vector with the given elements.
        NV_FORCEINLINE Array(const T * ptr, int num) : Buffer() {
            copy(ptr, num);
        }

        // Allocate array.
        NV_FORCEINLINE explicit Array(uint capacity) : Buffer() {
            allocate(capacity);
        }

        // Destructor.
        NV_FORCEINLINE ~Array() {
            clear();
        }


        /// Const element access.
        NV_FORCEINLINE const T & operator[]( uint index ) const
        {
            nvDebugCheck(index < m_size);
            return buffer()[index];
        }
        NV_FORCEINLINE const T & at( uint index ) const
        {
            nvDebugCheck(index < m_size);
            return buffer()[index];
        }

        /// Element access.
        NV_FORCEINLINE T & operator[] ( uint index )
        {
            nvDebugCheck(index < m_size);
            return buffer()[index];
        }
        NV_FORCEINLINE T & at( uint index )
        {
            nvDebugCheck(index < m_size);
            return buffer()[index];
        }

        /// Get vector size.
        NV_FORCEINLINE uint size() const { return m_size; }

        /// Get vector size.
        NV_FORCEINLINE uint count() const { return m_size; }

        /// Get const vector pointer.
        NV_FORCEINLINE const T * buffer() const { return (const T *)m_buffer; }

        /// Get vector pointer.
        NV_FORCEINLINE T * buffer() { return (T *)m_buffer; }

        /// Is vector empty.
        NV_FORCEINLINE bool isEmpty() const { return m_size == 0; }

        /// Is a null vector.
        NV_FORCEINLINE bool isNull() const { return m_buffer == NULL; }


        /// Push an element at the end of the vector.
        void push_back( const T & val )
        {
            uint new_size = m_size + 1;

            if (new_size > m_buffer_size)
            {
                const T copy(val);	// create a copy in case value is inside of this array. // @@ Create a copy without side effects. Do not call constructor/destructor here.

                Buffer::resize(new_size, sizeof(T));

                new (buffer()+new_size-1) T(copy);
            }
            else
            {
                m_size = new_size;
                new(buffer()+new_size-1) T(val);
            }
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
        NV_FORCEINLINE Array<T> & operator<< ( T & t )
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
            return buffer()[m_size-1];
        }

        /// Get back element.
        NV_FORCEINLINE T & back()
        {
            nvDebugCheck( m_size > 0 );
            return buffer()[m_size-1];
        }

        /// Get front element.
        NV_FORCEINLINE const T & front() const
        {
            nvDebugCheck( m_size > 0 );
            return buffer()[0];
        }

        /// Get front element.
        NV_FORCEINLINE T & front()
        {
            nvDebugCheck( m_size > 0 );
            return buffer()[0];
        }

        /// Check if the given element is contained in the array.
        NV_FORCEINLINE bool contains(const T & e) const
        {
            return find(e, NULL);
        }

        /// Return true if element found.
        NV_FORCEINLINE bool find(const T & element, uint * index) const
        {
            return find(element, 0, m_size, index);
        }

        /// Return true if element found within the given range.
        NV_FORCEINLINE bool find(const T & element, uint first, uint count, uint * index) const
        {
            return ::nv::find(element, buffer() + first, count, index);
        }

        /// Remove the element at the given index. This is an expensive operation!
        void removeAt(uint index)
        {
            nvCheck(index >= 0 && index < m_size);

            if (m_size == 1) {
                clear();
            }
            else {
                buffer()[index].~T();

                memmove(buffer()+index, buffer()+index+1, sizeof(T) * (m_size - 1 - index));
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
            nvCheck( index >= 0 && index <= m_size );

            resize( m_size + 1 );

            if (index < m_size - 1) {
                memmove(buffer()+index+1, buffer()+index, sizeof(T) * (m_size - 1 - index));
            }

            // Copy-construct into the newly opened slot.
            new(buffer()+index) T(val);
        }

        /// Append the given data to our vector.
        NV_FORCEINLINE void append(const Array<T> & other)
        {
            append(other.buffer(), other.m_size);
        }

        /// Append the given data to our vector.
        void append(const T other[], uint count)
        {
            if (count > 0) {
                const uint old_size = m_size;
                resize(m_size + count);
                // Must use operator=() to copy elements, in case of side effects (e.g. ref-counting).
                for (uint i = 0; i < count; i++ ) {
                    buffer()[old_size + i] = other[i];
                }
            }
        }


        /// Remove the given element by replacing it with the last one.
        void replaceWithLast(uint index)
        {
            nvDebugCheck( index < m_size );
            nv::swap(buffer()[index], back());
            (buffer()+m_size-1)->~T();
            m_size--;
        }


        /// Resize the vector preserving existing elements.
        void resize(uint new_size)
        {
            uint old_size = m_size;

            // Destruct old elements (if we're shrinking).
            destroy(buffer(), new_size, old_size);

            Buffer::resize(new_size, sizeof(T));

            // Call default constructors
            construct(buffer(), new_size, old_size);
        }


        /// Resize the vector preserving existing elements and initializing the
        /// new ones with the given value.
        void resize( uint new_size, const T &elem )
        {
            uint old_size = m_size;

            // Destruct old elements (if we're shrinking).
            destroy(buffer(), new_size, old_size);

            Buffer::resize(new_size, sizeof(T));

            // Call copy constructors
            construct(buffer(), new_size, old_size, elem);
        }

        /// Clear the buffer.
        NV_FORCEINLINE void clear()
        {
            resize(0);
        }

        /// Shrink the allocated vector.
        NV_FORCEINLINE void shrink()
        {
            if (m_size < m_buffer_size) {
                allocate(m_size);
            }
        }

        /// Preallocate space.
        NV_FORCEINLINE void reserve(uint desired_size)
        {
            if (desired_size > m_buffer_size) {
                allocate(desired_size);
            }
        }

        /// Copy elements to this array. Resizes it if needed.
        NV_FORCEINLINE void copy(const T * ptr, uint num)
        {
            resize( num );
            ::nv::copy(buffer(), ptr, num);
        }

        /// Assignment operator.
        NV_FORCEINLINE Array<T> & operator=( const Array<T> & a )
        {
            copy(a.buffer(), a.m_size);
            return *this;
        }

        // Release ownership of allocated memory and returns pointer to it.
        T * release() {
            return (T *)Buffer::release();
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
                s << buffer()[i];
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
            return at(i(this));
        }
        NV_FORCEINLINE const T & operator[]( const PseudoIndexWrapper & i ) const {
            return at(i(this));
        }
#endif

protected:

        NV_FORCEINLINE void allocate(uint count) {
            Buffer::allocate(count, sizeof(T));
        }

    };

} // nv namespace

#endif // NV_CORE_ARRAY_H

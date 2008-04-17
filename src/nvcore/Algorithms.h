// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_ALGORITHMS_H
#define NV_CORE_ALGORITHMS_H

namespace nv
{

	/// Return the maximum of two values.
	template <typename T> 
	inline const T & max(const T & a, const T & b)
	{
		//return std::max(a, b);
		if( a < b ) {
			return b; 
		}
		return a;
	}
	
	/// Return the minimum of two values.
	template <typename T> 
	inline const T & min(const T & a, const T & b)
	{
		//return std::min(a, b);
		if( b < a ) {
			return b; 
		}
		return a;
	}

	/// Clamp between two values.
	template <typename T> 
	inline const T & clamp(const T & x, const T & a, const T & b)
	{
		return min(max(x, a), b);
	}
	
	/// Delete all the elements of a container.
	template <typename T>
	void deleteAll(T & container)
	{
		for (typename T::PseudoIndex i = container.start(); !container.isDone(i); container.advance(i))
		{
			delete container[i];
		}
	}
	
	
	// @@ Swap should be implemented here.
	
	
#if 0
	// This does not use swap, but copies, in some cases swaps are much faster than copies!
	// Container should implement operator[], and size()
	template <class Container, class T>
	void insertionSort(Container<T> & container)
	{
		const uint n = container.size();
		for (uint i=1; i < n; ++i)
		{
			T value = container[i];
			uint j = i;
			while (j > 0 && container[j-1] > value)
			{
				container[j] = container[j-1];
				--j;
			}
			if (i != j)
			{
				container[j] = value;
			}
		}
	}

	template <class Container, class T>
	void quickSort(Container<T> & container)
	{
		quickSort(container, 0, container.count());
	}
	
	{
		/* threshhold for transitioning to insertion sort */
		while (n > 12) {
			int c01,c12,c,m,i,j;

			/* compute median of three */
			m = n >> 1;
			c = p[0] > p[m];
			c01 = c;
			c = &p[m] > &p[n-1];
			c12 = c;
			/* if 0 >= mid >= end, or 0 < mid < end, then use mid */
			if (c01 != c12) {
				/* otherwise, we'll need to swap something else to middle */
				int z;
				c = p[0] < p[n-1];
				/* 0>mid && mid<n:  0>n => n; 0<n => 0 */
				/* 0<mid && mid>n:  0>n => 0; 0<n => n */
				z = (c == c12) ? 0 : n-1;
				swap(p[z], p[m]);
			}
			/* now p[m] is the median-of-three */
			/* swap it to the beginning so it won't move around */
			swap(p[0], p[m]);

			/* partition loop */
			i=1;
			j=n-1;
			for(;;) {
				/* handling of equality is crucial here */
				/* for sentinels & efficiency with duplicates */
				for (;;++i) {
					c = p[i] > p[0];
					if (!c) break;
				}
				a = &p[0];
				for (;;--j) {
					b=&p[j];
					c = p[j] > p[0]
					if (!c) break;
				}
				/* make sure we haven't crossed */
				if (i >= j) break;
				swap(p[i], p[j]);

				++i;
				--j;
			}
			/* recurse on smaller side, iterate on larger */
			if (j < (n-i)) {
				quickSort(p, j);
				p = p+i;
				n = n-i;
			} 
			else {
				quickSort(p+i, n-i);
				n = j;
			}
		}
		
		insertionSort();
	}
#endif // 0

} // nv namespace

#endif // NV_CORE_ALGORITHMS_H

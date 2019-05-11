// Based on the .\coreclr\src\gc\gcdesc.h file which is:
// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
//
//
// GC Object Pointer Location Series Stuff
//
// Original file has been refactored to make more clean and self-understandable code
// (while not necessary equally performant), mostly:
// * DACCESS_COMPILE blocks removed

#pragma once
#include <cstdint>

#ifdef BIT64
typedef uint32_t HALF_SIZE_T;
#else   // BIT64
typedef uint16_t HALF_SIZE_T;
#endif


typedef size_t* JSlot;

//
// These two classes make up the apparatus with which the object references
// within an object can be found.
//
// CGCDescSeries:
//
// The CGCDescSeries class describes a series of object references within an
// object by describing the size of the series (which has an adjustment which
// will be explained later) and the starting point of the series.
//
// The series size is adjusted when the map is created by subtracting the
// GetBaseSize() of the object.   On retieval of the size the total size
// of the object is added back.   For non-array objects the total object
// size is equal to the base size, so this returns the same value.   For
// array objects this will yield the size of the data portion of the array.
// Since arrays containing object references will contain ONLY object references
// this is a fast way of handling arrays and normal objects without a
// conditional test
//
//
//
// CGCDesc:
//
// The CGCDesc is a collection of CGCDescSeries objects to describe all the
// different runs of pointers in a particular object.   
// <TODO> 
// add more on the strange way the CGCDesc grows backwards in memory behind 
// the MethodTable
//</TODO>

struct val_serie_item
{
	HALF_SIZE_T nptrs;
	HALF_SIZE_T skip;
	void set_val_serie_item(HALF_SIZE_T nptrs, HALF_SIZE_T skip)
	{
		this->nptrs = nptrs;
		this->skip = skip;
	}
};

struct val_array_series
{
	val_serie_item  items[1];
	size_t          m_startOffset;
	size_t          m_count;
};

typedef class CGCDescSeries* PTR_CGCDescSeries;
typedef class MethodTable* PTR_MethodTable;

class CGCDescSeries
{
public:
	union
	{
		size_t seriessize;              // adjusted length of series (see above) in bytes
		val_serie_item val_serie[1];    //coded serie for value class array
	};

	size_t startoffset;

	size_t GetSeriesCount()
	{
		return seriessize / sizeof(JSlot);
	}

	void SetSeriesCount(size_t newcount)
	{
		seriessize = newcount * sizeof(JSlot);
	}

	void IncSeriesCount(size_t increment = 1)
	{
		seriessize += increment * sizeof(JSlot);
	}

	size_t GetSeriesSize()
	{
		return seriessize;
	}

	void SetSeriesSize(size_t newsize)
	{
		seriessize = newsize;
	}

	void SetSeriesValItem(val_serie_item item, int index)
	{
		val_serie[index] = item;
	}

	void SetSeriesOffset(size_t newoffset)
	{
		startoffset = newoffset;
	}

	size_t GetSeriesOffset()
	{
		return startoffset;
	}
};

typedef class CGCDesc* PTR_CGCDesc;

class CGCDesc
{
	// Don't construct me, you have to hand me a ptr to the *top* of my storage in Init.
	CGCDesc() {}

	//
	// NOTE: for alignment reasons, NumSeries is stored as a size_t.
	//       This makes everything nicely 8-byte aligned on IA64.
	//
public:
	static size_t ComputeSize(size_t NumSeries)
	{
		assert(ptrdiff_t(NumSeries) > 0);

		return sizeof(size_t) + NumSeries * sizeof(CGCDescSeries);
	}

	// For value type array
	static size_t ComputeSizeRepeating(size_t NumSeries)
	{
		assert(ptrdiff_t(NumSeries) > 0);

		return sizeof(size_t) + sizeof(CGCDescSeries) +
			(NumSeries - 1) * sizeof(val_serie_item);
	}

	static PTR_CGCDesc GetCGCDescFromMT(MethodTable * pMT)
	{
		PTR_MethodTable mt(pMT);
		// TODO: Removed as currently MethodTable is incomplete here
		//_ASSERTE(mt->ContainsPointersOrCollectible()); // If it doesn't contain pointers, there isn't a GCDesc
		return PTR_CGCDesc(mt);
	}

	size_t GetNumSeries()
	{
		return *((size_t*)(PTR_CGCDesc(this)) - 1);
	}

	// Returns lowest series in memory.
	// Cannot be used for valuetype arrays
	PTR_CGCDescSeries GetLowestSeries()
	{
		assert(ptrdiff_t(GetNumSeries()) > 0);
		return PTR_CGCDescSeries((uint8_t*)(PTR_CGCDesc(this))
			- ComputeSize(GetNumSeries()));
	}

	// Returns highest series in memory.
	PTR_CGCDescSeries GetHighestSeries()
	{
		return PTR_CGCDescSeries((size_t*)(PTR_CGCDesc(this)) - 1) - 1;
	}

	// Returns number of immediate pointers this object has.
	// size is only used if you have an array of value types.
	static size_t GetNumPointers(MethodTable * pMT, size_t ObjectSize, size_t NumComponents)
	{
		size_t NumOfPointers = 0;
		CGCDesc* map = GetCGCDescFromMT(pMT);
		CGCDescSeries* cur = map->GetHighestSeries();
		ptrdiff_t cnt = (ptrdiff_t)map->GetNumSeries();

		if (cnt > 0)
		{
			CGCDescSeries* last = map->GetLowestSeries();
			while (cur >= last)
			{
				NumOfPointers += (cur->GetSeriesSize() + ObjectSize) / sizeof(JSlot);
				cur--;
			}
		}
		else
		{
			/* Handle the repeating case - array of valuetypes */
			for (ptrdiff_t __i = 0; __i > cnt; __i--)
			{
				NumOfPointers += cur->val_serie[__i].nptrs;
			}

			NumOfPointers *= NumComponents;
		}

		return NumOfPointers;
	}

	// Size of the entire slot map.
	size_t GetSize()
	{
		ptrdiff_t numSeries = (ptrdiff_t)GetNumSeries();
		if (numSeries < 0)
		{
			return ComputeSizeRepeating(-numSeries);
		}
		else
		{
			return ComputeSize(numSeries);
		}
	}

	uint8_t* GetStartOfGCData()
	{
		return ((uint8_t*)this) - GetSize();
	}

private:

	bool IsValueClassSeries()
	{
		return ((ptrdiff_t)GetNumSeries()) < 0;
	}

};
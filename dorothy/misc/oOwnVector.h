/* Copyright (c) 2013 Jin Li, http://www.luvfight.me

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef __DOROTHY_MISC_OOWNVECTOR_H__
#define __DOROTHY_MISC_OOWNVECTOR_H__

#include "misc/oOwn.h"

NS_DOROTHY_BEGIN

/** @brief vector of pointers, but accessed as values
 pointers pushed into oOwnVector are owned by the vector,
 pointers will be auto deleted when it`s erased/removed from the vector
 or the vector is destroyed.
 Used with Composition Relationship.
*/
template<class T>
class oOwnVector: public vector<oOwn<T>>
{
public:
	inline void push_back(T* item)
	{
		vector<oOwn<T>>::push_back(oOwnMake(item));
	}
	inline bool insert(size_t where, T* item)
	{
		if (where >= 0 && where < vector<oOwn<T>>::size())
		{
			auto it = vector<oOwn<T>>::begin();
			for (int i = 0; i < where; ++i, ++it);
			vector<oOwn<T>>::insert(it, oOwnMake(item));
			return true;
		}
		return false;
	}
	bool remove(T* item)
	{
		for (auto it = vector<oOwn<T>>::begin(); it != vector<oOwn<T>>::end(); ++it)
		{
			if ((*it) == item)
			{
				vector<oOwn<T>>::erase(it);
				return true;
			}
		}
		return false;
	}
};

NS_DOROTHY_END

#endif // __DOROTHY_MISC_OOWNVECTOR_H__

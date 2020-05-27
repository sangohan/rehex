/* Reverse Engineer's Hex Editor
 * Copyright (C) 2020 Daniel Collins <solemnwarning@solemnwarning.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <assert.h>

#include "ByteRangeSet.hpp"

void REHex::ByteRangeSet::set_range(off_t offset, off_t length)
{
	if(length <= 0)
	{
		return;
	}
	
	/* Find the range of elements that intersects the one we are inserting. They will be erased
	 * and the one we are creating will grow on either end as necessary to encompass them.
	*/
	
	auto next = ranges.lower_bound(Range((offset + length), 0));
	
	std::set<Range>::iterator erase_begin = next;
	std::set<Range>::iterator erase_end   = next;
	
	while(erase_begin != ranges.begin())
	{
		auto eb_prev = std::prev(erase_begin);
		
		if((eb_prev->offset + eb_prev->length) >= offset)
		{
			assert(eb_prev->offset <= offset);
			
			off_t merged_begin = std::min(eb_prev->offset, offset);
			off_t merged_end   = std::max((eb_prev->offset + eb_prev->length), (offset + length));
			
			offset = merged_begin;
			length = merged_end - merged_begin;
			
			erase_begin = eb_prev;
		}
		else{
			break;
		}
	}
	
	if(erase_end != ranges.end() && erase_end->offset == (offset + length))
	{
		length += erase_end->length;
		++erase_end;
	}
	
	/* Erase adjacent and/or overlapping ranges. */
	ranges.erase(erase_begin, erase_end);
	
	assert(length > 0);
	
	/* Insert new range. */
	ranges.insert(Range(offset, length));
}

void REHex::ByteRangeSet::clear_range(off_t offset, off_t length)
{
	if(length <= 0)
	{
		return;
	}
	
	/* Find the range of elements overlapping the range to be cleared. */
	
	auto next = ranges.lower_bound(Range((offset + length), 0));
	
	std::set<Range>::iterator erase_begin = next;
	std::set<Range>::iterator erase_end   = next;
	
	while(erase_begin != ranges.begin())
	{
		auto eb_prev = std::prev(erase_begin);
		
		if((eb_prev->offset + eb_prev->length) >= offset)
		{
			erase_begin = eb_prev;
		}
		else{
			break;
		}
	}
	
	/* If the elements to be erased to not fall fully within the given range, then we shall
	 * populate collateral_damage with up to two elements to re-instate the lost ranges.
	*/
	
	std::set<Range> collateral_damage;
	if(erase_begin != erase_end)
	{
		assert(erase_begin->offset <= offset);
		
		if(erase_begin->offset < offset)
		{
			/* Clear range is within erase range, so create a new Range from the start
			 * of the range to be erased up to the start of the range to be cleared.
			*/
			
			collateral_damage.insert(Range(erase_begin->offset, (offset - erase_begin->offset)));
		}
		
		auto erase_last = std::prev(erase_end);
		
		if((erase_last->offset + erase_last->length) > (offset + length))
		{
			/* Clear range falls short of the end of the range to be erased, so create
			 * a range from the end of the clear range to the end of the erase range.
			*/
			
			off_t from = offset + length;
			off_t to   = erase_last->offset + erase_last->length;
			
			assert(to > from);
			
			collateral_damage.insert(Range(from, (to - from)));
		}
	}
	
	ranges.erase(erase_begin, erase_end);
	ranges.insert(collateral_damage.begin(), collateral_damage.end());
}

bool REHex::ByteRangeSet::isset(off_t offset) const
{
	auto lb = ranges.lower_bound(Range(offset, 0));
	
	if(lb != ranges.end() && lb->offset == offset)
	{
		return true;
	}
	else if(lb != ranges.begin())
	{
		--lb;
		
		if(lb->offset <= offset && (lb->offset + lb->length) > offset)
		{
			return true;
		}
	}
	
	return false;
}

const std::set<REHex::ByteRangeSet::Range> &REHex::ByteRangeSet::get_ranges() const
{
	return ranges;
}
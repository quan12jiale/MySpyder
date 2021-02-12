#include "intervaltree/interval.h"


Interval::Interval(int begin, int end)
{
	this->begin = begin;
	this->end = end;
}

// Whether the interval overlapsÖØµş the given point, range or Interval.
bool Interval::overlaps(int begin, int end /*= INT32_MAX*/) const
{
	if (end != INT32_MAX)
	{
		return begin < this->end && end > this->begin;
	}
	return this->contains_point(begin);
}

bool Interval::overlaps(const Interval& begin) const
{
	return this->overlaps(begin.begin, begin.end);
}

bool Interval::contains_point(int p) const
{
	return this->begin <= p && p < this->end;
}

bool Interval::range_matches(const Interval& other) const
{
	return this->begin == other.begin && this->end == other.end;
}

bool Interval::contains_interval(const Interval& other) const
{
	return this->begin <= other.begin && this->end >= other.end;
}

int Interval::distance_to(const Interval& other) const
{
	if (this->overlaps(other))
	{
		return 0;
	}
	if (this->begin < other.begin)
		return other.begin - this->end;
	else
		return this->begin - other.end;
}

bool Interval::is_null() const
{
	return this->begin > this->end;
}

int Interval::length() const
{
	if (this->is_null())
	{
		return 0;
	}
	return this->end - this->begin;
}


int Interval::__cmp__(const Interval& other) const
{
	// #TODO Sorting is by begins, then by ends, then by data fields.
	auto this_pair = std::make_pair(this->begin, this->end);
	auto other_pair = std::make_pair(other.begin, other.end);
	if (this_pair < other_pair)
	{
		return -1;
	}
	if (this_pair > other_pair)
	{
		return 1;
	}
	return 0;
}

bool Interval::operator<(const Interval& other) const
{
	return this->__cmp__(other) < 0;
}

bool Interval::operator>(const Interval& other) const
{
	return this->__cmp__(other) > 0;
}

QString Interval::repr() const
{
	return QString("Interval(%0, %1)").arg(this->begin).arg(this->end);
}

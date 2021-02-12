#pragma once
#include <QSet>

class Interval
{
public:
	int begin;
	int end;

public:
	Interval(int begin, int end);
	bool overlaps(int begin, int end = INT32_MAX) const;
	bool overlaps(const Interval& begin) const;
	bool contains_point(int p) const;
	bool range_matches(const Interval& other) const;
	bool contains_interval(const Interval& other) const;

	int distance_to(const Interval& other) const;
	bool is_null() const;
	int length() const;

	// #注意: qHash函数和operator==函数需要在.h文件中实现
	friend uint qHash(const Interval &key, uint seed)
	{ return qHash(key.begin, seed) ^ key.end; }
	bool operator==(const Interval& other) const
	{ 
		return this->begin == other.begin && this->end == other.end;
		// self.data == other.data
	}

	int __cmp__(const Interval& other) const;
	bool operator<(const Interval& other) const;
	bool operator>(const Interval& other) const;

	QString repr() const;
};
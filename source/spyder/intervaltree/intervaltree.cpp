#include "intervaltree/intervaltree.h"

QString strErrorArg = QStringLiteral("IntervalTree: Null Interval objects not allowed in IntervalTree: %1");

IntervalTree* IntervalTree::from_tuples(const QList<int>& begins, const QList<int>& ends)
{
	if (begins.size() != ends.size())
	{
		return nullptr;
	}
	QList<Interval> ivs;
	ivs.reserve(begins.size());
	for (int i = 0; i < begins.size(); i++)
	{
		ivs.append(Interval(begins[i], ends[i]));
	}
	return new IntervalTree(ivs);
}

IntervalTree::IntervalTree(const QList<Interval>& interval_list /*= QList<Interval>()*/)
{
	this->all_intervals = interval_list.toSet();
	for (const auto& iv : this->all_intervals)
	{
		if (iv.is_null())
		{
			QString strError = strErrorArg.arg(iv.repr());
			throw std::exception(strError.toStdString().c_str());
		}
	}
	this->top_node = TreeNode::from_intervals(this->all_intervals.toList());
	for (const auto& iv : this->all_intervals)
	{
		this->_add_boundaries(iv);
	}
}

IntervalTree::~IntervalTree()
{
	if (this->top_node)
	{
		delete this->top_node;
	}
}

void IntervalTree::_add_boundaries(const Interval& interval)
{
	int begin = interval.begin;
	int end = interval.end;
	if (this->boundary_table.contains(begin))
	{
		this->boundary_table[begin] += 1;
	}
	else
	{
		this->boundary_table[begin] = 1;
	}

	if (this->boundary_table.contains(end))
	{
		this->boundary_table[end] += 1;
	}
	else
	{
		this->boundary_table[end] = 1;
	}
}

QSet<Interval> IntervalTree::envelop(int begin, int end) const
{
	QSet<Interval> result;
	TreeNode* root = this->top_node;
	if (!root)
	{
		return result;
	}
	if (begin >= end)
	{
		return result;
	}
	result = root->search_point(begin, QSet<Interval>());

	QMap<int, int> boundary_table = this->boundary_table;
	// std::distance(first, last)返回迭代器之间的距离
	int bound_begin = std::distance(boundary_table.begin(),
		boundary_table.lowerBound(begin));
	int bound_end = std::distance(boundary_table.begin(),
		boundary_table.lowerBound(end));

	QList<int> point_list = boundary_table.keys().mid(bound_begin, bound_end - bound_begin);
	QSet<Interval> tmp_set = root->search_overlap(point_list);
	result += tmp_set;

	QSet<Interval> res;
	for (const auto& iv :result)
	{
		if (iv.begin >= begin && iv.end <= end)
		{
			res.insert(iv);
		}
	}
	return res;
}


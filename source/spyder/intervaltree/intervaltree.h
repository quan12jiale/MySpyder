#pragma once
#include <QMap>
#include "intervaltree/treenode.h"

class IntervalTree
{
public:
	QSet<Interval> all_intervals;
	TreeNode* top_node;
	QMap<int, int> boundary_table;

public:
	static IntervalTree* from_tuples(const QList<int>& begins, const QList<int>& ends);
	IntervalTree(const QList<Interval>& interval_list = QList<Interval>());
	~IntervalTree();
	void _add_boundaries(const Interval& interval);

	QSet<Interval> envelop(int begin, int end) const;
};
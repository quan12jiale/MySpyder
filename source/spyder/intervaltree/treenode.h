#pragma once
#include "intervaltree/interval.h"

class TreeNode
{
public:
	int x_center;
	QSet<Interval> s_center;
	TreeNode* left_node;
	TreeNode* right_node;
	int depth;
	int balance;

public:
	TreeNode(int x_center = INT32_MAX,
		const QList<Interval>& s_center_list = QList<Interval>(),
		TreeNode* left_node = nullptr,
		TreeNode* right_node = nullptr);
	~TreeNode();
	static TreeNode* from_interval(const Interval& interval);
	static TreeNode* from_intervals(QList<Interval> intervals);
	TreeNode* init_from_sorted(const QList<Interval>& intervals);

	bool center_hit(const Interval& interval) const;
	bool hit_branch(const Interval& interval) const;
	void refresh_balance();
	int compute_depth() const;

	TreeNode* rotate();
	TreeNode* srotate();
	TreeNode* drotate();

	TreeNode* add(const Interval& interval);
	TreeNode* remove(const Interval& interval);
	TreeNode* discard(const Interval& interval);
	TreeNode* remove_interval_helper(const Interval& interval, QList<int>& done, bool should_raise_error);

	QSet<Interval> search_overlap(const QList<int>& point_list);
	QSet<Interval> search_point(int point, QSet<Interval>& result);
	TreeNode* prune();
	std::pair<TreeNode*, TreeNode*> pop_greatest_child();
	bool contains_point(int p);
	QSet<Interval> all_children();
	QSet<Interval> all_children_helper(QSet<Interval>& result);
	void verify(const QSet<int>& parents);

	TreeNode* &at(bool index);
	QString str() const;
	int count_nodes() const;
	double depth_score(int n, int m) const;
	int depth_score_helper(int d, int dopt) const;

	QString print_structure(int indent = 0) const;
};
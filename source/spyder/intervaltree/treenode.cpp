#include "intervaltree/treenode.h"
#include "qmath.h"

TreeNode::TreeNode(int x_center /*= INT32_MAX*/, const QList<Interval>& s_center_list /*= QList<Interval>()*/, TreeNode* left_node /*= nullptr*/, TreeNode* right_node /*= nullptr*/)
{
	this->x_center = x_center;
	this->s_center = s_center_list.toSet();
	this->left_node = left_node;
	this->right_node = right_node;
	this->depth = 0;
	this->balance = 0;
	this->rotate();
}

TreeNode::~TreeNode()
{
	if (this->left_node)
	{
		delete this->left_node;
	}
	if (this->right_node)
	{
		delete this->right_node;
	}
}

TreeNode* TreeNode::from_interval(const Interval& interval)
{
	int center = interval.begin;
	QList<Interval> s_center_list;
	s_center_list.append(interval);
	return new TreeNode(center, s_center_list);
}

TreeNode* TreeNode::from_intervals(QList<Interval> intervals)
{
	if (intervals.isEmpty())
	{
		return nullptr;
	}
	TreeNode* node = new TreeNode;
	std::sort(intervals.begin(), intervals.end());
	node = node->init_from_sorted(intervals);
	return node;
}

TreeNode* TreeNode::init_from_sorted(const QList<Interval>& intervals)
{
	Interval center_iv = intervals[intervals.size() / 2];
	this->x_center = center_iv.begin;
	this->s_center.clear();
	QList<Interval> s_left, s_right;
	for (const auto& k : intervals)
	{
		if (k.end <= this->x_center)
		{
			s_left.append(k);
		}
		else if (k.begin > this->x_center)
		{
			s_right.append(k);
		}
		else
		{
			this->s_center.insert(k);
		}
	}
	this->left_node = TreeNode::from_intervals(s_left);
	this->right_node = TreeNode::from_intervals(s_right);
	return this->rotate();
}

bool TreeNode::center_hit(const Interval& interval) const
{
	return interval.contains_point(this->x_center);
}

bool TreeNode::hit_branch(const Interval& interval) const
{
	return interval.begin > this->x_center;
}

void TreeNode::refresh_balance()
{
	int left_depth = this->left_node ? this->left_node->depth : 0;
	int right_depth = this->right_node ? this->right_node->depth : 0;
	this->depth = 1 + std::max(left_depth, right_depth);
	this->balance = right_depth - left_depth;
}

/*
Recursively computes true depth of the subtree. Should only
be needed for debugging. Unless something is wrong, the
depth field should reflect the correct depth of the subtree.
*/
int TreeNode::compute_depth() const
{
	int left_depth = this->left_node ? this->left_node->compute_depth() : 0;
	int right_depth = this->right_node ? this->right_node->compute_depth() : 0;
	return 1 + std::max(left_depth, right_depth);
}

TreeNode* TreeNode::rotate()
{
	this->refresh_balance();
	if (std::abs(this->balance) < 2)
		return this;
	bool my_heavy = this->balance > 0;
	bool child_heavy = this->at(my_heavy)->balance > 0;
	if (my_heavy == child_heavy || this->at(my_heavy)->balance == 0)
	{
		return this->srotate();
	}
	else
	{
		return this->drotate();
	}
}

TreeNode* TreeNode::srotate()
{
	bool heavy = this->balance > 0;
	bool light = !heavy;
	TreeNode* save = this->at(heavy);
	this->at(heavy) = save->at(light);
	save->at(light) = this->rotate();

	QList<Interval> promotees;
	QSet<Interval> save_s_center = save->at(light)->s_center;
	for (auto const& iv : save_s_center)
	{
		if (save->center_hit(iv))
		{
			promotees.append(iv);
		}
	}
	if (!promotees.isEmpty())
	{
		for (auto const& iv : promotees)
		{
			save->at(light) = save->at(light)->remove(iv);
		}
		save->s_center += promotees.toSet();
	}
	save->refresh_balance();
	return save;
}

TreeNode* TreeNode::drotate()
{
	bool my_heavy = this->balance > 0;
	this->at(my_heavy) = this->at(my_heavy)->srotate();
	this->refresh_balance();
	TreeNode* result = this->srotate();
	return result;
}

TreeNode* TreeNode::add(const Interval& interval)
{
	if (this->center_hit(interval))
	{
		this->s_center.insert(interval);
		return this;
	}
	else
	{
		bool direction = this->hit_branch(interval);
		if (!this->at(direction))
		{
			this->at(direction) = TreeNode::from_interval(interval);
			this->refresh_balance();
			return this;
		}
		else
		{
			this->at(direction) = this->at(direction)->add(interval);
			return this->rotate();
		}
	}
}

TreeNode* TreeNode::remove(const Interval& interval)
{
	QList<int> done;
	return this->remove_interval_helper(interval, done, true);
}

TreeNode* TreeNode::discard(const Interval& interval)
{
	QList<int> done;
	return this->remove_interval_helper(interval, done, false);
}

TreeNode* TreeNode::remove_interval_helper(const Interval& interval, QList<int>& done, bool should_raise_error)
{
	if (this->center_hit(interval))
	{
		if (!should_raise_error && !this->s_center.contains(interval))
		{
			done.append(1);
			return this;
		}
		if (this->s_center.contains(interval))
		{
			this->s_center.remove(interval);
		}
		else
		{
			QString structure = this->print_structure();
			throw std::runtime_error("KeyError");
		}
		if (!this->s_center.isEmpty())
		{
			done.append(1);
			return this;
		}
		return this->prune();
	}
	else
	{
		bool direction = this->hit_branch(interval);

		if (!this->at(direction))
		{
			if (should_raise_error)
			{
				throw std::runtime_error("ValueError");
			}
			done.append(1);
			return this;
		}

		this->at(direction) = this->at(direction)->remove_interval_helper(interval, done, should_raise_error);

		if (done.isEmpty())
		{
			return this->rotate();
		}
		return this;
	}
}

QSet<Interval> TreeNode::search_overlap(const QList<int>& point_list)
{
	QSet<Interval> result;
	for (const auto& j : point_list)
	{
		this->search_point(j, result);
	}
	return result;
}

QSet<Interval> TreeNode::search_point(int point, QSet<Interval>& result)
{
	for (const auto& k : this->s_center)
	{
		if (k.begin <= point && point < k.end)
		{
			result.insert(k);
		}
	}
	if (point < this->x_center && this->at(false))
	{
		return this->at(false)->search_point(point, result);
	}
	else if (point > this->x_center && this->at(true))
	{
		return this->at(true)->search_point(point, result);
	}
	return result;
}

TreeNode* TreeNode::prune()
{
	if (!this->at(false) || !this->at(true))
	{
		bool direction = !this->at(false);
		TreeNode* result = this->at(direction);
		return result;
	}
	else
	{
		auto pair = this->at(false)->pop_greatest_child();
		TreeNode* heir = pair.first;
		this->at(false) = pair.second;

		heir->at(false) = this->at(false);
		heir->at(true) = this->at(true);

		heir->refresh_balance();
		heir = heir->rotate();
		return heir;
	}
}

static bool sortfunc(const Interval& iv1, const Interval& iv2)
{
	if (iv1.end != iv2.end)
	{
		return iv1.end < iv2.end;
	}
	return iv1.begin < iv2.begin;
}

std::pair<TreeNode*, TreeNode*> TreeNode::pop_greatest_child()
{
	if (!this->right_node)
	{
		auto ivs = this->s_center.toList();
		std::sort(ivs.begin(), ivs.end(), sortfunc);
		auto max_iv = ivs.takeLast();
		int new_x_center = this->x_center;
		while (!ivs.isEmpty())
		{
			auto next_max_iv = ivs.takeLast();
			if (next_max_iv.end == max_iv.end)
			{
				continue;
			}
			new_x_center = std::max(new_x_center, next_max_iv.end);
		}

		auto get_new_s_center = [&]()->QList<Interval>
		{
			QList<Interval> result;
			for (const auto& iv : this->s_center)
			{
				if (iv.contains_point(new_x_center))
				{
					result.append(iv);
				}
			}
			return result;
		};

		TreeNode* child = new TreeNode(new_x_center, get_new_s_center());
		this->s_center -= child->s_center;
		if (!this->s_center.isEmpty())
		{
			return std::make_pair(child, this);
		}
		else
		{
			return std::make_pair(child, this->at(false));
		}
	}
	else
	{
		auto pair = this->at(true)->pop_greatest_child();
		TreeNode* greatest_child = pair.first;
		this->at(true) = pair.second;

		QSet<Interval> s_center_copy = this->s_center;
		for (const auto& iv : s_center_copy)
		{
			if (iv.contains_point(greatest_child->x_center))
			{
				this->s_center.remove(iv);
				greatest_child->add(iv);
			}
		}

		if (!this->s_center.isEmpty())
		{
			this->refresh_balance();
			TreeNode* new_self = this->rotate();
			return std::make_pair(greatest_child, new_self);
		}
		else
		{
			TreeNode* new_self = this->prune();
			return std::make_pair(greatest_child, new_self);
		}
	}
}

bool TreeNode::contains_point(int p)
{
	for (const auto& iv : this->s_center)
	{
		if (iv.contains_point(p))
		{
			return true;
		}
	}
	TreeNode* branch = this->at(p > this->x_center);
	return branch && branch->contains_point(p);
}

QSet<Interval> TreeNode::all_children()
{
	QSet<Interval> tmpResult;
	return this->all_children_helper(tmpResult);
}

QSet<Interval> TreeNode::all_children_helper(QSet<Interval>& result)
{
	result += this->s_center;
	if (this->at(false))
	{
		this->at(false)->all_children_helper(result);
	}
	if (this->at(true))
	{
		this->at(true)->all_children_helper(result);
	}
	return result;
}

/*
## DEBUG ONLY ##
Recursively ensures that the invariants of an interval subtree hold.
*/
void TreeNode::verify(const QSet<int>& parents)
{

}

/*
在类中好像没法调用operator[]，因此改用at函数实现
对应__getitem__、__setitem__函数
*/
TreeNode* & TreeNode::at(bool index)
{
	if (index)
	{
		return this->right_node;
	}
	else
	{
		return this->left_node;
	}
}

QString TreeNode::str() const
{
	return QString("Node<%0, depth=%1, balance=%2>").arg(this->x_center)
		.arg(this->depth).arg(this->balance);
}

int TreeNode::count_nodes() const
{
	int count = 1;
	if (this->left_node)
	{
		count += this->left_node->count_nodes();
	}
	if (this->right_node)
	{
		count += this->right_node->count_nodes();
	}
	return count;
}

double TreeNode::depth_score(int n, int m) const
{
	int dopt = 1 + qFloor(std::log2(m));
	double f = 1.0 / static_cast<double>(1 + n - dopt);
	return f * this->depth_score_helper(1, dopt);
}

/*
Gets a weighted count of the number of Intervals deeper than dopt.
:param d: current depth, starting from 0
:param dopt: optimal maximum depth of a leaf Node
*/
int TreeNode::depth_score_helper(int d, int dopt) const
{
	int di = d - dopt;
	int count;
	if (di > 0)
	{
		count = di * this->s_center.size();
	}
	else
	{
		count = 0;
	}
	if (this->right_node)
	{
		count += this->right_node->depth_score_helper(d + 1, dopt);
	}
	if (this->left_node)
	{
		count += this->left_node->depth_score_helper(d + 1, dopt);
	}
	return count;
}

QString TreeNode::print_structure(int indent /*= 0*/) const
{
	QString nl = "\n";
	QString sp;
	if (indent > 0)
	{
		sp = QString(4 * indent, ' ');
	}
	QStringList rlist;
	rlist.append(this->str() + nl);
	if (!this->s_center.isEmpty())
	{
		auto s_center_list = this->s_center.toList();
		std::sort(s_center_list.begin(), s_center_list.end());
		for (const auto& iv : s_center_list)
		{
			rlist.append(sp + " " + iv.repr() + nl);
		}
	}
	if (this->left_node)
	{
		rlist.append(sp + "<:  ");
		rlist.append(this->left_node->print_structure(indent + 1));
	}
	if (this->right_node)
	{
		rlist.append(sp + ">:  ");
		rlist.append(this->right_node->print_structure(indent + 1));
	}
	QString result = rlist.join("");
	return result;
}

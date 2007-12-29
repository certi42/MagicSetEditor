//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2007 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/graph.hpp>
#include <util/alignment.hpp>
#include <gfx/gfx.hpp>
#include <wx/dcbuffer.h>

DECLARE_TYPEOF_COLLECTION(GraphAxisP);
DECLARE_TYPEOF_COLLECTION(GraphElementP);
DECLARE_TYPEOF_COLLECTION(GraphGroup);
DECLARE_TYPEOF_COLLECTION(GraphP);
DECLARE_TYPEOF_COLLECTION(int);
DECLARE_TYPEOF_COLLECTION(vector<int>);
DECLARE_TYPEOF_COLLECTION(String);
DECLARE_TYPEOF_COLLECTION(UInt);
DECLARE_TYPEOF(map<String COMMA UInt>);

template <typename T> inline T sgn(T v) { return v < 0 ? -1 : 1; }

// ----------------------------------------------------------------------------- : Events

DEFINE_EVENT_TYPE(EVENT_GRAPH_SELECT);

// ----------------------------------------------------------------------------- : GraphData

GraphElement::GraphElement(const String& v1) {
	values.push_back(v1);
}
GraphElement::GraphElement(const String& v1, const String& v2) {
	values.push_back(v1);
	values.push_back(v2);
}

void GraphDataPre::splitList(size_t axis) {
	size_t count = elements.size(); // only the elements that were already there
	for (size_t i = 0 ; i < count ; ++i) {
		GraphElement& e = *elements[i];
		String& v = e.values[axis];
		size_t comma = v.find_first_of(_(','));
		while (comma != String::npos) {
			// split
			GraphElementP e2(new GraphElement(e));
			e2->values[axis] = v.substr(0,comma);
			elements.push_back(e2);
			if (is_substr(v, comma, _(", "))) ++comma; // skip space after it
			v = v.substr(comma + 1);
			comma = v.find_first_of(_(','));
		}
	}
}


GraphData::GraphData(const GraphDataPre& d)
	: axes(d.axes)
{
	// total size
	size = (UInt)d.elements.size();
	// find groups on each axis
	size_t i = 0;
	FOR_EACH(a, axes) {
		map<String,UInt> counts; // note: default constructor for UInt() does initialize to 0
		FOR_EACH_CONST(e, d.elements) {
			counts[e->values[i]] += 1;
		}
		if (a->numeric) {
			// TODO: start at something other than 0?
			// TODO: support fractions?
			size_t left = counts.size();
			int i = 0;
			while (!counts.empty() && i < 100) {
				String is = String() << i++;
				map<String,UInt>::const_iterator it = counts.find(is);
				if (it == counts.end()) {
					// not found, add a 0 bar
					a->groups.push_back(GraphGroup(is, 0));
				} else {
					a->groups.push_back(GraphGroup(is, it->second));
					a->max = max(a->max, it->second);
					a->total += it->second;
					counts.erase(is);
					left--;
				}
			}
			// drop empty tail
			while (a->groups.size() > 1 && a->groups.back().size == 0) {
				a->groups.pop_back();
			}
			// Also keep non-numeric entries
			FOR_EACH(c, counts) {
				a->groups.push_back(GraphGroup(c.first, c.second));
				a->max = max(a->max, c.second);
				a->total += c.second;
			}
		} else if (a->order) {
			// specific group order
			FOR_EACH_CONST(gn, *a->order) {
				UInt count = counts[gn];
				a->groups.push_back(GraphGroup(gn, count));
				a->max = max(a->max, count);
				a->total += count;
			}
		} else {
			FOR_EACH(c, counts) {
				a->groups.push_back(GraphGroup(c.first, c.second));
				a->max = max(a->max, c.second);
				a->total += c.second;
			}
		}
		// colors
		if (a->auto_color == AUTO_COLOR_NO && a->colors) {
			// use colors from the table
			FOR_EACH(g, a->groups) {
				map<String,Color>::const_iterator it = a->colors->find(g.name);
				if (it != a->colors->end()) {
					g.color = it->second;
				}
			}
		} else {
			// find some nice colors for the groups
			double hue = 0.6; // start hue
			bool first = true;
			FOR_EACH(g, a->groups) {
				double amount = a->auto_color == AUTO_COLOR_EVEN
				                  ? 1. / a->groups.size()
				                  : double(g.size) / a->total; // amount this group takes
				if (!first) hue += amount/2;
				g.color = hsl2rgb(hue, 1.0, 0.5);
				hue += amount / 2;
				first = false;
			}
		}
		++i;
	}
	// count elements in each position
	values.clear();
	FOR_EACH_CONST(e, d.elements) {
		// find index j in elements
		vector<int> group_nrs(axes.size(), -1);
		int i = 0;
		FOR_EACH(a, axes) {
			String v = e->values[i];
			int j = 0;
			FOR_EACH(g, a->groups) {
				if (v == g.name) {
					group_nrs[i] = j;
					break;
				}
				++j;
			}
			++i;
		}
		values.push_back(group_nrs);
	}
}

void GraphData::crossAxis(size_t axis1, size_t axis2, vector<UInt>& out) const {
	size_t a1_size = axes[axis1]->groups.size();
	size_t a2_size = axes[axis2]->groups.size();
	out.clear();
	out.resize(a1_size * a2_size, 0);
	FOR_EACH_CONST(v, values) {
		int v1 = v[axis1], v2 = v[axis2];
		if (v1 >= 0 && v2 >= 0) {
			out[a2_size * v1 + v2]++;
		}
	}
}

void GraphData::crossAxis(size_t axis1, size_t axis2, size_t axis3, vector<UInt>& out) const {
	size_t a1_size = axes[axis1]->groups.size();
	size_t a2_size = axes[axis2]->groups.size();
	size_t a3_size = axes[axis3]->groups.size();
	out.clear();
	out.resize(a1_size * a2_size * a3_size, 0);
	FOR_EACH_CONST(v, values) {
		int v1 = v[axis1], v2 = v[axis2], v3 = v[axis3];
		if (v1 >= 0 && v2 >= 0 && v3 >= 0) {
			out[a3_size * (a2_size * v1 + v2) + v3]++;
		}
	}
}


// ----------------------------------------------------------------------------- : Graph1D

void Graph1D::draw(RotatedDC& dc, const vector<int>& current, DrawLayer layer) const {
	draw(dc, axis < current.size() ? current.at(axis) : -1, layer);
}
bool Graph1D::findItem(const RealPoint& pos, const RealRect& rect, vector<int>& out) const {
	int i = findItem(pos, rect);
	if (i == -1) return false;
	else {
		out.clear();
		out.insert(out.begin(), data->axes.size(), -1);
		out.at(axis) = i;
		return true;
	}
}

// ----------------------------------------------------------------------------- : Graph2D

void Graph2D::setData(const GraphDataP& d) {
	Graph::setData(d);
	if (data->axes.size() <= max(axis1,axis2)) return;
	d->crossAxis(axis1,axis2,values);
}

// ----------------------------------------------------------------------------- : Bar Graph

/// Rectangle for the bar of a bar graph
RealRect bar_graph_bar(const RealRect& rect, int group, int group_count, int start, int end, int max) {
	double width_space = rect.width / group_count; // including spacing
	double width       = width_space / 5 * 4;
	double space       = width_space / 5;
	double step_height = rect.height / max; // multiplier for bar height
	int top    = (int)(rect.bottom() - start * step_height);
	int bottom = (int)(rect.bottom() - end   * step_height);
	if (bottom < top) swap(top,bottom);
	bottom += 1;
	return RealRect(
		rect.x + width_space * group + space / 2,
		top,
		width,
		bottom - top
	);
}
/// Which column of the bar graph with count bars is coordinate x in?
int find_bar_graph_column(double width, double x, int count) {
	double width_space = width / count; // including spacing
	double space       = width_space / 5;
	// Find column in which the point could be located
	int    col    = (int)floor(x / width_space);
	if (col < 0 || col >= count) return -1; // not a column
	double in_col = x - col * width_space;
	if (in_col < space / 2)               return -1; // left
	if (in_col > width_space - space / 2) return -1; // right
	return col;
}

void BarGraph::draw(RotatedDC& dc, int current, DrawLayer layer) const {
	if (!data) return;
	// Rectangle for bars
	RealRect rect = dc.getInternalRect();
	GraphAxis& axis = axis_data();
	int count = int(axis.groups.size());
	// Bar sizes
	if (layer == LAYER_SELECTION) {
		// Highlight current column
		Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		if (current >= 0) {
			const GraphGroup& group = axis.groups[current];
			RealRect bar = bar_graph_bar(rect, current, count, 0, group.size, axis.max);
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.SetBrush(lerp(bg, group.color, 0.25));
			dc.DrawRectangle(bar.move(-5,-5,10,5));
			dc.SetBrush(lerp(bg, group.color, 0.5));
			dc.DrawRectangle(bar.move(-2,-2,4,2));
		}
	} else if (layer == LAYER_VALUES) {
		// Draw bars
		dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		int i = 0;
		FOR_EACH_CONST(g, axis.groups) {
			// draw bar
			dc.SetBrush(g.color);
			dc.DrawRectangle(bar_graph_bar(rect, i++, count, 0, g.size, axis.max));
		}
	}
}
int BarGraph::findItem(const RealPoint& pos, const RealRect& rect) const {
	if (!data) return -1;
	if (pos.y > max(rect.top(), rect.bottom())) return -1; // below
	if (pos.y < min(rect.top(), rect.bottom())) return -1; // above
	return find_bar_graph_column(rect.width, pos.x - rect.x, (int)axis_data().groups.size());
}

// ----------------------------------------------------------------------------- : Bar Graph 2D

void BarGraph2D::draw(RotatedDC& dc, const vector<int>& current, DrawLayer layer) const {
	if (!data || data->axes.size() <= max(axis1,axis2)) return;
	// Rectangle for bars
	RealRect rect = dc.getInternalRect();
	GraphAxis& axis1 = axis1_data(); // the major axis
	GraphAxis& axis2 = axis2_data(); // the stacked axis
	int count = int(axis1.groups.size());
	// Draw
	if (layer == LAYER_SELECTION) {
		// Highlight current column
		Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		int cur1 = this->axis1 < current.size() ? current[this->axis1] : -1;
		int cur2 = this->axis2 < current.size() ? current[this->axis2] : -1;
		if (cur1 >= 0) {
			// draw selected bar
			int start = 0;
			int j = 0;
			FOR_EACH_CONST(g2, axis2.groups) {
				int end = start + values[j + axis2.groups.size() * cur1];
				if (j == cur2 || cur2 < 0) {
					RealRect bar = bar_graph_bar(rect, cur1, count, start, end, axis1.max);
					dc.SetBrush(lerp(bg, g2.color, 0.25));
					dc.DrawRectangle(bar.move(-5,0,10,0));
					dc.SetBrush(lerp(bg, g2.color, 0.5));
					dc.DrawRectangle(bar.move(-2,0,4,0));
				}
				start = end;
				++j;
			}
		} else if (cur2 >= 0) {
			// entire row
			// TODO
		}
	} else if (layer == LAYER_VALUES) {
		// Draw bars
		dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		for (int i = 0 ; i < count ; ++i) {
			// draw stacked bars
			int start = 0;
			int j = 0;
			FOR_EACH_CONST(g2, axis2.groups) {
				int end = start + values[j++ + axis2.groups.size() * i];
				dc.SetBrush(g2.color);
				dc.DrawRectangle(bar_graph_bar(rect, i, count, start, end, axis1.max));
				start = end;
			}
		}
	}
}
bool BarGraph2D::findItem(const RealPoint& pos, const RealRect& rect, vector<int>& out) const {
	if (!data || data->axes.size() <= max(axis1,axis2)) return false;
	if (pos.y > max(rect.top(), rect.bottom())) return false; // below
	if (pos.y < min(rect.top(), rect.bottom())) return false; // above
	// column
	GraphAxis& axis1 = axis1_data(); // the major axis
	int count = (int)axis1.groups.size();
	int col   = find_bar_graph_column(rect.width, pos.x - rect.x, count);
	if (col < 0) return false;
	// row
	int max_value = (int)axis1.max;
	int value = (int)((rect.bottom() - pos.y) / rect.height * max_value);
	if (value < 0 || value > max_value) return false;
	// find row
	int row = -1;
	size_t vs = col * axis2_data().groups.size();
	for (int i = 0 ; i < count ; ++i) {
		value -= values[vs+i];
		if (value < 0) {
			// in this layer of the stack
			row = i;
			break;
		}
	}
	if (row == -1) return false;
	// done
	out.clear();
	out.insert(out.begin(), data->axes.size(), -1);
	out.at(this->axis1) = col;
	out.at(this->axis2) = row;
	return true;
}

// ----------------------------------------------------------------------------- : Pie Graph

void PieGraph::draw(RotatedDC& dc, int current, DrawLayer layer) const {
	if (!data) return;
	// Rectangle for the pie
	GraphAxis& axis = axis_data();
	RealRect rect = dc.getInternalRect();
	double size = min(rect.width, rect.height);
	RealSize pie_size(size, size);
	RealSize pie_size_large(size+20, size+20);
	RealPoint pie_pos = rect.position() + rect.size() / 2;
	//RealPoint pos = align_in_rect(ALIGN_MIDDLE_CENTER, RealSize(size,size), rect);
	// draw items
	if (layer == LAYER_VALUES) {
		Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		dc.SetPen(fg);
		// draw pies
		double angle = 0;
		int i = 0;
		FOR_EACH_CONST(g, axis.groups) {
			// draw pie
			dc.SetBrush(g.color);
			if (g.size > 0) {
				double end_angle = angle + 2 * M_PI * (double)g.size / axis.total;
				dc.DrawEllipticArc(pie_pos, i == current ? pie_size_large : pie_size, angle, end_angle);
				angle = end_angle;
			}
			++i;
		}
		// draw spokes
		if (axis.groups.size() > 1) {
			angle = 0;
			FOR_EACH_CONST(g, axis.groups) {
				if (g.size > 0) {
					dc.DrawEllipticSpoke(pie_pos, pie_size, angle);
					angle += 2 * M_PI * (double)g.size / axis.total;
				}
			}
		}
	}
}
int PieGraph::findItem(const RealPoint& pos, const RealRect& rect) const {
	if (!data) return -1;
	// Rectangle for the pie
	GraphAxis& axis = axis_data();
	double size = min(rect.width, rect.height);
	RealPoint pie_pos = rect.position() + rect.size() / 2;
	// position in circle
	Vector2D delta = pos - pie_pos;
	if (delta.lengthSqr() > size*size) {
		return -1; // outside circle
	}
	double pos_angle = atan2(-delta.y, delta.x); // in range [-pi..pi]
	if (pos_angle < 0) pos_angle += 2 * M_PI;
	// find angle
	double angle = 0;
	int i = 0;
	FOR_EACH_CONST(g, axis.groups) {
		angle += 2 * M_PI * (double)g.size / axis.total;
		if (angle > pos_angle) return i;
		++i;
	}
	return -1; //should not happen
}

// ----------------------------------------------------------------------------- : Scatter Plot

void ScatterGraph::draw(RotatedDC& dc, const vector<int>& current, DrawLayer layer) const {
	if (!data || data->axes.size() <= max(axis1,axis2)) return;
	// Rectangle for drawing
	RealRect rect = dc.getInternalRect();
	GraphAxis& axis1 = axis1_data(); // the major axis
	GraphAxis& axis2 = axis2_data(); // the stacked axis
	RealSize size(rect.width / axis1.groups.size(), rect.height / axis2.groups.size()); // size for a single cell
	double step = min(size.width, size.height) / sqrt((double)max_value) / 2.01;
	// Draw
	if (layer == LAYER_SELECTION) {
		dc.SetPen(*wxTRANSPARENT_PEN);
		Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		int cur1 = this->axis1 < current.size() ? current[this->axis1] : -1;
		int cur2 = this->axis2 < current.size() ? current[this->axis2] : -1;
		if (cur1 >= 0 && cur2 >= 0) {
			UInt value = values[cur1 * axis2.groups.size() + cur2];
			if (value) {
				dc.SetBrush(lerp(bg,lerp(axis1.groups[cur1].color, axis2.groups[cur2].color, 0.5),0.5));
				dc.DrawCircle(RealPoint(rect.left() + cur1 * size.width, rect.bottom() - (cur2+1) * size.height) + size/2, sqrt((double)value) * step + 5);
			}
		} else if (cur1 >= 0) {
			dc.SetBrush(lerp(bg,axis1.groups[cur1].color,0.3));
			dc.DrawRectangle(RealRect(rect.x + cur1 * size.width, rect.y, size.width, rect.height));
		} else if (cur2 >= 0) {
			dc.SetBrush(lerp(bg,axis2.groups[cur2].color,0.3));
			dc.DrawRectangle(RealRect(rect.x, rect.bottom() - (cur2+1) * size.height, rect.width, size.height));
		}
	} else if (layer == LAYER_VALUES) {
		Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		dc.SetPen(fg);
		size_t i = 0;
		double x = rect.left();
		FOR_EACH_CONST(g1, axis1.groups) {
			double y = rect.bottom() - size.height;
			FOR_EACH_CONST(g2, axis2.groups) {
				UInt value = values[i++];
				dc.SetBrush(lerp(g1.color, g2.color, 0.5));
				dc.DrawCircle(RealPoint(x,y) + size/2, sqrt((double)value) * step);
				y -= size.height;
			}
			x += size.width;
		}
	}
}
bool ScatterGraph::findItem(const RealPoint& pos, const RealRect& rect, vector<int>& out) const {
	if (!data || data->axes.size() <= max(axis1,axis2)) return false;
	// clicked item
	GraphAxis& axis1 = axis1_data();
	GraphAxis& axis2 = axis2_data();
	int col = (int) floor((pos.x - rect.x)        / rect.width  * axis1.groups.size());
	int row = (int) floor((rect.bottom() - pos.y) / rect.height * axis2.groups.size());
	if (col < 0 || col >= (int)axis1.groups.size()) return false;
	if (row < 0 || row >= (int)axis2.groups.size()) return false;
	// done
	out.clear();
	out.insert(out.begin(), data->axes.size(), -1);
	out.at(this->axis1) = col;
	out.at(this->axis2) = row;
	return true;
}
void ScatterGraph::setData(const GraphDataP& d) {
	Graph2D::setData(d);
	// find maximum
	max_value = 0;
	FOR_EACH(v, values) {
		max_value = max(max_value, v);
	}
}

// ----------------------------------------------------------------------------- : Scatter Plot plus

void ScatterGraphPlus::setData(const GraphDataP& d) {
	ScatterGraph::setData(d);
	if (data->axes.size() <= max(max(axis1,axis2),axis3)) return;
	d->crossAxis(axis1,axis2,axis3,values3D);
}

// ----------------------------------------------------------------------------- : Scatter Pie graph

void ScatterPieGraph::draw(RotatedDC& dc, const vector<int>& current, DrawLayer layer) const {
	if (data->axes.size() <= max(max(axis1,axis2),axis3)) return;
	if (layer == LAYER_SELECTION) {
		ScatterGraph::draw(dc, current, layer);
	} else if (layer == LAYER_VALUES) {
		// Rectangle for drawing
		RealRect rect = dc.getInternalRect();
		GraphAxis& axis1 = axis1_data(); // the major axis
		GraphAxis& axis2 = axis2_data(); // the stacked axis
		GraphAxis& axis3 = axis3_data(); // the pie axis
		RealSize size(rect.width / axis1.groups.size(), rect.height / axis2.groups.size()); // size for a single cell
		double step = min(size.width, size.height) / sqrt((double)max_value) / 2.01;
		// Draw pies
		Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		dc.SetPen(fg);
		for (size_t x = 0 ; x < axis1.groups.size() ; ++x) {
			for (size_t y = 0 ; y < axis2.groups.size() ; ++y) {
				size_t i = x * axis2.groups.size() + y;
				UInt value = values[i];
				double radius = floor(sqrt((double)value) * step * 2);
				RealSize radius_s(radius,radius);
				RealPoint center(rect.left() + (x+0.5) * size.width + 0.5, rect.bottom() - (y+0.5) * size.height + 0.5);
				// draw pie slices
				double angle = 0;
				size_t j = 0;
				FOR_EACH(g, axis3.groups) {
					dc.SetBrush(g.color);
					UInt val = values3D[i * axis3.groups.size() + j++];
					if (val > 0) {
						double end_angle = angle + 2 * M_PI * (double)val / value;
						dc.DrawEllipticArc(center, radius_s, angle, end_angle);
						angle = end_angle;
					}
				}
				// draw spokes?
			}
		}
	}
}



// ----------------------------------------------------------------------------- : Graph Legend

RealSize GraphLegend::determineSize(RotatedDC& dc) const {
	if (!data) return RealSize(-1,-1);
	GraphAxis& axis = axis_data();
	dc.SetFont(*wxNORMAL_FONT);
	item_size = RealSize(0,0);
	FOR_EACH(g, axis.groups) {
		RealSize this_item_size = dc.GetTextExtent(g.name);
		this_item_size = RealSize(this_item_size.width + 34, this_item_size.height + 5);
		item_size = piecewise_max(item_size, this_item_size);
	}
	size = RealSize(item_size.width + 2, axis.groups.size() * item_size.height + 3); // margins
	return size;
}

void GraphLegend::draw(RotatedDC& dc, int current, DrawLayer layer) const {
	if (!size.width) determineSize(dc);
	if (layer == LAYER_VALUES) {
		RealRect rect = dc.getInternalRect();
		RealPoint pos = align_in_rect(alignment, size, rect);
		GraphAxis& axis = axis_data();
		Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
		Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
		// draw border
		dc.SetBrush(bg);
		dc.DrawRectangle(RealRect(pos,size));
		// draw items
		dc.SetFont(*wxNORMAL_FONT);
		dc.SetPen(fg);
		double y = pos.y + 1;
		for (int j = 0 ; j < (int)axis.groups.size() ; ++j) {
			int i = reverse ? (int)axis.groups.size() - j - 1 : j;
			const GraphGroup& g = axis.groups[i];
			if (i == current) {
				dc.SetBrush(lerp(bg,g.color,0.5));
				dc.SetPen(*wxTRANSPARENT_PEN);
				dc.DrawRectangle(RealRect(pos.x+1, y, item_size.width, item_size.height + 1));
				dc.SetPen(fg);
			}
			dc.SetBrush(g.color);
			dc.DrawRectangle(RealRect(pos.x+3, y + 2, 26, item_size.height - 3));
			dc.DrawText(g.name, RealPoint(pos.x + 32, y + 2));
			y += item_size.height;
		}
	}
}
int GraphLegend::findItem(const RealPoint& pos, const RealRect& rect) const {
	RealPoint mypos = align_in_rect(alignment, size, rect);
	RealPoint pos2(pos.x - mypos.x, pos.y - mypos.y);
	if (pos2.x < 0 || pos2.y < 0 || pos2.x >= size.width || pos2.y >= size.height) return -1;
	int col = (int) floor((pos2.y-1) / item_size.height);
	if (col < 0 || col >= (int)axis_data().groups.size()) return -1;
	return reverse ? (int)axis_data().groups.size() - col - 1 : col;
}

// ----------------------------------------------------------------------------- : Graph label axis

void GraphLabelAxis::draw(RotatedDC& dc, int current, DrawLayer layer) const {
	RealRect rect = dc.getInternalRect();
	GraphAxis& axis = axis_data();
	int count = int(axis.groups.size());
	// Draw
	dc.SetFont(*wxNORMAL_FONT);
	Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	if (layer == LAYER_SELECTION && current >= 0) {
		// highlight selection
		GraphGroup& group = axis.groups[current];
		if (direction == HORIZONTAL) {
			double width = rect.width / count; // width of an item
			dc.SetBrush(lerp(bg,group.color,0.5));
			dc.SetPen(*wxTRANSPARENT_PEN);
			RealSize text_size = dc.GetTextExtent(group.name);
			dc.DrawRectangle(RealRect(rect.x + current * width, rect.bottom(), width, text_size.height + 5));
		} else {
			double height = rect.height / count;
			dc.SetBrush(lerp(bg,group.color,0.5));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(RealRect(rect.x, rect.bottom() - (current+1)*height, -78, height));
		}
	} else if (layer == LAYER_AXES) {
		if (direction == HORIZONTAL) {
			double width = rect.width / count; // width of an item
			// Draw labels
			double x = rect.x;
			FOR_EACH_CONST(g, axis.groups) {
				// draw label, aligned bottom center
				RealSize text_size = dc.GetTextExtent(g.name);
				dc.SetClippingRegion(RealRect(x + 2, rect.bottom() + 3, width - 4, text_size.height));
				dc.DrawText(g.name, align_in_rect(ALIGN_TOP_CENTER, text_size, RealRect(x, rect.bottom() + 3, width, 0)));
				dc.DestroyClippingRegion();
				x += width;
			}
			// Draw lines
			if (draw_lines) {
				for (int i = 0 ; i < count ; ++i) {
					dc.SetPen(i == current ? fg : lerp(bg, fg, 0.3));
					if (draw_lines == DRAW_LINES_BETWEEN) {
						dc.DrawLine(RealPoint(rect.x + (i+1.0)*width, rect.top()), RealPoint(rect.x + (i+1.0)*width, rect.bottom()));
					} else {
						dc.DrawLine(RealPoint(rect.x + (i+0.5)*width, rect.top()), RealPoint(rect.x + (i+0.5)*width, rect.bottom() + 2));
					}
				}
			}
			// always draw axis line
			dc.SetPen(fg);
			dc.DrawLine(rect.topLeft(), rect.bottomLeft());
		} else {
			double height = rect.height / count;
			// Draw labels
			double y = rect.bottom();
			FOR_EACH_CONST(g, axis.groups) {
				// draw label, aligned middle right
				RealSize text_size = dc.GetTextExtent(g.name);
				//dc.SetClippingRegion(RealRect(x + 2, rect.bottom() + 3, width - 4, text_size.height));
				dc.DrawText(g.name, align_in_rect(ALIGN_MIDDLE_RIGHT, text_size, RealRect(-4, y, 0, -height)));
				//dc.DestroyClippingRegion();
				y -= height;
			}
			// Draw lines
			if (draw_lines) {
				for (int i = 0 ; i < count ; ++i) {
					dc.SetPen(i == current ? fg : lerp(bg, fg, 0.3));
					if (draw_lines == DRAW_LINES_BETWEEN) {
						dc.DrawLine(RealPoint(rect.left(),     rect.bottom() - (i+1.0)*height), RealPoint(rect.right(), rect.bottom() - (i+1.0)*height));
					} else {
						dc.DrawLine(RealPoint(rect.left() - 2, rect.bottom() - (i+0.5)*height), RealPoint(rect.right(), rect.bottom() - (i+0.5)*height));
					}
				}
			}
			// always draw axis line
			dc.SetPen(fg);
			dc.DrawLine(rect.bottomLeft(), rect.bottomRight());
		}
	}
}
int GraphLabelAxis::findItem(const RealPoint& pos, const RealRect& rect) const {
	GraphAxis& axis = axis_data();
	int col;
	if (direction == HORIZONTAL) {
		col = (int) floor((pos.x - rect.x) / rect.width  * axis.groups.size());
		if (pos.y < rect.bottom()) return -1;
	} else {
		col = (int) floor((rect.bottom() - pos.y) / rect.height * axis.groups.size());
		if (pos.x > rect.left()) return -1;
	}
	if (col < 0 || col >= (int)axis.groups.size()) return -1;
	return col;
}

// ----------------------------------------------------------------------------- : Graph value axis

void GraphValueAxis::draw(RotatedDC& dc, int current, DrawLayer layer) const {
	if (layer != LAYER_AXES) return;
	if (!data) return;
	// How many labels and lines to draw?
	RealRect rect = dc.getInternalRect();
	GraphAxis& axis = axis_data();
	double step_height = rect.height / axis.max; // height of a single value
	dc.SetFont(*wxNORMAL_FONT);
	int label_step = (int) ceil(max(1.0, (dc.GetCharHeight()) / step_height)); // values per labeled line
	// Colors
	Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	// Draw backlines (horizontal) and value labels
	dc.SetPen(lerp(bg, fg, 0.3));
	int highlight = (highlight_value && current >= 0) ? (int)axis.groups[current].size : -1;
	for (int i = 0 ; i <= (int)axis.max ; ++i) {
		if (i % label_step == 0 || i == highlight) {
			// highlight?
			if (i == highlight) {
				wxFont font(*wxNORMAL_FONT);
				font.SetWeight(wxBOLD);
				dc.SetFont(font);
				dc.SetPen(fg);
			}
			// draw line
			int y = (int) (rect.bottom() - i * step_height);
			dc.DrawLine(RealPoint(rect.left() - 2, y), RealPoint(rect.right(), y));
			// draw label, aligned middle right
			if (! ((i < highlight && i + label_step > highlight) ||
			       (i > highlight && i - label_step < highlight)) || highlight == -1) {
				// don't draw labels before/after current to make room
				String label; label << i;
				RealSize text_size = dc.GetTextExtent(label);
				dc.DrawText(label, align_in_rect(ALIGN_MIDDLE_RIGHT, text_size, RealRect(rect.x - 4, y, 0, 0)));
			}
			// restore font/pen
			if (i == highlight) {
				dc.SetFont(*wxNORMAL_FONT);
				dc.SetPen(lerp(bg, fg, 0.5));
			}
		}
	}
	// Draw axis
	dc.SetPen(fg);
	dc.DrawLine(rect.bottomLeft() - RealSize(2,0), rect.bottomRight());
}

// ----------------------------------------------------------------------------- : Graph with margins

void GraphWithMargins::draw(RotatedDC& dc, const vector<int>& current, DrawLayer layer) const {
	RealRect inner = dc.getInternalRect().move(margin_left, margin_top, - margin_left - margin_right, - margin_top - margin_bottom);
	if (upside_down) { inner.y += inner.height; inner.height = -inner.height; }
	Rotation new_size(0, inner);
	Rotater rot(dc, new_size);
	graph->draw(dc, current, layer);
}
bool GraphWithMargins::findItem(const RealPoint& pos, const RealRect& rect, vector<int>& out) const {
	RealRect inner = rect.move(margin_left, margin_top, - margin_left - margin_right, - margin_top - margin_bottom);
	if (upside_down) { inner.y += inner.height; inner.height = -inner.height; }
	return graph->findItem(pos, inner, out);
}
void GraphWithMargins::setData(const GraphDataP& d) {
	Graph::setData(d);
	graph->setData(d);
}

// ----------------------------------------------------------------------------- : Graph Container

void GraphContainer::draw(RotatedDC& dc, const vector<int>& current, DrawLayer layer) const {
	FOR_EACH_CONST(g, items) {
		g->draw(dc, current, layer);
	}
}
bool GraphContainer::findItem(const RealPoint& pos, const RealRect& rect, vector<int>& out) const {
	FOR_EACH_CONST_REVERSE(g, items) {
		if (g->findItem(pos, rect, out)) return true;
	}
	return false;
}
void GraphContainer::setData(const GraphDataP& d) {
	Graph::setData(d);
	FOR_EACH(g, items) {
		g->setData(d);
	}
}
void GraphContainer::add(const GraphP& graph) {
	items.push_back(graph);
}


// ----------------------------------------------------------------------------- : GraphControl

GraphControl::GraphControl(Window* parent, int id)
	: wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
{}

void GraphControl::setLayout(GraphType type) {
	if (graph && type == layout) return;
	GraphDataP data = graph ? graph->getData() : GraphDataP();
	switch (type) {
		case GRAPH_TYPE_BAR: {
			intrusive_ptr<GraphContainer> combined(new GraphContainer());
			combined->add(new_intrusive2<GraphValueAxis>(0, true));
			combined->add(new_intrusive2<GraphLabelAxis>(0, HORIZONTAL));
			combined->add(new_intrusive1<BarGraph>(0));
			graph = new_intrusive5<GraphWithMargins>(combined, 23,8,7,20);
			break;
		} case GRAPH_TYPE_PIE: {
			intrusive_ptr<GraphContainer> combined(new GraphContainer());
			combined->add(new_intrusive1<PieGraph>(0));
			graph = new_intrusive5<GraphWithMargins>(combined, 20,20,20,20);
			break;
		} case GRAPH_TYPE_STACK: {
			intrusive_ptr<GraphContainer> combined(new GraphContainer());
			combined->add(new_intrusive2<GraphValueAxis>(0, false));
			combined->add(new_intrusive2<GraphLabelAxis>(0, HORIZONTAL));
			combined->add(new_intrusive2<BarGraph2D>(0,1));
			combined->add(new_intrusive3<GraphLegend>(1, ALIGN_TOP_RIGHT, true));
			graph = new_intrusive5<GraphWithMargins>(combined, 23,8,7,20);
			break;
		} case GRAPH_TYPE_SCATTER: {
			intrusive_ptr<GraphContainer> combined(new GraphContainer());
			combined->add(new_intrusive4<GraphLabelAxis>(0, HORIZONTAL, false, DRAW_LINES_MID));
			combined->add(new_intrusive4<GraphLabelAxis>(1, VERTICAL,   false, DRAW_LINES_MID));
			combined->add(new_intrusive2<ScatterGraph>(0,1));
			graph = new_intrusive5<GraphWithMargins>(combined, 80,8,7,20);
			break;
		} case GRAPH_TYPE_SCATTER_PIE: {
			intrusive_ptr<GraphContainer> combined(new GraphContainer());
			combined->add(new_intrusive4<GraphLabelAxis>(0, HORIZONTAL, false, DRAW_LINES_MID));
			combined->add(new_intrusive4<GraphLabelAxis>(1, VERTICAL,   false, DRAW_LINES_MID));
			combined->add(new_intrusive3<ScatterPieGraph>(0,1,2));
			graph = new_intrusive5<GraphWithMargins>(combined, 80,8,7,20);
			break;
		} default:
			graph = GraphP();
	}
	if (data && graph) graph->setData(data);
	layout = type;
}

void GraphControl::setData(const GraphDataPre& data) {
	setData(new_intrusive1<GraphData>(data));
}
void GraphControl::setData(const GraphDataP& data) {
	if (graph) {
		graph->setData(data);
		current_item.clear(); // TODO : preserve selection
	}
	Refresh(false);
}

void GraphControl::onPaint(wxPaintEvent&) {
	wxBufferedPaintDC dc(this);
	wxSize cs = GetClientSize();
	RotatedDC rdc(dc, 0, RealRect(RealPoint(0,0),cs), 1, QUALITY_LOW);
	rdc.SetPen(*wxTRANSPARENT_PEN);
	rdc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	rdc.DrawRectangle(rdc.getInternalRect());
	if (graph) {
		for (int layer = LAYER_BOTTOM ; layer < LAYER_COUNT ; ++layer) {
			graph->draw(rdc, current_item, (DrawLayer)layer);
		}
	}
}

void GraphControl::onSize(wxSizeEvent&) {
	Refresh(false);
}

void GraphControl::onMouseDown(wxMouseEvent& ev) {
	if (!graph) return;
	wxSize cs = GetClientSize();
	if (graph->findItem(RealPoint(ev.GetX(), ev.GetY()), RealRect(RealPoint(0,0),cs), current_item)) {
		onSelectionChange();
	}
	ev.Skip(); // focus
}
void GraphControl::onChar(wxKeyEvent& ev) {
	if (!graph) return;
	GraphDataP data = graph->getData();
	if (!data) return;
	switch (ev.GetKeyCode()) {
		case WXK_LEFT:
			if (current_item.size() >= 1 && data->axes.size() >= 1 && current_item[0] != -1) {
				current_item[0]--;
				onSelectionChange();
			}
			break;
		case WXK_RIGHT:
			if (current_item.size() >= 1 && data->axes.size() >= 1 && current_item[0] + 1 < (int)data->axes[0]->groups.size()) {
				current_item[0]++;
				onSelectionChange();
			}
			break;
		case WXK_UP:
			if (current_item.size() >= 2 && data->axes.size() >= 2 && current_item[1] + 1 < (int)data->axes[1]->groups.size()) {
				current_item[1]++;
				onSelectionChange();
			}
			break;
		case WXK_DOWN:
			if (current_item.size() >= 2 && data->axes.size() >= 2 && current_item[1] != -1) {
				current_item[1]--;
				onSelectionChange();
			}
			break;
		case WXK_TAB: {
			// send a navigation event to our parent, to select another control
			// we need this because of wxWANTS_CHARS
			wxNavigationKeyEvent nev;
			nev.SetDirection(!ev.ShiftDown());
			GetParent()->ProcessEvent(nev);
			} break;
	}
}

void GraphControl::onSelectionChange() {
	wxCommandEvent ev(EVENT_GRAPH_SELECT, GetId());
	ProcessEvent(ev);
	Refresh(false);
}

bool GraphControl::hasSelection(size_t axis) const {
	return axis < current_item.size() && current_item[axis] >= 0;
}
String GraphControl::getSelection(size_t axis) const {
	if (!graph || axis >= current_item.size() || axis >= graph->getData()->axes.size()) return wxEmptyString;
	GraphAxis& a = *graph->getData()->axes[axis];
	int i = current_item[axis];
	if (i == -1 || (size_t)i >= a.groups.size()) return wxEmptyString;
	return a.groups[current_item[axis]].name;
}

BEGIN_EVENT_TABLE(GraphControl, wxControl)
	EVT_PAINT		(GraphControl::onPaint)
	EVT_SIZE		(GraphControl::onSize)
	EVT_LEFT_DOWN	(GraphControl::onMouseDown)
	EVT_CHAR		(GraphControl::onChar)
END_EVENT_TABLE  ()

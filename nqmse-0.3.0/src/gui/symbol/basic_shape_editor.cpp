//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <gui/symbol/basic_shape_editor.hpp>
#include <util/window_id.hpp>
#include <data/action/symbol.hpp>
#include <wx/spinctrl.h>

// ----------------------------------------------------------------------------- : SymbolBasicShapeEditor

SymbolBasicShapeEditor::SymbolBasicShapeEditor(SymbolControl* control)
	: SymbolEditorBase(control)
	, drawing(false)
	, mode(ID_SHAPE_CIRCLE)
{
	control->SetCursor(*wxCROSS_CURSOR);
}

// ----------------------------------------------------------------------------- : Drawing

void SymbolBasicShapeEditor::draw(DC& dc) {
	// highlight the part we are drawing
	if (drawing) {
		control.highlightPart(dc, *shape, HIGHLIGHT_BORDER);
	}
}

// ----------------------------------------------------------------------------- : UI

void SymbolBasicShapeEditor::initUI(wxToolBar* tb, wxMenuBar* mb) {
	sides  = new wxSpinCtrl(  tb, ID_SIDES, _("3"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 3, 50, 3);
	sidesL = new wxStaticText(tb, ID_SIDES, _(" sides: "));
	sides->SetSize(50, -1);
	tb->AddSeparator();
	tb->AddTool(ID_SHAPE_CIRCLE,		_("Ellipse"),		Bitmap(_("TOOL_CIRCLE")),		wxNullBitmap, wxITEM_CHECK, _("Circle / Ellipse"),			_("Draw circles and ellipses"));
	tb->AddTool(ID_SHAPE_RECTANGLE,		_("Rectangle"),		Bitmap(_("TOOL_RECTANGLE")),	wxNullBitmap, wxITEM_CHECK, _("Square / Rectangle"),		_("Draw squares and rectangles"));
	tb->AddTool(ID_SHAPE_POLYGON,		_("Polygon"),		Bitmap(_("TOOL_TRIANGLE")),		wxNullBitmap, wxITEM_CHECK, _("Polygon"),					_("Draw triangles, pentagons and other regular polygons"));
	tb->AddTool(ID_SHAPE_STAR,			_("Star"),			Bitmap(_("TOOL_STAR")),			wxNullBitmap, wxITEM_CHECK, _("Star"),						_("Draw stars"));
	tb->AddControl(sidesL);
	tb->AddControl(sides);
	tb->Realize();
	control.SetCursor(*wxCROSS_CURSOR);
}

void SymbolBasicShapeEditor::destroyUI(wxToolBar* tb, wxMenuBar* mb) {
	tb->DeleteTool(ID_SHAPE_CIRCLE);
	tb->DeleteTool(ID_SHAPE_RECTANGLE);
	tb->DeleteTool(ID_SHAPE_POLYGON);
	tb->DeleteTool(ID_SHAPE_STAR);
	tb->RemoveChild(sidesL);
	tb->RemoveChild(sides);
	// HACK: hardcoded size of rest of toolbar
	tb->DeleteToolByPos(4); // delete separator
	tb->DeleteToolByPos(4); // delete sidesL
	tb->DeleteToolByPos(4); // delete sides
	#if wxVERSION_NUMBER < 2600
		delete sides;
		delete sidesL;
	#endif
}

void SymbolBasicShapeEditor::onUpdateUI(wxUpdateUIEvent& ev) {
	if (ev.GetId() >= ID_SHAPE && ev.GetId() < ID_SHAPE_MAX) {
		ev.Check(ev.GetId() == mode);
	} else if (ev.GetId() == ID_SIDES) {
		ev.Enable(mode == ID_SHAPE_POLYGON || mode == ID_SHAPE_STAR);
	} else {
		ev.Enable(false); // we don't know about this item
	}
}

void SymbolBasicShapeEditor::onCommand(int id) {
	if (id >= ID_SHAPE && id < ID_SHAPE_MAX) {
		// change shape mode
		mode = id;
	}
}

int SymbolBasicShapeEditor::modeToolId() { return ID_MODE_SHAPES; }

// ----------------------------------------------------------------------------- : Mouse events

void SymbolBasicShapeEditor::onLeftDown   (const Vector2D& pos, wxMouseEvent& ev) {
	// Start drawing
	drawing = true;
	start = end = pos;
	SetStatusText(_("Drag to resize shape, Ctrl constrains shape, Shift centers shape"));
}

void SymbolBasicShapeEditor::onLeftUp     (const Vector2D& pos, wxMouseEvent& ev) {
	if (drawing && shape) {
		// Finalize the shape
		getSymbol()->actions.add(new AddSymbolPartAction(*getSymbol(), shape));
		// Select the part
		control.selectPart(shape);
		// no need to clean up, this editor is replaced
		// // Clean up
		// stopActions()
	}
}

void SymbolBasicShapeEditor::onMouseDrag  (const Vector2D& from, const Vector2D& to, wxMouseEvent& ev) {
	// Resize the object
	if (drawing) {
		end = to;
		makeShape(start, end, ev.ControlDown(), ev.ShiftDown());
		control.Refresh(false);
	}
}

// ----------------------------------------------------------------------------- : Other events

void SymbolBasicShapeEditor::onKeyChange(wxKeyEvent& ev) {
	if (drawing) {
		if (ev.GetKeyCode() == WXK_CONTROL || ev.GetKeyCode() == WXK_SHIFT) {
			// changed constrains
			makeShape(start, end, ev.ControlDown(), ev.ShiftDown());
			control.Refresh(false);
		} else if (ev.GetKeyCode() == WXK_ESCAPE) {
			// cancel drawing
			stopActions();
		}
	}
}

bool SymbolBasicShapeEditor::isEditing() { return drawing; }

// ----------------------------------------------------------------------------- : Generating shapes

void SymbolBasicShapeEditor::stopActions() {
	shape = SymbolPartP();
	drawing = false;
	switch (mode) {
		case ID_SHAPE_CIRCLE:
			SetStatusText(_("Click and drag to draw a ellipse, hold Ctrl for a circle"));
			break;
		case ID_SHAPE_RECTANGLE:
			SetStatusText(_("Click and drag to draw a rectangle, hold Ctrl for a square"));
			break;
		case ID_SHAPE_POLYGON:
			SetStatusText(_("Click and drag to draw a polygon"));
			break;
		case ID_SHAPE_STAR:
			SetStatusText(_("Click and drag to draw a star"));
			break;
	}
	control.Refresh(false);
}

inline double sgn(double d) {
	return d < 0 ? - 1 : 1;
}

void SymbolBasicShapeEditor::makeShape(const Vector2D& a, const Vector2D& b, bool constrained, bool centered) {
	// constrain
	Vector2D size = b - a;
	if (constrained) {
		if (abs(size.x) > abs(size.y)) {
			size.y = sgn(size.y) * abs(size.x);
		} else {
			size.x = sgn(size.x) * abs(size.y);
		}
	}
	// make shape
	if (centered) {
		makeCenteredShape(a, size, constrained);
	} else {
		makeCenteredShape(a + size / 2, size / 2, constrained);
	}
}

// TODO : Move out of this class
void SymbolBasicShapeEditor::makeCenteredShape(const Vector2D& c, Vector2D r, bool constrained) {
	shape = new_shared<SymbolPart>();
	// What shape to make?
	switch (mode) {
		case ID_SHAPE_CIRCLE: {
			// A circle / ellipse
			if (constrained) {
				shape->name = _("Circle");
			} else {
				shape->name = _("Ellipse");
			}
			// a circle has 4 control points, the first is: (x+r, y) db(0, kr) da(0, -kr)
			// kr is a magic constant
			const double kr = 0.5522847498f; // = 4/3 * (sqrt(2) - 1)
			shape->points.push_back(new_shared7<ControlPoint>(c.x + r.x, c.y,  0, kr  * r.y,  0, -kr * r.y,  LOCK_SIZE));
			shape->points.push_back(new_shared7<ControlPoint>(c.x, c.y - r.y,  kr  * r.x, 0,  -kr * r.x, 0,  LOCK_SIZE));
			shape->points.push_back(new_shared7<ControlPoint>(c.x - r.x, c.y,  0, -kr * r.y,  0, kr  * r.y,  LOCK_SIZE));
			shape->points.push_back(new_shared7<ControlPoint>(c.x, c.y + r.y,  -kr * r.x, 0,  kr  * r.x, 0,  LOCK_SIZE));
			break;
		} case ID_SHAPE_RECTANGLE: {
			// A rectangle / square
			if (constrained) {
				shape->name = _("Square");
			} else {
				shape->name = _("Rectangle");
			}
			// a rectangle just has four corners
			shape->points.push_back(new_shared2<ControlPoint>(c.x - r.x, c.y - r.y));
			shape->points.push_back(new_shared2<ControlPoint>(c.x + r.x, c.y - r.y));
			shape->points.push_back(new_shared2<ControlPoint>(c.x + r.x, c.y + r.y));
			shape->points.push_back(new_shared2<ControlPoint>(c.x - r.x, c.y + r.y));
			break;
		} default: {
			// A polygon or star
			int n = sides->GetValue();  // number of sides
			switch (n) {
				case 3:  shape->name = _("Triangle");
				case 4:  shape->name = _("Rhombus");
				case 5:  shape->name = _("Pentagon");
				case 6:  shape->name = _("Hexagon");
				default: shape->name = _("Polygon");
			}
			// Example: n == 7
			//         a           a..g = corners
			//      g     b        O    = center
			//     f   O   c       ra   = radius, |Oa|
			//       e   d
			double alpha = 2 * M_PI / n; // internal angle /_aOb
			// angle between point touching side and point on top
			// floor((n+1)/4) == number of sides between these two points
			// beta = /_aOc
			double beta = alpha * ((n+1)/4);
			// define:
			//  width = 2 = |fc|
			//  lb = |ac|
			//  gamma = (pi - beta) / 2
			// equations:
			//  lb * sin(gamma) == 1               (right angled tri /_\ aXc where X is halfway fc)
			//  lb / sin(beta) == ra / sin(gamma)  (law of sines in /_\ abc)
			// solving leads to:
			//  sin(gamma) == cos(beta/2)
			double lb = 1 / cos(beta/2);
			double ra = lb / sin(beta) * cos(beta/2);
			// now we know the center of the polygon:
			double y = c.y + (ra - 1) * r.y;
			if (mode == ID_SHAPE_POLYGON) {
				// we can generate points
				for(int i = 0 ; i < n ; ++i) {
					double theta = alpha * i;
					shape->points.push_back(new_shared2<ControlPoint>(
							c.x + ra * r.x * sin(theta),
							y   - ra * r.y * cos(theta)
						));
				}
			} else {
				// a star is made using a smaller, inverted polygon at the inside
				// points are interleaved
				// rb = radius of smaller polygon
				// lc = length of a side
				double lc = ra * sin(alpha) / cos(alpha/2);
				// ld = length of side skipping one corner
				double delta = alpha * 2;
				double ld = ra * sin(delta) / cos(delta/2);
				// Using symmetry: /_\gab ~ /_\axb where x is intersection
				// gives ratio lc/ld
				// converting back to radius using ra/lb = cos(beta/2) / sin(beta)
				// NOTE: This is only correct for n<=6, but gives acceptable results for higher n
				double rb = (ld - 2 * lc * (lc/ld)) * ra / lb;
				for(int i = 0 ; i < n ; ++i) {
					double theta = alpha * i;
					// from a
					shape->points.push_back(new_shared2<ControlPoint>(
							c.x + ra * r.x * sin(theta),
							y   - ra * r.y * cos(theta)
						));
					// from b
					theta = alpha * (i + 0.5);
					shape->points.push_back(new_shared2<ControlPoint>(
							c.x + rb * r.x * sin(theta),
							y   - rb * r.y * cos(theta)
						));
				}
			}
			break;
		}
	}
}

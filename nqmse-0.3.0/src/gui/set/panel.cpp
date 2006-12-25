//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <gui/set/panel.hpp>

// ----------------------------------------------------------------------------- : SetWindowPanel

SetWindowPanel::SetWindowPanel(Window* parent, int id, bool autoTabbing)
	: wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, autoTabbing ? wxTAB_TRAVERSAL : 0)
{}

//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2012 Twan van Laarhoven and Sean Hunt             |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#ifndef HEADER_UTIL_PREC
#define HEADER_UTIL_PREC

/** @file util/prec.hpp
 *
 *  @brief Precompiled header, and aliasses for common types
 */

// ----------------------------------------------------------------------------- : Includes

// Wx headers
#include <wx/setup.h>
#include <wx/wxprec.h>
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/datetime.h>
#include <wx/colour.h>
#include <wx/bitmap.h>

// Std headers
#include <vector>
#include <map>
#include <set>
using std::vector;
using std::map;
using std::set;

#undef RGB

// ----------------------------------------------------------------------------- : Wx Aliasses

// Remove some of the wxUglyness

typedef wxWindow Window;

typedef wxBitmap Bitmap;
typedef wxImage  Image;
typedef wxColour Color;
typedef wxDC     DC;

typedef wxDateTime DateTime;

// ----------------------------------------------------------------------------- : Compatability fixes

#if wxVERSION_NUMBER < 2805
	#define wxBORDER_THEME wxSUNKEN_BORDER
#endif
#if wxVERSION_NUMBER < 2900 && defined(__WXMSW__)
	// see http://docs.wxwidgets.org/2.8.11/wx_wxmswport.html
	#define wxBORDER_THEME_FIX(x) (x&wxBORDER_THEME ? (x&~wxBORDER_THEME)|wxWindow::GetThemedBorderStyle() : x)
#else
	#define wxBORDER_THEME_FIX(x) x
#endif
#if wxVERSION_NUMBER < 2900
	// wx >= 2.9 requires the use of HandleWindowEvent on windows, instead of ProcessEvent
	#define HandleWindowEvent ProcessEvent
#endif
#if wxVERSION_NUMBER < 2700
	// is it worth it to still support wx2.6?
	#define wxFD_SAVE             wxSAVE
	#define wxFD_OPEN             wxOPEN
	#define wxFD_OVERWRITE_PROMPT wxOVERWRITE_PROMPT
	typedef wxEvent wxMouseCaptureLostEvent;
	#define EVT_MOUSE_CAPTURE_LOST(handler) // ignore
	#define wxEVT_MOUSE_CAPTURE_LOST 12345678 // not an actual event type
	#define wxAutoBufferedPaintDC wxBufferedPaintDC
#endif
#if wxVERSION_NUMBER < 2811
	#define SetDeviceClippingRegion SetClippingRegion
#endif

// #if !wxCHECK_VERSION(3,1,1)
// namespace std {
// 	template <> inline void iter_swap<wxString::iterator>(wxString::iterator i1, wxString::iterator i2){
// 		wxUniCharRef r1 = *i1;
// 		wxUniCharRef r2 = *i2;
// 		std::swap(r1, r2);
// 	}
// }
// #endif

// ----------------------------------------------------------------------------- : Other aliasses

typedef unsigned char Byte;
typedef unsigned int  UInt;

// ----------------------------------------------------------------------------- : MSE Headers

// MSE utility headers (ones unlikely to change and used everywhere)
#include "string.hpp"
#include "smart_ptr.hpp"
#include "index_map.hpp"
#include "locale.hpp"
#include "error.hpp"
#include "reflect.hpp"
#include "regex.hpp"

// ----------------------------------------------------------------------------- : Debugging fixes

#ifdef _MSC_VER
	#if defined(_DEBUG) && defined(_CRT_WIDE)
		// Use OutputDebugString/DebugBreak for assertions if in debug mode
		void msvc_assert(const wchar_t*, const wchar_t*, const wchar_t*, unsigned);
		#undef assert
		#define assert(exp) (void)( (exp) || (msvc_assert(nullptr, _CRT_WIDE(#exp), _CRT_WIDE(__FILE__), __LINE__), 0) )
	#endif
#endif

// ----------------------------------------------------------------------------- : EOF
#endif

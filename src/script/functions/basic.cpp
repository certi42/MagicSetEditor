
//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2012 Twan van Laarhoven and Sean Hunt             |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <script/functions/functions.hpp>
#include <script/functions/util.hpp>
#include <util/tagged_string.hpp>
#include <util/spec_sort.hpp>
#include <util/error.hpp>
#include <data/set.hpp>
#include <data/card.hpp>
#include <data/game.hpp>

using std::make_pair;
using std::pair;
using std::reverse;
using std::sort;
using std::unique;

// ----------------------------------------------------------------------------- : Debugging

SCRIPT_FUNCTION(trace) {
	SCRIPT_PARAM_C(String, input);
	#if defined(_DEBUG) && 0
		wxLogDebug((L"Trace:\t") + input);
	#else
		queue_message(MESSAGE_INFO, (L"Trace: ") + input);
	#endif
	SCRIPT_RETURN(input);
}

SCRIPT_FUNCTION(warning) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_DEFAULT_C(bool, condition, true);
	if (condition) {
		queue_message(MESSAGE_WARNING, input);
	}
	return script_nil;
}
SCRIPT_FUNCTION(warning_if_neq) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_N(ScriptValueP, SCRIPT_VAR__1, v1);
	SCRIPT_PARAM_N(ScriptValueP, SCRIPT_VAR__2, v2);
	if (!equal(v1,v2)) {
		String s1 = (L"?"), s2 = (L"?");
		try {
			s1 = v1->toCode();
		} catch (...) {}
		try {
			s2 = v2->toCode();
		} catch (...) {}
		queue_message(MESSAGE_WARNING, input + s1 + (L" != ") + s2);
	}
	return script_nil;
}

SCRIPT_FUNCTION(error) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_DEFAULT_C(bool, condition, true);
	if (condition) {
		queue_message(MESSAGE_ERROR, input);
	}
	return script_nil;
}

// ----------------------------------------------------------------------------- : Conversion

/// Format the input variable based on a printf like style specification
String format_input(const String& format, const ScriptValue& input) {
	// determine type of input
	ScriptType type = input.type();
	if (type == SCRIPT_DATETIME) {
		return input.toDateTime().Format(format.c_str());
	} else {
		// determine type expected by format string
		String fmt = (L"%") + replace_all(format, (L"%"), (L""));
		if (format.find_first_of((L"DdIiOoXx")) != String::npos) {
			return String::Format(fmt, input.toInt());
		} else if (format.find_first_of((L"EeFfGg")) != String::npos) {
			return String::Format(fmt, input.toDouble());
		} else if (format.find_first_of((L"Ss")) != String::npos) {
			return format_string(fmt, input.toString());
		} else {
			throw ScriptError(_ERROR_1_("unsupported format", format));
		}
	}
}

SCRIPT_FUNCTION(to_string) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	ScriptValueP format = ctx.getVariable(SCRIPT_VAR_format);
	try {
		if (format && format->type() == SCRIPT_STRING) {
			// format specifier. Be careful, the built in function 'format' has the same name
			SCRIPT_RETURN(format_input(format->toString(), *input));
		} else {
			// simple conversion
			SCRIPT_RETURN(input->toString());
		}
	} catch (const ScriptError& e) {
		return intrusive(new ScriptDelayedError(e));
	}
}

SCRIPT_FUNCTION(to_int) {
	ScriptValueP input = ctx.getVariable(SCRIPT_VAR_input);
	ScriptType t = input->type();
	try {
		int result;
		if (t == SCRIPT_BOOL) {
			result = input->toBool() ? 1 : 0;
		} else if (t == SCRIPT_COLOR) {
			AColor c = input->toColor();
			result = (c.Red() + c.Blue() + c.Green()) / 3;
		} else if (t == SCRIPT_STRING) {
			long l;
			String str = input->toString();
			if (str.ToLong(&l)) {
				result = l;
			} else if (str.empty()) {
				result = 0;
			} else {
				return delay_error(ScriptErrorConversion(str, input->typeName(), _TYPE_("integer")));
			}
		} else {
			result = input->toInt();
		}
		SCRIPT_RETURN(result);
	} catch (const ScriptError& e) {
		return delay_error(e);
	}
}

SCRIPT_FUNCTION(to_real) {
	ScriptValueP input = ctx.getVariable(SCRIPT_VAR_input);
	ScriptType t = input->type();
	try {
		double result;
		if (t == SCRIPT_BOOL) {
			result = input->toBool() ? 1.0 : 0.0;
		} else if (t == SCRIPT_COLOR) {
			AColor c = input->toColor();
			result = (c.Red() + c.Blue() + c.Green()) / 3.0;
		} else if (t == SCRIPT_STRING) {
			String str = input->toString();
			if (str.empty()) {
				result = 0.0;
			} else if (!str.ToDouble(&result)) {
				return delay_error(ScriptErrorConversion(str, input->typeName(), _TYPE_("double")));
			}
		} else {
			result = input->toDouble();
		}
		SCRIPT_RETURN(result);
	} catch (const ScriptError& e) {
		return delay_error(e);
	}
}

SCRIPT_FUNCTION(to_number) {
	ScriptValueP input = ctx.getVariable(SCRIPT_VAR_input);
	ScriptType t = input->type();
	try {
		if (t == SCRIPT_BOOL) {
			SCRIPT_RETURN(input->toBool() ? 1 : 0);
		} else if (t == SCRIPT_COLOR) {
			AColor c = input->toColor();
			SCRIPT_RETURN( (c.Red() + c.Blue() + c.Green()) / 3 );
		} else if (t == SCRIPT_DOUBLE || t == SCRIPT_INT) {
			return input;
		} else if (t == SCRIPT_NIL) {
			SCRIPT_RETURN(0);
		} else {
			String str = input->toString();
			long l; double d;
			if (str.ToLong(&l)) {
				SCRIPT_RETURN((int)l);
			} else if (str.ToDouble(&d)) {
				SCRIPT_RETURN((double)d);
			} else if (str.empty()) {
				SCRIPT_RETURN(0);
			} else {
				return delay_error(ScriptErrorConversion(str, input->typeName(), _TYPE_("double")));
			}
		}
	} catch (const ScriptError& e) {
		return delay_error(e);
	}
}

SCRIPT_FUNCTION(to_boolean) {
	ScriptValueP input = ctx.getVariable(SCRIPT_VAR_input);
	try {
		ScriptType t = input->type();
		bool result;
		if (t == SCRIPT_INT) {
			result = input->toInt() != 0;
		} else {
			result = input->toBool();
		}
		SCRIPT_RETURN(result);
	} catch (const ScriptError& e) {
		return delay_error(e);
	}
}

SCRIPT_FUNCTION(to_color) {
	try {
		SCRIPT_PARAM_C(AColor, input);
		SCRIPT_RETURN(input);
	} catch (const ScriptError& e) {
		return delay_error(e);
	}
}

SCRIPT_FUNCTION(to_date) {
	try {
		SCRIPT_PARAM_C(wxDateTime, input);
		SCRIPT_RETURN(input);
	} catch (const ScriptError& e) {
		return delay_error(e);
	}
}

SCRIPT_FUNCTION(to_code) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	SCRIPT_RETURN(input->toCode());
}

// ----------------------------------------------------------------------------- : Math

SCRIPT_FUNCTION(abs) {
	ScriptValueP input = ctx.getVariable(SCRIPT_VAR_input);
	ScriptType t = input->type();
	if (t == SCRIPT_DOUBLE) {
		SCRIPT_RETURN(fabs(input->toDouble()));
	} else {
		SCRIPT_RETURN(abs(input->toInt()));
	}
}

SCRIPT_FUNCTION(random_real) {
	SCRIPT_PARAM_DEFAULT_C(double, begin, 0.0);
	SCRIPT_PARAM_DEFAULT_C(double, end,   1.0);
	SCRIPT_RETURN( (double)rand() / RAND_MAX * (end - begin) + begin );
}

SCRIPT_FUNCTION(random_int) {
	SCRIPT_PARAM_DEFAULT_C(int, begin, 0);
	SCRIPT_PARAM_C(        int, end);
	SCRIPT_RETURN( rand() % (end - begin) + begin );
}

SCRIPT_FUNCTION(random_boolean) {
	SCRIPT_PARAM_DEFAULT_C(double, input, 0.5);
	SCRIPT_RETURN( rand() < RAND_MAX * input  );
}


SCRIPT_FUNCTION(sin) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(sin(input));
}
SCRIPT_FUNCTION(cos) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(cos(input));
}
SCRIPT_FUNCTION(tan) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(tan(input));
}
SCRIPT_FUNCTION(sin_deg) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(sin(deg_to_rad(input)));
}
SCRIPT_FUNCTION(cos_deg) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(cos(deg_to_rad(input)));
}
SCRIPT_FUNCTION(tan_deg) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(tan(deg_to_rad(input)));
}
SCRIPT_FUNCTION(exp) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(exp(input));
}
SCRIPT_FUNCTION(log) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(log(input));
}
SCRIPT_FUNCTION(log10) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(log(input) / log(10.0));
}
SCRIPT_FUNCTION(sqrt) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_RETURN(sqrt(input));
}
SCRIPT_FUNCTION(pow) {
	SCRIPT_PARAM_C(double, input);
	SCRIPT_PARAM(double, exponent);
	SCRIPT_RETURN(pow(input,exponent));
}

// ----------------------------------------------------------------------------- : String stuff

// convert a string to upper case
SCRIPT_FUNCTION(to_upper_case) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(input.Upper());
}

// convert a string to lower case
SCRIPT_FUNCTION(to_lower_case) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(input.Lower());
}

// convert a string to title case
SCRIPT_FUNCTION(to_title_case) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(capitalize(input.Lower()));
}
// convert a string to sentence case
SCRIPT_FUNCTION(to_sentence_case) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(capitalize_sentence(input.Lower()));
}

// reverse a string
SCRIPT_FUNCTION(reverse_text) {
	SCRIPT_PARAM_C(String, input);
	reverse(input.begin(), input.end());
	SCRIPT_RETURN(input);
}

// remove leading and trailing whitespace from a string
SCRIPT_FUNCTION(trim) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(trim(input));
}

// extract a substring
SCRIPT_FUNCTION(substring) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_DEFAULT_C(int, begin, 0);
	SCRIPT_PARAM_DEFAULT_C(int, end,   INT_MAX);
	if (begin < 0) begin = 0;
	if (end   < 0) end   = 0;
	if (begin >= end || (size_t)begin >= input.size()) {
		SCRIPT_RETURN(wxEmptyString);
	} else if ((size_t)end >= input.size()) {
		SCRIPT_RETURN(input.substr(begin));
	} else {
		SCRIPT_RETURN(input.substr(begin, end - begin));
	}
}

// does a string contain a substring?
SCRIPT_FUNCTION(contains) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_C(String, match);
	SCRIPT_RETURN(input.find(match) != String::npos);
}

SCRIPT_FUNCTION(format) {
	SCRIPT_PARAM_C(String, format);
	SCRIPT_PARAM_C(ScriptValueP, input);
	SCRIPT_RETURN(format_input(format,*input));
}

SCRIPT_FUNCTION(curly_quotes) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(curly_quotes(input,true));
}

// regex escape a string
SCRIPT_FUNCTION(regex_escape) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_RETURN(regex_escape(input));
}

// sort/filter characters
SCRIPT_FUNCTION(sort_text) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_OPTIONAL_PARAM_C(String, order) {
		SCRIPT_RETURN(spec_sort(order, input));
	} else {
		sort(input.begin(), input.end());
		SCRIPT_RETURN(input);
	}
}

// ----------------------------------------------------------------------------- : Tagged string

/// Replace the contents of a specific tag with the value of a script function
String replace_tag_contents(String input, const String& tag, const ScriptValueP& contents, Context& ctx) {
	assert_tagged(input, false);
	String ret;
	size_t start = 0, pos = input.find(tag);
	while (pos != String::npos) {
		// find end of tag and contents
		size_t after     = skip_tag(input, pos);
		size_t end       = match_close_tag(input, pos);
		size_t after_end = skip_tag(input, end);
		if (end == String::npos) break; // missing close tag
		// prepare for call
		String old_contents = input.substr(after, end - after);
		ctx.setVariable(SCRIPT_VAR_input, to_script(old_contents));
		// replace
		ret.append(input, start, after-start); // before and including tag
		ret += contents->eval(ctx)->toString();// new contents (call)
		ret.append(input, end, after_end-end); // close tag
		// next
		start = after_end;
		pos = input.find(tag, start);
	}
	ret.append(input, start, pos-start);
	assert_tagged(ret, false);
	return ret;
}

// Replace the contents of a specific tag
SCRIPT_FUNCTION(replace_tag_contents) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_C(String, tag);
	SCRIPT_PARAM_C(ScriptValueP, contents);
	SCRIPT_RETURN(replace_tag_contents(input, tag, contents, ctx));
}

SCRIPT_FUNCTION(remove_tag) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_C(String, tag);
	assert_tagged(input, false);
	SCRIPT_RETURN(remove_tag(input, tag));
}

SCRIPT_FUNCTION(remove_tags) {
	SCRIPT_PARAM_C(String, input);
	assert_tagged(input, false);
	SCRIPT_RETURN(untag_no_escape(input));
}

// ----------------------------------------------------------------------------- : Collection stuff


/// position of some element in a vector
/** 0 based index, -1 if not found */
int position_in_vector(const ScriptValueP& of, const ScriptValueP& in, const ScriptValueP& order_by, const ScriptValueP& filter) {
	ScriptType of_t = of->type(), in_t = in->type();
	if (of_t == SCRIPT_STRING || in_t == SCRIPT_STRING) {
		// string finding
		return (int)of->toString().find(in->toString()); // (int)npos == -1
	} else if (order_by || filter) {
		ScriptObject<Set*>*  s = dynamic_cast<ScriptObject<Set*>* >(in.get());
		ScriptObject<CardP>* c = dynamic_cast<ScriptObject<CardP>*>(of.get());
		if (s && c) {
			return s->getValue()->positionOfCard(c->getValue(), order_by, filter);
		} else {
			throw ScriptError((L"position: using 'order_by' or 'filter' is only supported for finding cards in the set"));
		}
	} else {
		// unordered position
		ScriptValueP it = in->makeIterator();
		int i = 0;
		while (ScriptValueP v = it->next()) {
			if (equal(of, v)) return i;
			i++;
		}
	}
	return -1; // TODO?
}

inline bool smart_less_first(const pair<String,ScriptValueP>& a, const pair<String,ScriptValueP>& b) {
	return smart_less(a.first, b.first);
}
inline bool smart_equal_first(const pair<String,ScriptValueP>& a, const pair<String,ScriptValueP>& b) {
	return smart_equal(a.first, b.first);
}

// sort a script list
ScriptValueP sort_script(Context& ctx, const ScriptValueP& list, ScriptValue& order_by, bool remove_duplicates) {
	ScriptType list_t = list->type();
	if (list_t == SCRIPT_STRING) {
		// sort a string
		String s = list->toString();
		sort(s.begin(), s.end());
		if (remove_duplicates) {
			s.erase( unique(s.begin(), s.end()), s.end() );
		}
		SCRIPT_RETURN(s);
	} else {
		// are we sorting a set?
		ScriptObject<Set*>* set = dynamic_cast<ScriptObject<Set*>*>(list.get());
		// sort a collection
		vector<pair<String,ScriptValueP> > values;
		ScriptValueP it = list->makeIterator();
		while (ScriptValueP v = it->next()) {
			ctx.setVariable(set ? (L"card") : (L"input"), v);
			values.push_back(make_pair(order_by.eval(ctx)->toString(), v));
		}
		sort(values.begin(), values.end(), smart_less_first);
		// unique
		if (remove_duplicates) {
			values.erase( unique(values.begin(), values.end(), smart_equal_first), values.end() );
		}
		// return collection
		ScriptCustomCollectionP ret(new ScriptCustomCollection());
		for(auto& v : values) {
			ret->value.push_back(v.second);
		}
		return ret;
	}
}

// finding positions, also of substrings
SCRIPT_FUNCTION_WITH_DEP(position_of) {
	ScriptValueP of       = ctx.getVariable((L"of"));
	ScriptValueP in       = ctx.getVariable((L"in"));
	ScriptValueP order_by = ctx.getVariableOpt((L"order_by"));
	ScriptValueP filter   = ctx.getVariableOpt((L"filter"));
	if (filter == script_nil) filter = ScriptValueP();
	SCRIPT_RETURN(position_in_vector(of, in, order_by, filter));
}
SCRIPT_FUNCTION_DEPENDENCIES(position_of) {
	ScriptValueP of       = ctx.getVariable((L"of"));
	ScriptValueP in       = ctx.getVariable((L"in"));
	ScriptValueP order_by = ctx.getVariableOpt((L"order_by"));
	ScriptValueP filter   = ctx.getVariableOpt((L"filter"));
	ScriptObject<Set*>*  s = dynamic_cast<ScriptObject<Set*>* >(in.get());
	ScriptObject<CardP>* c = dynamic_cast<ScriptObject<CardP>*>(of.get());
	if (s && c) {
		// dependency on cards
		mark_dependency_member(*s->getValue(), (L"cards"), dep);
		if (order_by) {
			// dependency on order_by function
			order_by->dependencies(ctx, dep.makeCardIndependend());
		}
		if (filter && filter != script_nil) {
			// dependency on filter function
			filter->dependencies(ctx, dep.makeCardIndependend());
		}
	}
	return dependency_dummy;
};

// finding sizes
int script_length_of(Context& ctx, const ScriptValueP& collection) {
	if (ScriptObject<Set*>* setobj = dynamic_cast<ScriptObject<Set*>*>(collection.get())) {
		Set* set = setobj->getValue();
		SCRIPT_OPTIONAL_PARAM_C_(ScriptValueP, filter);
		return set->numberOfCards(filter);
	} else {
		return collection->itemCount();
	}
}
SCRIPT_FUNCTION(length) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	SCRIPT_RETURN(script_length_of(ctx, input));
}
SCRIPT_FUNCTION(number_of_items) {
	SCRIPT_PARAM_C(ScriptValueP, in);
	SCRIPT_RETURN(script_length_of(ctx, in));
}

// filtering items from a list
SCRIPT_FUNCTION(filter_list) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	SCRIPT_PARAM_C(ScriptValueP, filter);
	// filter a collection
	ScriptCustomCollectionP ret(new ScriptCustomCollection());
	ScriptValueP it = input->makeIterator();
	while (ScriptValueP v = it->next()) {
		ctx.setVariable(SCRIPT_VAR_input, v);
		if (filter->eval(ctx)->toBool()) {
			ret->value.push_back(v);
		}
	}
	// TODO : somehow preserve keys
	return ret;
}

SCRIPT_FUNCTION(sort_list) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	SCRIPT_PARAM_DEFAULT(ScriptValueP, order_by, script_nil);
	SCRIPT_PARAM_DEFAULT(bool, remove_duplicates, false);
	return sort_script(ctx, input, *order_by, remove_duplicates);
}


SCRIPT_FUNCTION(random_shuffle) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	// convert to CustomCollection
	ScriptCustomCollectionP ret(new ScriptCustomCollection());
	ScriptValueP it = input->makeIterator();
	while (ScriptValueP v = it->next()) {
		ret->value.push_back(v);
	}
	// shuffle
	random_shuffle(ret->value.begin(), ret->value.end());
	return ret;
}

SCRIPT_FUNCTION(random_select) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	// pick a single one
	int itemCount = input->itemCount();
	if (itemCount == 0) {
		throw ScriptError((L"Can not select a random item from an empty collection"));
	}
	return input->getIndex( rand() % itemCount );
}

SCRIPT_FUNCTION(random_select_many) {
	SCRIPT_PARAM_C(ScriptValueP, input);
	SCRIPT_PARAM(int, count) ;
	SCRIPT_OPTIONAL_PARAM_C_(ScriptValueP, replace);
	bool with_replace = replace && replace->type() != SCRIPT_FUNCTION && replace->toBool();
	// pick many
	ScriptCustomCollectionP ret(new ScriptCustomCollection);
	int itemCount = input->itemCount();
	if (with_replace) {
		if (itemCount == 0) {
			throw ScriptError(String::Format((L"Can not select %d items from an empty collection"), count));
		}
		for (int i = 0 ; i < count ; ++i) {
			ret->value.push_back( input->getIndex( rand() % itemCount ) );
		}
	} else {
		if (count > itemCount) {
			throw ScriptError(String::Format((L"Can not select %d items from a collection conaining only %d items"), count, input->itemCount()));
		}
		// transfer all to ret and shuffle
		ScriptValueP it = input->makeIterator();
		while (ScriptValueP v = it->next()) {
			ret->value.push_back(v);
		}
		random_shuffle(ret->value.begin(), ret->value.end());
		// keep only the first 'count'
		ret->value.resize(count);
	}
	return ret;
}

// ----------------------------------------------------------------------------- : Keywords


SCRIPT_FUNCTION_WITH_DEP(expand_keywords) {
	SCRIPT_PARAM_C(String, input);
	SCRIPT_PARAM_C(Set*, set);
	SCRIPT_OPTIONAL_PARAM_N_(ScriptValueP, (L"condition"), match_condition);
	SCRIPT_OPTIONAL_PARAM_(ScriptValueP, default_expand);
	SCRIPT_PARAM(ScriptValueP, combine);
	KeywordDatabase& db = set->keyword_db;
	if (db.empty()) {
		db.prepare_parameters(set->game->keyword_parameter_types, set->keywords);
		db.prepare_parameters(set->game->keyword_parameter_types, set->game->keywords);
		db.add(set->keywords);
		db.add(set->game->keywords);
	}
	SCRIPT_OPTIONAL_PARAM_C_(CardP, card);
	WITH_DYNAMIC_ARG(keyword_usage_statistics, card ? &card->keyword_usage : nullptr);
	try {
		SCRIPT_RETURN(db.expand(input, match_condition, default_expand, combine, ctx));
	} catch (const Error& e) {
		throw ScriptError(_ERROR_2_("in function", e.what().c_str(), (L"expand_keywords")));
	}
}
SCRIPT_FUNCTION_DEPENDENCIES(expand_keywords) {
	SCRIPT_PARAM_C(Set*, set);
	SCRIPT_OPTIONAL_PARAM_N_(ScriptValueP, (L"condition"), match_condition);
	SCRIPT_OPTIONAL_PARAM_(ScriptValueP, default_expand);
	SCRIPT_PARAM(ScriptValueP, combine);
	if (match_condition) match_condition->dependencies(ctx,dep);
	default_expand ->dependencies(ctx,dep);
	combine        ->dependencies(ctx,dep);
	set->game->dependent_scripts_keywords.add(dep); // this depends on the set's keywords
	return ctx.getVariable(SCRIPT_VAR_input);
}

SCRIPT_FUNCTION(keyword_usage) {
	SCRIPT_PARAM_C(CardP, card);
	SCRIPT_OPTIONAL_PARAM_(bool, unique);
	// make a list "kw1, kw2, kw3" of keywords used on card
	String ret;
	for (KeywordUsageStatistics::const_iterator it = card->keyword_usage.begin() ; it != card->keyword_usage.end() ; ++it) {
		bool keep = true;
		if (unique) {
			// prevent duplicates
			for (KeywordUsageStatistics::const_iterator it2 = card->keyword_usage.begin() ; it != it2 ; ++it2) {
				if (it->second == it2->second) {
					keep = false;
					break;
				}
			}
		}
		if (keep) {
			if (!ret.empty()) ret += (L", ");
			ret += it->second->keyword;
		}
	}
	SCRIPT_RETURN(ret);
}

// ----------------------------------------------------------------------------- : Rule form

/// Turn a script function into a rule, a.k.a. a delayed closure
SCRIPT_FUNCTION(rule) {
	SCRIPT_PARAM(ScriptValueP, input);
	return intrusive(new ScriptRule(input));
}

// ----------------------------------------------------------------------------- : Init

void init_script_basic_functions(Context& ctx) {
	// debugging
	ctx.setVariable((L"trace"),                script_trace);
	ctx.setVariable((L"warning"),              script_warning);
	ctx.setVariable((L"error"),                script_error);
	// conversion
	ctx.setVariable((L"to_string"),            script_to_string);
	ctx.setVariable((L"to_int"),               script_to_int);
	ctx.setVariable((L"to_real"),              script_to_real);
	ctx.setVariable((L"to_number"),            script_to_number);
	ctx.setVariable((L"to_boolean"),           script_to_boolean);
	ctx.setVariable((L"to_color"),             script_to_color);
	ctx.setVariable((L"to_date"),              script_to_date);
	ctx.setVariable((L"to_code"),              script_to_code);
	// math
	ctx.setVariable((L"random_real"),          script_random_real);
	ctx.setVariable((L"random_int"),           script_random_int);
	ctx.setVariable((L"random_boolean"),       script_random_boolean);
	ctx.setVariable((L"abs"),                  script_abs);
	ctx.setVariable((L"sin"),                  script_sin);
	ctx.setVariable((L"cos"),                  script_cos);
	ctx.setVariable((L"tan"),                  script_tan);
	ctx.setVariable((L"sin_deg"),              script_sin_deg);
	ctx.setVariable((L"cos_deg"),              script_cos_deg);
	ctx.setVariable((L"tan_deg"),              script_tan_deg);
	ctx.setVariable((L"exp"),                  script_exp);
	ctx.setVariable((L"log"),                  script_log);
	ctx.setVariable((L"log10"),                script_log10);
	ctx.setVariable((L"sqrt"),                 script_sqrt);
	ctx.setVariable((L"pow"),                  script_pow);
	// string
	ctx.setVariable((L"to_upper_case"),        script_to_upper_case);
	ctx.setVariable((L"to_lower_case"),        script_to_lower_case);
	ctx.setVariable((L"to_title_case"),        script_to_title_case);
	ctx.setVariable((L"to_sentence_case"),     script_to_sentence_case);
	ctx.setVariable((L"to_upper"),             script_to_upper_case); // old name
	ctx.setVariable((L"to_lower"),             script_to_lower_case); // old name
	ctx.setVariable((L"to_title"),             script_to_title_case); // old name
	ctx.setVariable((L"reverse"),              script_reverse_text); // old name
	ctx.setVariable((L"reverse_text"),         script_reverse_text);
	ctx.setVariable((L"trim"),                 script_trim);
	ctx.setVariable((L"substring"),            script_substring);
	ctx.setVariable((L"contains"),             script_contains);
	ctx.setVariable((L"format"),               script_format);
	ctx.setVariable((L"format_rule"),          intrusive(new ScriptRule(script_format))); // compatability
	ctx.setVariable((L"curly_quotes"),         script_curly_quotes);
	ctx.setVariable((L"regex_escape"),         script_regex_escape);
	ctx.setVariable((L"sort_text"),            script_sort_text);
	ctx.setVariable((L"sort_rule"),            intrusive(new ScriptRule(script_sort_text))); // compatability
	// tagged string
	ctx.setVariable((L"tag_contents"),         script_replace_tag_contents); // old name
	ctx.setVariable((L"replace_tag_contents"), script_replace_tag_contents);
	ctx.setVariable((L"remove_tag"),           script_remove_tag);
	ctx.setVariable((L"remove_tags"),          script_remove_tags);
	ctx.setVariable((L"tag_contents_rule"),    intrusive(new ScriptRule(script_replace_tag_contents))); // compatability
	ctx.setVariable((L"tag_remove_rule"),      intrusive(new ScriptRule(script_remove_tag))); // compatability
	// collection
	ctx.setVariable((L"position"),             script_position_of);
	ctx.setVariable((L"length"),               script_length);
	ctx.setVariable((L"number_of_items"),      script_number_of_items); // deprecated
	ctx.setVariable((L"filter_list"),          script_filter_list);
	ctx.setVariable((L"sort_list"),            script_sort_list);
	ctx.setVariable((L"random_shuffle"),       script_random_shuffle);
	ctx.setVariable((L"random_select"),        script_random_select);
	ctx.setVariable((L"random_select_many"),   script_random_select_many);
	// keyword
	ctx.setVariable((L"expand_keywords"),      script_expand_keywords);
	ctx.setVariable((L"expand_keywords_rule"), intrusive(new ScriptRule(script_expand_keywords))); // compatability
	ctx.setVariable((L"keyword_usage"),        script_keyword_usage);
}

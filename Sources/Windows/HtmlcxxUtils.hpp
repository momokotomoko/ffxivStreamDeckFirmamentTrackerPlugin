//==============================================================================
/**
@file       HtmlcxxUtils.hpp
@brief      utilities for parsing html pages
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#pragma once
#include "ParserDom.h"
#include <string>

namespace htmlcxxutils
{
	/*
		@brief case-insensitive string comparison

		@param[in] a string one
		@param[in] b string two

		@return true if equal, false if not
	*/
	static bool strCaseCmp(const std::string& a, const std::string& b)
	{
		return std::equal(a.begin(), a.end(), b.begin(),
			[](char a, char b) {
				return tolower(a) == tolower(b);
			});
	}

	/*
	@brief Find in htmlcxx tree for attribute with name and value

	@param[in] name name of attribute
	@param[in] value value of attribute
	@param[in] begin start of range
	@param[in] end end of range

	@return iterator to first match or end if not found
*/
	static tree<htmlcxx::HTML::Node>::pre_order_iterator htmlcxxFindNextAttribute(const std::string& name, const std::string& value,
		tree<htmlcxx::HTML::Node>::pre_order_iterator begin,
		tree<htmlcxx::HTML::Node>::pre_order_iterator end)
	{
		auto it = begin;
		for (; it != end; it++)
		{
			if (it->isTag())
			{
				it->parseAttributes();
				if (it->attribute(name).second == value)
				{
					return it;
				}
			}
		}
		return it;
	}

	/*
		@brief Find in htmlcxx tree for next iterator with matching tag

		@param[in] name name of tag
		@param[in] begin start of range
		@param[in] end end of range

		@return iterator to first match or end if not found
	*/
	static tree<htmlcxx::HTML::Node>::pre_order_iterator htmlcxxFindNextTag(const std::string& name,
		tree<htmlcxx::HTML::Node>::pre_order_iterator begin,
		tree<htmlcxx::HTML::Node>::pre_order_iterator end)
	{
		auto it = begin;
		for (; it != end; it++)
		{
			if (it->isTag())
			{
				if (it->tagName() == name)
				{
					return it;
				}
			}
		}
		return it;
	}
}
/*
 * Copyright (c) 2016 Evgenii Dobrovidov
 * This file is part of "Bashorg".
 *
 * "Bashorg" is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * "Bashorg" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with "Bashorg".  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef QUOTE_H_
#define QUOTE_H_

#include <FBase.h>

using namespace Osp::Base;

enum BashPage {
	PAGE_ALL_QUOTES = 0,
	PAGE_SAVED_QUOTES,
	PAGE_RECENT_QUOTES,
	PAGE_RANDOM_QUOTES,
	PAGE_BEST_QUOTES,
	//PAGE_MOST_RATED_QUOTES,
	PAGE_ABYSS_QUOTES,
	PAGE_ABYSS_BEST_QUOTES,
	PAGE_ABYSS_TOP_QUOTES
};

class Quote {
public:
	Quote(void) {}
	Quote(const Quote &quote, BashPage page = PAGE_SAVED_QUOTES) {
		SetSerialized(false);

		SetPage(page);
		SetID(quote.GetID());
		SetRating(quote.GetRating());
		SetDateTime(quote.GetDateTime());
		SetText(quote.GetText());
		SetRead(true);
		SetPageNumber(0);
	}

	bool operator==(const Quote &other) {
		return (__page == other.GetPage() && __id == other.GetID() && __date.Equals(other.GetDateTime(), false) &&
				__text.Equals(other.GetText(), false));
	}
	bool operator!=(const Quote &other) {
		return (__page != other.GetPage() || __id != other.GetID() || !__date.Equals(other.GetDateTime(), false) ||
				!__text.Equals(other.GetText(), false));
	}

	int GetEntryId(void) const { return __entryId; }

	bool GetSerialized(void) const { return __serialized; }
	void SetSerialized(bool val) { __serialized = val; }

	BashPage GetPage(void) const { return __page; }
	void SetPage(BashPage page) { __page = page; }

	int GetPageNumber(void) const { return __pageNum; }
	void SetPageNumber(int num) { __pageNum = num; }

	int GetID(void) const { return __id; }
	void SetID(int id) { __id = id; }

	int GetRating(void) const { return __rating; }
	void SetRating(int rate) { __rating = rate; }

	String GetDateTime(void) const { return __date; }
	void SetDateTime(const String &date) { __date = date; }

	String GetText(void) const { return __text; }
	void SetText(const String &text) { __text = text; }

	bool GetRead(void) const { return __read; }
	void SetRead(bool val) { __read = val; }

private:
	bool __serialized;
	int __entryId;
	BashPage __page;

	int __pageNum;
	int __id;
	int __rating;
	String __date;
	String __text;
	bool __read;

	void SetEntryID(int id) {
		__entryId = id;
	}

	friend class QuotesManager;
};

#endif

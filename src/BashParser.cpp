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

#include "BashParser.h"

const wchar_t* ConvertCP1251ToUCS2(const byte* buf, int len) {
	wchar_t *res = new wchar_t[len + 1];
	for(int i = 0; i < len; i++) {
		res[i] = __pCP1251ToUCS2CodeTable[buf[i]];
	}
	res[len] = L'\0';
	return res;
}

LinkedListT<Quote *> *ParseBashPageN(const String &str, BashPage parsed_page) {
	if (parsed_page < PAGE_ABYSS_QUOTES) {
		return ParseBashPageNormalN(str, parsed_page);
	} else if (parsed_page == PAGE_ABYSS_QUOTES) {
		return ParseBashPageAbyssN(str);
	} else if (parsed_page == PAGE_ABYSS_TOP_QUOTES) {
		return ParseBashPageAbyssTopN(str);
	} else if (parsed_page == PAGE_ABYSS_BEST_QUOTES) {
		return ParseBashPageAbyssBestN(str);
	} else {
		return null;
	}
}

#define FAIL 		if (res == E_OBJ_NOT_FOUND) {																	\
						break;																				\
					} else if (IsFailed(res)) {																		\
						AppLogException("Failed to parse page at some stage, error: [%s]", GetErrorMessage(res));	\
						SetLastResult(res);																			\
						return null;																				\
					}

LinkedListT<Quote *> *ParseBashPageNormalN(const String &str, BashPage parsed_page) {
	LinkedListT<Quote *> *pQuotes = new LinkedListT<Quote *>;

	int startIndex = 0;
	int firstClosingDivIndex = 0;

	int quoteStartIndex = -1;
	int quoteEndIndex = -1;
	int dataStartIndex = -1;
	int dataEndIndex = -1;
	int dataIdStart = -1;
	int dataIdEnd = -1;
	int dataRatingStart = -1;
	int dataRatingEnd = -1;
	int dataDateStart = -1;
	int dataDateEnd = -1;
	int currentPageNumber = 0;

	result res = E_SUCCESS;

	int pagerStartIndex = 0;
	res = str.IndexOf(L"<input type=\"text\" name=\"page\" class=\"page\" pattern=\"[0-9]+\" numeric=\"integer\" min=\"1\" max=\"", 0, pagerStartIndex);
	if (!IsFailed(res)) {
		pagerStartIndex += 92;

		int valueStartIndex = 0;
		res = str.IndexOf(L"value=\"", pagerStartIndex, valueStartIndex);
		if (!IsFailed(res)) {
			valueStartIndex += 7;

			int quoteCloseIndex = 0;
			res = str.IndexOf(L"\"", valueStartIndex, quoteCloseIndex);
			if (!IsFailed(res)) {
				String page;
				str.SubString(valueStartIndex, quoteCloseIndex - valueStartIndex, page);
				Integer::Parse(page, currentPageNumber);
			}
		}
	}

	String data = L"";
	while(true) {
		res = str.IndexOf(L"<div class=\"quote\">", startIndex, startIndex);
		FAIL;
		startIndex += 19;

		res = str.IndexOf(L"</div>", startIndex, firstClosingDivIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"text\">", startIndex, quoteStartIndex);
		FAIL;
		quoteStartIndex += 18;

		res = str.IndexOf(L"</div>", quoteStartIndex, quoteEndIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"actions\">", startIndex, dataStartIndex);
		FAIL;
		dataStartIndex += 21;

		if (dataStartIndex >= firstClosingDivIndex) {
			//this is an ad
			continue;
		}

		res = str.IndexOf(L"</div>", dataStartIndex, dataEndIndex);
		FAIL;

		if (dataStartIndex >= quoteEndIndex || dataEndIndex >= quoteEndIndex) {
			continue;
		}

		str.SubString(dataStartIndex, dataEndIndex - dataStartIndex, data);
		if (data.IsEmpty()) {
			continue;
		}

		res = data.IndexOf(L"class=\"id\">", 0, dataIdStart);
		FAIL;
		dataIdStart += 11;

		res = data.IndexOf(L"</a>", dataIdStart, dataIdEnd);
		FAIL;

		String quoteNumber;
		data.SubString(dataIdStart, dataIdEnd - dataIdStart, quoteNumber);
		quoteNumber.Remove(0, 1);//remove '#' symbol

		int number = -1;
		res = Integer::Parse(quoteNumber, number);
		if (IsFailed(res)) {
			AppLogException("Failed to parse quote's ID string [%S], left blank. Error: [%s]", quoteNumber.GetPointer(), GetErrorMessage(res));
		}

		res = data.IndexOf(L"class=\"rating\">", 0, dataRatingStart);
		FAIL;
		dataRatingStart += 15;

		res = data.IndexOf(L"</span>", dataRatingStart, dataRatingEnd);
		FAIL;

		int rating = -1;
		String quoteRating;
		data.SubString(dataRatingStart, dataRatingEnd - dataRatingStart, quoteRating);

		quoteRating.Trim();
		if (quoteRating.Equals(L"...", false)) {
			rating = 0;
		} else if (!quoteRating.Equals(L"???", false)) {
			res = Integer::Parse(quoteRating, rating);
			if (IsFailed(res)) {
				AppLogException("Failed to parse quote's rating string [%S], left blank. Error: [%s]", quoteRating.GetPointer(), GetErrorMessage(res));
			}
		}

		res = data.IndexOf(L"class=\"date\">", 0, dataDateStart);
		FAIL;
		dataDateStart += 13;

		res = data.IndexOf(L"</span>", dataDateStart, dataDateEnd);
		FAIL;

		String date;
		data.SubString(dataDateStart, dataDateEnd - dataDateStart, date);

		String text;
		str.SubString(quoteStartIndex, quoteEndIndex - quoteStartIndex, text);

		text.Replace(L"&lt;", L"<");
		text.Replace(L"&gt;", L">");
		text.Replace(L"<br>", L"\n");
		text.Replace(L"<br />", L"\n");
		text.Replace(L"&quot;", L"\"");
		text.Replace(L"&amp;", L"&");

		String num;
		int ch;
		int repInd = 0;
		int stInd = 0;
		while (!IsFailed(text.IndexOf(L"&#", stInd, repInd))) {
			repInd += 2;
			text.IndexOf(L';', repInd, stInd);

			if (!IsFailed(text.SubString(repInd, stInd - repInd, num))) {
				if (!IsFailed(Integer::Parse(num, ch))) {
					text.Remove(repInd - 2, 3 + num.GetLength());
					text.Insert((mchar)ch, repInd - 2);
				}
			}
			stInd++;
		}

		Quote *pQuote = new Quote;
		pQuote->SetPage(parsed_page);
		pQuote->SetSerialized(false);

		pQuote->SetPageNumber(currentPageNumber);
		pQuote->SetID(number);
		pQuote->SetRating(rating);
		pQuote->SetDateTime(date);
		pQuote->SetText(text);
		pQuote->SetRead(true);

		pQuotes->Add(pQuote);
	}
	return pQuotes;
}

LinkedListT<Quote *> *ParseBashPageAbyssN(const String &str) {
	LinkedListT<Quote *> *pQuotes = new LinkedListT<Quote *>;

	int startIndex = 0;
	int firstClosingDivIndex = 0;

	int quoteStartIndex = -1;
	int quoteEndIndex = -1;
	int dataStartIndex = -1;
	int dataEndIndex = -1;
	int dataIdStart = -1;
	int dataIdEnd = -1;
	int dataRatingStart = -1;
	int dataRatingEnd = -1;
	int dataDateStart = -1;
	int dataDateEnd = -1;

	result res = E_SUCCESS;

	String data = L"";
	while(true) {
		res = str.IndexOf(L"<div class=\"quote\">", startIndex, startIndex);
		FAIL;
		startIndex += 19;

		res = str.IndexOf(L"</div>", startIndex, firstClosingDivIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"text\">", startIndex, quoteStartIndex);
		FAIL;
		quoteStartIndex += 18;

		res = str.IndexOf(L"</div>", quoteStartIndex, quoteEndIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"actions\">", startIndex, dataStartIndex);
		FAIL;
		dataStartIndex += 21;

		if (dataStartIndex >= firstClosingDivIndex) {
			//this is an ad
			continue;
		}

		res = str.IndexOf(L"</div>", dataStartIndex, dataEndIndex);
		FAIL;

		if (dataStartIndex >= quoteEndIndex || dataEndIndex >= quoteEndIndex) {
			continue;
		}

		str.SubString(dataStartIndex, dataEndIndex - dataStartIndex, data);
		if (data.IsEmpty()) {
			continue;
		}

		res = data.IndexOf(L"class=\"id\">", 0, dataIdStart);
		FAIL;
		dataIdStart += 11;

		res = data.IndexOf(L"</span>", dataIdStart, dataIdEnd);
		FAIL;

		String quoteNumber;
		data.SubString(dataIdStart, dataIdEnd - dataIdStart, quoteNumber);
		quoteNumber.Remove(0, 1);//remove '#' symbol

		int number = -1;
		res = Integer::Parse(quoteNumber, number);
		if (IsFailed(res)) {
			AppLogException("Failed to parse quote's ID string [%S], left blank. Error: [%s]", quoteNumber.GetPointer(), GetErrorMessage(res));
		}

		res = data.IndexOf(L"class=\"rating\">", 0, dataRatingStart);
		FAIL;
		dataRatingStart += 15;

		res = data.IndexOf(L"</span>", dataRatingStart, dataRatingEnd);
		FAIL;

		int rating = -1;
		String quoteRating;
		data.SubString(dataRatingStart, dataRatingEnd - dataRatingStart, quoteRating);

		quoteRating.Trim();
		if (quoteRating.Equals(L"...", false)) {
			rating = 0;
		} else if (!quoteRating.Equals(L"???", false)) {
			res = Integer::Parse(quoteRating, rating);
			if (IsFailed(res)) {
				AppLogException("Failed to parse quote's rating string [%S], left blank. Error: [%s]", quoteRating.GetPointer(), GetErrorMessage(res));
			}
		}

		res = data.IndexOf(L"class=\"date\">", 0, dataDateStart);
		FAIL;
		dataDateStart += 13;

		res = data.IndexOf(L"</span>", dataDateStart, dataDateEnd);
		FAIL;

		String date;
		data.SubString(dataDateStart, dataDateEnd - dataDateStart, date);

		String text;
		str.SubString(quoteStartIndex, quoteEndIndex - quoteStartIndex, text);

		text.Replace(L"&lt;", L"<");
		text.Replace(L"&gt;", L">");
		text.Replace(L"<br>", L"\n");
		text.Replace(L"<br />", L"\n");
		text.Replace(L"&quot;", L"\"");
		text.Replace(L"&amp;", L"&");

		String num;
		int ch;
		int repInd = 0;
		int stInd = 0;
		while (!IsFailed(text.IndexOf(L"&#", stInd, repInd))) {
			repInd += 2;
			text.IndexOf(L';', repInd, stInd);

			if (!IsFailed(text.SubString(repInd, stInd - repInd, num))) {
				if (!IsFailed(Integer::Parse(num, ch))) {
					text.Remove(repInd - 2, 3 + num.GetLength());
					text.Insert((mchar)ch, repInd - 2);
				}
			}
			stInd++;
		}

		Quote *pQuote = new Quote;
		pQuote->SetPage(PAGE_ABYSS_QUOTES);
		pQuote->SetSerialized(false);

		pQuote->SetPageNumber(0);
		pQuote->SetID(number);
		//pQuote->SetRating(-2);
		pQuote->SetRating(rating);
		pQuote->SetDateTime(date);
		pQuote->SetText(text);
		pQuote->SetRead(true);

		pQuotes->Add(pQuote);
	}
	return pQuotes;
}

LinkedListT<Quote *> *ParseBashPageAbyssTopN(const String &str) {
	LinkedListT<Quote *> *pQuotes = new LinkedListT<Quote *>;

	int startIndex = 0;
	int firstClosingDivIndex = 0;

	int quoteStartIndex = -1;
	int quoteEndIndex = -1;
	int dataStartIndex = -1;
	int dataEndIndex = -1;
	int dataDateStart = -1;
	int dataDateEnd = -1;

	result res = E_SUCCESS;

	String data = L"";
	while(true) {
		res = str.IndexOf(L"<div class=\"quote\">", startIndex, startIndex);
		FAIL;
		startIndex += 19;

		res = str.IndexOf(L"</div>", startIndex, firstClosingDivIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"text\">", startIndex, quoteStartIndex);
		FAIL;
		quoteStartIndex += 18;

		res = str.IndexOf(L"</div>", quoteStartIndex, quoteEndIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"actions\">", startIndex, dataStartIndex);
		FAIL;
		dataStartIndex += 21;

		if (dataStartIndex >= firstClosingDivIndex) {
			//this is an ad
			continue;
		}

		res = str.IndexOf(L"</div>", dataStartIndex, dataEndIndex);
		FAIL;

		if (dataStartIndex >= quoteEndIndex || dataEndIndex >= quoteEndIndex) {
			continue;
		}

		str.SubString(dataStartIndex, dataEndIndex - dataStartIndex, data);
		if (data.IsEmpty()) {
			continue;
		}

		res = data.IndexOf(L"class=\"abysstop-date\">", 0, dataDateStart);
		FAIL;
		dataDateStart += 22;

		res = data.IndexOf(L"</span>", dataDateStart, dataDateEnd);
		FAIL;

		String date;
		data.SubString(dataDateStart, dataDateEnd - dataDateStart, date);

		String text;
		str.SubString(quoteStartIndex, quoteEndIndex - quoteStartIndex, text);

		text.Replace(L"&lt;", L"<");
		text.Replace(L"&gt;", L">");
		text.Replace(L"<br>", L"\n");
		text.Replace(L"<br />", L"\n");
		text.Replace(L"&quot;", L"\"");
		text.Replace(L"&amp;", L"&");

		String num;
		int ch;
		int repInd = 0;
		int stInd = 0;
		while (!IsFailed(text.IndexOf(L"&#", stInd, repInd))) {
			repInd += 2;
			text.IndexOf(L';', repInd, stInd);

			if (!IsFailed(text.SubString(repInd, stInd - repInd, num))) {
				if (!IsFailed(Integer::Parse(num, ch))) {
					text.Remove(repInd - 2, 3 + num.GetLength());
					text.Insert((mchar)ch, repInd - 2);
				}
			}
			stInd++;
		}

		Quote *pQuote = new Quote;
		pQuote->SetPage(PAGE_ABYSS_TOP_QUOTES);
		pQuote->SetSerialized(false);

		pQuote->SetPageNumber(0);
		pQuote->SetID(0);
		pQuote->SetRating(-2);
		pQuote->SetDateTime(date);
		pQuote->SetText(text);
		pQuote->SetRead(true);

		pQuotes->Add(pQuote);
	}
	return pQuotes;
}

LinkedListT<Quote *> *ParseBashPageAbyssBestN(const String &str) {
	LinkedListT<Quote *> *pQuotes = new LinkedListT<Quote *>;

	int startIndex = 0;
	int firstClosingDivIndex = 0;

	int quoteStartIndex = -1;
	int quoteEndIndex = -1;
	int dataStartIndex = -1;
	int dataEndIndex = -1;
	int dataIdStart = -1;
	int dataIdEnd = -1;
	int dataDateStart = -1;
	int dataDateEnd = -1;
	int currentPageNumber = 0;

	result res = E_SUCCESS;

	int pagerStartIndex = 0;
	res = str.IndexOf(L"<input type=\"text\" name=\"page\" class=\"page\" pattern=\"[0-9]+\" numeric=\"integer\" min=\"1\" max=\"", 0, pagerStartIndex);
	if (!IsFailed(res)) {
		pagerStartIndex += 92;

		int valueStartIndex = 0;
		res = str.IndexOf(L"value=\"", pagerStartIndex, valueStartIndex);
		if (!IsFailed(res)) {
			valueStartIndex += 7;

			int quoteCloseIndex = 0;
			res = str.IndexOf(L"\"", valueStartIndex, quoteCloseIndex);
			if (!IsFailed(res)) {
				String page;
				str.SubString(valueStartIndex, quoteCloseIndex - valueStartIndex, page);
				Integer::Parse(page, currentPageNumber);
			}
		}
	}

	String data = L"";
	while(true) {
		res = str.IndexOf(L"<div class=\"quote\">", startIndex, startIndex);
		FAIL;
		startIndex += 19;

		res = str.IndexOf(L"</div>", startIndex, firstClosingDivIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"text\">", startIndex, quoteStartIndex);
		FAIL;
		quoteStartIndex += 18;

		res = str.IndexOf(L"</div>", quoteStartIndex, quoteEndIndex);
		FAIL;

		res = str.IndexOf(L"<div class=\"actions\">", startIndex, dataStartIndex);
		FAIL;
		dataStartIndex += 21;

		if (dataStartIndex >= firstClosingDivIndex) {
			//this is an ad
			continue;
		}

		res = str.IndexOf(L"</div>", dataStartIndex, dataEndIndex);
		FAIL;

		if (dataStartIndex >= quoteEndIndex || dataEndIndex >= quoteEndIndex) {
			continue;
		}

		str.SubString(dataStartIndex, dataEndIndex - dataStartIndex, data);
		if (data.IsEmpty()) {
			continue;
		}

		res = data.IndexOf(L"class=\"id\">", 0, dataIdStart);
		FAIL;
		dataIdStart += 11;

		res = data.IndexOf(L"</span>", dataIdStart, dataIdEnd);
		FAIL;

		String quoteNumber;
		data.SubString(dataIdStart, dataIdEnd - dataIdStart, quoteNumber);
		quoteNumber.Remove(0, 4);//remove '#AA-' symbols

		int number = -1;
		res = Integer::Parse(quoteNumber, number);
		if (IsFailed(res)) {
			AppLogException("Failed to parse quote's ID string [%S], left blank. Error: [%s]", quoteNumber.GetPointer(), GetErrorMessage(res));
		}

		res = data.IndexOf(L"class=\"date\">", 0, dataDateStart);
		FAIL;
		dataDateStart += 13;

		res = data.IndexOf(L"</span>", dataDateStart, dataDateEnd);
		FAIL;

		String date;
		data.SubString(dataDateStart, dataDateEnd - dataDateStart, date);

		String text;
		str.SubString(quoteStartIndex, quoteEndIndex - quoteStartIndex, text);

		text.Replace(L"&lt;", L"<");
		text.Replace(L"&gt;", L">");
		text.Replace(L"<br>", L"\n");
		text.Replace(L"<br />", L"\n");
		text.Replace(L"&quot;", L"\"");
		text.Replace(L"&amp;", L"&");

		String num;
		int ch;
		int repInd = 0;
		int stInd = 0;
		while (!IsFailed(text.IndexOf(L"&#", stInd, repInd))) {
			repInd += 2;
			text.IndexOf(L';', repInd, stInd);

			if (!IsFailed(text.SubString(repInd, stInd - repInd, num))) {
				if (!IsFailed(Integer::Parse(num, ch))) {
					text.Remove(repInd - 2, 3 + num.GetLength());
					text.Insert((mchar)ch, repInd - 2);
				}
			}
			stInd++;
		}

		Quote *pQuote = new Quote;
		pQuote->SetPage(PAGE_ABYSS_BEST_QUOTES);
		pQuote->SetSerialized(false);

		pQuote->SetPageNumber(currentPageNumber);
		pQuote->SetID(number);
		pQuote->SetRating(-2);
		pQuote->SetDateTime(date);
		pQuote->SetText(text);
		pQuote->SetRead(true);

		pQuotes->Add(pQuote);
	}
	return pQuotes;
}

#undef FAIL

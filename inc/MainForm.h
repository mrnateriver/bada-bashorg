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
#ifndef _MAINFORM_H_
#define _MAINFORM_H_

#include "QuoteForm.h"

class MainForm: public BaseForm, public IQuotesUpdateCallbackHandler, public IGroupedItemEventListener, public ITimerEventListener {
public:
	MainForm(void);
	virtual ~MainForm(void);

	virtual result Construct(void);

private:
	static const int ID_LIST_FORMAT_RATING = 501;
	static const int ID_LIST_FORMAT_DATE = 502;
	static const int ID_LIST_FORMAT_TEXT = 503;
	static const int ID_LIST_FORMAT_UNREAD_BADGE = 504;
	static const int ID_LIST_FORMAT_UNREAD_DATE = 505;
	static const int ID_LIST_FORMAT_LOAD_MORE_BRIGHT = 506;
	static const int ID_LIST_FORMAT_LOAD_MORE_DARK = 507;

	bool CheckControls(void) const;
	result LoadQuotes(bool update = true);
	result SwitchColorScheme(void);
	int GetListCount(void) const;

	virtual result Initialize(void);
	virtual result Terminate(void);

	virtual void OnTimerExpired(Timer &timer);

	result OnOptionKeyClicked(const Control &src);
	result OnOptionUpdateClicked(const Control &src);
	result OnOptionClearSavedClicked(const Control &src);
	result OnOptionScrollToTopClicked(const Control &src);
	result OnOptionMarkAllReadClicked(const Control &src);
	result OnOptionDarkSchemeClicked(const Control &src);
	result OnOptionBrightSchemeClicked(const Control &src);

	result OnTabRecentClicked(const Control &src);
	result OnTabRandomClicked(const Control &src);
	result OnTabBestClicked(const Control &src);
	result OnTabAbyssClicked(const Control &src);
	result OnTabAbyssTopClicked(const Control &src);
	result OnTabAbyssBestClicked(const Control &src);
	result OnTabSavedQuotesClicked(const Control &src);

	result SwitchToOtherQuote(int index);

	virtual void HandleUpdateCallback(BashPage updated_page, LinkedListT<Quote *> *quotesN, int back_index = -1, int gr_back_index = -1);

	virtual void OnItemStateChanged(const Control &source, int groupIndex, int itemIndex, int itemId, int elementId, ItemStatus status) { }
	virtual void OnItemStateChanged(const Control &source, int groupIndex, int itemIndex, int itemId, ItemStatus status);

	DEF_ACTION(ID_OPTION_KEY_CLICKED, 102);
	DEF_ACTION(ID_OPTION_UPDATE_CLICKED, 103);
	DEF_ACTION(ID_OPTION_CLEAR_SAVED_CLICKED, 104);
	DEF_ACTION(ID_OPTION_DARK_SCHEME_CLICKED, 105);
	DEF_ACTION(ID_OPTION_BRIGHT_SCHEME_CLICKED, 106);
	DEF_ACTION(ID_OPTION_MARK_ALL_READ_CLICKED, 107);
	DEF_ACTION(ID_OPTION_SCROLL_TO_TOP_CLICKED, 108);

	DEF_ACTION(ID_TAB_SAVED_QUOTES_CLICKED, 109);
	DEF_ACTION(ID_TAB_RECENT_CLICKED, 110);
	DEF_ACTION(ID_TAB_RANDOM_CLICKED, 111);
	DEF_ACTION(ID_TAB_BEST_CLICKED, 112);
	DEF_ACTION(ID_TAB_ABYSS_CLICKED, 113);
	DEF_ACTION(ID_TAB_ABYSS_BEST_CLICKED, 114);
	DEF_ACTION(ID_TAB_ABYSS_TOP_CLICKED, 115);

	DEF_ACTION(ID_SOFTKEY0_CLICKED, 117);
	DEF_ACTION(ID_SOFTKEY1_CLICKED, 118);

	GroupedList *__pQuotesList;
	Tab *__pTabPanel;
	OptionMenu *__pOptionMenu;

	CustomListItemFormat *__pQuotesListDarkItemFormat;
	CustomListItemFormat *__pQuotesListBrightItemFormat;
	CustomListItemFormat *__pLoadMoreItemFormat;
	BashPage __currentTab;

	CustomListItemFormat *__pCurrentItemFormat;
	ColorScheme __currentColorScheme;

	CachingQuotesManager *__pQuotesManager;
	QuoteForm *__pQuoteForm;

	Bitmap *__pUnreadBadgeWhite;
	Bitmap *__pUnreadBadgeBlack;

	int __lastLoadedPage;

	friend class QuoteForm;
};

#endif

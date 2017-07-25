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
#ifndef _QUOTEFORM_H_
#define _QUOTEFORM_H_

#include "BaseForm.h"
#include "CachingQuotesManager.h"
#include "FormManager.h"

class MainForm;

class QuoteForm: public BaseForm {
public:
	QuoteForm();
	virtual ~QuoteForm(void);

	virtual result Construct(void);

	result SetData(MainForm *cb, CachingQuotesManager *mgr, Quote *quote, ColorScheme color_sc, bool marked, bool saved_tab, int index, int list_len, int back_index, int gr_back_index);

private:
	bool CheckControls(void) const;
	int GetLinesForText(const String &text) const;
	result RefreshStarButton(void);
	result UpdateQuoteSavedStatus(void) const;

	virtual result Initialize(void);
	virtual result Terminate(void);

	result OnPrevQuoteClicked(const Control &src);
	result OnNextQuoteClicked(const Control &src);
	result OnBackToListClicked(const Control &src);
	result OnStarButtonClicked(const Control &src);
	result OnShareButtonClicked(const Control &src);
	result OnCopyButtonClicked(const Control &src);
	result OnSmsButtonClicked(const Control &src);
	result OnEmailButtonClicked(const Control &src);

	DEF_ACTION(ID_RATE_UP_CLICKED, 102);
	DEF_ACTION(ID_RATE_DOWN_CLICKED, 103);
	DEF_ACTION(ID_STAR_CLICKED, 104);
	DEF_ACTION(ID_PREV_QUOTE_CLICKED, 105);
	DEF_ACTION(ID_NEXT_QUOTE_CLICKED, 106);
	DEF_ACTION(ID_BACK_TO_LIST_CLICKED, 107);
	DEF_ACTION(ID_SHARE_CLICKED, 108);
	DEF_ACTION(ID_SMS_CLICKED, 110);
	DEF_ACTION(ID_EMAIL_CLICKED, 111);

	Label *__pQuoteRatingLabel;
	Label *__pQuoteDateLabel;
	Label *__pQuoteTextLabel;
	ScrollPanel *__pMainScrollPanel;
	Button *__pShareButton;
	Button *__pStarButton;
	Button *__pPrevQuoteButton;
	Button *__pNextQuoteButton;
	Button *__pBackToListButton;
	ContextMenu *__pConMenu;

	Quote *__pQuote;
	ColorScheme __colorScheme;
	int __quoteIndex;
	MainForm *__pQuoteSwitchCallback;
	bool __startedMarked;
	bool __marked;
	bool __savedTab;
	int __backIndex;
	int __grBackIndex;
	CachingQuotesManager *__pQuotesManager;
};

#endif

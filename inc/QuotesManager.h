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
#ifndef NOTESMANAGER_H_
#define NOTESMANAGER_H_

#include <FNet.h>
#include <FUi.h>
#include <FIo.h>

#include "Quote.h"

using namespace Osp::Base::Collection;
using namespace Osp::Net::Http;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::Base::Runtime;
using namespace Osp::Io;

class IQuotesUpdateCallbackHandler {
public:
	virtual void HandleUpdateCallback(BashPage updated_page, LinkedListT<Quote *> *quotesN, int back_index = -1, int gr_back_index = -1) = 0;
};

class QuotesManager: public IHttpTransactionEventListener, public ITimerEventListener {
public:
	QuotesManager(void);
	~QuotesManager(void);

	virtual result Construct(const String &path);

	String GetPath(void) const { return __dataPath; }

	virtual result AddQuote(Quote *val);
	virtual result AddQuotes(const ICollectionT<Quote *> &pQuotes);

	virtual result SerializeQuotes(const ICollectionT<Quote *> &pQuotes);

	virtual result UpdateQuote(Quote *val) const;

	virtual result RemoveAll(BashPage page = PAGE_ALL_QUOTES);

	virtual result RemoveQuote(Quote *val) const;
	virtual result RemoveQuote(int entry_id) const;

	virtual LinkedListT<Quote *> *GetQuotesN(BashPage page = PAGE_ALL_QUOTES) const;
	virtual result UpdateQuotes(IQuotesUpdateCallbackHandler *pCbHandler, BashPage page = PAGE_RECENT_QUOTES, int page_num = 0);

private:
	result UpdateDatabaseFromTwoToThree(Database *pDb) const;
	result UpdateDatabaseFromThreeToFour(Database *pDb) const;

	void OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen);
	void OnTransactionAborted(HttpSession &httpSession, HttpTransaction &httpTransaction, result r) { delete &httpTransaction; __pTransForRemoval = null; }
	void OnTransactionReadyToWrite(HttpSession &httpSession, HttpTransaction &httpTransaction, int recommendedChunkSize) { }
	void OnTransactionHeaderCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction, int headerLen, bool rs) { }
	void OnTransactionCompleted(HttpSession &httpSession, HttpTransaction &httpTransaction) { delete &httpTransaction; __pTransForRemoval = null; }
	void OnTransactionCertVerificationRequiredN(HttpSession &httpSession, HttpTransaction &httpTransaction, String *pCert) { }

	void OnTimerExpired(Timer& timer);

	result SubtractQuotesList(LinkedListT<Quote *> *pArg1) const;

	result Load(void);

	String __dataPath;
	int __lastEntryId;

	HttpSession *__pHttpSession;
	Popup *__pUpdateMBox;
	BashPage __lastUpdatingPage;
	bool __removeBeforeUpdating;
	IQuotesUpdateCallbackHandler *__pCbHandler;
	Timer *__pPingTimer;
	HttpTransaction *__pTransForRemoval;
};

#endif

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
#ifndef CACHINGNOTESMANAGER_H_
#define CACHINGNOTESMANAGER_H_

#include "QuotesManager.h"

using namespace Osp::Base::Runtime;

class CachingQuotesManager;

class SerializerThread: public ITimerEventListener, public Thread {
public:
	SerializerThread(void);

	result Construct(CachingQuotesManager *pS);

	static const long REQUEST_SERIALIZATION = 150;
private:
	bool OnStart(void);
	void OnStop(void);

	void OnTimerExpired(Timer& timer);
	virtual void OnUserEventReceivedN(RequestId requestId, IList *pArgs);

	Timer* __pTimer;
	CachingQuotesManager *__pSerializer;
};

class CachingQuotesManager: public QuotesManager {
public:
	CachingQuotesManager(void);
	virtual ~CachingQuotesManager(void);

	virtual result Construct(const String &path);

	result DisposeAllQuotes(void);
	void SerializeAllQuotes(void);

	virtual result AddQuote(Quote *val);
	virtual result AddQuotes(const ICollectionT<Quote *> &pQuotes);

	virtual result UpdateQuote(Quote *val);

	void MarkQuoteAsRead(Quote *val);
	result MarkAllQuotesAsRead(BashPage page = PAGE_ALL_QUOTES);

	virtual result RemoveQuote(Quote *val);
	virtual result RemoveQuote(int entry_id);

	result RemoveQuoteByID(int id, BashPage page);

	virtual result RemoveAll(BashPage page = PAGE_ALL_QUOTES);

	virtual LinkedListT<Quote *> *GetQuotesN(BashPage page = PAGE_RECENT_QUOTES) const;

	Quote *GetQuote(int index) const;
	Quote *GetQuoteByPage(int index, BashPage page) const;
	Quote *GetQuoteByID(int id, BashPage page) const;

	bool IsQuoteSavedByID(int id) const;

private:
	LinkedListT<Quote *> *__pQuotes;
	SerializerThread *__pSerializer;
	bool __smtChanged;

	friend class SerializerThread;
};

#endif

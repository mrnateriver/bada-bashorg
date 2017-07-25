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
#include <FApp.h>
#include <FGraphics.h>
#include <FSystem.h>
#include "zlib.h"
#include "zconf.h"

#include "QuotesManager.h"
#include "BashParser.h"

using namespace Osp::App;
using namespace Osp::Graphics;
using namespace Osp::System;
using namespace Osp::Base::Utility;

#define DB_VERSION 4

QuotesManager::QuotesManager(void) {
	__dataPath = L"";
	__lastEntryId = 0;

	__pHttpSession = null;
	__lastUpdatingPage = PAGE_SAVED_QUOTES;
	__removeBeforeUpdating = true;
	__pUpdateMBox = null;
	__pCbHandler = null;
	__pPingTimer = null;
	__pTransForRemoval = null;
}

QuotesManager::~QuotesManager(void) {
	if (__pHttpSession) delete __pHttpSession;
	if (__pUpdateMBox) delete __pUpdateMBox;
	if (__pPingTimer) delete __pPingTimer;
	if (__pTransForRemoval) delete __pTransForRemoval;
}

result QuotesManager::Construct(const String &path) {
	__dataPath = path;
	return Load();
}

result QuotesManager::UpdateDatabaseFromTwoToThree(Database *pDb) const {
	if (!pDb) {
		return E_INVALID_ARG;
	}
	result res = pDb->ExecuteSql(L"ALTER TABLE entries ADD read INTEGER", true);
	if (IsFailed(res)) {
		return res;
	}
	res = pDb->ExecuteSql(L"UPDATE entries SET read = 1", true);
	if (IsFailed(res)) {
		return res;
	}
	res = pDb->ExecuteSql(L"UPDATE db_info SET ver = 3", true);
	return res;
}

result QuotesManager::UpdateDatabaseFromThreeToFour(Database *pDb) const {
	if (!pDb) {
		return E_INVALID_ARG;
	}
	result res = pDb->ExecuteSql(L"ALTER TABLE entries ADD page_num INTEGER", true);
	if (IsFailed(res)) {
		return res;
	}
	res = pDb->ExecuteSql(L"UPDATE entries SET page_num = 0", true);
	if (IsFailed(res)) {
		return res;
	}
	res = pDb->ExecuteSql(L"UPDATE db_info SET ver = 4", true);
	return res;
}

result QuotesManager::Load(void) {
	if (File::IsFileExist(__dataPath)) {
		Database *pDb = new Database;
		result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
		if (IsFailed(res)) {
			AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		DbEnumerator *pEnum = pDb->QueryN(L"SELECT ver FROM db_info"); res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to query database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}
		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				int ver;
				res = pEnum->GetIntAt(0, ver);
				if (IsFailed(res)) {
					AppLogException("Failed to get version data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

					delete pEnum;
					delete pDb;
					return res;
				}

				if (ver == 2) {
					UpdateDatabaseFromTwoToThree(pDb);
					UpdateDatabaseFromThreeToFour(pDb);
					break;
				} else if (ver == 3) {
					UpdateDatabaseFromThreeToFour(pDb);
					break;
				} else if (ver != DB_VERSION) {
					delete pEnum;
					delete pDb;

					return E_INVALID_FORMAT;
				}
			}
		} else {
			delete pDb;
			return E_INVALID_FORMAT;
		}
		delete pEnum;

		pEnum = pDb->QueryN(L"SELECT MAX(entry_id) FROM entries"); res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to query database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		if (pEnum) {
			while (!IsFailed(pEnum->MoveNext())) {
				if (pEnum->GetColumnType(0) == DB_COLUMNTYPE_NULL) {
					break;
				}

				int maxID;
				res = pEnum->GetIntAt(0, maxID);
				if (IsFailed(res)) {
					AppLogException("Failed to get last entry ID for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

					delete pEnum;
					delete pDb;
					return res;
				}
				__lastEntryId = ++maxID;
				break;
			}
		} else {
			delete pDb;
			return E_INVALID_FORMAT;
		}
		delete pEnum;
		delete pDb;
	} else {
		Database *pDb = new Database;
		result res = pDb->Construct(__dataPath, true);//DB_OPEN_READ_WRITE, 0);
		if (IsFailed(res)) {
			AppLogException("Failed to initialize database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pDb;
			return res;
		}

		result mres[3];
		mres[0] = pDb->ExecuteSql(L"CREATE TABLE db_info (ver INTEGER)", true);
		mres[1] = pDb->ExecuteSql(L"INSERT INTO db_info (ver) VALUES (" + Integer::ToString(DB_VERSION) + L")", true);

		mres[2] = pDb->ExecuteSql(L"CREATE TABLE entries (entry_id INTEGER, page_id INTEGER, quote_id INTEGER, rating INTEGER, datetime TEXT, text TEXT, read INTEGER, page_num INTEGER)", true);

		delete pDb;

		for(int i = 0; i < 3; i++) {
			if (IsFailed(mres[i])) {
				AppLogException("Failed to initialize database structure at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(mres[i]));

				return mres[i];
			}
		}
	}
	return E_SUCCESS;
}

result QuotesManager::AddQuote(Quote *val) {
	if (val->GetEntryId() >= 0) {
		return UpdateQuote(val);
	}

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO entries (entry_id, page_id, quote_id, rating, datetime, text, read, page_num) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	int cur_id = __lastEntryId + 1;

	result bind_res[8];
	bind_res[0] = pEntries->BindInt(0, cur_id);
	bind_res[1] = pEntries->BindInt(1, (int)val->GetPage());
	bind_res[2] = pEntries->BindInt(2, val->GetID());
	bind_res[3] = pEntries->BindInt(3, val->GetRating());
	bind_res[4] = pEntries->BindString(4, val->GetDateTime());
	bind_res[5] = pEntries->BindString(5, val->GetText());
	bind_res[6] = pEntries->BindInt(6, (int)val->GetRead());
	bind_res[7] = pEntries->BindInt(7, val->GetPageNumber());

	for(int i = 0; i < 8; i++) {
		if (IsFailed(bind_res[i])) {
			AppLogException("Failed to bind transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

			delete pEntries;
			delete pDb;
			return bind_res[i];
		}
	}

	pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
	delete pEntries;

	if (IsFailed(res)) {
		AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}

	val->SetEntryID(cur_id);
	val->SetSerialized(true);

	__lastEntryId++;
	return E_SUCCESS;
}

result QuotesManager::AddQuotes(const ICollectionT<Quote *> &pQuotes) {
	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO entries (entry_id, page_id, quote_id, rating, datetime, text, read, page_num) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	int cur_id = __lastEntryId;

	IEnumeratorT<Quote *> *pEnum = pQuotes.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Quote *pQuote; pEnum->GetCurrent(pQuote);

		result bind_res[8];
		bind_res[0] = pEntries->BindInt(0, cur_id++);
		bind_res[1] = pEntries->BindInt(1, (int)pQuote->GetPage());
		bind_res[2] = pEntries->BindInt(2, pQuote->GetID());
		bind_res[3] = pEntries->BindInt(3, pQuote->GetRating());
		bind_res[4] = pEntries->BindString(4, pQuote->GetDateTime());
		bind_res[5] = pEntries->BindString(5, pQuote->GetText());
		bind_res[6] = pEntries->BindInt(6, (int)pQuote->GetRead());
		bind_res[7] = pEntries->BindInt(7, pQuote->GetPageNumber());

		for(int i = 0; i < 8; i++) {
			if (IsFailed(bind_res[i])) {
				AppLogException("Failed to bind transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

				delete pEntries;
				delete pDb;
				delete pEnum;
				return bind_res[i];
			}
		}

		pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pEntries;
			delete pDb;
			delete pEnum;
			return res;
		}

		pQuote->SetEntryID(cur_id);
		pQuote->SetSerialized(true);
	}
	delete pEnum;
	delete pEntries;

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}

	__lastEntryId = cur_id;
	return E_SUCCESS;
}

result QuotesManager::SerializeQuotes(const ICollectionT<Quote *> &pQuotes) {
	result res = QuotesManager::RemoveAll();
	if (IsFailed(res)) {
		AppLogException("Failed to remove all quotes before serialization for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));
		return res;
	}

	Database *pDb = new Database;
	res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"INSERT INTO entries (entry_id, page_id, quote_id, rating, datetime, text, read, page_num) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	int cur_id = 0;

	IEnumeratorT<Quote *> *pEnum = pQuotes.GetEnumeratorN();
	while (!IsFailed(pEnum->MoveNext())) {
		Quote *pQuote; pEnum->GetCurrent(pQuote);

		result bind_res[8];
		bind_res[0] = pEntries->BindInt(0, cur_id++);
		bind_res[1] = pEntries->BindInt(1, (int)pQuote->GetPage());
		bind_res[2] = pEntries->BindInt(2, pQuote->GetID());
		bind_res[3] = pEntries->BindInt(3, pQuote->GetRating());
		bind_res[4] = pEntries->BindString(4, pQuote->GetDateTime());
		bind_res[5] = pEntries->BindString(5, pQuote->GetText());
		bind_res[6] = pEntries->BindInt(6, (int)pQuote->GetRead());
		bind_res[7] = pEntries->BindInt(7, pQuote->GetPageNumber());

		for(int i = 0; i < 8; i++) {
			if (IsFailed(bind_res[i])) {
				AppLogException("Failed to bind transaction data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

				delete pEntries;
				delete pDb;
				delete pEnum;
				return bind_res[i];
			}
		}

		pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

			delete pEntries;
			delete pDb;
			delete pEnum;
			return res;
		}

		pQuote->SetEntryID(cur_id);
		pQuote->SetSerialized(true);
	}
	delete pEnum;
	delete pEntries;

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}

	__lastEntryId = cur_id;
	return E_SUCCESS;
}

result QuotesManager::UpdateQuote(Quote *val) const {
	if (val->GetEntryId() < 0) {
		AppLogException("Attempt to update quote that is not yet saved to database");
		return E_INVALID_ARG;
	} else if (val->GetSerialized()) {
		AppLogException("Attempt to update quote that is already serialized");
		return E_INVALID_ARG;
	}

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result trans_res[2];
	trans_res[0] = pDb->BeginTransaction();

	DbStatement *pEntries = pDb->CreateStatementN(L"UPDATE entries SET quote_id = ?, page_id = ?, page_num = ?, rating = ?, datetime = ?, text = ?, read = ? WHERE entry_id = ?");

	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to initialize transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	result bind_res[8];
	bind_res[0] = pEntries->BindInt(0, val->GetID());
	bind_res[1] = pEntries->BindInt(1, (int)val->GetPage());
	bind_res[2] = pEntries->BindInt(2, val->GetPageNumber());
	bind_res[3] = pEntries->BindInt(3, val->GetRating());
	bind_res[4] = pEntries->BindString(4, val->GetDateTime());
	bind_res[5] = pEntries->BindString(5, val->GetText());
	bind_res[6] = pEntries->BindInt(6, (int)val->GetRead());
	bind_res[7] = pEntries->BindInt(7, val->GetEntryId());

	for(int i = 0; i < 8; i++) {
		if (IsFailed(bind_res[i])) {
			AppLogException("Failed to bind transaction data with index [%d] for database at [%S], error: [%s]", i, __dataPath.GetPointer(), GetErrorMessage(bind_res[i]));

			delete pEntries;
			delete pDb;
			return bind_res[i];
		}
	}

	pDb->ExecuteStatementN(*pEntries); res = GetLastResult();
	delete pEntries;

	if (IsFailed(res)) {
		AppLogException("Failed to execute transaction statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	trans_res[1] = pDb->CommitTransaction();
	delete pDb;

	for(int i = 0; i < 2; i++) {
		if (IsFailed(trans_res[i])) {
			AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(trans_res[i]));

			return trans_res[i];
		}
	}

	val->SetSerialized(true);

	return E_SUCCESS;
}

result QuotesManager::RemoveAll(BashPage page) {
	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	res = pDb->ExecuteSql(L"DELETE FROM entries" + (page == PAGE_ALL_QUOTES ? String(L"") : String(L" WHERE page_id = ") + Integer::ToString((int)page)), false);

	if (IsFailed(res)) {
		AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));
	}

	delete pDb;
	return res;
}

result QuotesManager::RemoveQuote(Quote *val) const {
	return RemoveQuote(val->GetEntryId());
}

result QuotesManager::RemoveQuote(int entry_id) const {
	if (entry_id < 0)
		return E_INVALID_ARG;

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pDb;
		return res;
	}

	res = pDb->ExecuteSql(L"DELETE FROM entries WHERE entry_id=" + Integer::ToString(entry_id), false);

	if (IsFailed(res)) {
		AppLogException("Failed to commit transaction for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));
	}

	delete pDb;
	return E_SUCCESS;
}

LinkedListT<Quote *> *QuotesManager::GetQuotesN(BashPage page) const {
	LinkedListT<Quote *> *pQuotes = new LinkedListT<Quote *>;

	Database *pDb = new Database;
	result res = pDb->Construct(__dataPath, false);//DB_OPEN_READ_WRITE, 0);
	if (IsFailed(res)) {
		AppLogException("Failed to load database file at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pQuotes;
		delete pDb;
		SetLastResult(res);

		return null;
	}

	DbEnumerator *pEnum = pDb->QueryN(L"SELECT * FROM entries" + (page == PAGE_ALL_QUOTES ? String(L"") : String(L" WHERE page_id = ") + Integer::ToString((int)page)));
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to execute query statement for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(res));

		delete pQuotes;
		delete pDb;
		SetLastResult(res);

		return null;
	}

	if (pEnum) {
		while (!IsFailed(pEnum->MoveNext())) {
			int entry_id = -1;
			int page_id = 0;
			int page_num;
			int quote_id = -1;
			int rating = -1;
			String datetime;
			String text;
			int read = 0;

			result gres[8];
			gres[0] = pEnum->GetIntAt(0, entry_id);
			gres[1] = pEnum->GetIntAt(1, page_id);
			gres[2] = pEnum->GetIntAt(2, quote_id);
			gres[3] = pEnum->GetIntAt(3, rating);
			gres[4] = pEnum->GetStringAt(4, datetime);
			gres[5] = pEnum->GetStringAt(5, text);
			gres[6] = pEnum->GetIntAt(6, read);
			gres[7] = pEnum->GetIntAt(7, page_num);

			for(int i = 0; i < 8; i++) {
				if (IsFailed(gres[i])) {
					AppLogException("Failed to retrieve query data for database at [%S], error: [%s]", __dataPath.GetPointer(), GetErrorMessage(gres[i]));

					delete pEnum;
					delete pQuotes;
					delete pDb;
					SetLastResult(gres[i]);

					return null;
				}
			}

			Quote *pQuote = new Quote;

			pQuote->SetEntryID(entry_id);
			pQuote->SetSerialized(true);
			pQuote->SetPage((BashPage)page_id);
			pQuote->SetPageNumber(page_num);

			pQuote->SetID(quote_id);
			pQuote->SetRating(rating);
			pQuote->SetDateTime(datetime);
			pQuote->SetText(text);
			pQuote->SetRead((bool)read);

			res = pQuotes->Add(pQuote);
			if (IsFailed(res)) {
				delete pQuote;
				delete pEnum;
				delete pQuotes;
				delete pDb;
				SetLastResult(res);

				return null;
			}
		}
		delete pEnum;
	}

	delete pDb;
	return pQuotes;
}

result QuotesManager::UpdateQuotes(IQuotesUpdateCallbackHandler *pCbHandler, BashPage page, int page_num) {
	if (page == PAGE_SAVED_QUOTES || page == PAGE_ALL_QUOTES) {
		return E_INVALID_ARG;
	}
	if (__pCbHandler || __pHttpSession) {
		AppLogException("Attempt to update quotes while another request is not yet processed");
		return E_INVALID_STATE;
	}

	String str = L"QUOTESMANAGER_UPDATE_MBOX_TEXT";
	if (page_num > 0) {
		str = L"QUOTESMANAGER_UPDATE_MBOX_LOAD_TEXT";
	}

	result res = Application::GetInstance()->GetAppResource()->GetString(str, str);
	if (IsFailed(res)) {
		AppLogException("Could not retrieve string by id [%S]", str.GetPointer());
	}

	if (!__pUpdateMBox) {
		__pUpdateMBox = new Popup();
		__pUpdateMBox->Construct(false, Dimension(388,130));

		Label *pLabel = new Label;
		pLabel->SetName(L"TEXT_LABEL");
		pLabel->Construct(Rectangle(54,15,280,70), str);
		pLabel->SetTextConfig(pLabel->GetTextSize() - 3, LABEL_TEXT_STYLE_NORMAL);
		pLabel->SetTextHorizontalAlignment(ALIGNMENT_CENTER);
		pLabel->SetTextVerticalAlignment(ALIGNMENT_MIDDLE);
		__pUpdateMBox->AddControl(*pLabel);
	} else {
		Label *pLabel = static_cast<Label *>(__pUpdateMBox->GetControl(L"TEXT_LABEL", true));
		pLabel->SetText(str);
	}
	__pUpdateMBox->SetShowState(true);
	__pUpdateMBox->Show();

	DateTime curDate;
	SystemTime::GetCurrentTime(WALL_TIME, curDate);

	String hostUri = L"bash.im";
	if (page_num > 0) {
		String urls[] = {
			String(L"/index/") + Integer::ToString(page_num),
			String(L"/random"),
			String(L"/bestmonth/") + Integer::ToString(curDate.GetYear()) + String(L"/") + Integer::ToString(curDate.GetMonth()) + String(L"/"),
			String(L"/abyss"),
			String(L"/abyssbest/") + Integer::ToString(page_num),
			String(L"/abysstop")
		};
		hostUri.Append(urls[(int)page - 2]);
	} else {
		String urls[] = {
			String(L"/random"),
			String(L"/bestmonth/") + Integer::ToString(curDate.GetYear()) + String(L"/") + Integer::ToString(curDate.GetMonth()) + String(L"/"),
			String(L"/abyss"),
			String(L"/abyssbest"),
			String(L"/abysstop")
		};
		if (page > PAGE_RECENT_QUOTES) {
			hostUri.Append(urls[(int)page - 3]);
		}
	}

	HttpTransaction *pTransaction = null;
	HttpRequest *pRequest = null;

	__pHttpSession = new HttpSession();
	res = __pHttpSession->Construct(NET_HTTP_SESSION_MODE_NORMAL, null, L"http://bash.im", null);
	if (IsFailed(res)) {
		AppLogException("Failed to initialize HTTP session, error: [%s]", GetErrorMessage(res));
		delete __pHttpSession;
		__pHttpSession = null;
		goto ERROR;
	}

	pTransaction = __pHttpSession->OpenTransactionN();
	res = GetLastResult();
	if (IsFailed(res) || !pTransaction) {
		AppLogException("Failed to open transaction for HTTP session, error: [%s]", GetErrorMessage(res));
		delete pTransaction;
		goto ERROR;
	}
	pTransaction->AddHttpTransactionListener(*this);

	pRequest = pTransaction->GetRequest();
	res = GetLastResult();
	if (IsFailed(res) || !pRequest) {
		AppLogException("Failed to initialize HTTP request, error: [%s]", GetErrorMessage(res));

		delete __pHttpSession;
		__pHttpSession = null;
		delete pTransaction;

		goto ERROR;
	}

	pRequest->SetUri(hostUri);
	pRequest->SetMethod(NET_HTTP_METHOD_GET);
	pRequest->GetHeader()->AddField("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 6.1; Trident/4.0)");

	res = pTransaction->Submit();
	if (IsFailed(res)) {
		AppLogException("Failed to submit HTTP request, error: [%s]", GetErrorMessage(res));

		delete __pHttpSession;
		__pHttpSession = null;
		delete pTransaction;

		goto ERROR;
	}

	if (!__pPingTimer) {
		__pPingTimer = new Timer;
		__pPingTimer->Construct(*this/*, true*/);
	}
	__pPingTimer->Start(30000);

	__lastUpdatingPage = page;
	__pCbHandler = pCbHandler;
	__pTransForRemoval = pTransaction;
	if (page_num > 0) {
		__removeBeforeUpdating = false;
	} else {
		__removeBeforeUpdating = true;
	}
	return E_SUCCESS;

	ERROR:
	__pUpdateMBox->SetShowState(false);

	MessageBox *pBox = new MessageBox;

	String title;
	String msg;
	result sres = Application::GetInstance()->GetAppResource()->GetString(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TITLE", title);
	if (IsFailed(sres)) {
		AppLogException("Could not retrieve string by id [%S]", String(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TITLE").GetPointer());
	}
	sres = Application::GetInstance()->GetAppResource()->GetString(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TEXT", msg);
	if (IsFailed(sres)) {
		AppLogException("Could not retrieve string by id [%S]", String(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TEXT").GetPointer());
	}

	sres = pBox->Construct(title, msg, MSGBOX_STYLE_OK);
	if (!IsFailed(sres)) {
		int mres;
		pBox->ShowAndWait(mres);
	} else {
		AppLogException("Failed to construct error message box, error: [%s]", GetErrorMessage(sres));
	}
	delete pBox;

	return res;
}

void QuotesManager::OnTimerExpired(Timer& timer) {
	if (__pHttpSession) {
		delete __pHttpSession;
		__pHttpSession = null;
		__pCbHandler = null;

		if (__pTransForRemoval) {
			delete __pTransForRemoval;
			__pTransForRemoval = null;
		}

		__pUpdateMBox->SetShowState(false);

		MessageBox *pBox = new MessageBox;

		String title;
		String msg;
		result sres = Application::GetInstance()->GetAppResource()->GetString(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TITLE", title);
		if (IsFailed(sres)) {
			AppLogException("Could not retrieve string by id [%S]", String(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TITLE").GetPointer());
		}
		sres = Application::GetInstance()->GetAppResource()->GetString(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TEXT", msg);
		if (IsFailed(sres)) {
			AppLogException("Could not retrieve string by id [%S]", String(L"QUOTESMANAGER_CONNECTION_ERROR_MBOX_TEXT").GetPointer());
		}

		sres = pBox->Construct(title, msg, MSGBOX_STYLE_OK);
		if (!IsFailed(sres)) {
			int mres;
			pBox->ShowAndWait(mres);
		} else {
			AppLogException("Failed to construct error message box, error: [%s]", GetErrorMessage(sres));
		}

		delete pBox;
	}
}

result QuotesManager::SubtractQuotesList(LinkedListT<Quote *> *pArg1) const {
	IEnumeratorT<Quote *> *pEnum1 = pArg1->GetEnumeratorN();
	result res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire enumerator for the first argument, error: [%s]", GetErrorMessage(res));
		return res;
	}

	LinkedListT<Quote *> *pCurQuotes = GetQuotesN(__lastUpdatingPage);
	IEnumeratorT<Quote *> *pEnum2 = pCurQuotes->GetEnumeratorN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to acquire enumerator for the second argument, error: [%s]", GetErrorMessage(res));

		delete pEnum1;
		delete pCurQuotes;
		return res;
	}

	while (!IsFailed(pEnum1->MoveNext())) {
		Quote *pQuote; pEnum1->GetCurrent(pQuote);

		bool found = false;
		while (!IsFailed(pEnum2->MoveNext())) {
			Quote *pResQuote; pEnum2->GetCurrent(pResQuote);
			if (*pQuote == *pResQuote) {
				if (pResQuote->GetRead()) {
					found = true;
				}
				break;
			}
		}
		if (!found) {
			pQuote->SetRead(false);
		}
		pEnum2->Reset();
	}

	delete pEnum1;
	delete pEnum2;
	delete pCurQuotes;

	return E_SUCCESS;
}

void QuotesManager::OnTransactionReadyToRead(HttpSession &httpSession, HttpTransaction &httpTransaction, int availableBodyLen) {
	if (__pUpdateMBox) {
		__pUpdateMBox->SetShowState(false);
	}
	if (__pPingTimer) {
		__pPingTimer->Cancel();
	}

	HttpResponse* pHttpResponse = httpTransaction.GetResponse();
	if(!pHttpResponse) {
		return;
	}
	if (pHttpResponse->GetStatusCode() == NET_HTTP_STATUS_OK) {
		HttpHeader* pHttpHeader = pHttpResponse->GetHeader();
		if(pHttpHeader) {
			ByteBuffer* pBuffer = pHttpResponse->ReadBodyN();
			if (pBuffer) {
				String encoding;
				int enc_len = 0;
				IList *pFields = pHttpHeader->GetFieldNamesN();
				IEnumerator *pEnum = pFields->GetEnumeratorN();
				while (!IsFailed(pEnum->MoveNext())) {
					String *field = static_cast<String *>(pEnum->GetCurrent());
					if (field->Equals(L"Content-Encoding", false)) {
						IEnumerator *pVals = pHttpHeader->GetFieldValuesN(*field);
						if (!IsFailed(pVals->MoveNext())) {
							encoding = *static_cast<String *>(pVals->GetCurrent());
						}
						delete pVals;
					} else if (field->Equals(L"Content-Length", false)) {
						IEnumerator *pVals = pHttpHeader->GetFieldValuesN(*field);
						if (!IsFailed(pVals->MoveNext())) {
							Integer::Parse(*static_cast<String *>(pVals->GetCurrent()), enc_len);
						}
						delete pVals;
					}
				}
				delete pEnum;
				delete pFields;

				pBuffer->SetPosition(0);
				if (encoding.Equals(L"gzip", false) && enc_len > 0) {
					//inflate data
					unsigned full_length = pBuffer->GetLimit();
					unsigned half_length = pBuffer->GetLimit() / 2;

					unsigned uncompLength = full_length;
					char* uncomp = new char[uncompLength];

					z_stream strm;
					strm.next_in = (Bytef*)pBuffer->GetPointer();
					strm.avail_in = pBuffer->GetLimit();
					strm.total_out = 0;
					strm.zalloc = Z_NULL;
					strm.zfree = Z_NULL;

					bool done = false ;

					if (inflateInit2(&strm, (16+MAX_WBITS)) != Z_OK) {
						delete []uncomp;
						AppLogException("Failed to inflate HTTP response body, error: [%s]", GetErrorMessage(GetLastResult()));
					} else {
						while (!done) {
							// If our output buffer is too small
							if (strm.total_out >= uncompLength ) {
								// Increase size of output buffer
								char* uncomp2 = new char[uncompLength + half_length];
								memcpy(uncomp2, uncomp, uncompLength);
								uncompLength += half_length ;
								delete []uncomp;
								uncomp = uncomp2;
							}
							strm.next_out = (Bytef*)(uncomp + strm.total_out);
							strm.avail_out = uncompLength - strm.total_out;
							// Inflate another chunk.
							int err = inflate(&strm, Z_SYNC_FLUSH);
							if (err == Z_STREAM_END) {
								done = true;
							} else if (err != Z_OK)  {
								break;
							}
						}
						if (inflateEnd (&strm) != Z_OK) {
							delete []uncomp;

							AppLogException("Failed to inflate HTTP response body, error: [%s]", GetErrorMessage(GetLastResult()));
						} else {
							ByteBuffer *inf = new ByteBuffer;
							inf->Construct(strm.total_out);
							inf->SetArray((byte*)uncomp, 0, strm.total_out);
							inf->SetPosition(0);

							delete []uncomp;
							delete pBuffer;
							pBuffer = inf;
						}
					}
				}

				const wchar_t *pConv = ConvertCP1251ToUCS2(pBuffer->GetPointer(), pBuffer->GetCapacity());
				String html = String(pConv);
				if (__pCbHandler) {
					LinkedListT<Quote *> *pQuotes = ParseBashPageN(html, __lastUpdatingPage);
					result res = GetLastResult();
					if (IsFailed(res) || !pQuotes) {
						AppLogException("Failed to parse HTTP response body, error: [%s]", GetErrorMessage(res));

						delete[] pConv;
						delete pBuffer;

						__pCbHandler->HandleUpdateCallback(__lastUpdatingPage, pQuotes);
						__pCbHandler = null;

						if (__pHttpSession) {
							delete __pHttpSession;
							__pHttpSession = null;
						}
						__pTransForRemoval = null;
						return;
					}

					SubtractQuotesList(pQuotes);
					if (__removeBeforeUpdating) {
						RemoveAll(__lastUpdatingPage);
					}
					AddQuotes(*pQuotes);

					__pCbHandler->HandleUpdateCallback(__lastUpdatingPage, pQuotes);
					__pCbHandler = null;
				}
				delete[] pConv;
				delete pBuffer;
			} else {
				AppLogException("Failed to get proper HTTP response buffer");
			}
		} else {
			AppLogException("Failed to get proper HTTP response header");
		}
	} else {
		AppLogException("Failed to get proper HTTP response, status code: [%d]", (int)pHttpResponse->GetStatusCode());
		__pCbHandler = null;
	}
	if (__pHttpSession) {
		delete __pHttpSession;
		__pHttpSession = null;
	}
	__pTransForRemoval = null;
}

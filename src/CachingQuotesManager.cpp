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
#include <FSystem.h>

#include "CachingQuotesManager.h"

using namespace Osp::System;

SerializerThread::SerializerThread(void) {
	__pTimer = null;
}

result SerializerThread::Construct(CachingQuotesManager *pS) {
	__pSerializer = pS;
	return Thread::Construct(THREAD_TYPE_EVENT_DRIVEN);
}

bool SerializerThread::OnStart(void) {
   /*__pTimer = new Timer;
   result res = __pTimer->Construct(*this);
   if (IsFailed(res)) {
	   AppLogException("Failed to construct timer object in serializer thread, error: [%s]", GetErrorMessage(res));

	   SetLastResult(res);
	   return false;
   }

   res = __pTimer->Start(60*1000);
   if (IsFailed(res)) {
	   AppLogException("Failed to start timer in serializer thread, error: [%s]", GetErrorMessage(res));

	   SetLastResult(res);
	   return false;
   } else return true;*/
	return true;
}

void SerializerThread::OnStop(void) {
   /*result res = __pTimer->Cancel();
   if (IsFailed(res)) {
	   AppLogException("Failed to cease timer in serializer thread, error: [%s]", GetErrorMessage(res));
	   SetLastResult(res);
   }
   delete __pTimer;
   __pTimer = null;*/
}

void SerializerThread::OnTimerExpired(Timer& timer) {
	if (__pSerializer->__smtChanged) {
		__pSerializer->SerializeQuotes(*__pSerializer->__pQuotes);
		__pSerializer->__smtChanged = false;
	}

	result res = __pTimer->Start(60*1000);
	if (IsFailed(res)) {
		AppLogException("Failed to restart timer in serializer thread, error: [%s]", GetErrorMessage(res));
		SetLastResult(res);
	}
}

void SerializerThread::OnUserEventReceivedN(RequestId requestId, IList *pArgs) {
	if (requestId == REQUEST_SERIALIZATION) {
		///__pTimer->Cancel();
		//OnTimerExpired(*__pTimer);

		__pSerializer->SerializeQuotes(*__pSerializer->__pQuotes);
		__pSerializer->__smtChanged = false;
	}
}

CachingQuotesManager::CachingQuotesManager(void) {
	__pQuotes = null;
	__pSerializer = null;
	__smtChanged = false;
}

CachingQuotesManager::~CachingQuotesManager(void) {
	if (__pSerializer) {
		__pSerializer->Stop();
		//delete __pSerializer;//crashes app for some reason
	}
	if (__pQuotes) {
		DisposeAllQuotes();
		delete __pQuotes;
	}
}

result CachingQuotesManager::Construct(const String &path) {
    __pSerializer = new SerializerThread;
    result res = __pSerializer->Construct(this);
	if (IsFailed(res)) {
		AppLogException("Failed to construct serialization thread, error: [%s]", GetErrorMessage(res));
		return res;
	}

	res = QuotesManager::Construct(path);
	if (IsFailed(res)) {
		AppLogException("Failed to construct underlying quotes manager for serialization, error: [%s]", GetErrorMessage(res));
		return res;
	}

	__pQuotes = QuotesManager::GetQuotesN();
	res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to precache quotes, error: [%s]", GetErrorMessage(res));
		return res;
	}

	__pSerializer->Start();

	return E_SUCCESS;
}

result CachingQuotesManager::DisposeAllQuotes(void) {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return E_INVALID_CONDITION;
		}
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Quote *pQuote; pEnum->GetCurrent(pQuote);
				delete pQuote;
			}
		}
		delete pEnum;
		__pQuotes->RemoveAll();

		return E_SUCCESS;
	} else {
		AppLogException("Attempt to dispose quotes in the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

void CachingQuotesManager::SerializeAllQuotes(void) {
	if (__smtChanged) {
		SerializeQuotes(*__pQuotes);
		__smtChanged = false;
	}
}

result CachingQuotesManager::AddQuote(Quote *val) {
	if (__pQuotes) {
		__smtChanged = true;
		result res = __pQuotes->Add(val);

		if (!IsFailed(res)) __pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return res;
	} else {
		AppLogException("Attempt to add quote to the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingQuotesManager::AddQuotes(const ICollectionT<Quote *> &pQuotes) {
	if (__pQuotes) {
		__smtChanged = true;
		result res = __pQuotes->AddItems(pQuotes);

		if (!IsFailed(res)) __pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return res;
	} else {
		AppLogException("Attempt to add quotes to the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingQuotesManager::UpdateQuote(Quote *val) {
	if (__pQuotes) {
		__smtChanged = true;
		__pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return E_SUCCESS;
	} else {
		AppLogException("Attempt to update quote in the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

void CachingQuotesManager::MarkQuoteAsRead(Quote *val) {
	val->SetRead(true);
	__smtChanged = true;
}

result CachingQuotesManager::MarkAllQuotesAsRead(BashPage page) {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return E_INVALID_CONDITION;
		}
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Quote *pQuote; pEnum->GetCurrent(pQuote);
				if (pQuote->GetPage() == page || page == PAGE_ALL_QUOTES) {
					pQuote->SetRead(true);
				}
			}
		}
		delete pEnum;
		__smtChanged = true;

		if (!IsFailed(res)) {
			__pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		} else {
			AppLogException("Failed to mark all quotes as read, error: [%s]", GetErrorMessage(res));
		}
		return res;
	} else {
		AppLogException("Attempt to mark all quotes read in the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingQuotesManager::RemoveQuote(Quote *val) {
	if (__pQuotes) {
		__smtChanged = true;
		result res = __pQuotes->Remove(val);

		if (!IsFailed(res)) {
			__pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
			delete val;
		} else {
			AppLogException("Failed to remove quote from the list, error: [%s]", GetErrorMessage(res));
		}
		return res;
	} else {
		AppLogException("Attempt to remove quote from the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingQuotesManager::RemoveQuote(int entry_id) {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return E_INVALID_CONDITION;
		}
		Quote *pToRemove = null;
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Quote *pQuote; pEnum->GetCurrent(pQuote);
				if (pQuote->GetEntryId() == entry_id) {
					pToRemove = pQuote;
					break;
				}
			}
		}
		delete pEnum;

		if (pToRemove) {
			__smtChanged = true;
			res = __pQuotes->Remove(pToRemove);

			if (!IsFailed(res)) {
				__pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
				delete pToRemove;
			} else {
				AppLogException("Failed to remove quote from the list, error: [%s]", GetErrorMessage(res));
			}
			return res;
		} else {
			return E_OBJ_NOT_FOUND;
		}
	} else {
		AppLogException("Attempt to remove quote from the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

result CachingQuotesManager::RemoveQuoteByID(int id, BashPage page) {
	Quote *pQuote = GetQuoteByID(id, page);
	result res = GetLastResult();
	if (IsFailed(res)) {
		return res;
	}
	if (pQuote) {
		return RemoveQuote(pQuote);
	} else {
		AppLogException("Attempt to remove quote that doesn't exist from the list");
		return E_INVALID_STATE;
	}
}

result CachingQuotesManager::RemoveAll(BashPage page) {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return E_INVALID_CONDITION;
		}

		LinkedListT<Quote *> *pToRemove = new LinkedListT<Quote *>;
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Quote *pQuote; pEnum->GetCurrent(pQuote);
				if (page == PAGE_ALL_QUOTES || pQuote->GetPage() == page) {
					pToRemove->Add(pQuote);
					delete pQuote;
				}
			}
		}
		delete pEnum;

		__pQuotes->RemoveItems(*pToRemove);
		delete pToRemove;

		__smtChanged = true;
		__pSerializer->SendUserEvent(SerializerThread::REQUEST_SERIALIZATION, null);
		return E_SUCCESS;
	} else {
		AppLogException("Attempt to remove quotes from the list which wasn't cached yet");
		return E_INVALID_STATE;
	}
}

LinkedListT<Quote *> *CachingQuotesManager::GetQuotesN(BashPage page) const {
	if (__pQuotes) {
		LinkedListT<Quote *> *pRet = new LinkedListT<Quote *>;
		if (page == PAGE_ALL_QUOTES) {
			pRet->AddItems(*__pQuotes);
			return pRet;
		}

		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			SetLastResult(E_INVALID_CONDITION);

			delete pRet;
			return null;
		}
		while (!IsFailed(pEnum->MoveNext())) {
			Quote *pQuote; pEnum->GetCurrent(pQuote);
			if (pQuote->GetPage() == page) {
				pRet->Add(pQuote);
			}
		}
		delete pEnum;
		return pRet;
	} else {
		AppLogException("Attempt to get quotes list when it wasn't pre-cached yet");
		SetLastResult(E_INVALID_STATE);
		return null;
	}
}

Quote *CachingQuotesManager::GetQuote(int index) const {
	if (__pQuotes) {
		Quote *pRet;
		result res = __pQuotes->GetAt(index, pRet);
		SetLastResult(res);
		if (IsFailed(res)) {
			return null;
		} else {
			return pRet;
		}
	} else {
		AppLogException("Attempt to get quote from the list which wasn't cached yet");
		SetLastResult(E_INVALID_STATE);
		return null;
	}
}

Quote *CachingQuotesManager::GetQuoteByPage(int index, BashPage page) const {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			SetLastResult(E_INVALID_CONDITION);
			return null;
		}
		int ind = 0;
		while (!IsFailed(pEnum->MoveNext())) {
			Quote *pQuote; pEnum->GetCurrent(pQuote);
			if (pQuote->GetPage() == page) {
				if (ind == index) {
					delete pEnum;
					return pQuote;
				} else {
					ind++;
				}
			}
		}
		delete pEnum;
		return null;
	} else {
		AppLogException("Attempt to get quote from the list when it wasn't pre-cached yet");
		SetLastResult(E_INVALID_STATE);
		return null;
	}
}

Quote *CachingQuotesManager::GetQuoteByID(int id, BashPage page) const {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return null;
		}
		Quote *pRes = null;
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Quote *pQuote; pEnum->GetCurrent(pQuote);
				if (pQuote->GetPage() == page && pQuote->GetID() == id) {
					pRes = pQuote;
					break;
				}
			}
		}
		delete pEnum;
		return pRes;
	} else {
		AppLogException("Attempt to get quote from the list which wasn't cached yet");
		SetLastResult(E_INVALID_STATE);
		return null;
	}
}

bool CachingQuotesManager::IsQuoteSavedByID(int id) const {
	if (__pQuotes) {
		IEnumeratorT<Quote *> *pEnum = __pQuotes->GetEnumeratorN();
		result res = GetLastResult();
		if (IsFailed(res)) {
			AppLogException("Failed to acquire DB cache enumerator, error: [%s]", GetErrorMessage(res));
			return false;
		}
		bool found = false;
		if (pEnum) {
			while(!IsFailed(pEnum->MoveNext())) {
				Quote *pQuote; pEnum->GetCurrent(pQuote);
				if (pQuote->GetPage() == PAGE_SAVED_QUOTES && pQuote->GetID() == id) {
					found = true;
					break;
				}
			}
		}
		delete pEnum;
		return found;
	} else {
		AppLogException("Attempt to check quote in the list which wasn't cached yet");
		SetLastResult(E_INVALID_STATE);
		return false;
	}
}

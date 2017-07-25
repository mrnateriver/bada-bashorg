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

#include "MainForm.h"

using namespace Osp::App;

MainForm::MainForm(void) {
	__pQuotesList = null;
	__pTabPanel = null;
	__pOptionMenu = null;

	__pQuotesListBrightItemFormat = null;
	__pQuotesListDarkItemFormat = null;
	__pLoadMoreItemFormat = null;
	__currentTab = PAGE_SAVED_QUOTES;

	__currentColorScheme = SCHEME_DARK;

	__pQuotesManager = null;
	__pQuoteForm = null;

	__pUnreadBadgeWhite = null;
	__pUnreadBadgeBlack = null;

	__lastLoadedPage = 0;
}

MainForm::~MainForm(void) {
	if (__pQuotesListDarkItemFormat) delete __pQuotesListDarkItemFormat;
	if (__pQuotesListBrightItemFormat) delete __pQuotesListBrightItemFormat;
	if (__pLoadMoreItemFormat) delete __pLoadMoreItemFormat;
	if (__pOptionMenu) delete __pOptionMenu;
	if (__pQuotesManager) delete __pQuotesManager;

	if (__pUnreadBadgeWhite) delete __pUnreadBadgeWhite;
	if (__pUnreadBadgeBlack) delete __pUnreadBadgeBlack;
}

result MainForm::Construct(void) {
	result res = Form::Construct(L"IDF_MAINFORM");
	if (IsFailed(res)) {
		AppLogException("Failed to construct form's XML structure, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pTabPanel = GetTab();
	if (__pTabPanel) {
		__pTabPanel->SetEditModeEnabled(true);
	}

	__pQuotesList = static_cast<GroupedList*>(GetControl(L"IDC_MAINFORM_QUOTES_LIST"));

	if (!CheckControls()) {
		AppLogException("Failed to initialize custom form structure");
		return E_INIT_FAILED;
	}

	__pOptionMenu = new OptionMenu;
	res = __pOptionMenu->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct option menu, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pQuotesListDarkItemFormat = new CustomListItemFormat;
	res = __pQuotesListDarkItemFormat->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct custom list item format, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pQuotesListDarkItemFormat->AddElement(ID_LIST_FORMAT_DATE, Rectangle(10,10,233,30), 32, Color::COLOR_WHITE, Color::COLOR_WHITE);
	__pQuotesListDarkItemFormat->AddElement(ID_LIST_FORMAT_RATING, Rectangle(376,10,103,30), 32, Color::COLOR_WHITE, Color::COLOR_WHITE);
	__pQuotesListDarkItemFormat->AddElement(ID_LIST_FORMAT_TEXT, Rectangle(10,50,470,50), 25, Color(176,177,161,255), Color::COLOR_WHITE);
	__pQuotesListDarkItemFormat->AddElement(ID_LIST_FORMAT_UNREAD_BADGE, Rectangle(10,17,16,16));
	__pQuotesListDarkItemFormat->AddElement(ID_LIST_FORMAT_UNREAD_DATE, Rectangle(35,10,233,30), 32, Color::COLOR_WHITE, Color::COLOR_WHITE);

	__pQuotesListBrightItemFormat = new CustomListItemFormat;
	res = __pQuotesListBrightItemFormat->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct custom list item format, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pQuotesListBrightItemFormat->AddElement(ID_LIST_FORMAT_DATE, Rectangle(10,10,233,30), 32, Color::COLOR_BLACK, Color::COLOR_WHITE);
	__pQuotesListBrightItemFormat->AddElement(ID_LIST_FORMAT_RATING, Rectangle(376,10,103,30), 32, Color::COLOR_BLACK, Color::COLOR_WHITE);
	__pQuotesListBrightItemFormat->AddElement(ID_LIST_FORMAT_TEXT, Rectangle(10,50,470,50), 25, Color(94,94,88,255), Color::COLOR_WHITE);
	__pQuotesListBrightItemFormat->AddElement(ID_LIST_FORMAT_UNREAD_BADGE, Rectangle(10,17,16,16));
	__pQuotesListBrightItemFormat->AddElement(ID_LIST_FORMAT_UNREAD_DATE, Rectangle(35,10,233,30), 32, Color::COLOR_BLACK, Color::COLOR_WHITE);

	__pLoadMoreItemFormat = new CustomListItemFormat;
	res = __pLoadMoreItemFormat->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct custom list item format, error [%s]", GetErrorMessage(res));
		return res;
	}
	__pLoadMoreItemFormat->AddElement(ID_LIST_FORMAT_LOAD_MORE_BRIGHT, Rectangle(112,35,330,40), 40, Color(94,94,88,255), Color::COLOR_WHITE);
	__pLoadMoreItemFormat->AddElement(ID_LIST_FORMAT_LOAD_MORE_DARK, Rectangle(112,35,330,40), 40, Color(176,177,161,255), Color::COLOR_WHITE);

	__pQuotesManager = new CachingQuotesManager;
	res = __pQuotesManager->Construct(L"/Home/data.bin");
	if (IsFailed(res)) {
		AppLogException("Failed to construct saved quotes manager, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pQuoteForm = new QuoteForm;
	res = __pQuoteForm->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct quote form, error [%s]", GetErrorMessage(res));
		return res;
	}
	res = FormManager::AddFormToFrame(__pQuoteForm);
	if (IsFailed(res)) {
		AppLogException("Failed to add quote form to application's frame, error [%s]", GetErrorMessage(res));
		return res;
	}

	return E_SUCCESS;
}

bool MainForm::CheckControls(void) const {
	if (!__pQuotesList || !__pTabPanel) {
		return false;
	} else return true;
}

result MainForm::LoadQuotes(bool update) {
	__pQuotesList->RemoveAllItems();
	__pQuotesList->RemoveAllGroups();

	LinkedListT<Quote *> *pQuotes = __pQuotesManager->GetQuotesN(__currentTab);
	result res = GetLastResult();
	if (pQuotes && !IsFailed(res)) {
		if (pQuotes->GetCount() > 0) {
			int cur_num = -1;
			int index = 0;
			int gr_index = -1;
			IEnumeratorT<Quote *> *pEnum = pQuotes->GetEnumeratorN();
			while (!IsFailed(pEnum->MoveNext())) {
				Quote *quote; pEnum->GetCurrent(quote);

				if (cur_num != quote->GetPageNumber()) {
					cur_num = quote->GetPageNumber();

					String str = L"";
					if (cur_num > 0) {
						str = GetString(L"MAINFORM_LIST_GROUP_PAGE_TITLE");
						str.Replace(L"%", Integer::ToString(cur_num));
					}
					__pQuotesList->AddGroup(str, null, cur_num);
					gr_index++;
				}

				CustomListItem *pItem = new CustomListItem;
				pItem->Construct(110);
				pItem->SetItemFormat(*__pCurrentItemFormat);

				int format = ID_LIST_FORMAT_DATE;
				if (!quote->GetRead()) {
					format = ID_LIST_FORMAT_UNREAD_DATE;
				}
				pItem->SetElement(format, quote->GetDateTime());

				String rating = L"";
				if (quote->GetRating() == 0) {
					rating.Append(L"...");
				} else if (quote->GetRating() > 0) {
					rating.Append(quote->GetRating());
				} else if (quote->GetRating() == -1) {
					rating.Append(L"???");
				}
				if (rating.GetLength() < 8) {
					int diff = 8 - rating.GetLength();
					String prefix = String(diff);
					for(int i = 0; i < diff; i++) {
						prefix.Append(' ');
					}
					rating.Insert(prefix, 0);
				}
				pItem->SetElement(ID_LIST_FORMAT_RATING, rating);

				String text = quote->GetText();
				text.Replace(L"\n",L" ");

				if (!quote->GetRead()) {
					if (__currentColorScheme == SCHEME_DARK) {
						pItem->SetElement(ID_LIST_FORMAT_UNREAD_BADGE, *__pUnreadBadgeWhite, __pUnreadBadgeWhite);
					} else {
						pItem->SetElement(ID_LIST_FORMAT_UNREAD_BADGE, *__pUnreadBadgeBlack, __pUnreadBadgeBlack);
					}
				}
				pItem->SetElement(ID_LIST_FORMAT_TEXT, text);

				__pQuotesList->AddItem(gr_index, *pItem, index++);
			}
			__lastLoadedPage = cur_num;

			if (cur_num > 1 && (__currentTab == PAGE_RECENT_QUOTES || __currentTab == PAGE_ABYSS_BEST_QUOTES)) {
				CustomListItem *pItem = new CustomListItem;
				pItem->Construct(130);
				pItem->SetItemFormat(*__pLoadMoreItemFormat);

				int elid = ID_LIST_FORMAT_LOAD_MORE_BRIGHT;
				if (__currentColorScheme == SCHEME_DARK) {
					elid = ID_LIST_FORMAT_LOAD_MORE_DARK;
				}
				pItem->SetElement(elid, GetString(L"MAINFORM_LIST_LOAD_MORE_TEXT"));

				__pQuotesList->AddItem(gr_index, *pItem, -1);
			}
		} else if (__currentTab != PAGE_SAVED_QUOTES && update) {
			delete pQuotes;
			return OnOptionUpdateClicked(*this);
		}
		delete pQuotes;
	} else if (IsFailed(res)) {
		AppLogException("Failed to acquire quotes list, error: [%s]", GetErrorMessage(res));
	}
	return res;
}

result MainForm::SwitchColorScheme(void) {
	Color bg;
	switch (__currentColorScheme) {
	case SCHEME_BRIGHT:
		__pCurrentItemFormat = __pQuotesListBrightItemFormat;
		bg = Color(223,228,224,255);
		break;
	default:
		__pCurrentItemFormat = __pQuotesListDarkItemFormat;
		bg = Color(24,24,16,255);
		break;
	}
	SetBackgroundColor(bg);

	int gr_top_index, top_index;
	__pQuotesList->GetTopDrawnItemIndex(gr_top_index, top_index);
	result res = LoadQuotes(false);
	if (IsFailed(res)) {
		return res;
	} else {
		__pQuotesList->ScrollToTop(gr_top_index, top_index);
		return RefreshForm();
	}
}

result MainForm::OnOptionKeyClicked(const Control &src) {
	if (__pOptionMenu) {
		result res = __pOptionMenu->SetShowState(true);
		if (IsFailed(res)) return res;
		return __pOptionMenu->Show();
	} else return E_INVALID_STATE;
}

result MainForm::OnOptionUpdateClicked(const Control &src) {
	if (__pQuotesManager) {
		__pQuotesList->ScrollToTop();
		__pQuotesManager->UpdateQuotes(this, __currentTab);
		return RefreshForm();
	} else {
		AppLogException("Attempt to update quotes with null quotes manager");
		return E_INVALID_STATE;
	}
}

result MainForm::OnOptionClearSavedClicked(const Control &src) {
	MessageBoxModalResult r = ShowMessageBox(GetString(L"MAINFORM_CLEAR_SAVED_CONFIRMATION_TITLE"), GetString(L"MAINFORM_CLEAR_SAVED_CONFIRMATION_TEXT"), MSGBOX_STYLE_YESNO);
	if (r == MSGBOX_RESULT_YES) {
		result res = __pQuotesManager->RemoveAll(PAGE_SAVED_QUOTES);
		if (IsFailed(res)) {
			AppLogException("Failed to remove all saved quotes, error: [%s]", GetErrorMessage(res));
			return res;
		}

		if (__currentTab == PAGE_SAVED_QUOTES) {
			res = LoadQuotes(false);
			if (IsFailed(res)) {
				return res;
			}
			return RefreshForm();
		}
	}
	return E_SUCCESS;
}

result MainForm::OnOptionScrollToTopClicked(const Control &src) {
	__pQuotesList->ScrollToTop();
	return RefreshForm();
}

result MainForm::OnOptionMarkAllReadClicked(const Control &src) {
	if (__pQuotesManager) {
		result res = __pQuotesManager->MarkAllQuotesAsRead(__currentTab);
		if (IsFailed(res)) {
			AppLogException("Failed to mark all quotes as read, error: [%s]", GetErrorMessage(res));
			return res;
		}

		int gr_top_index, top_index;
		__pQuotesList->GetTopDrawnItemIndex(gr_top_index, top_index);
		res = LoadQuotes(false);
		if (IsFailed(res)) {
			return res;
		} else {
			__pQuotesList->ScrollToTop(gr_top_index, top_index);
			return RefreshForm();
		}
	} else {
		AppLogException("Attempt to mark all quotes as read with null quotes manager");
		return E_INVALID_STATE;
	}
}

result MainForm::OnOptionDarkSchemeClicked(const Control &src) {
	__currentColorScheme = SCHEME_DARK;
	return SwitchColorScheme();
}

result MainForm::OnOptionBrightSchemeClicked(const Control &src) {
	__currentColorScheme = SCHEME_BRIGHT;
	return SwitchColorScheme();
}

result MainForm::OnTabRecentClicked(const Control &src) {
	if (__currentTab != PAGE_RECENT_QUOTES) {
		__currentTab = PAGE_RECENT_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

result MainForm::OnTabRandomClicked(const Control &src) {
	if (__currentTab != PAGE_RANDOM_QUOTES) {
		__currentTab = PAGE_RANDOM_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

result MainForm::OnTabBestClicked(const Control &src) {
	if (__currentTab != PAGE_BEST_QUOTES) {
		__currentTab = PAGE_BEST_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

result MainForm::OnTabAbyssClicked(const Control &src) {
	if (__currentTab != PAGE_ABYSS_QUOTES) {
		__currentTab = PAGE_ABYSS_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

result MainForm::OnTabAbyssTopClicked(const Control &src) {
	if (__currentTab != PAGE_ABYSS_TOP_QUOTES) {
		__currentTab = PAGE_ABYSS_TOP_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

result MainForm::OnTabAbyssBestClicked(const Control &src) {
	if (__currentTab != PAGE_ABYSS_BEST_QUOTES) {
		__currentTab = PAGE_ABYSS_BEST_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

result MainForm::OnTabSavedQuotesClicked(const Control &src) {
	if (__currentTab != PAGE_SAVED_QUOTES) {
		__currentTab = PAGE_SAVED_QUOTES;

		__pQuotesList->ScrollToTop();
		result res = LoadQuotes();
		if (IsFailed(res)) {
			return res;
		} else return RefreshForm();
	} else return E_SUCCESS;
}

int MainForm::GetListCount(void) const {
	int res = 0;
	for(int i = 0; i < __pQuotesList->GetGroupCount(); i++) {
		res += __pQuotesList->GetItemCountAt(i);
	}
	if (__currentTab == PAGE_RECENT_QUOTES || __currentTab == PAGE_ABYSS_BEST_QUOTES) {
		res--;
	}
	return res;
}

result MainForm::SwitchToOtherQuote(int index) {
	Quote *pQuote = __pQuotesManager->GetQuoteByPage(index, __currentTab);
	if (pQuote) {
		int gr_index, it_index;
		__pQuotesList->GetItemIndexFromItemId(index, gr_index, it_index);

		return __pQuoteForm->SetData(this, __pQuotesManager, pQuote, __currentColorScheme, __pQuotesManager->IsQuoteSavedByID(pQuote->GetID()), __currentTab == PAGE_SAVED_QUOTES, index, GetListCount(), it_index, gr_index);
	} else return E_OBJ_NOT_FOUND;
}

result MainForm::Initialize(void) {
	result res = E_SUCCESS;

	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_COLOR_SCHEME"), 1);
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_UPDATE_QUOTES"), ID_OPTION_UPDATE_CLICKED);
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_CLEAR_SAVED"), ID_OPTION_CLEAR_SAVED_CLICKED);
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_SCROLL_TO_TOP"), ID_OPTION_SCROLL_TO_TOP_CLICKED);
	__pOptionMenu->AddItem(GetString(L"MAINFORM_OPTIONMENU_MARK_ALL_READ"), ID_OPTION_MARK_ALL_READ_CLICKED);
	__pOptionMenu->AddSubItem(0, GetString(L"MAINFORM_OPTIONMENU_SCHEME_DARK"), ID_OPTION_DARK_SCHEME_CLICKED);
	__pOptionMenu->AddSubItem(0, GetString(L"MAINFORM_OPTIONMENU_SCHEME_CONTRAST"), ID_OPTION_BRIGHT_SCHEME_CLICKED);

	RegisterAction(ID_OPTION_KEY_CLICKED, HANDLER(MainForm::OnOptionKeyClicked));
	RegisterAction(ID_OPTION_UPDATE_CLICKED, HANDLER(MainForm::OnOptionUpdateClicked));
	RegisterAction(ID_OPTION_CLEAR_SAVED_CLICKED, HANDLER(MainForm::OnOptionClearSavedClicked));
	RegisterAction(ID_OPTION_SCROLL_TO_TOP_CLICKED, HANDLER(MainForm::OnOptionScrollToTopClicked));
	RegisterAction(ID_OPTION_MARK_ALL_READ_CLICKED, HANDLER(MainForm::OnOptionMarkAllReadClicked));
	RegisterAction(ID_OPTION_DARK_SCHEME_CLICKED, HANDLER(MainForm::OnOptionDarkSchemeClicked));
	RegisterAction(ID_OPTION_BRIGHT_SCHEME_CLICKED, HANDLER(MainForm::OnOptionBrightSchemeClicked));
	__pOptionMenu->AddActionEventListener(*this);
	SetOptionkeyActionId(ID_OPTION_KEY_CLICKED);
	AddOptionkeyActionListener(*this);

	RegisterAction(ID_TAB_RECENT_CLICKED, HANDLER(MainForm::OnTabRecentClicked));
	RegisterAction(ID_TAB_RANDOM_CLICKED, HANDLER(MainForm::OnTabRandomClicked));
	RegisterAction(ID_TAB_BEST_CLICKED, HANDLER(MainForm::OnTabBestClicked));
	RegisterAction(ID_TAB_ABYSS_CLICKED, HANDLER(MainForm::OnTabAbyssClicked));
	RegisterAction(ID_TAB_ABYSS_TOP_CLICKED, HANDLER(MainForm::OnTabAbyssTopClicked));
	RegisterAction(ID_TAB_ABYSS_BEST_CLICKED, HANDLER(MainForm::OnTabAbyssBestClicked));
	RegisterAction(ID_TAB_SAVED_QUOTES_CLICKED, HANDLER(MainForm::OnTabSavedQuotesClicked));
	__pTabPanel->AddActionEventListener(*this);

	__pQuotesList->AddGroupedItemEventListener(*this);

	__pUnreadBadgeWhite = GetBitmapN(L"not_read_icon_white.png");
	__pUnreadBadgeBlack = GetBitmapN(L"not_read_icon_black.png");

	AppRegistry *appReg = Application::GetInstance()->GetAppRegistry();

	String prefs[7] = {
		L"MAINFORM_TAB_FIRST_INDEX",
		L"MAINFORM_TAB_SECOND_INDEX",
		L"MAINFORM_TAB_THIRD_INDEX",
		L"MAINFORM_TAB_FOURTH_INDEX",
		L"MAINFORM_TAB_FIFTH_INDEX",
		L"MAINFORM_TAB_SIXTH_INDEX",
		L"MAINFORM_TAB_SEVENTH_INDEX"
	};
	String strings[7] = {
		"MAINFORM_TAB_TITLE_SAVED_QUOTES",
		"MAINFORM_TAB_TITLE_RECENT",
		"MAINFORM_TAB_TITLE_RANDOM",
		"MAINFORM_TAB_TITLE_BEST",
		"MAINFORM_TAB_TITLE_ABYSS",
		"MAINFORM_TAB_TITLE_ABYSS_BEST",
		"MAINFORM_TAB_TITLE_ABYSS_TOP_RATED"
	};

	int index = -1;
	for(int i = 0; i < 7; i++) {
		res = appReg->Get(prefs[i], index);
		if (IsFailed(res)) {
			index = i;
			res = appReg->Add(prefs[i], index);

			if (IsFailed(res)) {
				AppLogException("Failed to add registry key for storing tab order, error [%s]", GetErrorMessage(res));
				return res;
			}
		}
		if (index == 0) {
			Bitmap *pStar = GetBitmapN(L"small_mark_icon.png");
			__pTabPanel->AddItem(*pStar, GetString(strings[index]), ID_TAB_SAVED_QUOTES_CLICKED + index);
			delete pStar;
		} else {
			__pTabPanel->AddItem(GetString(strings[index]), ID_TAB_SAVED_QUOTES_CLICKED + index);
		}
	}

	int sel_tab = 0;
	res = appReg->Get(L"MAINFORM_SELECTED_TAB", sel_tab);
	if (IsFailed(res)) {
		res = appReg->Add(L"MAINFORM_SELECTED_TAB", sel_tab);
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing selected tab. Selected tab will not be saved, error [%s]", GetErrorMessage(res));
		}
	}
	__pTabPanel->SetSelectedItem(sel_tab);
	__currentTab = (BashPage)(__pTabPanel->GetItemActionIdAt(sel_tab) - ID_TAB_SAVED_QUOTES_CLICKED + 1);

	int sel_scheme = 1;
	res = appReg->Get(L"MAINFORM_SELECTED_COLOR_SCHEME", sel_scheme);
	if (IsFailed(res)) {
		res = appReg->Add(L"MAINFORM_SELECTED_COLOR_SCHEME", sel_scheme);
		if (IsFailed(res)) {
			AppLogException("Failed to add registry key for storing selected color scheme. Selected scheme will not be saved, error [%s]", GetErrorMessage(res));
		}
	}
	__currentColorScheme = (ColorScheme)sel_scheme;

	appReg->Save();

	Color bg;
	switch (__currentColorScheme) {
	case SCHEME_BRIGHT:
		__pCurrentItemFormat = __pQuotesListBrightItemFormat;
		bg = Color(223,228,224,255);
		break;
	default:
		__pCurrentItemFormat = __pQuotesListDarkItemFormat;
		bg = Color(24,24,16,255);
		break;
	}
	SetBackgroundColor(bg);

	res = LoadQuotes(false);

	Timer *timer = new Timer;
	timer->Construct(*this);
	timer->Start(1000);

	return res;
}

void MainForm::OnTimerExpired(Timer &timer) {
	AppRegistry *pReg = Application::GetInstance()->GetAppRegistry();
	int not_first_launch = 0;
	if (IsFailed(pReg->Get(L"NOT_FIRST_LAUNCH", not_first_launch))) {
		pReg->Add(L"NOT_FIRST_LAUNCH", 1);
		pReg->Save();
	} else if (not_first_launch) {
		int read_man = 0;
		if (IsFailed(pReg->Get(L"HAS_READ_AD", read_man))) {
			pReg->Add(L"HAS_READ_AD", 1);
			pReg->Save();
		}

		if (!read_man) {
			MessageBox *pBox = new MessageBox;
			pBox->Construct(GetString(L"AD_TITLE"), GetString(L"AD_MSG"), MSGBOX_STYLE_OK);
			int res;
			pBox->ShowAndWait(res);
			delete pBox;
		}
	}

	delete &timer;
}

result MainForm::Terminate(void) {
	__pQuotesManager->SerializeAllQuotes();

	AppRegistry *appReg = Application::GetInstance()->GetAppRegistry();

	String prefs[7] = {
		L"MAINFORM_TAB_FIRST_INDEX",
		L"MAINFORM_TAB_SECOND_INDEX",
		L"MAINFORM_TAB_THIRD_INDEX",
		L"MAINFORM_TAB_FOURTH_INDEX",
		L"MAINFORM_TAB_FIFTH_INDEX",
		L"MAINFORM_TAB_SIXTH_INDEX",
		L"MAINFORM_TAB_SEVENTH_INDEX"
	};
	result res = E_SUCCESS;
	for(int i = 0; i < __pTabPanel->GetItemCount(); i++) {
		res = appReg->Set(prefs[i], __pTabPanel->GetItemActionIdAt(i) - ID_TAB_SAVED_QUOTES_CLICKED);
		if (IsFailed(res)) {
			AppLogException("Failed to save tab order to registry, error: [%s]", GetErrorMessage(res));
			break;
		}
	}

	res = appReg->Set(L"MAINFORM_SELECTED_TAB", (int)__pTabPanel->GetSelectedItemIndex());
	if (IsFailed(res)) {
		AppLogException("Failed to save selected tab to registry, error: [%s]", GetErrorMessage(res));
	}

	res = appReg->Set(L"MAINFORM_SELECTED_COLOR_SCHEME", (int)__currentColorScheme);
	if (IsFailed(res)) {
		AppLogException("Failed to save selected color scheme to registry, error: [%s]", GetErrorMessage(res));
	}

	appReg->Save();
	return res;
}

void MainForm::HandleUpdateCallback(BashPage updated_page, LinkedListT<Quote *> *quotesN, int back_index, int gr_back_index) {
	if (quotesN) {
		delete quotesN;
	}

	result res = LoadQuotes(false);
	if (IsFailed(res)) {
		AppLogException("Failed to reload quotes list, error: [%s]", GetErrorMessage(res));
	}
	if (back_index >= 0 && gr_back_index >= 0) {
		__pQuotesList->ScrollToTop(gr_back_index, back_index);
	}

	res = RefreshForm();
	if (IsFailed(res)) {
		AppLogException("Failed to refresh form after reloading quotes list, error: [%s]", GetErrorMessage(res));
	}
}

void MainForm::OnItemStateChanged(const Control &source, int groupIndex, int itemIndex, int itemId, ItemStatus status) {
	if (itemId < 0) {
		if (__pQuotesManager) {
			__pQuotesManager->UpdateQuotes(this, __currentTab, __lastLoadedPage - 1);
			RefreshForm();
		} else {
			AppLogException("Attempt to update quotes with null quotes manager");
		}
	} else {
		Quote *pQuote = __pQuotesManager->GetQuoteByPage(itemId, __currentTab);
		if (pQuote) {
			__pQuoteForm->SetData(this, __pQuotesManager, pQuote, __currentColorScheme, __pQuotesManager->IsQuoteSavedByID(pQuote->GetID()), __currentTab == PAGE_SAVED_QUOTES, itemId, GetListCount(), itemIndex, groupIndex);
			FormManager::SetActiveForm(__pQuoteForm);
		}
	}
}

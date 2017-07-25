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
#include <FWeb.h>
#include "MainForm.h"

using namespace Osp::App;
using namespace Osp::Web::Controls;

QuoteForm::QuoteForm() {
	__pQuoteRatingLabel = null;
	__pQuoteDateLabel = null;
	__pQuoteTextLabel = null;
	__pMainScrollPanel = null;
	__pShareButton = null;
	__pStarButton = null;
	__pPrevQuoteButton = null;
	__pNextQuoteButton = null;
	__pBackToListButton = null;
	__pConMenu = null;

	__pQuote = null;
	__colorScheme = SCHEME_DARK;
	__quoteIndex = -1;
	__pQuoteSwitchCallback = null;
	__marked = false;
	__savedTab = false;
	__startedMarked = false;
	__backIndex = -1;
	__grBackIndex = -1;
	__pQuotesManager = null;
}

QuoteForm::~QuoteForm(void) {
	if (__pConMenu) {
		delete __pConMenu;
	}
}

result QuoteForm::Construct(void) {
	result res = Form::Construct(L"IDF_QUOTEFORM");
	if (IsFailed(res)) {
		AppLogException("Failed to construct form's XML structure, error [%s]", GetErrorMessage(res));
		return res;
	}

	__pQuoteRatingLabel = static_cast<Label *>(GetControl(L"IDC_QUOTEFORM_RATING_LABEL"));
	__pQuoteDateLabel = static_cast<Label *>(GetControl(L"IDC_QUOTEFORM_DATE_LABEL"));
	__pShareButton = static_cast<Button *>(GetControl(L"IDC_QUOTEFORM_SHARE_BUTTON"));
	__pStarButton = static_cast<Button *>(GetControl(L"IDC_QUOTEFORM_STAR_BUTTON"));
	__pPrevQuoteButton = static_cast<Button *>(GetControl(L"IDC_QUOTEFORM_PREV_BUTTON"));
	__pNextQuoteButton = static_cast<Button *>(GetControl(L"IDC_QUOTEFORM_NEXT_BUTTON"));
	__pBackToListButton = static_cast<Button *>(GetControl(L"IDC_QUOTEFORM_LIST_BUTTON"));
	__pMainScrollPanel = static_cast<ScrollPanel *>(GetControl(L"IDC_QUOTEFORM_TEXT_SCROLLPANEL"));
	if (__pMainScrollPanel) {
		__pQuoteTextLabel = static_cast<Label *>(__pMainScrollPanel->GetControl(L"IDPC_QUOTEFORM_TEXT_LABEL"));
	}

	if (!CheckControls()) {
		AppLogException("Failed to initialize custom form structure");
		return E_INIT_FAILED;
	}

	return res;
}

result QuoteForm::SetData(MainForm *cb, CachingQuotesManager *mgr, Quote *quote, ColorScheme color_sc, bool marked, bool saved_tab, int index, int list_len, int back_index, int gr_back_index) {
	__pQuote = quote;
	__colorScheme = color_sc;
	__quoteIndex = index;
	__pQuoteSwitchCallback = cb;
	__marked = marked;
	__savedTab = saved_tab;
	__startedMarked = marked;
	__pQuotesManager = mgr;
	__backIndex = back_index;
	__grBackIndex = gr_back_index;

	if (__pQuote->GetID() == 0) {
		SetTitleText(GetString(L"MAINFORM_TAB_TITLE_ABYSS_TOP_RATED"), ALIGNMENT_LEFT);
	} else {
		SetTitleText(GetString(L"QUOTEFORM_TITLE") + Integer::ToString(__pQuote->GetID()), ALIGNMENT_LEFT);
	}

	String rating = L"";
	if (quote->GetRating() == 0) {
		rating.Append(L"...");
	} else if (quote->GetRating() > 0) {
		rating.Append(__pQuote->GetRating());
	} else if (quote->GetRating() == -1) {
		rating.Append(L"???");
	}
	__pQuoteRatingLabel->SetText(rating.IsEmpty() ? rating : GetString(L"QUOTEFORM_RATING_LABEL") + rating);
	__pQuoteDateLabel->SetText(GetString(L"QUOTEFORM_DATE_LABEL") + __pQuote->GetDateTime());

	Label *pLabel = new Label;
	pLabel->Construct(Rectangle(0,10,480,GetLinesForText(__pQuote->GetText()) * 35), __pQuote->GetText());
	pLabel->SetTextColor(Color(176,177,161,255));
	pLabel->SetTextConfig(34, LABEL_TEXT_STYLE_NORMAL);
	pLabel->SetTextHorizontalAlignment(ALIGNMENT_LEFT);
	pLabel->SetTextVerticalAlignment(ALIGNMENT_TOP);

	__pMainScrollPanel->RemoveControl(*__pQuoteTextLabel);

	__pQuoteTextLabel = pLabel;
	__pMainScrollPanel->AddControl(*__pQuoteTextLabel);

	Color bg;
	switch (__colorScheme) {
	case SCHEME_BRIGHT:
		bg = Color(223,228,224,255);
		break;
	default:
		bg = Color(24,24,16,255);
		break;
	}
	SetBackgroundColor(bg);
	__pMainScrollPanel->SetBackgroundColor(bg);
	if (__colorScheme != SCHEME_DARK) {
		__pQuoteRatingLabel->SetTextColor(Color::COLOR_BLACK);
		__pQuoteDateLabel->SetTextColor(Color::COLOR_BLACK);
		__pQuoteTextLabel->SetTextColor(Color(94,94,88,255));
	} else {
		__pQuoteRatingLabel->SetTextColor(Color::COLOR_WHITE);
		__pQuoteDateLabel->SetTextColor(Color::COLOR_WHITE);
		__pQuoteTextLabel->SetTextColor(Color(176,177,161,255));
	}

	if (list_len == 1) {
		__pNextQuoteButton->SetEnabled(false);
		__pPrevQuoteButton->SetEnabled(false);
	} else if (index == list_len - 1) {
		__pNextQuoteButton->SetEnabled(false);
		__pPrevQuoteButton->SetEnabled(true);
	} else if (index == 0) {
		__pNextQuoteButton->SetEnabled(true);
		__pPrevQuoteButton->SetEnabled(false);
	} else {
		__pNextQuoteButton->SetEnabled(true);
		__pPrevQuoteButton->SetEnabled(true);
	}

	result res = RefreshStarButton();
	if (IsFailed(res)) {
		return res;
	}

	if (FormManager::GetActiveForm() == this) {
		res = RefreshForm();
		if (IsFailed(res)) {
			return res;
		}
	}

	mgr->MarkQuoteAsRead(quote);
	return E_SUCCESS;
}

bool QuoteForm::CheckControls(void) const {
	if (!__pQuoteRatingLabel || !__pQuoteDateLabel || !__pQuoteTextLabel || !__pMainScrollPanel
		|| !__pStarButton || !__pPrevQuoteButton || !__pNextQuoteButton || !__pBackToListButton
		|| !__pShareButton) {
		return false;
	} else return true;
}

int QuoteForm::GetLinesForText(const String &text) const {
	Font font;
	font.Construct(FONT_STYLE_PLAIN, 34);

	TextElement elem;
	elem.Construct(text);
	elem.SetFont(font);

	EnrichedText ent;
	ent.Construct(Dimension(460, 500));
	ent.SetTextWrapStyle(TEXT_WRAP_WORD_WRAP);
	ent.Add(elem);

	return ent.GetTotalLineCount();
}

result QuoteForm::RefreshStarButton(void) {
	Bitmap *pPressedStar = null;
	Bitmap *pUnpressedStar = null;

	result res = E_SUCCESS;
	if (__marked) {
		pPressedStar = GetBitmapN(L"star_button_checked_pressed.png");

		res = GetLastResult();
		if (IsFailed(res)) { return res; }

		pUnpressedStar = GetBitmapN(L"star_button_checked.png");

		res = GetLastResult();
		if (IsFailed(res)) { return res; }
	} else {
		pPressedStar = GetBitmapN(L"star_button_pressed.png");

		res = GetLastResult();
		if (IsFailed(res)) { return res; }

		pUnpressedStar = GetBitmapN(L"star_button.png");

		res = GetLastResult();
		if (IsFailed(res)) { return res; }
	}

	__pStarButton->SetPressedBackgroundBitmap(*pPressedStar);
	__pStarButton->SetNormalBackgroundBitmap(*pUnpressedStar);

	delete pPressedStar;
	delete pUnpressedStar;

	return res;
}

result QuoteForm::UpdateQuoteSavedStatus(void) const {
	if (__pQuote && __pQuotesManager) {
		result res = E_SUCCESS;
		if (__marked && !__startedMarked) {
			Quote *pSavedQuote = new Quote(*__pQuote, PAGE_SAVED_QUOTES);
			res = __pQuotesManager->AddQuote(pSavedQuote);
			if (IsFailed(res)) {
				AppLogException("Failed to add quote to the saved list, error: [%s]", GetErrorMessage(res));
				return res;
			}
		} else if (!__marked && __startedMarked) {
			res = __pQuotesManager->RemoveQuoteByID(__pQuote->GetID(), PAGE_SAVED_QUOTES);
			if (IsFailed(res)) {
				AppLogException("Failed to remove quote from the saved list, error: [%s]", GetErrorMessage(res));
				return res;
			}
		}
		return res;
	} else {
		AppLogException("Attempt to update quote with null pointer to itself or to quotes manager");
		return E_INVALID_STATE;
	}
}

result QuoteForm::OnPrevQuoteClicked(const Control &src) {
	result res = UpdateQuoteSavedStatus();
	if (IsFailed(res)) {
		return res;
	} else {
		if (__pQuoteSwitchCallback) __pQuoteSwitchCallback->HandleUpdateCallback(PAGE_SAVED_QUOTES, null);
		return __pQuoteSwitchCallback->SwitchToOtherQuote(__quoteIndex - 1);
	}
}

result QuoteForm::OnNextQuoteClicked(const Control &src) {
	result res = UpdateQuoteSavedStatus();
	if (IsFailed(res)) {
		return res;
	} else {
		if (__savedTab && __startedMarked != __marked) {
			__quoteIndex--;
		}
		if (__pQuoteSwitchCallback) __pQuoteSwitchCallback->HandleUpdateCallback(PAGE_SAVED_QUOTES, null);
		return __pQuoteSwitchCallback->SwitchToOtherQuote(__quoteIndex + 1);
	}
}

result QuoteForm::OnBackToListClicked(const Control &src) {
	result res = UpdateQuoteSavedStatus();
	if (IsFailed(res)) {
		return res;
	}

	if (__pQuoteSwitchCallback) __pQuoteSwitchCallback->HandleUpdateCallback(PAGE_SAVED_QUOTES, null, __backIndex, __grBackIndex);
	return FormManager::SetPreviousFormActive(false);
}

result QuoteForm::OnStarButtonClicked(const Control &src) {
	__marked = !__marked;

	result res = RefreshStarButton();
	if (IsFailed(res)) {
		return res;
	}
	return RefreshForm();
}

result QuoteForm::OnShareButtonClicked(const Control &src) {
	__pConMenu->SetShowState(true);
	return __pConMenu->Show();
}

result QuoteForm::OnSmsButtonClicked(const Control &src) {
	ArrayList *pDataList = new ArrayList;
	pDataList->Construct(2);

	String* pData = new String(L"type:MMS");
	pDataList->Add(*pData);

	String* pData2 = new String(L"text:" + __pQuoteTextLabel->GetText() + L"\n\n\u00A9 bash.org.ru");
	pDataList->Add(*pData2);

	AppControl *pAc = AppManager::FindAppControlN(APPCONTROL_MESSAGE, OPERATION_EDIT);
	if(pAc) {
		result res = pAc->Start(pDataList, null);
		if (IsFailed(res)) {
			ShowMessageBox(GetString(L"APPCONTROL_FAILURE_MBOX_TITLE"), GetString(L"APPCONTROL_FAILURE_MBOX_TEXT"), MSGBOX_STYLE_OK);
		}
		delete pAc;
	}
	pDataList->RemoveAll(true);
	delete pDataList;

	return E_SUCCESS;
}

result QuoteForm::OnEmailButtonClicked(const Control &src) {
	ArrayList* pDataList = new ArrayList();
	pDataList->Construct(2);

	String str = GetString(L"EMAIL_SUBJECT_TEXT");
	str.Replace(L"%", Integer::ToString(__pQuote->GetID()));
	String* pData = new String(L"subject:" + str);
	pDataList->Add(*pData);

	String* pData2 = new String(L"text:" + __pQuoteTextLabel->GetText() + L"\n\n\u00A9 bash.org.ru");
	pDataList->Add(*pData2);

	AppControl* pAc = AppManager::FindAppControlN(APPCONTROL_EMAIL, OPERATION_EDIT);
	if(pAc) {
		result res = pAc->Start(pDataList, null);
		if (IsFailed(res)) {
			ShowMessageBox(GetString(L"APPCONTROL_FAILURE_MBOX_TITLE"), GetString(L"APPCONTROL_FAILURE_MBOX_TEXT"), MSGBOX_STYLE_OK);
		}
		delete pAc;
	}
	pDataList->RemoveAll(true);
	delete pDataList;

	return E_SUCCESS;
}

result QuoteForm::Initialize(void) {
	__pPrevQuoteButton->SetText(GetString(L"QUOTEFORM_PREV_QUOTE_BUTTON_TEXT"));
	__pNextQuoteButton->SetText(GetString(L"QUOTEFORM_NEXT_QUOTE_BUTTON_TEXT"));
	__pBackToListButton->SetText(GetString(L"QUOTEFORM_BACK_BUTTON_TEXT"));

	RegisterButtonPressEvent(__pPrevQuoteButton, ID_PREV_QUOTE_CLICKED, HANDLER(QuoteForm::OnPrevQuoteClicked));
	RegisterButtonPressEvent(__pNextQuoteButton, ID_NEXT_QUOTE_CLICKED, HANDLER(QuoteForm::OnNextQuoteClicked));
	RegisterButtonPressEvent(__pBackToListButton, ID_BACK_TO_LIST_CLICKED, HANDLER(QuoteForm::OnBackToListClicked));
	RegisterButtonPressEvent(__pStarButton, ID_STAR_CLICKED, HANDLER(QuoteForm::OnStarButtonClicked));
	RegisterButtonPressEvent(__pShareButton, ID_SHARE_CLICKED, HANDLER(QuoteForm::OnShareButtonClicked));

	Bitmap *pSms = GetBitmapN(L"sms_icon.png");
	Bitmap *pMail = GetBitmapN(L"email_icon.png");

	Bitmap *pSmsPressed = GetBitmapN(L"sms_icon_pressed.png");
	Bitmap *pMailPressed = GetBitmapN(L"email_icon_pressed.png");

	__pConMenu = new ContextMenu;
	__pConMenu->Construct(Point(377, 157), CONTEXT_MENU_STYLE_ICON);

	__pConMenu->AddItem(*pSms, pSmsPressed, ID_SMS_CLICKED);
	__pConMenu->AddItem(*pMail, pMailPressed, ID_EMAIL_CLICKED);
	__pConMenu->AddActionEventListener(*this);

	RegisterAction(ID_SMS_CLICKED, HANDLER(QuoteForm::OnSmsButtonClicked));
	RegisterAction(ID_EMAIL_CLICKED, HANDLER(QuoteForm::OnEmailButtonClicked));

	delete pSmsPressed;
	delete pMailPressed;
	delete pSms;
	delete pMail;

	return E_SUCCESS;
}

result QuoteForm::Terminate(void) {
	return E_SUCCESS;
}

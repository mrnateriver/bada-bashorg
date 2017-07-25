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
#include <FSystem.h>
#include <FMedia.h>
#include <typeinfo>

#include "BaseForm.h"

using namespace Osp::App;
using namespace Osp::System;
using namespace Osp::Media;

CallbackInfo CreateCallbackInfo(int taskID, BaseForm *handler) {
	CallbackInfo info = { taskID, handler };
	return info;
}

BaseForm::BaseForm(void) {
	__pCurrentPopup = null;
	__pEventMap = new std::map<int, EventDelegate>;
	__initialized = false;
	__pDTFormatter = null;
}

BaseForm::~BaseForm(void) {
	if (__pCurrentPopup) delete __pCurrentPopup;
	if (__pEventMap) delete __pEventMap;
	if (__pDTFormatter) delete __pDTFormatter;
}

result BaseForm::RegisterButtonPressEvent(Button *pCtrl, int actionId, EventDelegate handler) {
	if (pCtrl && handler) {
		std::map<int, EventDelegate>::iterator iter = (__pEventMap->insert(std::make_pair(actionId, handler))).first;
		if (iter != __pEventMap->end()) {
			pCtrl->SetActionId(actionId);
			pCtrl->AddActionEventListener(*this);
			return E_SUCCESS;
		} else {
			AppLogException("Handler for action [%d] is already registered", actionId);
			return E_ALREADY_CONNECTED;
		}
	} else {
		AppLogException("Attempt to register action handler for null control or passed null handler");
		return E_INVALID_ARG;
	}
}

result BaseForm::RegisterAction(int actionId, EventDelegate handler) {
	if (handler) {
		std::map<int, EventDelegate>::iterator iter = (__pEventMap->insert(std::make_pair(actionId, handler))).first;
		if (iter == __pEventMap->end()) {
			AppLogException("Handler for action [%d] is already registered", actionId);
			return E_ALREADY_CONNECTED;
		}
		return E_SUCCESS;
	} else {
		AppLogException("Attempt to register action handler for null control or passed null handler");
		return E_INVALID_ARG;
	}
}

void BaseForm::UnregisterAction(int actionId) {
	std::map<int, EventDelegate>::iterator iter = __pEventMap->find(actionId);
	if (iter != __pEventMap->end()) {
		__pEventMap->erase(iter);
	} else {
		AppLogException("Event [%d] is not registered", actionId);
	}
}

result BaseForm::ShowInputBox(CallbackInfo cbInfo, KeypadInputModeCategory inputMode, const String &title, const String &msg, const String &acceptButton, const String &cancelButton) {
	if (__pCurrentPopup) {
		__pCurrentPopup->SetShowState(false);

		UnregisterAction(ID_POPUP_ACCEPT_CLICKED);
		UnregisterAction(ID_POPUP_CANCEL_CLICKED);

		delete __pCurrentPopup;
		__pCurrentPopup = null;
	}

	__pCurrentPopup = new Popup;
	result res = __pCurrentPopup->Construct(L"IDP_INPUT_POPUP");
	if (IsFailed(res)) {
		AppLogException("Failed to construct input popup, error: [%s]", GetErrorMessage(res));

		delete __pCurrentPopup;
		__pCurrentPopup = null;

		return res;
	}

	__pCurrentPopup->SetTitleText(title);

	Label *pLabel = static_cast<Label*>(__pCurrentPopup->GetControl(L"IDC_INPUT_POPUP_TEXT"));
	Button *pAcceptButton = static_cast<Button*>(__pCurrentPopup->GetControl(L"IDC_INPUT_POPUP_ACCEPT_BUTTON"));
	Button *pCancelButton = static_cast<Button*>(__pCurrentPopup->GetControl(L"IDC_INPUT_POPUP_CANCEL_BUTTON"));
	EditField *pField = static_cast<EditField*>(__pCurrentPopup->GetControl(L"IDC_INPUT_POPUP_FIELD"));
	if (!pLabel || !pAcceptButton || !pCancelButton || !pField) {
		AppLogException("Failed to acquire controls from input popup");

		delete __pCurrentPopup;
		__pCurrentPopup = null;

		return E_INIT_FAILED;
	}

	pLabel->SetText(msg);
	pAcceptButton->SetText(acceptButton);
	pCancelButton->SetText(cancelButton);
	pField->SetInputModeCategory(inputMode, true);

	RegisterButtonPressEvent(pAcceptButton, ID_POPUP_ACCEPT_CLICKED, HANDLER(BaseForm::OnPopupAccept));
	RegisterButtonPressEvent(pCancelButton, ID_POPUP_CANCEL_CLICKED, HANDLER(BaseForm::OnPopupCancel));

	__popupCallbackInfo = cbInfo;

	res = __pCurrentPopup->SetShowState(true);
	if (IsFailed(res)) {
		AppLogException("Failed to set show state for popup, error: [%s]", GetErrorMessage(res));
		return res;
	}
	return __pCurrentPopup->Show();
}

result BaseForm::RefreshForm(void) {
	result res = E_SUCCESS;

	res = Draw();
	if (IsFailed(res)) {
		AppLogException("Failed to redraw form, error: [%s]", GetErrorMessage(res));
		return res;
	}

	/*res = */Show();
	/*if (IsFailed(res)) {
		AppLogException("Failed to switch buffers upon redrawing, error: [%s]", GetErrorMessage(res));
		return res;
	}*/

	/*Frame *pFrame = static_cast<Frame*>(GetParent());
	if (pFrame) {
		res = pFrame->Draw();
		if (IsFailed(res)) {
			AppLogException("Failed to redraw form, error: [%s]", GetErrorMessage(res));
			return res;
		}
		res = pFrame->Show();
		if (IsFailed(res)) {
			AppLogException("Failed to switch buffers upon redrawing, error: [%s]", GetErrorMessage(res));
			return res;
		}
	} else {
		AppLogException("Could not retrieve parent frame interface");
		return E_INVALID_STATE;
	}*/

	return res;
}

String BaseForm::GetLocaleSpecificDatetime(const DateTime &dt) {
	String datetime;
	result res = E_SUCCESS;

	if (__pDTFormatter) {
		res = __pDTFormatter->Format(dt, datetime);
	} else {
		datetime = dt.ToString();
	}

	if (IsFailed(res)) {
		AppLogException("Failed to format datetime to current locale, returning default string. Error: [%s]", GetErrorMessage(res));
		SetLastResult(res);
		return dt.ToString();
	}

	return datetime;
}

String BaseForm::GetLocaleSpecificDatetime(long long seconds) {
	DateTime dt = __epoch;
	dt.AddSeconds(seconds);

	return GetLocaleSpecificDatetime(dt);
}

String BaseForm::GetLocaleSpecificDatetime(void) {
	DateTime curTime;
	SystemTime::GetCurrentTime(curTime);

	LocaleManager locMgr;
	result res = locMgr.Construct();

	if (IsFailed(res)) {
		AppLogException("Failed to construct locale manager. Error: [%s]", GetErrorMessage(res));
		SetLastResult(res);
		return curTime.ToString();
	}

	TimeZone tz = locMgr.GetSystemTimeZone();

	DateTime date = tz.UtcTimeToStandardTime(curTime);
	if (tz.IsDstUsed()) {
		date.AddHours(1);
	}

	return GetLocaleSpecificDatetime(date);
}

result BaseForm::OnInitializing(void) {
	__epoch = GetLocalDatetimeObject(0);

	__pDTFormatter = DateTimeFormatter::CreateDateTimeFormatterN();
	result res = GetLastResult();
	if (IsFailed(res)) {
		AppLogException("Failed to construct locale-aware datetime formatter. Error: [%s]", GetErrorMessage(res));
	}

	VersionInfo ver;
    String platformVersion;
    res = SystemInfo::GetValue(L"APIVersion", platformVersion);
    if (!IsFailed(res)) {
    	do {
			int dotIndex = -1;
			res = platformVersion.IndexOf(L".", 0, dotIndex);
			if (IsFailed(res)) {
				AppLogException("Failed to acquire API version string, falling down to SDK 1.0. Error: [%s]", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			String majorStr(L"1");
			res = platformVersion.SubString(0, dotIndex, majorStr);
			if (IsFailed(res)) {
				AppLogException("Failed to acquire API version string, falling down to SDK 1.0. Error: [%s]", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			int majorVersion = 1;
			res = Integer::Parse(majorStr, majorVersion);
			if (IsFailed(res)) {
				AppLogException("Failed to acquire API version string, falling down to SDK 1.0. Error: [%s]", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			String minorStr(L"0");
			res = platformVersion.SubString(dotIndex + 1, minorStr);
			if (IsFailed(res)) {
				AppLogException("Failed to acquire API version string, falling down to SDK 1.0. Error: [%s]", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			int minorVersion = 1;
			res = Integer::Parse(minorStr, minorVersion);
			if (IsFailed(res)) {
				AppLogException("Failed to acquire API version string, falling down to SDK 1.0. Error: [%s]", GetErrorMessage(res));
		    	ver.major = 1;
		    	ver.minor = 0;
		    	break;
			}

			ver.major = majorVersion;
			ver.minor = minorVersion;
    	} while (false);
    } else {
		AppLogException("Failed to acquire API version string, falling down to SDK 1.0. Error: [%s]", GetErrorMessage(res));

    	ver.major = 1;
    	ver.minor = 0;
    }
    __sdkVer = ver;

	res = Initialize();
	if (!IsFailed(res)) {
		__initialized = true;
	}
	return res;
}

result BaseForm::OnTerminating(void) {
	return Terminate();
}

result BaseForm::OnPopupAccept(const Control &src) {
	if (__pCurrentPopup && __popupCallbackInfo.callbackHandler) {
		EditField *pField = static_cast<EditField*>(__pCurrentPopup->GetControl(L"IDC_INPUT_POPUP_FIELD"));
		if (!pField) {
			AppLogException("Failed to acquire field control from popup");
			return E_INVALID_STATE;
		}

		String *pCbData = new String(pField->GetText());
		__popupCallbackInfo.callbackHandler->DialogCallback(__popupCallbackInfo.taskID, this, DIALOG_RESULT_OK, (void*)pCbData);

		result res = __pCurrentPopup->SetShowState(false);
		if (IsFailed(res)) {
			AppLogException("Failed to hide popup, error: [%s]", GetErrorMessage(res));
		}
		delete __pCurrentPopup;
		__pCurrentPopup = null;

		res = RefreshForm();
		if (IsFailed(res)) {
			AppLogException("Failed to redraw form after hiding popup, error: [%s]", GetErrorMessage(res));
		}
		return res;
	} else return E_INVALID_STATE;
}

result BaseForm::OnPopupCancel(const Control &src) {
	if (__pCurrentPopup && __popupCallbackInfo.callbackHandler) {
		__popupCallbackInfo.callbackHandler->DialogCallback(__popupCallbackInfo.taskID, this, DIALOG_RESULT_CANCEL, null);

		result res = __pCurrentPopup->SetShowState(false);
		if (IsFailed(res)) {
			AppLogException("Failed to hide popup, error: [%s]", GetErrorMessage(res));
		}
		delete __pCurrentPopup;
		__pCurrentPopup = null;

		res = RefreshForm();
		if (IsFailed(res)) {
			AppLogException("Failed to redraw form after hiding popup, error: [%s]", GetErrorMessage(res));
		}
		return res;
	} else return E_INVALID_STATE;
}

void BaseForm::OnActionPerformed(const Control &source, int actionId) {
	bool triggered = false;
	std::map<int, EventDelegate>::iterator iter = __pEventMap->begin();
	for(; iter != __pEventMap->end(); iter++) {
		if (iter->first == actionId) {
			EventDelegate del = iter->second;
			result res = (this->*del)(source);
			if (IsFailed(res)) {
				AppLogException("Event handler [%S] for [%d] failed with error: [%s]", source.GetName().GetPointer(), actionId, GetErrorMessage(res));
			}
			triggered = true;
		}
	}
	if (!triggered) {
		AppLogException("Event [%d] is not registered for [%S]", actionId, source.GetName().GetPointer());
	}
}

result BaseForm::Localize(Container *pCont) {
	if (!pCont) {
		AppLogException("Attempt to localize null container");
		return E_INVALID_ARG;
	}

	result res = E_SUCCESS;
	if (typeid(*pCont) == typeid(Form)) {
		Form *pForm = static_cast<Form*>(pCont);
		if (pForm->HasTitle()) {
			pForm->SetTitleText(GetString(pForm->GetName() + L"_TITLE"), ALIGNMENT_CENTER);
		}

		if (pForm->HasSoftkey(SOFTKEY_0)) {
			pForm->SetSoftkeyText(SOFTKEY_0, GetString(pForm->GetName() + L"_SOFTKEY0_TEXT"));
		}
		if (pForm->HasSoftkey(SOFTKEY_1)) {

			pForm->SetSoftkeyText(SOFTKEY_1, GetString(pForm->GetName() + L"_SOFTKEY1_TEXT"));
		}

		res = GetLastResult();
	}

	String str = L"";
	for (int i = 0; i < pCont->GetControlCount(); i++) {
		Control *pCtrl = pCont->GetControl(i);

		bool is_button = typeid(*pCtrl) == typeid(Button);
		bool is_label = typeid(*pCtrl) == typeid(Label);
		bool is_checkbutton = typeid(*pCtrl) == typeid(CheckButton);

		if (is_button || is_label || is_checkbutton) {
			str = GetString(pCtrl->GetName() + String(L"_TEXT"));
			if (!IsFailed(GetLastResult())) {
				if (is_button) {
					static_cast<Button *>(pCtrl)->SetText(str);
				} else if (is_label) {
					static_cast<Label *>(pCtrl)->SetText(str);
				} else if (is_checkbutton) {
					static_cast<CheckButton *>(pCtrl)->SetText(str);
				}
			} else {
				res = GetLastResult();
			}
		}
	}

	AppLogException("Container [%S] was not fully localized. Last error: [%s]", pCont->GetName().GetPointer(), GetErrorMessage(res));
	return res;
}

String BaseForm::GetString(const String &id) {
	String str = id;
	result res = Application::GetInstance()->GetAppResource()->GetString(id, str);
	if (IsFailed(res)) {
		AppLogDebug("Could not retrieve string by id [%S]", id.GetPointer());
		SetLastResult(res);
	}
	return str;
}

DateTime BaseForm::GetLocalDatetimeObject(long long ticks) {
	LocaleManager locMgr;
	result res = locMgr.Construct();

	if (IsFailed(res)) {
		AppLogException("Failed to construct locale manager. Error: [%s]", GetErrorMessage(res));
		SetLastResult(res);
		return DateTime();
	}

	TimeZone tz = locMgr.GetSystemTimeZone();

	GregorianCalendar calendar;
	calendar.Construct();

	res = calendar.SetTimeInMillisecFromEpoch(ticks);

	if (res == E_OUT_OF_RANGE) {
		AppLogException("Specified argument was out of range, expect random behavior");
	}

	DateTime date = tz.UtcTimeToStandardTime(calendar.GetTime());
	if (tz.IsDstUsed()) {
		date.AddHours(1);
	}

	return date;
}

long long BaseForm::GetCurrentTimeInUTCUnixTicks(void) {
	long long ticks;
	SystemTime::GetTicks(ticks);

	return ticks;
}

MessageBoxModalResult BaseForm::ShowMessageBox(const String &title, const String &msg, MessageBoxStyle style) {
	MessageBox *pBox = new MessageBox;
	int mres = 2;
	result res = pBox->Construct(title, msg, style);
	if (!IsFailed(res)) {
		pBox->ShowAndWait(mres);
	}
	SetLastResult(res);
	delete pBox;
	return (MessageBoxModalResult)mres;
}

Bitmap *BaseForm::GetBitmapN(const String &name) {
	if (__sdkVer.major == 1 && __sdkVer.minor == 0) {
		Image *pImage = new Image();
		result res = pImage->Construct();
		if (IsFailed(res)) {
			AppLogException("Failed to construct image decoder, error [%s]", GetErrorMessage(res));
			SetLastResult(res);
			return null;
		}

		String fullname = String(L"/Res/480x800/");
		fullname.Append(name);

		Bitmap *pBitmap = null;
		if(fullname.EndsWith(L"jpg"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_RGB565);
		else if(fullname.EndsWith(L"bmp"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_RGB565);
		else if(fullname.EndsWith(L"png"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_ARGB8888);
		else if (fullname.EndsWith(L"gif"))
			pBitmap = pImage->DecodeN(fullname, BITMAP_PIXEL_FORMAT_RGB565);

		delete pImage;

		if (IsFailed(GetLastResult())) {
			AppLogException("Failed to decode image, error [%s]", GetErrorMessage(GetLastResult()));
			return null;
		}
		return pBitmap;
	} else return Application::GetInstance()->GetAppResource()->GetBitmapN(name);
}

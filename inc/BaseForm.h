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
#ifndef BASEFORM_H_
#define BASEFORM_H_

#include <FBase.h>
#include <FUi.h>
#include <FLocales.h>
#include <FGraphics.h>
#include <map>

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;
using namespace Osp::Locales;
using namespace Osp::Graphics;

struct VersionInfo {
	int major;
	int minor;
};

enum ColorScheme {
	SCHEME_DARK,
	SCHEME_BRIGHT
};

enum DialogResult {
	DIALOG_RESULT_OK,
	DIALOG_RESULT_CANCEL
};

class BaseForm;
typedef result (BaseForm::*EventDelegate)(const Control &src);

struct CallbackInfo {
	int taskID;
	BaseForm *callbackHandler;
};

CallbackInfo CreateCallbackInfo(int taskID, BaseForm *handler);

#define DEF_ACTION(id, val) static const int id = val;
#define HANDLER(x) (static_cast<EventDelegate>(&x))
#define CALLBACK(x) (CreateCallbackInfo(x, this))

class BaseForm: public Form, public IActionEventListener {
public:
	BaseForm(void);
	virtual ~BaseForm(void);

	virtual void DialogCallback(int taskId, BaseForm *sender, DialogResult ret, void *dataN) {}

protected:
	result RegisterButtonPressEvent(Button *pCtrl, int actionId, EventDelegate handler);
	result RegisterAction(int actionId, EventDelegate handler);
	void UnregisterAction(int actionId);

	result ShowInputBox(CallbackInfo cbInfo, KeypadInputModeCategory inputMode, const String &title, const String &msg, const String &acceptButton, const String &cancelButton);

	result RefreshForm(void);

	bool IsInitialized(void) const { return __initialized; }

	String GetLocaleSpecificDatetime(const DateTime &dt);
	String GetLocaleSpecificDatetime(long long ticks);
	String GetLocaleSpecificDatetime(void);

	Bitmap* GetBitmapN(const String &name);

	virtual result Initialize(void) = 0;
	virtual result Terminate(void) = 0;

	virtual void OnRemovedFromFormManager(void) {}
	virtual void OnBecameInactive(void) {}
	virtual void OnBecameActive(void) {}

	static DateTime GetLocalDatetimeObject(long long ticks);
	static long long GetCurrentTimeInUTCUnixTicks(void);

	static MessageBoxModalResult ShowMessageBox(const String &title, const String &msg, MessageBoxStyle style);

	static result Localize(Container *pCont);
	static String GetString(const String &id);

private:
	DEF_ACTION(ID_POPUP_ACCEPT_CLICKED, 999);
	DEF_ACTION(ID_POPUP_CANCEL_CLICKED, 998);

	result OnPopupAccept(const Control &src);
	result OnPopupCancel(const Control &src);

	virtual result OnInitializing(void);
	virtual result OnTerminating(void);

	void OnActionPerformed(const Control &source, int actionId);

	Popup *__pCurrentPopup;
	CallbackInfo __popupCallbackInfo;

	std::map<int, EventDelegate> *__pEventMap;
	bool __initialized;
	VersionInfo __sdkVer;

	DateTime __epoch;
	DateTimeFormatter *__pDTFormatter;

	friend class FormManager;
};

#endif

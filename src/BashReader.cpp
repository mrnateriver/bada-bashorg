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
#include "BashReader.h"
#include "FormManager.h"
#include "MainForm.h"

BashReader::BashReader() {
}

BashReader::~BashReader() {
}

Application *BashReader::CreateInstance(void) {
	return new BashReader();
}

bool BashReader::OnAppInitializing(AppRegistry &appRegistry) {
	MainForm *pMainForm = new MainForm();
	result res = pMainForm->Construct();
	if (IsFailed(res)) {
		AppLogException("Failed to construct main application form, error: [%s]", GetErrorMessage(res));
		return false;
	}

	res = FormManager::SetActiveForm(pMainForm);
	if (IsFailed(res)) {
		AppLogException("Failed to switch to main application form, error: [%s]", GetErrorMessage(res));
		return false;
	}

	return true;
}

bool BashReader::OnAppTerminating(AppRegistry &appRegistry, bool forcedTermination) {
	return true;
}

void BashReader::OnBatteryLevelChanged(BatteryLevel batteryLevel) {
	if (batteryLevel == BATTERY_CRITICAL) {
		String title;
		String msg;
		result sres = Application::GetInstance()->GetAppResource()->GetString(L"APPLICATION_BATTERY_WARNING_TITLE", title);
		if (IsFailed(sres)) {
			AppLogException("Could not retrieve string by id [%S]", String(L"APPLICATION_BATTERY_WARNING_TITLE").GetPointer());
		}
		sres = Application::GetInstance()->GetAppResource()->GetString(L"APPLICATION_BATTERY_WARNING_MSG", msg);
		if (IsFailed(sres)) {
			AppLogException("Could not retrieve string by id [%S]", String(L"APPLICATION_BATTERY_WARNING_MSG").GetPointer());
		}

		MessageBox *pMsg = new MessageBox;
		pMsg->Construct(title, msg, MSGBOX_STYLE_OK);

		int res = -1;
		pMsg->ShowAndWait(res);

		delete pMsg;
	}
}

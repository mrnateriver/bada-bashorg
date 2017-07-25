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

#include "FormManager.h"

using namespace Osp::App;

BaseForm *FormManager::__pActiveForm = null;
BaseForm *FormManager::__pPrevForm = null;

result FormManager::SetActiveForm(BaseForm *pForm, bool release) {
	if (pForm == __pPrevForm) {
		return SetPreviousFormActive(release);
	}
	if (pForm == __pActiveForm) {
		return E_SUCCESS;
	}

	IAppFrame *appFrame = Application::GetInstance()->GetAppFrame();
	if (appFrame) {
		Frame *pFrame = appFrame->GetFrame();
		if (pFrame) {
			if (pForm) {
				if (__pActiveForm) {
					__pActiveForm->OnBecameInactive();
				}

				result res = pFrame->AddControl(*pForm);
				if (IsFailed(res)) {
					AppLogException("Failed to add form to the frame object, error: [%s]", GetErrorMessage(res));
					//well, let's at least try to get things back
					__pActiveForm->OnBecameActive();
					return res;
				}

				res = pFrame->SetCurrentForm(*pForm);
				if (IsFailed(res)) {
					AppLogException("Failed to set current form for application, error: [%s]", GetErrorMessage(res));
					//well, let's at least try to get things back
					pFrame->RemoveControl(*pForm);
					pFrame->SetCurrentForm(*__pActiveForm);
					__pActiveForm->OnBecameActive();
					return res;
				}

				res = pForm->RefreshForm();

				result rr = E_SUCCESS;
				if (__pActiveForm) {
					if (release) {
						__pActiveForm->OnRemovedFromFormManager();

						rr = pFrame->RemoveControl(*__pActiveForm);
					} else if (__pPrevForm) {
						__pPrevForm->OnRemovedFromFormManager();

						rr = pFrame->RemoveControl(*__pPrevForm);
						__pPrevForm = __pActiveForm;
					} else {
						__pPrevForm = __pActiveForm;
					}
				}

				__pActiveForm = pForm;
				__pActiveForm->OnBecameActive();

				//moved here because we still need to trigger events in other forms even if invalidation failed
				if (IsFailed(res)) {
					AppLogException("Failed to invalidate form, error: [%s]", GetErrorMessage(res));
					return res;
				}

				if (IsFailed(rr)) {
					AppLogException("Failed to remove previous form(s) from frame object, thus leaking memory! Error: [%s]", GetErrorMessage(res));
					return rr;
				}
			} else if (__pActiveForm) {
				__pActiveForm->OnBecameInactive();
				__pActiveForm->OnRemovedFromFormManager();

				result res = pFrame->RemoveControl(*__pActiveForm);
				__pActiveForm = null;

				if (IsFailed(res)) {
					AppLogException("Failed to remove previous form(s) from frame object, thus leaking memory! Error: [%s]", GetErrorMessage(res));
					return res;
				}

				if (__pPrevForm) {
					return SetPreviousFormActive();
				}
			}

			return E_SUCCESS;
		}
	}

	AppLogException("Could not retrieve application frame interface");
	return E_INVALID_STATE;
}

result FormManager::SetPreviousFormActive(bool release) {
	if (!__pPrevForm) {
		AppLogException("No previous form set to return to");
		return E_INVALID_STATE;
	}

	IAppFrame *appFrame = Application::GetInstance()->GetAppFrame();
	if (appFrame) {
		Frame *pFrame = appFrame->GetFrame();
		if (pFrame) {
			result res = pFrame->SetCurrentForm(*__pPrevForm);
			if (IsFailed(res)) {
				AppLogException("Failed to set current form for application, error: [%s]", GetErrorMessage(res));
				return res;
			}

			res = __pPrevForm->Draw();
			if (IsFailed(res)) {
				AppLogException("Failed to redraw form, error: [%s]", GetErrorMessage(res));
				return res;
			}

			res = __pPrevForm->Show();
			if (IsFailed(res)) {
				AppLogException("Failed to switch buffers upon redrawing, error: [%s]", GetErrorMessage(res));
				return res;
			}

			if (__pActiveForm) {
				__pActiveForm->OnBecameInactive();
			}

			BaseForm *pTmp = __pActiveForm;

			__pActiveForm = __pPrevForm;
			__pPrevForm = pTmp;

			__pActiveForm->OnBecameActive();

			if (release && __pPrevForm) {
				__pPrevForm->OnRemovedFromFormManager();

				res = pFrame->RemoveControl(*__pPrevForm);
				__pPrevForm = null;

				if (IsFailed(res)) {
					AppLogException("Failed to remove previous form(s) from frame object, thus leaking memory! Error: [%s]", GetErrorMessage(res));
					return res;
				}
			}

			return E_SUCCESS;
		}
	}

	AppLogException("Could not retrieve application frame interface");
	return E_INVALID_STATE;
}

result FormManager::AddFormToFrame(BaseForm *pForm, bool release) {
	if (pForm == __pPrevForm || pForm == __pActiveForm) {
		return E_SUCCESS;
	}

	IAppFrame *appFrame = Application::GetInstance()->GetAppFrame();
	if (appFrame) {
		Frame *pFrame = appFrame->GetFrame();
		if (pFrame) {
			if (pForm) {
				result res = pFrame->AddControl(*pForm);
				if (IsFailed(res)) {
					AppLogException("Failed to add form to the frame object, error: [%s]", GetErrorMessage(res));
					return res;
				}

				if (__pPrevForm) {
					__pPrevForm->OnRemovedFromFormManager();
					if (release) {
						res = pFrame->RemoveControl(*__pPrevForm);
					}
				}
				__pPrevForm = pForm;

				if (IsFailed(res)) {
					AppLogException("Failed to remove previous form from the frame object, error: [%s]", GetErrorMessage(res));
					return res;
				}

				return E_SUCCESS;
			} else return E_INVALID_ARG;
		}
	}

	AppLogException("Could not retrieve application frame interface");
	return E_INVALID_STATE;
}

result FormManager::RemoveAllForms(void) {
	IAppFrame *appFrame = Application::GetInstance()->GetAppFrame();
	if (appFrame) {
		Frame *pFrame = appFrame->GetFrame();
		if (pFrame) {
			pFrame->RemoveAllControls();

			if (__pActiveForm) {
				__pActiveForm->OnBecameInactive();
				__pActiveForm->OnRemovedFromFormManager();
			}

			if (__pPrevForm) {
				__pPrevForm->OnBecameInactive();
				__pPrevForm->OnRemovedFromFormManager();
			}

			__pActiveForm = __pPrevForm = null;
			return E_SUCCESS;
		}
	}

	AppLogException("Could not retrieve application frame interface");
	return E_INVALID_STATE;
}

BaseForm *FormManager::GetActiveForm(void) {
	return __pActiveForm;
}

BaseForm *FormManager::GetPreviousForm(void) {
	return __pPrevForm;
}

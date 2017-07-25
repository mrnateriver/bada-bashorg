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
#ifndef FORMMANAGER_H_
#define FORMMANAGER_H_

#include "BaseForm.h"

class FormManager {
public:
	static result SetActiveForm(BaseForm *pForm, bool release = false);
	static result SetPreviousFormActive(bool release = false);
	static result AddFormToFrame(BaseForm *pForm, bool release = false);

	static result RemoveAllForms(void);

	static BaseForm *GetActiveForm(void);
	static BaseForm *GetPreviousForm(void);

private:
	static BaseForm *__pActiveForm;
	static BaseForm *__pPrevForm;
};

#endif

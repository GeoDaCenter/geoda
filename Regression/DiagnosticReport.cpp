/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 * 
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/msgdlg.h>
#include "mix.h"
#include "DiagnosticReport.h"

DiagnosticReport::DiagnosticReport(long obs, int nvar,
								   bool inclconst, bool w, int m)
: nObs(obs), nVar(nvar), inclConstant(inclconst), model(m), hasWeight(w)
{
	if (Allocate()) {
		SetDiagStatus(false);
	}
	return;
}

DiagnosticReport::~DiagnosticReport()
{

}

bool DiagnosticReport::Allocate()
{
	if (nObs < nVar || nVar < 1) return false;

	varNames.resize(nVar+1);
	typedef double* double_ptr_type;
	cov	= new double_ptr_type[nVar+3]; 
	for (int i=0;i<nVar;i++) {
		cov[i] = new double[nVar+3];
	}
	
	coeff = new double[nVar+1];
	sterr = new double[nVar+1];
	stats = new double[nVar+1];
	probs = new double[nVar+1];
	bptest= new double[6];
	alloc(resid, nObs+1);
	yhat	= new double[nObs+1];
	eigval	= new double[nVar];

	if (resid == NULL) {
		wxMessageBox("Not enough memory!");
		return false;
	}

	if (model==2) // lag
	{
		rho = new double[4];
		sbptest = new double[3];
		lr_test = new double[3];
		lm_test = new double[3];
		prederr	= new double[nObs];
	}
	else if(model==3) // error
	{
		lambda = new double[4];
		sbptest = new double[3];
		lr_test = new double[3];
		lm_test = new double[3];
		lrcf_test = new double[3];
		wald_test = new double[3];
		prederr	= new double[nObs];
	}
	else // ols
	{
		moranI= new double[3];
		jbtest= new double[3];
		kbtest= new double[3];
		white = new double[3];
		if (hasWeight) {
			lmlag = new double[3];
			lmerr = new double[3];
			lmlagr = new double[3];
			lmerrr = new double[3];
			lmsarma = new double[3];
			kelrob = new double[3];
		}
	}
	return true;

}

void DiagnosticReport::SetDiagStatus(bool status)
{
	diagStatus = status;
}

void DiagnosticReport::release_Var()
{
	for (int i=0;i<nVar;i++) {
		release(&cov[i]);
	}
	release(&cov);
		
	release(&coeff);
	release(&sterr);
	release(&stats);
	release(&probs);
	release(&bptest);
	release(&eigval);

	if (model==2) // lag
	{
		release(&rho);
		release(&sbptest);
		release(&lr_test);
		release(&lm_test);
	}
	else if(model==3) // error
	{
		release(&lambda);
		release(&sbptest);
		release(&lr_test);
		release(&lm_test);
		release(&lrcf_test);
		release(&wald_test);
	}
	else // ols
	{
		release(&moranI);
		release(&jbtest);
		release(&kbtest);
		release(&white);
		if (hasWeight)
		{
			release(&lmlag);
			release(&lmerr);
			release(&lmlagr);
			release(&lmerrr);
			release(&lmsarma);
			release(&kelrob);
		}
	}
}

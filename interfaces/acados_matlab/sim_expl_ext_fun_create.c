// system
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// acados
#include "acados/utils/external_function_generic.h"
#include "acados_c/external_function_interface.h"
// mex
#include "mex.h"



// casadi functions for the model
#include "sim_model.h"



void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
	{

//	mexPrintf("\nin sim_expl_ext_fun_create\n");

	// sizeof(long long) == sizeof(void *) = 64 !!!
	long long *ptr;



	/* RHS */

	// opts

	// TODO use them !!!
	bool sens_forw = mxGetScalar( mxGetField( prhs[0], 0, "sens_forw" ) );
//	mexPrintf("\n%d\n", sens_forw);
	char *method = mxArrayToString( mxGetField( prhs[0], 0, "method" ) );
//	mexPrintf("\n%s\n", method);



	/* LHS */

	// field names of output struct
	char *fieldnames[6];
	fieldnames[0] = (char*)mxMalloc(50);
	fieldnames[1] = (char*)mxMalloc(50);
	fieldnames[2] = (char*)mxMalloc(50);
	fieldnames[3] = (char*)mxMalloc(50);
	fieldnames[4] = (char*)mxMalloc(50);
	fieldnames[5] = (char*)mxMalloc(50);

	memcpy(fieldnames[0],"expl_ode_fun",sizeof("expl_ode_fun"));
	memcpy(fieldnames[1],"expl_vde_for",sizeof("expl_vde_for"));
	memcpy(fieldnames[2],"expl_vde_adj",sizeof("expl_vde_adj"));
	memcpy(fieldnames[3],"impl_ode_fun",sizeof("impl_ode_fun"));
	memcpy(fieldnames[4],"impl_ode_fun_jac_x_xdot",sizeof("impl_ode_fun_jac_x_xdot"));
	memcpy(fieldnames[5],"impl_ode_jac_x_xdot_u",sizeof("impl_ode_jac_x_xdot_u"));

	// create output struct
	plhs[0] = mxCreateStructMatrix(1, 1, 6, (const char **) fieldnames);

	mxFree( fieldnames[0] );
	mxFree( fieldnames[1] );
	mxFree( fieldnames[2] );
	mxFree( fieldnames[3] );
	mxFree( fieldnames[4] );
	mxFree( fieldnames[5] );



	external_function_casadi *ext_fun_ptr;

	// TODO templetize the casadi function names !!!
	if(!strcmp(method, "erk"))
		{
		// expl_ode_fun
		ext_fun_ptr = (external_function_casadi *) malloc(1*sizeof(external_function_casadi));
		external_function_casadi_set_fun(ext_fun_ptr, &sim_model_expl_ode_fun);
		external_function_casadi_set_work(ext_fun_ptr, &sim_model_expl_ode_fun_work);
		external_function_casadi_set_sparsity_in(ext_fun_ptr, &sim_model_expl_ode_fun_sparsity_in);
		external_function_casadi_set_sparsity_out(ext_fun_ptr, &sim_model_expl_ode_fun_sparsity_out);
		external_function_casadi_set_n_in(ext_fun_ptr, &sim_model_expl_ode_fun_n_in);
		external_function_casadi_set_n_out(ext_fun_ptr, &sim_model_expl_ode_fun_n_out);
		external_function_casadi_create(ext_fun_ptr);
		// populate output struct
		mxArray *expl_ode_fun_mat  = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
		ptr = mxGetData(expl_ode_fun_mat);
		ptr[0] = (long long) ext_fun_ptr;
		mxSetField(plhs[0], 0, "expl_ode_fun", expl_ode_fun_mat);

		// expl_vde_for
		ext_fun_ptr = (external_function_casadi *) malloc(1*sizeof(external_function_casadi));
		external_function_casadi_set_fun(ext_fun_ptr, &sim_model_expl_vde_for);
		external_function_casadi_set_work(ext_fun_ptr, &sim_model_expl_vde_for_work);
		external_function_casadi_set_sparsity_in(ext_fun_ptr, &sim_model_expl_vde_for_sparsity_in);
		external_function_casadi_set_sparsity_out(ext_fun_ptr, &sim_model_expl_vde_for_sparsity_out);
		external_function_casadi_set_n_in(ext_fun_ptr, &sim_model_expl_vde_for_n_in);
		external_function_casadi_set_n_out(ext_fun_ptr, &sim_model_expl_vde_for_n_out);
		external_function_casadi_create(ext_fun_ptr);
		// populate output struct
		mxArray *expl_vde_for_mat  = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
		ptr = mxGetData(expl_vde_for_mat);
		ptr[0] = (long long) ext_fun_ptr;
		mxSetField(plhs[0], 0, "expl_vde_for", expl_vde_for_mat);
		}
	else
		{
		mexPrintf("\nsim_expl_ext_fun_create: method not supported %s\n", method);
		return;
		}
	
	return;

	}




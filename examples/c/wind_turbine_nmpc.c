/*
 *    This file is part of acados.
 *
 *    acados is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    acados is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with acados; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// #include <xmmintrin.h>

#include "blasfeo/include/blasfeo_target.h"
#include "blasfeo/include/blasfeo_common.h"
#include "blasfeo/include/blasfeo_d_aux_ext_dep.h"
#include "blasfeo/include/blasfeo_i_aux_ext_dep.h"

#include "acados_c/external_function_interface.h"
#include "acados_c/ocp_nlp_interface.h"

// TODO(dimitris): use only the strictly necessary includes here

#include "acados/utils/mem.h"
#include "acados/utils/print.h"
#include "acados/utils/timing.h"
#include "acados/utils/types.h"

#include "acados/ocp_qp/ocp_qp_partial_condensing_solver.h"

#include "acados/ocp_nlp/ocp_nlp_sqp.h"
#include "acados/ocp_nlp/ocp_nlp_cost_common.h"
#include "acados/ocp_nlp/ocp_nlp_cost_ls.h"
#include "acados/ocp_nlp/ocp_nlp_cost_nls.h"
#include "acados/ocp_nlp/ocp_nlp_cost_external.h"
#include "acados/ocp_nlp/ocp_nlp_dynamics_cont.h"

#include "examples/c/wt_model_nx6_expl/wt_model.h"
#include "examples/c/wt_model_nx6_expl/setup.c"

#define NN 40

#define MAX_SQP_ITERS 10
#define NREP 1


static void shift_states(ocp_nlp_dims *dims, ocp_nlp_out *out, double *x_end)
{
	int N = dims->N;

    for (int i = 0; i < N; i++)
 		blasfeo_dveccp(dims->nx[i], &out->ux[i], dims->nu[i], &out->ux[i+1], dims->nu[i+1]);
 	blasfeo_pack_dvec(dims->nx[N], x_end, &out->ux[N], dims->nu[N]);
}



static void shift_controls(ocp_nlp_dims *dims, ocp_nlp_out *out, double *u_end)
{
	int N = dims->N;

    for (int i = 0; i < N-1; i++)
 		blasfeo_dveccp(dims->nu[i], &out->ux[i], 0, &out->ux[i+1], 0);
 	blasfeo_pack_dvec(dims->nu[N-1], u_end, &out->ux[N-1], 0);
}



static void select_dynamics_wt_casadi(int N, external_function_casadi *forw_vde)
{
	for (int ii = 0; ii < N; ii++)
	{
		forw_vde[ii].casadi_fun = &expl_forw_vde;
		forw_vde[ii].casadi_work = &expl_forw_vde_work;
		forw_vde[ii].casadi_sparsity_in = &expl_forw_vde_sparsity_in;
		forw_vde[ii].casadi_sparsity_out = &expl_forw_vde_sparsity_out;
		forw_vde[ii].casadi_n_in = &expl_forw_vde_n_in;
		forw_vde[ii].casadi_n_out = &expl_forw_vde_n_out;
	}
}



/************************************************
* nonlinear constraint
************************************************/

void ext_fun_h1(void *fun, double *in, double *out)
{

	int ii;

	int nu = 3;
	int nx = 8;
	int nh = 1;

	// x
	double *x = in+0;
	// u
	// double *u = in+nx;

	// scaling
	double alpha = 0.944*97/100;

	// h
	double *h = out+0;
	h[0] = alpha * x[0] * x[5];

	// J
	double *J = out+nh;
	for (ii=0; ii<nh*(nu+nx); ii++)
		J[ii] = 0.0;
	J[0] = alpha * x[5];
	J[5] = alpha * x[0];

	return;

}



/************************************************
* main
************************************************/

int main()
{
    // _MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);

	int nx_ = 8;
    int nu_ = 3;
	int ny_ = 5;

    /************************************************
    * problem dimensions
    ************************************************/

    int nx[NN + 1] = {0};
    int nu[NN + 1] = {0};
    int nbx[NN + 1] = {0};
    int nbu[NN + 1] = {0};
    int nb[NN + 1] = {0};
    int ng[NN + 1] = {0};
    int nh[NN + 1] = {0};
    int nq[NN + 1] = {0};
    int ns[NN+1] = {0};
	int ny[NN+1] = {0};

	// TODO(dimitris): setup bounds on states and controls based on ACADO controller
    nx[0] = nx_;
    nu[0] = nu_;
    nbx[0] = nx_;
    nbu[0] = nu_;
    nb[0] = nbu[0]+nbx[0];
	ng[0] = 0;

	// TODO(dimitris): add bilinear constraints later
	nh[0] = 0;
	ny[0] = 5; // ny_

    for (int i = 1; i < NN; i++)
    {
        nx[i] = nx_;
        nu[i] = nu_;
        nbx[i] = 3;
        nbu[i] = nu_;
		nb[i] = nbu[i]+nbx[i];
		ng[i] = 0;
		nh[i] = 0; // 1;
		ny[i] = 5; // ny_
    }

    nx[NN] = nx_;
    nu[NN] = 0;
    nbx[NN] = 3;
    nbu[NN] = 0;
    nb[NN] = nbu[NN]+nbx[NN];
	ng[NN] = 0;
	nh[NN] = 0;
	ny[NN] = 2;

    /************************************************
    * problem data
    ************************************************/

    double *x_end = malloc(sizeof(double)*nx_);
    double *u_end = malloc(sizeof(double)*nu_);

	// value of last stage when shifting states and controls
	for (int i = 0; i < nx_; i++) x_end[i] = 0.0;
	for (int i = 0; i < nu_; i++) u_end[i] = 0.0;



	/* constraints */

	// pitch angle rate
	double dbeta_min = - 8.0;
	double dbeta_max =   8.0;
	// generator torque
	double dM_gen_min = - 1.0;
	double dM_gen_max =   1.0;
	// generator angular velocity
	double OmegaR_min =  6.0/60*2*3.14159265359;
	double OmegaR_max = 13.0/60*2*3.14159265359;
	// pitch angle
	double beta_min =  0.0;
	double beta_max = 35.0;
	// generator torque
	double M_gen_min = 0.0;
	double M_gen_max = 5.0;
	// electric power
	double Pel_min = 0.0;
	double Pel_max = 5.0;

	/* box constraints */

	// acados inf
	double acados_inf = 1e8;

	// first stage
	int *idxb0 = malloc(nb[0]*sizeof(int));
	double *lb0 = malloc((nb[0])*sizeof(double));
	double *ub0 = malloc((nb[0])*sizeof(double));

	// pitch angle rate
	idxb0[0] = 0;
	lb0[0] = dbeta_min;
	ub0[0] = dbeta_max;

	// generator torque
	idxb0[1] = 1;
	lb0[1] = dM_gen_min;
	ub0[1] = dM_gen_max;

	// disturbance as input
	idxb0[2] = 2;
	lb0[2] = 12.0; // XXX dummy
	ub0[2] = 12.0; // XXX dummy

	// dummy state bounds
	for (int ii=0; ii<nbx[0]; ii++)
	{
		idxb0[nbu[0]+ii] = nbu[0]+ii;
		lb0[nbu[0]+ii] = - acados_inf;
		ub0[nbu[0]+ii] =   acados_inf;
	}


	// middle stages
	int *idxb1 = malloc(nb[1]*sizeof(int));
	double *lb1 = malloc((nb[1])*sizeof(double));
	double *ub1 = malloc((nb[1])*sizeof(double));

	// pitch angle rate
	idxb1[0] = 0;
	lb1[0] = dbeta_min;
	ub1[0] = dbeta_max;

	// generator torque rate
	idxb1[1] = 1;
	lb1[1] = dM_gen_min;
	ub1[1] = dM_gen_max;

	// disturbance as input
	// TODO(dimitris): isn't this replaced online anyway?
	idxb1[2] = 2;
	lb1[2] = 12.0; // XXX dummy
	ub1[2] = 12.0; // XXX dummy

	// generator angular velocity
	idxb1[3] = 3;
	lb1[3] = OmegaR_min;
	ub1[3] = OmegaR_max;

	// pitch angle
	idxb1[4] = 9;
	lb1[4] = beta_min;
	ub1[4] = beta_max;

	// generator torque
	idxb1[5] = 10;
	lb1[5] = M_gen_min;
	ub1[5] = M_gen_max;

	// last stage
	int *idxbN = malloc(nb[NN]*sizeof(int));
	double *lbN = malloc((nb[NN])*sizeof(double));
	double *ubN = malloc((nb[NN])*sizeof(double));

	// generator angular velocity
	idxbN[0] = 0;
	lbN[0] = OmegaR_min;
	ubN[0] = OmegaR_max;

	// pitch angle
	idxbN[1] = 6;
	lbN[1] = beta_min;
	ubN[1] = beta_max;

	// generator torque
	idxbN[2] = 7;
	lbN[2] = M_gen_min;
	ubN[2] = M_gen_max;

#if 0
	int_print_mat(1, nb[0], idxb0, 1);
	d_print_mat(1, nb[0], lb0, 1);
	d_print_mat(1, nb[0], ub0, 1);
	int_print_mat(1, nb[1], idxb1, 1);
	d_print_mat(1, nb[1], lb1, 1);
	d_print_mat(1, nb[1], ub1, 1);
	int_print_mat(1, nb[NN], idxbN, 1);
	d_print_mat(1, nb[NN], lbN, 1);
	d_print_mat(1, nb[NN], ubN, 1);
	exit(1);
#endif



	/* nonlinear constraints */

	// middle stages
	external_function_generic h1;
	double *lh1;
	double *uh1;
	if (nh[1]>0)
	{
		h1.evaluate = &ext_fun_h1;

		lh1 = malloc((nh[1])*sizeof(double));
		uh1 = malloc((nh[1])*sizeof(double));

		// electric power
		lh1[0] = Pel_min;
		uh1[0] = Pel_max;
	}



	/* linear least squares */

	// output definition
	// y = {x[0], x[4]; u[0]; u[1]; u[2]};
	//   = Vx * x + Vu * u

	double *Vx = malloc((ny_*nx_)*sizeof(double));
	for (int ii=0; ii<ny_*nx_; ii++)
		Vx[ii] = 0.0;
	Vx[0+ny_*0] = 1.0;
	Vx[1+ny_*4] = 1.0;

	double *Vu = malloc((ny_*nu_)*sizeof(double));
	for (int ii=0; ii<ny_*nu_; ii++)
		Vu[ii] = 0.0;
	Vu[2+ny_*0] = 1.0;
	Vu[3+ny_*1] = 1.0;
	Vu[4+ny_*2] = 1.0;

	double *W = malloc((ny_*ny_)*sizeof(double));
	for (int ii=0; ii<ny_*ny_; ii++)
		W[ii] = 0.0;
	W[0+ny_*0] = 1.5114;
	W[1+ny_*0] = -0.0649;
	W[0+ny_*1] = -0.0649;
	W[1+ny_*1] = 0.0180;
	W[2+ny_*2] = 0.01;
	W[3+ny_*3] = 0.001;
	W[4+ny_*4] = 0.00001;  // small weight on wind speed to ensure pos. definiteness

#if 0
	d_print_mat(ny_, nx_, Vx, ny_);
	d_print_mat(ny_, nu_, Vu, ny_);
	d_print_mat(ny_, ny_, W, ny_);
//	exit(1);
#endif

    /************************************************
    * plan + config
    ************************************************/

	ocp_nlp_solver_plan *plan = ocp_nlp_plan_create(NN);

	plan->nlp_solver = SQP_GN;

	for (int i = 0; i <= NN; i++)
		plan->nlp_cost[i] = LINEAR_LS;

	plan->ocp_qp_solver_plan.qp_solver = PARTIAL_CONDENSING_HPIPM;
//	plan->ocp_qp_solver_plan.qp_solver = FULL_CONDENSING_HPIPM;
//	plan->ocp_qp_solver_plan.qp_solver = FULL_CONDENSING_QPOASES;

	for (int i = 0; i < NN; i++)
	{
		plan->nlp_dynamics[i] = CONTINUOUS_MODEL;
		plan->sim_solver_plan[i].sim_solver = ERK;
	}

	ocp_nlp_solver_config *config = ocp_nlp_config_create(*plan, NN);

    /************************************************
    * ocp_nlp_dims
    ************************************************/

	ocp_nlp_dims *dims = ocp_nlp_dims_create(config);
	ocp_nlp_dims_initialize(config, nx, nu, ny, nbx, nbu, ng, nh, ns, nq, dims);

    /************************************************
    * dynamics
    ************************************************/

	external_function_casadi *forw_vde_casadi = malloc(NN*sizeof(external_function_casadi));

	select_dynamics_wt_casadi(NN, forw_vde_casadi);

	external_function_casadi_create_array(NN, forw_vde_casadi);

    /************************************************
    * nlp_in
    ************************************************/

	ocp_nlp_in *nlp_in = ocp_nlp_in_create(config, dims);

	// sampling times
	for (int ii=0; ii<NN; ii++)
	{
    	nlp_in->Ts[ii] = 0.2;
	}

	// output definition: y = [x; u]

	/* cost */
	ocp_nlp_cost_ls_model *stage_cost_ls;

	for (int i = 0; i <= NN; i++)
	{
		switch (plan->nlp_cost[i])
		{
			case LINEAR_LS:

				stage_cost_ls = (ocp_nlp_cost_ls_model *) nlp_in->cost[i];

				// Cyt
				blasfeo_pack_tran_dmat(ny[i], nu[i], Vu, ny_, &stage_cost_ls->Cyt, 0, 0);
				blasfeo_pack_tran_dmat(ny[i], nx[i], Vx, ny_, &stage_cost_ls->Cyt, nu[i], 0);

				// W
				blasfeo_pack_dmat(ny[i], ny[i], W, ny_, &stage_cost_ls->W, 0, 0);

//				blasfeo_print_dmat(nu[i]+nx[i], ny[i], &stage_cost_ls->Cyt, 0, 0);
//				blasfeo_print_dmat(ny[i], ny[i], &stage_cost_ls->W, 0, 0);
				break;
			default:
				break;
		}
	}

	/* dynamics */

	int set_fun_status;

	for (int i=0; i<NN; i++)
	{
		set_fun_status = nlp_set_model_in_stage(config, nlp_in, i, "forward_vde", &forw_vde_casadi[i]);
		if (set_fun_status != 0) exit(1);
		// set_fun_status = nlp_set_model_in_stage(config, nlp_in, i, "explicit_jacobian", &jac_ode_casadi[i]);
		// if (set_fun_status != 0) exit(1);
	}

    nlp_in->freezeSens = false;

    /* constraints */

	ocp_nlp_constraints_model **constraints = (ocp_nlp_constraints_model **) nlp_in->constraints;

	/* box constraints */

	// fist stage
	blasfeo_pack_dvec(nb[0], lb0, &constraints[0]->d, 0);
	blasfeo_pack_dvec(nb[0], ub0, &constraints[0]->d, nb[0]+ng[0]+nh[0]);
    for (int ii=0; ii<nb[0]; ii++) constraints[0]->idxb[ii] = idxb0[ii];
	// middle stages
    for (int i = 1; i < NN; i++)
	{
		blasfeo_pack_dvec(nb[i], lb1, &constraints[i]->d, 0);
		blasfeo_pack_dvec(nb[i], ub1, &constraints[i]->d, nb[i]+ng[i]+nh[i]);
		for (int ii=0; ii<nb[i]; ii++) constraints[i]->idxb[ii] = idxb1[ii];
    }
	// last stage
	blasfeo_pack_dvec(nb[NN], lbN, &constraints[NN]->d, 0);
	blasfeo_pack_dvec(nb[NN], ubN, &constraints[NN]->d, nb[NN]+ng[NN]+nh[NN]);
    for (int ii=0; ii<nb[NN]; ii++) constraints[NN]->idxb[ii] = idxbN[ii];

	/* nonlinear constraints */

	// middle stages
    for (int i = 1; i < NN; i++)
	{
		if(nh[i]>0)
		{
			blasfeo_pack_dvec(nh[i], lh1, &constraints[i]->d, nb[i]+ng[i]);
			blasfeo_pack_dvec(nh[i], uh1, &constraints[i]->d, 2*nb[i]+2*ng[i]+nh[i]);
			constraints[i]->h = &h1;
		}
    }

    /************************************************
    * sqp opts
    ************************************************/

	void *nlp_opts = ocp_nlp_opts_create(config, dims);
	ocp_nlp_sqp_opts *sqp_opts = (ocp_nlp_sqp_opts *) nlp_opts;

    for (int i = 0; i < NN; ++i)
	{
		ocp_nlp_dynamics_cont_opts *dynamics_stage_opts = sqp_opts->dynamics[i];
        sim_rk_opts *sim_opts = dynamics_stage_opts->sim_solver;

		if (plan->sim_solver_plan[i].sim_solver == ERK)
		{
			sim_opts->ns = 4;
			sim_opts->num_steps = 10;
		}
		else if (plan->sim_solver_plan[i].sim_solver == LIFTED_IRK)
		{
			sim_opts->ns = 2;
		}
		else if (plan->sim_solver_plan[i].sim_solver == IRK)
		{
			sim_opts->ns = 2;
			sim_opts->jac_reuse = true;
		}
    }

    sqp_opts->maxIter = MAX_SQP_ITERS;
    sqp_opts->min_res_g = 1e-6;
    sqp_opts->min_res_b = 1e-6;
    sqp_opts->min_res_d = 1e-6;
    sqp_opts->min_res_m = 1e-6;

	// // partial condensing
	// if (plan->ocp_qp_solver_plan.qp_solver == PARTIAL_CONDENSING_HPIPM)
	// {
	// 	ocp_nlp_sqp_opts *sqp_opts = nlp_opts;
	// 	ocp_qp_partial_condensing_solver_opts *pcond_solver_opts = sqp_opts->qp_solver_opts;
	// 	pcond_solver_opts->pcond_opts->N2 = 10;
	// }

	// update after user-defined opts
	config->opts_update(config, dims, nlp_opts);

    /************************************************
    * ocp_nlp out
    ************************************************/

	ocp_nlp_out *nlp_out = ocp_nlp_out_create(config, dims);

	ocp_nlp_solver *solver = ocp_nlp_create(config, dims, nlp_opts);

    /************************************************
    * sqp solve
    ************************************************/

	int nmpc_problems = 40;

    int status;

    acados_timer timer;
    acados_tic(&timer);

    for (int rep = 0; rep < NREP; rep++)
    {
		// warm start output initial guess of solution
		for (int i=0; i<=NN; i++)
		{
			blasfeo_pack_dvec(2, u0_ref, nlp_out->ux+i, 0);
			blasfeo_pack_dvec(1, wind0_ref+i, nlp_out->ux+i, 2);
			blasfeo_pack_dvec(nx[i], x0_ref, nlp_out->ux+i, nu[i]);
		}

		// update x0 as box constraint
		blasfeo_pack_dvec(nx[0], x0_ref, &constraints[0]->d, nbu[0]);
		blasfeo_pack_dvec(nx[0], x0_ref, &constraints[0]->d, nb[0]+ng[0]+nbu[0]);

   	 	for (int idx = 0; idx < nmpc_problems; idx++)
		{
			// update wind disturbance as box constraint
			for (int ii=0; ii<NN; ii++)
			{
				BLASFEO_DVECEL(&constraints[ii]->d, 2) = wind0_ref[idx + ii];
				BLASFEO_DVECEL(&constraints[ii]->d, nb[ii]+ng[ii]+2) = wind0_ref[idx + ii];
			}

			// update reference
			for (int i = 0; i <= NN; i++)
			{
				stage_cost_ls = nlp_in->cost[i];

				BLASFEO_DVECEL(&stage_cost_ls->y_ref, 0) = y_ref[(idx + i)*4+0];
				BLASFEO_DVECEL(&stage_cost_ls->y_ref, 1) = y_ref[(idx + i)*4+1];
				if (i < NN)
				{
					BLASFEO_DVECEL(&stage_cost_ls->y_ref, 2) = y_ref[(idx + i)*4+2];
					BLASFEO_DVECEL(&stage_cost_ls->y_ref, 3) = y_ref[(idx + i)*4+3];
					BLASFEO_DVECEL(&stage_cost_ls->y_ref, 4) = wind0_ref[idx + i];
				}
			}

			// solve NLP
        	status = ocp_nlp_solve(solver, nlp_in, nlp_out);

			// update initial condition
			// TODO(dimitris): maybe simulate system instead of passing x[1] as next state
			// blasfeo_print_tran_dvec(nb[0], &constraints[0]->d, 0);
			// blasfeo_print_tran_dvec(nb[0], &constraints[0]->d, nb[0]+ng[0]);
			blasfeo_dveccp(nx_, &nlp_out->ux[1], nu_, &constraints[0]->d, nbu[0]);
			blasfeo_dveccp(nx_, &nlp_out->ux[1], nu_, &constraints[0]->d, nbu[0]+nb[0]+ng[0]);

			// blasfeo_print_tran_dvec(nb[0], &constraints[0]->d, 0);
			// blasfeo_print_tran_dvec(nb[0], &constraints[0]->d, nb[0]+ng[0]);

			// shift trajectories
			if (true)
			{
				blasfeo_unpack_dvec(dims->nx[NN], &nlp_out->ux[NN-1], dims->nu[NN-1], x_end);
				blasfeo_unpack_dvec(dims->nu[NN-1], &nlp_out->ux[NN-2], dims->nu[NN-2], u_end);

				shift_states(dims, nlp_out, x_end);
				shift_controls(dims, nlp_out, u_end);
			}
			// print info
			if (true)
			{
				printf("\nproblem #%d, status %d, iters %d\n", idx, status, ((ocp_nlp_sqp_memory *)solver->mem)->sqp_iter);
				printf("xsim = \n");
				blasfeo_print_tran_dvec(dims->nx[0], &nlp_out->ux[0], dims->nu[0]);
			}

			if (status!=0)
			{
				if (!(status == 1 && MAX_SQP_ITERS == 1))  // if not RTI
				{
					printf("\nresiduals\n");
					ocp_nlp_res_print(dims, ((ocp_nlp_sqp_memory *)solver->mem)->nlp_res);
					exit(1);
				}
			}

		}

    }

    double time = acados_toc(&timer)/NREP;

    printf("\n\ntotal time (including printing) = %f ms (time per SQP = %f)\n\n", time*1e3, time*1e3/nmpc_problems);


    /************************************************
    * free memory
    ************************************************/

	// TODO(dimitris): VALGRIND!
 	external_function_casadi_free(forw_vde_casadi);
	free(forw_vde_casadi);

	free(nlp_opts);
	free(nlp_in);
	free(nlp_out);
	free(solver);
	free(dims);
	free(config);
	free(plan);

	free(lb0);
	free(ub0);
	free(lb1);
	free(ub1);
	free(lbN);
	free(ubN);
	free(idxb0);
	free(idxb1);
	free(idxbN);

	free(x_end);
	free(u_end);

	/************************************************
	* return
	************************************************/

	if (status == 0 || (status == 1 && MAX_SQP_ITERS == 1))
		printf("\nsuccess!\n\n");
	else
		printf("\nfailure!\n\n");

	return 0;
}

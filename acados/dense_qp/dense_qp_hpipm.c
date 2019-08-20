/*
 * Copyright 2019 Gianluca Frison, Dimitris Kouzoupis, Robin Verschueren,
 * Andrea Zanelli, Niels van Duijkeren, Jonathan Frey, Tommaso Sartor,
 * Branimir Novoselnik, Rien Quirynen, Rezart Qelibari, Dang Doan,
 * Jonas Koenemann, Yutao Chen, Tobias Schöls, Jonas Schlagenhauf, Moritz Diehl
 *
 * This file is part of acados.
 *
 * The 2-Clause BSD License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.;
 */


// external
#include <stdlib.h>
#include <assert.h>
#include <string.h>
// hpipm
#include "hpipm/include/hpipm_d_dense_qp.h"
#include "hpipm/include/hpipm_d_dense_qp_ipm.h"
#include "hpipm/include/hpipm_d_dense_qp_sol.h"
// acados
#include "acados/dense_qp/dense_qp_common.h"
#include "acados/dense_qp/dense_qp_hpipm.h"
#include "acados/utils/mem.h"
#include "acados/utils/timing.h"



/************************************************
 * opts
 ************************************************/

int dense_qp_hpipm_opts_calculate_size(void *config_, void *dims_)
{
    dense_qp_dims *dims = dims_;

    int size = 0;
    size += sizeof(dense_qp_hpipm_opts);
    size += sizeof(struct d_dense_qp_ipm_arg);
    size += d_memsize_dense_qp_ipm_arg(dims);

    return size;
}



void *dense_qp_hpipm_opts_assign(void *config_, void *dims_, void *raw_memory)
{
    dense_qp_dims *dims = dims_;
    dense_qp_hpipm_opts *opts;

    char *c_ptr = (char *) raw_memory;

    opts = (dense_qp_hpipm_opts *) c_ptr;
    c_ptr += sizeof(dense_qp_hpipm_opts);

    opts->hpipm_opts = (struct d_dense_qp_ipm_arg *) c_ptr;
    c_ptr += sizeof(struct d_dense_qp_ipm_arg);

    assert((size_t) c_ptr % 8 == 0 && "memory not 8-byte aligned!");

    d_create_dense_qp_ipm_arg(dims, opts->hpipm_opts, c_ptr);
    c_ptr += d_memsize_dense_qp_ipm_arg(dims);

    assert((char *) raw_memory + dense_qp_hpipm_opts_calculate_size(config_, dims) == c_ptr);

    return (void *) opts;
}



void dense_qp_hpipm_opts_initialize_default(void *config_, void *dims_, void *opts_)
{
    dense_qp_hpipm_opts *opts = opts_;

    d_set_default_dense_qp_ipm_arg(BALANCE, opts->hpipm_opts);
    // overwrite some default options
    opts->hpipm_opts->res_g_max = 1e-6;
    opts->hpipm_opts->res_b_max = 1e-8;
    opts->hpipm_opts->res_d_max = 1e-8;
    opts->hpipm_opts->res_m_max = 1e-8;
    opts->hpipm_opts->iter_max = 50;
    opts->hpipm_opts->stat_max = 50;
    opts->hpipm_opts->alpha_min = 1e-8;
    opts->hpipm_opts->mu0 = 1e0;

    return;
}



void dense_qp_hpipm_opts_update(void *config_, void *dims_, void *opts_)
{
    //    dense_qp_hpipm_opts *opts = (dense_qp_hpipm_opts *)opts_;

    return;
}



void dense_qp_hpipm_opts_set(void *config_, void *opts_, const char *field, void *value)
{
    dense_qp_hpipm_opts *opts = opts_;

    if (!strcmp(field, "iter_max"))
    {
        int* tmp_ptr = (int *) value;
		d_set_dense_qp_ipm_arg_iter_max(*tmp_ptr, opts->hpipm_opts);
    }
    else if (!strcmp(field, "mu0"))
    {
        double* tmp_ptr = (double *) value;
		d_set_dense_qp_ipm_arg_mu0(*tmp_ptr, opts->hpipm_opts);
    }
    else if (!strcmp(field, "tol_stat"))
    {
        double* tmp_ptr = (double *) value;
		d_set_dense_qp_ipm_arg_tol_stat(*tmp_ptr, opts->hpipm_opts);
    }
    else if (!strcmp(field, "tol_eq"))
    {
        double* tmp_ptr = (double *) value;
		d_set_dense_qp_ipm_arg_tol_eq(*tmp_ptr, opts->hpipm_opts);
    }
    else if (!strcmp(field, "tol_ineq"))
    {
        double* tmp_ptr = (double *) value;
		d_set_dense_qp_ipm_arg_tol_ineq(*tmp_ptr, opts->hpipm_opts);
    }
    else if (!strcmp(field, "tol_comp"))
    {
        double* tmp_ptr = (double *) value;
		d_set_dense_qp_ipm_arg_tol_comp(*tmp_ptr, opts->hpipm_opts);
    }
    else if (!strcmp(field, "warm_start"))
    {
        int* tmp_ptr = (int *) value;
		d_set_dense_qp_ipm_arg_warm_start(*tmp_ptr, opts->hpipm_opts);
    }
	else
	{
		printf("\nerror: dense_qp_hpipm_opts_set: wrong field: %s\n", field);
		exit(1);
	}

	return;
}



/************************************************
 * memory
 ************************************************/

int dense_qp_hpipm_memory_calculate_size(void *config_, void *dims_, void *opts_)
{
    dense_qp_dims *dims = dims_;
    dense_qp_hpipm_opts *opts = opts_;

    int size = 0;
    size += sizeof(dense_qp_hpipm_memory);
    size += sizeof(struct d_dense_qp_ipm_workspace);

    size += d_memsize_dense_qp_ipm(dims, opts->hpipm_opts);

    return size;
}



void *dense_qp_hpipm_memory_assign(void *config_, void *dims_, void *opts_, void *raw_memory)
{
    dense_qp_dims *dims = dims_;
    dense_qp_hpipm_opts *opts = opts_;
    dense_qp_hpipm_memory *mem;

    char *c_ptr = (char *) raw_memory;

    mem = (dense_qp_hpipm_memory *) c_ptr;
    c_ptr += sizeof(dense_qp_hpipm_memory);

    mem->hpipm_workspace = (struct d_dense_qp_ipm_workspace *) c_ptr;
    c_ptr += sizeof(struct d_dense_qp_ipm_workspace);

    struct d_dense_qp_ipm_workspace *ipm_workspace = mem->hpipm_workspace;

    assert((size_t) c_ptr % 8 == 0 && "memory not 8-byte aligned!");

    // ipm workspace structure
    d_create_dense_qp_ipm(dims, opts->hpipm_opts, ipm_workspace, c_ptr);
    c_ptr += ipm_workspace->memsize;

    assert((char *) raw_memory + dense_qp_hpipm_memory_calculate_size(config_, dims, opts) ==
           c_ptr);

    return mem;
}



int dense_qp_hpipm_workspace_calculate_size(void *config_, void *dims_, void *opts_)
{
	return 0;
}



int dense_qp_hpipm(void *config, void *qp_in_, void *qp_out_, void *opts_, void *mem_, void *work_)
{
    dense_qp_in *qp_in = qp_in_;
    dense_qp_out *qp_out = qp_out_;

    qp_info *info = (qp_info *) qp_out->misc;
    acados_timer tot_timer, qp_timer;

    acados_tic(&tot_timer);

    // cast structures
    dense_qp_hpipm_opts *opts = opts_;
    dense_qp_hpipm_memory *memory = mem_;

	// zero primal solution
	// TODO add a check if warm start of first SQP iteration is implemented !!!!!!
	int nv = qp_in->dim->nv;
	int ns = qp_in->dim->ns;
	blasfeo_dvecse(nv+2*ns, 0.0, qp_out->v, 0);

    // solve ipm
    acados_tic(&qp_timer);
    int hpipm_status =
        d_solve_dense_qp_ipm(qp_in, qp_out, opts->hpipm_opts, memory->hpipm_workspace);

    info->solve_QP_time = acados_toc(&qp_timer);
    info->interface_time = 0;  // there are no conversions for hpipm
    info->total_time = acados_toc(&tot_timer);
    info->num_iter = memory->hpipm_workspace->iter;
    info->t_computed = 1;

    // check exit conditions
    int acados_status = hpipm_status;
    if (hpipm_status == 0) acados_status = ACADOS_SUCCESS;
    if (hpipm_status == 1) acados_status = ACADOS_MAXITER;
    if (hpipm_status == 2) acados_status = ACADOS_MINSTEP;
    return acados_status;
}



void dense_qp_hpipm_eval_sens(void *config_, void *qp_in, void *qp_out, void *opts_, void *mem_, void *work_)
{
	printf("\nerror: dense_qp_hpipm_eval_sens: not implemented yet\n");
	exit(1);
}



void dense_qp_hpipm_config_initialize_default(void *config_)
{
    qp_solver_config *config = config_;

    config->dims_set = &dense_qp_dims_set;
    config->opts_calculate_size = &dense_qp_hpipm_opts_calculate_size;
    config->opts_assign = &dense_qp_hpipm_opts_assign;
    config->opts_initialize_default = &dense_qp_hpipm_opts_initialize_default;
    config->opts_update = &dense_qp_hpipm_opts_update;
    config->opts_set = &dense_qp_hpipm_opts_set;
    config->memory_calculate_size = &dense_qp_hpipm_memory_calculate_size;
    config->memory_assign = &dense_qp_hpipm_memory_assign;
    config->workspace_calculate_size = &dense_qp_hpipm_workspace_calculate_size;
    config->evaluate = &dense_qp_hpipm;
    config->eval_sens = &dense_qp_hpipm_eval_sens;

    return;
}

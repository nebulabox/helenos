/*
 * Copyright (c) 2013 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <async.h>
#include <errno.h>
#include <macros.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>
#include <stddef.h>
#include <types/inetcfg.h>

#include "addrobj.h"
#include "inetsrv.h"
#include "inet_link.h"
#include "inetcfg.h"
#include "sroute.h"

static int inetcfg_addr_create_static(char *name, inet_naddr_t *naddr,
    sysarg_t link_id, sysarg_t *addr_id)
{
	inet_link_t *ilink;
	inet_addrobj_t *addr;
	inet_addr_t iaddr;
	int rc;

	ilink = inet_link_get_by_id(link_id);
	if (ilink == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Link %lu not found.",
		    (unsigned long) link_id);
		return ENOENT;
	}

	addr = inet_addrobj_new();
	if (addr == NULL) {
		*addr_id = 0;
		return ENOMEM;
	}

	addr->naddr = *naddr;
	addr->ilink = ilink;
	addr->name = str_dup(name);
	rc = inet_addrobj_add(addr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Duplicate address name '%s'.", addr->name);
		inet_addrobj_delete(addr);
		return rc;
	}

	inet_naddr_addr(&addr->naddr, &iaddr);
	rc = iplink_addr_add(ilink->iplink, &iaddr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed setting IP address on internet link.");
		inet_addrobj_remove(addr);
		inet_addrobj_delete(addr);
		return rc;
	}

	return EOK;
}

static int inetcfg_addr_delete(sysarg_t addr_id)
{
	inet_addrobj_t *addr;

	addr = inet_addrobj_get_by_id(addr_id);
	if (addr == NULL)
		return ENOENT;

	inet_addrobj_remove(addr);
	inet_addrobj_delete(addr);

	return EOK;
}

static int inetcfg_addr_get(sysarg_t addr_id, inet_addr_info_t *ainfo)
{
	inet_addrobj_t *addr;

	addr = inet_addrobj_get_by_id(addr_id);
	if (addr == NULL)
		return ENOENT;

	ainfo->naddr = addr->naddr;
	ainfo->ilink = addr->ilink->svc_id;
	ainfo->name = str_dup(addr->name);

	return EOK;
}

static int inetcfg_addr_get_id(char *name, sysarg_t link_id, sysarg_t *addr_id)
{
	inet_link_t *ilink;
	inet_addrobj_t *addr;

	ilink = inet_link_get_by_id(link_id);
	if (ilink == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Link %zu not found.", (size_t) link_id);
		return ENOENT;
	}

	addr = inet_addrobj_find_by_name(name, ilink);
	if (addr == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Address '%s' not found.", name);
		return ENOENT;
	}

	*addr_id = addr->id;
	return EOK;
}

static int inetcfg_get_addr_list(sysarg_t **addrs, size_t *count)
{
	return inet_addrobj_get_id_list(addrs, count);
}

static int inetcfg_get_link_list(sysarg_t **addrs, size_t *count)
{
	return inet_link_get_id_list(addrs, count);
}

static int inetcfg_get_sroute_list(sysarg_t **sroutes, size_t *count)
{
	return inet_sroute_get_id_list(sroutes, count);
}

static int inetcfg_link_add(sysarg_t link_id)
{
	return inet_link_open(link_id);
}

static int inetcfg_link_get(sysarg_t link_id, inet_link_info_t *linfo)
{
	inet_link_t *ilink;

	ilink = inet_link_get_by_id(link_id);
	if (ilink == NULL) {
		return ENOENT;
	}

	linfo->name = str_dup(ilink->svc_name);
	linfo->def_mtu = ilink->def_mtu;
	if (ilink->mac_valid) {
		addr48(ilink->mac, linfo->mac_addr);
	} else {
		memset(linfo->mac_addr, 0, sizeof(linfo->mac_addr));
	}

	return EOK;
}

static int inetcfg_link_remove(sysarg_t link_id)
{
	return ENOTSUP;
}

static int inetcfg_sroute_create(char *name, inet_naddr_t *dest,
    inet_addr_t *router, sysarg_t *sroute_id)
{
	inet_sroute_t *sroute;

	sroute = inet_sroute_new();
	if (sroute == NULL) {
		*sroute_id = 0;
		return ENOMEM;
	}

	sroute->dest = *dest;
	sroute->router = *router;
	sroute->name = str_dup(name);
	inet_sroute_add(sroute);

	*sroute_id = sroute->id;
	return EOK;
}

static int inetcfg_sroute_delete(sysarg_t sroute_id)
{
	inet_sroute_t *sroute;

	sroute = inet_sroute_get_by_id(sroute_id);
	if (sroute == NULL)
		return ENOENT;

	inet_sroute_remove(sroute);
	inet_sroute_delete(sroute);

	return EOK;
}

static int inetcfg_sroute_get(sysarg_t sroute_id, inet_sroute_info_t *srinfo)
{
	inet_sroute_t *sroute;

	sroute = inet_sroute_get_by_id(sroute_id);
	if (sroute == NULL)
		return ENOENT;

	srinfo->dest = sroute->dest;
	srinfo->router = sroute->router;
	srinfo->name = str_dup(sroute->name);

	return EOK;
}

static int inetcfg_sroute_get_id(char *name, sysarg_t *sroute_id)
{
	inet_sroute_t *sroute;

	sroute = inet_sroute_find_by_name(name);
	if (sroute == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Static route '%s' not found.", name);
		return ENOENT;
	}

	*sroute_id = sroute->id;
	return EOK;
}

static void inetcfg_addr_create_static_srv(ipc_callid_t iid,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_create_static_srv()");
	
	sysarg_t link_id = IPC_GET_ARG1(*icall);
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	inet_naddr_t naddr;
	int rc = async_data_write_finalize(callid, &naddr, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	char *name;
	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	sysarg_t addr_id = 0;
	rc = inetcfg_addr_create_static(name, &naddr, link_id, &addr_id);
	free(name);
	async_answer_1(iid, rc, addr_id);
}

static void inetcfg_addr_delete_srv(ipc_callid_t callid, ipc_call_t *call)
{
	sysarg_t addr_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_delete_srv()");

	addr_id = IPC_GET_ARG1(*call);

	rc = inetcfg_addr_delete(addr_id);
	async_answer_0(callid, rc);
}

static void inetcfg_addr_get_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_get_srv()");
	
	sysarg_t addr_id = IPC_GET_ARG1(*icall);
	
	inet_addr_info_t ainfo;
	
	inet_naddr_any(&ainfo.naddr);
	ainfo.ilink = 0;
	ainfo.name = NULL;
	
	int rc = inetcfg_addr_get(addr_id, &ainfo);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_read_finalize(callid, &ainfo.naddr, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	rc = async_data_read_finalize(callid, ainfo.name,
	    min(size, str_size(ainfo.name)));
	free(ainfo.name);
	
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	async_answer_1(iid, (sysarg_t) rc, ainfo.ilink);
}

static void inetcfg_addr_get_id_srv(ipc_callid_t callid, ipc_call_t *call)
{
	char *name;
	sysarg_t link_id;
	sysarg_t addr_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_get_id_srv()");

	link_id = IPC_GET_ARG1(*call);

	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	addr_id = 0;
	rc = inetcfg_addr_get_id(name, link_id, &addr_id);
	free(name);
	async_answer_1(callid, rc, addr_id);
}

static void inetcfg_get_addr_list_srv(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_callid_t rcallid;
	size_t count;
	size_t max_size;
	size_t act_size;
	size_t size;
	sysarg_t *id_buf;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_get_addr_list_srv()");

	if (!async_data_read_receive(&rcallid, &max_size)) {
		async_answer_0(rcallid, EREFUSED);
		async_answer_0(callid, EREFUSED);
		return;
	}

	rc = inetcfg_get_addr_list(&id_buf, &count);
	if (rc != EOK) {
		async_answer_0(rcallid, rc);
		async_answer_0(callid, rc);
		return;
	}

	act_size = count * sizeof(sysarg_t);
	size = min(act_size, max_size);

	sysarg_t retval = async_data_read_finalize(rcallid, id_buf, size);
	free(id_buf);

	async_answer_1(callid, retval, act_size);
}

static void inetcfg_get_link_list_srv(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_callid_t rcallid;
	size_t count;
	size_t max_size;
	size_t act_size;
	size_t size;
	sysarg_t *id_buf;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_get_addr_list_srv()");

	if (!async_data_read_receive(&rcallid, &max_size)) {
		async_answer_0(rcallid, EREFUSED);
		async_answer_0(callid, EREFUSED);
		return;
	}

	rc = inetcfg_get_link_list(&id_buf, &count);
	if (rc != EOK) {
		async_answer_0(rcallid, rc);
		async_answer_0(callid, rc);
		return;
	}

	act_size = count * sizeof(sysarg_t);
	size = min(act_size, max_size);

	sysarg_t retval = async_data_read_finalize(rcallid, id_buf, size);
	free(id_buf);

	async_answer_1(callid, retval, act_size);
}

static void inetcfg_get_sroute_list_srv(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_callid_t rcallid;
	size_t count;
	size_t max_size;
	size_t act_size;
	size_t size;
	sysarg_t *id_buf;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_get_sroute_list_srv()");

	if (!async_data_read_receive(&rcallid, &max_size)) {
		async_answer_0(rcallid, EREFUSED);
		async_answer_0(callid, EREFUSED);
		return;
	}

	rc = inetcfg_get_sroute_list(&id_buf, &count);
	if (rc != EOK) {
		async_answer_0(rcallid, rc);
		async_answer_0(callid, rc);
		return;
	}

	act_size = count * sizeof(sysarg_t);
	size = min(act_size, max_size);

	sysarg_t retval = async_data_read_finalize(rcallid, id_buf, size);
	free(id_buf);

	async_answer_1(callid, retval, act_size);
}

static void inetcfg_link_add_srv(ipc_callid_t callid, ipc_call_t *call)
{
	sysarg_t link_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_link_add_srv()");

	link_id = IPC_GET_ARG1(*call);

	rc = inetcfg_link_add(link_id);
	async_answer_0(callid, rc);
}

static void inetcfg_link_get_srv(ipc_callid_t callid, ipc_call_t *call)
{
	ipc_callid_t name_callid;
	ipc_callid_t laddr_callid;
	size_t name_max_size;
	size_t laddr_max_size;

	sysarg_t link_id;
	inet_link_info_t linfo;
	int rc;

	link_id = IPC_GET_ARG1(*call);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_link_get_srv()");

	linfo.name = NULL;

	if (!async_data_read_receive(&name_callid, &name_max_size)) {
		async_answer_0(name_callid, EREFUSED);
		async_answer_0(callid, EREFUSED);
		return;
	}

	if (!async_data_read_receive(&laddr_callid, &laddr_max_size)) {
		async_answer_0(name_callid, EREFUSED);
		async_answer_0(callid, EREFUSED);
		return;
	}

	rc = inetcfg_link_get(link_id, &linfo);
	if (rc != EOK) {
		async_answer_0(laddr_callid, rc);
		async_answer_0(name_callid, rc);
		async_answer_0(callid, rc);
		return;
	}

	sysarg_t retval = async_data_read_finalize(name_callid, linfo.name,
	    min(name_max_size, str_size(linfo.name)));
	if (retval != EOK) {
		free(linfo.name);
		async_answer_0(laddr_callid, retval);
		async_answer_0(callid, retval);
		return;
	}

	retval = async_data_read_finalize(laddr_callid, &linfo.mac_addr,
	    min(laddr_max_size, sizeof(linfo.mac_addr)));

	free(linfo.name);

	async_answer_1(callid, retval, linfo.def_mtu);
}

static void inetcfg_link_remove_srv(ipc_callid_t callid, ipc_call_t *call)
{
	sysarg_t link_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_link_remove_srv()");

	link_id = IPC_GET_ARG1(*call);

	rc = inetcfg_link_remove(link_id);
	async_answer_0(callid, rc);
}

static void inetcfg_sroute_create_srv(ipc_callid_t iid,
    ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_create_srv()");

	ipc_callid_t callid;
	size_t size;
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	inet_naddr_t dest;
	int rc = async_data_write_finalize(callid, &dest, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_write_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	inet_addr_t router;
	rc = async_data_write_finalize(callid, &router, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	char *name;
	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	sysarg_t sroute_id = 0;
	rc = inetcfg_sroute_create(name, &dest, &router, &sroute_id);
	free(name);
	async_answer_1(iid, rc, sroute_id);
}

static void inetcfg_sroute_delete_srv(ipc_callid_t callid, ipc_call_t *call)
{
	sysarg_t sroute_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_delete_srv()");

	sroute_id = IPC_GET_ARG1(*call);

	rc = inetcfg_sroute_delete(sroute_id);
	async_answer_0(callid, rc);
}

static void inetcfg_sroute_get_srv(ipc_callid_t iid, ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_get_srv()");
	
	sysarg_t sroute_id = IPC_GET_ARG1(*icall);
	
	inet_sroute_info_t srinfo;
	
	inet_naddr_any(&srinfo.dest);
	inet_addr_any(&srinfo.router);
	srinfo.name = NULL;
	
	int rc = inetcfg_sroute_get(sroute_id, &srinfo);
	if (rc != EOK) {
		async_answer_0(iid, rc);
		return;
	}
	
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_read_finalize(callid, &srinfo.dest, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	if (size != sizeof(inet_addr_t)) {
		async_answer_0(callid, EINVAL);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	rc = async_data_read_finalize(callid, &srinfo.router, size);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		async_answer_0(iid, rc);
		return;
	}
	
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EREFUSED);
		async_answer_0(iid, EREFUSED);
		return;
	}
	
	rc = async_data_read_finalize(callid, srinfo.name,
	    min(size, str_size(srinfo.name)));
	free(srinfo.name);
	
	async_answer_0(iid, (sysarg_t) rc);
}

static void inetcfg_sroute_get_id_srv(ipc_callid_t callid, ipc_call_t *call)
{
	char *name;
	sysarg_t sroute_id;
	int rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_get_id_srv()");

	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return;
	}

	sroute_id = 0;
	rc = inetcfg_sroute_get_id(name, &sroute_id);
	free(name);
	async_answer_1(callid, rc, sroute_id);
}

void inet_cfg_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_cfg_conn()");

	/* Accept the connection */
	async_answer_0(iid, EOK);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		sysarg_t method = IPC_GET_IMETHOD(call);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "method %d", (int)method);
		if (!method) {
			/* The other side has hung up */
			async_answer_0(callid, EOK);
			return;
		}

		switch (method) {
		case INETCFG_ADDR_CREATE_STATIC:
			inetcfg_addr_create_static_srv(callid, &call);
			break;
		case INETCFG_ADDR_DELETE:
			inetcfg_addr_delete_srv(callid, &call);
			break;
		case INETCFG_ADDR_GET:
			inetcfg_addr_get_srv(callid, &call);
			break;
		case INETCFG_ADDR_GET_ID:
			inetcfg_addr_get_id_srv(callid, &call);
			break;
		case INETCFG_GET_ADDR_LIST:
			inetcfg_get_addr_list_srv(callid, &call);
			break;
		case INETCFG_GET_LINK_LIST:
			inetcfg_get_link_list_srv(callid, &call);
			break;
		case INETCFG_GET_SROUTE_LIST:
			inetcfg_get_sroute_list_srv(callid, &call);
			break;
		case INETCFG_LINK_ADD:
			inetcfg_link_add_srv(callid, &call);
			break;
		case INETCFG_LINK_GET:
			inetcfg_link_get_srv(callid, &call);
			break;
		case INETCFG_LINK_REMOVE:
			inetcfg_link_remove_srv(callid, &call);
			break;
		case INETCFG_SROUTE_CREATE:
			inetcfg_sroute_create_srv(callid, &call);
			break;
		case INETCFG_SROUTE_DELETE:
			inetcfg_sroute_delete_srv(callid, &call);
			break;
		case INETCFG_SROUTE_GET:
			inetcfg_sroute_get_srv(callid, &call);
			break;
		case INETCFG_SROUTE_GET_ID:
			inetcfg_sroute_get_id_srv(callid, &call);
			break;
		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

/** @}
 */

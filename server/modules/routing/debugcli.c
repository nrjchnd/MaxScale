/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-07-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */

/**
 * @file debugcli.c - A "routing module" that in fact merely gives
 * access to debug commands within the gateway
 *
 * @verbatim
 * Revision History
 *
 * Date     Who             Description
 * 18/06/13 Mark Riddoch    Initial implementation
 *
 * @endverbatim
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <service.h>
#include <session.h>
#include <router.h>
#include <modules.h>
#include <modinfo.h>
#include <atomic.h>
#include <spinlock.h>
#include <dcb.h>
#include <maxscale/alloc.h>
#include <maxscale/poll.h>
#include <debugcli.h>
#include <skygw_utils.h>
#include <log_manager.h>

/* The router entry points */
static  ROUTER *createInstance(SERVICE *service, char **options);
static  void   *newSession(ROUTER *instance, SESSION *session);
static  void   closeSession(ROUTER *instance, void *router_session);
static  void   freeSession(ROUTER *instance, void *router_session);
static  int    execute(ROUTER *instance, void *router_session, GWBUF *queue);
static  void   diagnostics(ROUTER *instance, DCB *dcb);
static  int    getCapabilities ();

/** The module object definition */
static ROUTER_OBJECT MyObject =
{
    createInstance,
    newSession,
    closeSession,
    freeSession,
    execute,
    diagnostics,
    NULL,
    NULL,
    getCapabilities
};

static SPINLOCK     instlock;
static CLI_INSTANCE *instances;
#define VERSION_STR "V1.1.1"

/**
 * The module initialisation routine, called when the module
 * is first loaded.
 */
void
ModuleInit()
{
    MXS_NOTICE("Initialise debug CLI router module %s.", VERSION_STR);
    spinlock_init(&instlock);
    instances = NULL;
}

MXS_DECLARE_MODULE(ROUTER)
{
    MODULE_GA,
    "The debug user interface",
    VERSION_STR,
    ModuleInit,
    &MyObject
};

extern int execute_cmd(CLI_SESSION *cli);

/**
 * Create an instance of the router for a particular service
 * within the gateway.
 *
 * @param service   The service this router is being create for
 * @param options   Any array of options for the query router
 *
 * @return The instance data for this new instance
 */
static  ROUTER  *
createInstance(SERVICE *service, char **options)
{
    CLI_INSTANCE    *inst;
    int     i;

    if ((inst = MXS_MALLOC(sizeof(CLI_INSTANCE))) == NULL)
    {
        return NULL;
    }

    inst->service = service;
    spinlock_init(&inst->lock);
    inst->sessions = NULL;
    inst->mode = CLIM_USER;

    if (options)
    {
        for (i = 0; options[i]; i++)
        {
            if (!strcasecmp(options[i], "developer"))
            {
                inst->mode = CLIM_DEVELOPER;
            }
            else if (!strcasecmp(options[i], "user"))
            {
                inst->mode = CLIM_USER;
            }
            else
            {
                MXS_ERROR("Unknown option for CLI '%s'", options[i]);
            }
        }
    }

    /*
     * We have completed the creation of the instance data, so now
     * insert this router instance into the linked list of routers
     * that have been created with this module.
     */
    spinlock_acquire(&instlock);
    inst->next = instances;
    instances = inst;
    spinlock_release(&instlock);

    return (ROUTER *)inst;
}

/**
 * Associate a new session with this instance of the router.
 *
 * @param instance  The router instance data
 * @param session   The session itself
 * @return Session specific data for this session
 */
static  void    *
newSession(ROUTER *instance, SESSION *session)
{
    CLI_INSTANCE    *inst = (CLI_INSTANCE *)instance;
    CLI_SESSION *client;

    if ((client = (CLI_SESSION *)MXS_MALLOC(sizeof(CLI_SESSION))) == NULL)
    {
        return NULL;
    }
    client->session = session;

    memset(client->cmdbuf, 0, 80);

    spinlock_acquire(&inst->lock);
    client->next = inst->sessions;
    inst->sessions = client;
    spinlock_release(&inst->lock);

    session->state = SESSION_STATE_READY;
    client->mode = inst->mode;

    dcb_printf(session->client_dcb, "Welcome the MariaDB Corporation MaxScale Debug Interface (%s).\n",
               VERSION_STR);
    if (client->mode == CLIM_DEVELOPER)
    {
        dcb_printf(session->client_dcb, "WARNING: This interface is meant for developer usage,\n");
        dcb_printf(session->client_dcb,
                   "passing incorrect addresses to commands can endanger your MaxScale server.\n\n");
    }
    dcb_printf(session->client_dcb, "Type help for a list of available commands.\n\n");

    return (void *)client;
}

/**
 * Close a session with the router, this is the mechanism
 * by which a router may cleanup data structure etc.
 *
 * @param instance      The router instance data
 * @param router_session    The session being closed
 */
static  void
closeSession(ROUTER *instance, void *router_session)
{
    CLI_INSTANCE    *inst = (CLI_INSTANCE *)instance;
    CLI_SESSION *session = (CLI_SESSION *)router_session;


    spinlock_acquire(&inst->lock);
    if (inst->sessions == session)
    {
        inst->sessions = session->next;
    }
    else
    {
        CLI_SESSION *ptr = inst->sessions;
        while (ptr && ptr->next != session)
        {
            ptr = ptr->next;
        }
        if (ptr)
        {
            ptr->next = session->next;
        }
    }
    spinlock_release(&inst->lock);
    /**
     * Router session is freed in session.c:session_close, when session who
     * owns it, is freed.
     */
}

/**
 * Free a debugcli session
 *
 * @param router_instance   The router session
 * @param router_client_session The router session as returned from newSession
 */
static void freeSession(
    ROUTER* router_instance,
    void*   router_client_session)
{
    MXS_FREE(router_client_session);
    return;
}

/**
 * We have data from the client, we must route it to the backend.
 * This is simply a case of sending it to the connection that was
 * chosen when we started the client session.
 *
 * @param instance      The router instance
 * @param router_session    The router session returned from the newSession call
 * @param queue         The queue of data buffers to route
 * @return The number of bytes sent
 */
static  int
execute(ROUTER *instance, void *router_session, GWBUF *queue)
{
    CLI_SESSION *session = (CLI_SESSION *)router_session;


    char *cmdbuf = session->cmdbuf;
    int cmdlen = 0;

    *cmdbuf = 0;

    /* Extract the characters */
    while (queue && (cmdlen < CMDBUFLEN - 1))
    {
        const char* data = GWBUF_DATA(queue);
        int len = GWBUF_LENGTH(queue);
        int n = MIN(len, CMDBUFLEN - cmdlen - 1);

        if (n != len)
        {
            MXS_WARNING("Too long user command truncated.");
        }

        strncat(cmdbuf, data, n);

        cmdlen += n;
        cmdbuf += n;

        queue = gwbuf_consume(queue, GWBUF_LENGTH(queue));
    }

    if (strrchr(session->cmdbuf, '\n'))
    {
        if (execute_cmd(session))
        {
            dcb_printf(session->session->client_dcb, "MaxScale> ");
        }
        else
        {
            dcb_close(session->session->client_dcb);
        }
    }
    return 1;
}

/**
 * Display router diagnostics
 *
 * @param instance  Instance of the router
 * @param dcb       DCB to send diagnostics to
 */
static  void
diagnostics(ROUTER *instance, DCB *dcb)
{
    return; /* Nothing to do currently */
}

static int getCapabilities()
{
    return 0;
}

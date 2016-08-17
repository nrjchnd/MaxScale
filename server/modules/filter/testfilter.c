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
#include <stdio.h>
#include <modinfo.h>
#include <filter.h>
#include <maxscale/alloc.h>
#include <modutil.h>
#include <atomic.h>

/**
 * @file testfilter.c - a very simple test filter.
 * @verbatim
 *
 * This filter is a very simple example used to test the filter API,
 * it merely counts the number of statements that flow through the
 * filter pipeline.
 *
 * Reporting is done via the diagnostics print routine.
 * @endverbatim
 */

static  FILTER *createInstance(char **options, FILTER_PARAMETER **params);
static  void   *newSession(FILTER *instance, SESSION *session);
static  void    closeSession(FILTER *instance, void *session);
static  void    freeSession(FILTER *instance, void *session);
static  void    setDownstream(FILTER *instance, void *fsession, DOWNSTREAM *downstream);
static  int     routeQuery(FILTER *instance, void *fsession, GWBUF *queue);
static  void    diagnostic(FILTER *instance, void *fsession, DCB *dcb);

static FILTER_OBJECT MyObject =
{
    createInstance,
    newSession,
    closeSession,
    freeSession,
    setDownstream,
    NULL,       // No upstream requirement
    routeQuery,
    NULL,
    diagnostic,
};

MXS_DECLARE_MODULE(FILTER)
{
    MODULE_BETA_RELEASE,
    "A simple query counting filter",
    "V1.0.0",
    NULL,
    &MyObject
};

/**
 * A dummy instance structure
 */
typedef struct
{
    int sessions;
} TEST_INSTANCE;

/**
 * A dummy session structure for this test filter
 */
typedef struct
{
    DOWNSTREAM  down;
    int     count;
} TEST_SESSION;

/**
 * Create an instance of the filter for a particular service
 * within MaxScale.
 *
 * @param options   The options for this filter
 * @param params    The array of name/value pair parameters for the filter
 *
 * @return The instance data for this new instance
 */
static  FILTER  *
createInstance(char **options, FILTER_PARAMETER **params)
{
    TEST_INSTANCE   *my_instance;

    if ((my_instance = MXS_CALLOC(1, sizeof(TEST_INSTANCE))) != NULL)
    {
        my_instance->sessions = 0;
    }
    return (FILTER *)my_instance;
}

/**
 * Associate a new session with this instance of the filter.
 *
 * @param instance  The filter instance data
 * @param session   The session itself
 * @return Session specific data for this session
 */
static  void    *
newSession(FILTER *instance, SESSION *session)
{
    TEST_INSTANCE   *my_instance = (TEST_INSTANCE *)instance;
    TEST_SESSION    *my_session;

    if ((my_session = MXS_CALLOC(1, sizeof(TEST_SESSION))) != NULL)
    {
        atomic_add(&my_instance->sessions, 1);
        my_session->count = 0;
    }

    return my_session;
}

/**
 * Close a session with the filter, this is the mechanism
 * by which a filter may cleanup data structure etc.
 *
 * @param instance  The filter instance data
 * @param session   The session being closed
 */
static  void
closeSession(FILTER *instance, void *session)
{
}

/**
 * Free the memory associated with this filter session.
 *
 * @param instance  The filter instance data
 * @param session   The session being closed
 */
static void
freeSession(FILTER *instance, void *session)
{
    MXS_FREE(session);
    return;
}

/**
 * Set the downstream component for this filter.
 *
 * @param instance  The filter instance data
 * @param session   The session being closed
 * @param downstream    The downstream filter or router
 */
static void
setDownstream(FILTER *instance, void *session, DOWNSTREAM *downstream)
{
    TEST_SESSION    *my_session = (TEST_SESSION *)session;

    my_session->down = *downstream;
}

/**
 * The routeQuery entry point. This is passed the query buffer
 * to which the filter should be applied. Once applied the
 * query shoudl normally be passed to the downstream component
 * (filter or router) in the filter chain.
 *
 * @param instance  The filter instance data
 * @param session   The filter session
 * @param queue     The query data
 */
static  int
routeQuery(FILTER *instance, void *session, GWBUF *queue)
{
    TEST_SESSION    *my_session = (TEST_SESSION *)session;

    if (modutil_is_SQL(queue))
    {
        my_session->count++;
    }
    return my_session->down.routeQuery(my_session->down.instance,
                                       my_session->down.session, queue);
}

/**
 * Diagnostics routine
 *
 * If fsession is NULL then print diagnostics on the filter
 * instance as a whole, otherwise print diagnostics for the
 * particular session.
 *
 * @param   instance    The filter instance
 * @param   fsession    Filter session, may be NULL
 * @param   dcb     The DCB for diagnostic output
 */
static  void
diagnostic(FILTER *instance, void *fsession, DCB *dcb)
{
    TEST_INSTANCE   *my_instance = (TEST_INSTANCE *)instance;
    TEST_SESSION    *my_session = (TEST_SESSION *)fsession;

    if (my_session)
        dcb_printf(dcb, "\t\tNo. of queries routed by filter: %d\n",
                   my_session->count);
    else
        dcb_printf(dcb, "\t\tNo. of sessions created: %d\n",
                   my_instance->sessions);
}

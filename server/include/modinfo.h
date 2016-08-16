#ifndef _MODINFO_H
#define _MODINFO_H
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
 * @file modinfo.h The module information interface
 *
 * @verbatim
 * Revision History
 *
 * Date     Who             Description
 * 02/06/14 Mark Riddoch    Initial implementation
 *
 * @endverbatim
 */

struct filter_object;
struct router_object;
struct monitor_object;
struct gw_protocol;
struct gw_authenticator;


/**
 * The status of the module. This gives some idea of the module
 * maturity.
 */
typedef enum
{
    MODULE_IN_DEVELOPMENT = 0,
    MODULE_ALPHA_RELEASE,
    MODULE_BETA_RELEASE,
    MODULE_GA,
    MODULE_EXPERIMENTAL
} MODULE_STATUS;

/**
 * The API implemented by the module
 */
typedef enum
{
    MODULE_API_PROTOCOL = 1,
    MODULE_API_ROUTER,
    MODULE_API_MONITOR,
    MODULE_API_FILTER,
    MODULE_API_AUTHENTICATOR,
    MODULE_API_QUERY_CLASSIFIER,
} MODULE_API;

/**
 * The module version structure.
 *
 * The rules for changing these values are:
 *
 * Any change that affects an inexisting call in the API in question,
 * making the new API no longer compatible with the old,
 * must increment the major version.
 *
 * Any change that adds to the API, but does not alter the existing API
 * calls, must increment the minor version.
 *
 * Any change that is purely cosmetic and does not affect the calling
 * conventions of the API must increment only the patch version number.
 */
typedef struct
{
    int     major;
    int     minor;
    int     patch;
} MODULE_VERSION;

/**
 * The module information structure
 */
typedef struct
{
    MODULE_STATUS   status;            /**< Module maturity */
    const char     *description;       /**< Module description */
    const char     *version;           /**< Module version */
    void          (*moduleinit)(void); /**< Module global initialization, optional */
    void           *object;            /**< Type specific entry points */
} MODULE_INFO;

/**
 * Modules are declared using this macro. Each module must call this macro with
 * the type of the module. The type needs to be one of PROTOCOL, ROUTER,
 * MONITOR, FILTER, AUTHENTICATOR or QUERY_CLASSIFIER. After the macro, the
 * module must provide an implementation of the MODULE_INFO structure.
 *
 * Here is an example module definition for a filter:
 *
 * static FILTER_OBJECT MyObject =
 * {
 *     ... // Here would be module specific entry points
 * };
 *
 * MXS_DECLARE_MODULE(FILTER)
 * {
 *     MODULE_GA, // Module maturity
 *     "A simple filter", // Module description
 *     "V1.1.0", // Module version string
 *     NULL, // Global initiation function of type void (*)(void), optional
 *     &MyObject // Pointer to the module specific entry points, in this case a FILTER_OBJECT
 * };
 */
#define MXS_DECLARE_MODULE(type) \
    MODULE_API modapi = MODULE_API_ ## type; \
    MODULE_VERSION api_version = MXS_ ## type ## _VERSION; \
    MODULE_INFO info =

#endif

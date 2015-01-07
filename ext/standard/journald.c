/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2014 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Mark Montague <mark@catseye.org>                             |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"

#ifdef HAVE_JOURNALD

#include "php_ini.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_globals.h"
#include "php_globals.h"

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>

#define SD_JOURNAL_SUPPRESS_LOCATION
#include <systemd/sd-journal.h>

#include "basic_functions.h"
#include "php_journald.h"

ZEND_DECLARE_MODULE_GLOBALS(journald)

static PHP_INI_MH(OnUpdateFacility) { /* {{{ */

    int facility;
    char *v;

    if (!new_value) {
        return FAILURE;
    }
    v = new_value->val;
    if (!strncasecmp(v, "LOG_", strlen("LOG_"))) {
        v += strlen("LOG_");
    }

    if (!strcasecmp(v, "kern")) {
        facility = LOG_FAC(LOG_KERN);
    } else if (!strcasecmp(v, "user")) {
        facility = LOG_FAC(LOG_USER);
    } else if (!strcasecmp(v, "mail")) {
        facility = LOG_FAC(LOG_MAIL);
    } else if (!strcasecmp(v, "daemon")) {
        facility = LOG_FAC(LOG_DAEMON);
    } else if (!strcasecmp(v, "auth")) {
        facility = LOG_FAC(LOG_AUTH);
    } else if (!strcasecmp(v, "syslog")) {
        facility = LOG_FAC(LOG_SYSLOG);
    } else if (!strcasecmp(v, "lpr")) {
        facility = LOG_FAC(LOG_LPR);
#ifdef LOG_NEWS
    } else if (!strcasecmp(v, "news")) {
        facility = LOG_FAC(LOG_NEWS);
#endif
#ifdef LOG_UUCP
    } else if (!strcasecmp(v, "uucp")) {
        facility = LOG_FAC(LOG_UUCP);
#endif
#ifdef LOG_CRON
    } else if (!strcasecmp(v, "cron")) {
        facility = LOG_FAC(LOG_CRON);
#endif
#ifdef LOG_AUTHPRIV
    } else if (!strcasecmp(v, "authpriv")) {
        facility = LOG_FAC(LOG_AUTHPRIV);
#endif
#ifdef LOG_FTP
    } else if (!strcasecmp(v, "ftp")) {
        facility = LOG_FAC(LOG_FTP);
#endif
#ifndef PHP_WIN32
    } else if (!strcasecmp(v, "local0")) {
        facility = LOG_FAC(LOG_LOCAL0);
    } else if (!strcasecmp(v, "local1")) {
        facility = LOG_FAC(LOG_LOCAL1);
    } else if (!strcasecmp(v, "local2")) {
        facility = LOG_FAC(LOG_LOCAL2);
    } else if (!strcasecmp(v, "local3")) {
        facility = LOG_FAC(LOG_LOCAL3);
    } else if (!strcasecmp(v, "local4")) {
        facility = LOG_FAC(LOG_LOCAL4);
    } else if (!strcasecmp(v, "local5")) {
        facility = LOG_FAC(LOG_LOCAL5);
    } else if (!strcasecmp(v, "local6")) {
        facility = LOG_FAC(LOG_LOCAL6);
    } else if (!strcasecmp(v, "local7")) {
        facility = LOG_FAC(LOG_LOCAL7);
#endif
    } else {
        return FAILURE;
    }

    JOURNALD_G(syslog_facility) = facility;
    return SUCCESS;

} /* }}} */

static PHP_INI_MH(OnUpdatePriority) { /* {{{ */

    int priority;
    char *v;

    if (!new_value) {
        return FAILURE;
    }
    v = new_value->val;
    if (!strncasecmp(v, "LOG_", strlen("LOG_"))) {
        v += strlen("LOG_");
    }

    if (!strcasecmp(v, "EMERG")) {
        priority = LOG_EMERG;
    } else if (!strcasecmp(v, "ALERT")) {
        priority = LOG_ALERT;
    } else if (!strcasecmp(v, "CRIT")) {
        priority = LOG_CRIT;
    } else if (!strcasecmp(v, "ERR")) {
        priority = LOG_ERR;
    } else if (!strcasecmp(v, "WARNING")) {
        priority = LOG_WARNING;
    } else if (!strcasecmp(v, "NOTICE")) {
        priority = LOG_NOTICE;
    } else if (!strcasecmp(v, "INFO")) {
        priority = LOG_INFO;
    } else if (!strcasecmp(v, "DEBUG")) {
        priority = LOG_DEBUG;
    } else {
        return FAILURE;
    }

    JOURNALD_G(priority) = priority;
    return SUCCESS;

} /* }}} */

static void php_journald_init_globals(zend_journald_globals *journald_globals_p TSRMLS_DC) /* {{{ */
{
    journald_globals_p->syslog_identifier = NULL;
    journald_globals_p->syslog_facility = LOG_FAC(LOG_USER);
    journald_globals_p->priority = LOG_NOTICE;
    journald_globals_p->suppress_location = 0;
    journald_globals_p->suppress_syslog_fields = 0;
}
/* }}} */

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("journald.syslog_identifier", NULL, PHP_INI_ALL, OnUpdateString, syslog_identifier, zend_journald_globals, journald_globals)
    STD_PHP_INI_ENTRY("journald.syslog_facility", "LOG_USER", PHP_INI_ALL, OnUpdateFacility, syslog_facility, zend_journald_globals, journald_globals)
    STD_PHP_INI_ENTRY("journald.priority", "LOG_NOTICE", PHP_INI_ALL, OnUpdatePriority, priority, zend_journald_globals, journald_globals)
    STD_PHP_INI_ENTRY("journald.suppress_location", "0", PHP_INI_ALL, OnUpdateLong, suppress_location, zend_journald_globals, journald_globals)
    STD_PHP_INI_ENTRY("journald.suppress_syslog_fields", "0", PHP_INI_ALL, OnUpdateLong, suppress_syslog_fields, zend_journald_globals, journald_globals)
PHP_INI_END()

PHP_MINIT_FUNCTION(journald) /* {{{ */
{
    ZEND_INIT_MODULE_GLOBALS(journald, php_journald_init_globals, NULL);
    REGISTER_INI_ENTRIES();
    return SUCCESS;
} /* }}} */

PHP_MSHUTDOWN_FUNCTION(journald) /* {{{ */
{
    if (JOURNALD_G(syslog_identifier)) {
        pefree(JOURNALD_G(syslog_identifier), 1);
        JOURNALD_G(syslog_identifier) = NULL;
    }
    return SUCCESS;
}
/* }}} */

PHP_MINFO_FUNCTION(journald) /* {{{ */
{
    DISPLAY_INI_ENTRIES();
}
/* }}} */

PHPAPI const char *php_get_caller_info(const char **filename, uint *lineno, const char **class_name, const char **call_type TSRMLS_DC) /* {{{ */
{
    const char *fn;
    const char *function_name = "[unknown function]";
    zend_execute_data *ex = EG(current_execute_data);
    zend_execute_data *ex1;

    *filename = "[unknown file]";
    *lineno = 0;
    *class_name = "";
    *call_type = "";

    if (!ex || !zend_is_executing(TSRMLS_C)) {
        return function_name;
    }

    ex1 = ex;
    while (ex1 && (!ex->func || !ZEND_USER_CODE(ex1->func->type))) {
        ex1 = ex1->prev_execute_data;
    }
    if (ex1) {
        *filename = ex1->func->op_array.filename->val;
        *lineno = ex1->opline->lineno;
        ex = ex1;
    }

    if (ex->func) {
        zend_object *object;
        zend_function *func = ex->func;

        object = Z_OBJ(ex->This);
        if (object &&
            ex->func->type == ZEND_INTERNAL_FUNCTION &&
            !ex->func->common.scope) {
                object = NULL;
        }

        fn = (func->common.scope &&
                         func->common.scope->trait_aliases) ?
                zend_resolve_method_name(
                        (object ? object->ce : func->common.scope), func)->val :
                (func->common.function_name ?
                        func->common.function_name->val : NULL);
        if (fn) {
            function_name = fn;
            if (object) {
                if (func->common.scope) {
                        *class_name = func->common.scope->name->val;
                } else {
                        *class_name = object->ce->name->val;
                }
                *call_type = "->";
            } else if (func->common.scope) {
                *class_name = func->common.scope->name->val;
                *call_type = "::";
            }
        }
    }

    return function_name;
}
/* }}} */

static void do_journald_print(int priority, const char *message TSRMLS_DC) { /* {{{ */
    uint i, len;
    char *s;
    struct iovec iov[7]; /* MESSAGE, PRIORITY, SYSLOG_IDENTIFIER, SYSLOG_FACILTY, CODE_FILE, CODE_LINE, CODE_FUNC */
    char tmp[16 + 21 + 1]; /* "SYSLOG_FACILITY=" + str(2**64) + '\0' */
    int iov_current = 0;

    len = strlen("MESSAGE=") + strlen(message);
    s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
    strcpy(s, "MESSAGE=");
    strcat(s, message);
    iov[iov_current].iov_base = s;
    iov[iov_current].iov_len = len;
    iov_current++;

    if (!JOURNALD_G(suppress_syslog_fields)) {
        const char *ident = JOURNALD_G(syslog_identifier);
        len = snprintf(tmp, sizeof(tmp), "PRIORITY=%u", JOURNALD_G(priority));
        if (len > 0) {
            s = estrdup(tmp);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }
        len = snprintf(tmp, sizeof(tmp), "SYSLOG_FACILITY=%u", JOURNALD_G(syslog_facility));
        if (len > 0) {
            s = estrdup(tmp);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }
        if (ident && *ident != '\0') {
            len = strlen("SYSLOG_IDENTIFIER=") + strlen(ident);
            s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
            strcpy(s, "SYSLOG_IDENTIFIER=");
            strcat(s, ident);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }
    }

    if (!JOURNALD_G(suppress_location)) {
        uint lineno = 0;
        const char *filename = "";
        const char *class_name = "";
        const char *call_type = "";
        const char *function_name;
        function_name = php_get_caller_info(&filename, &lineno, &class_name, &call_type TSRMLS_CC);
        len = strlen("CODE_FILE=") + strlen(filename);
        s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
        strcpy(s, "CODE_FILE=");
        strcat(s, filename);
        iov[iov_current].iov_base = s;
        iov[iov_current].iov_len = len;
        iov_current++;

        len = snprintf(tmp, sizeof(tmp), "CODE_LINE=%u", lineno);
        if (len > 0) {
            s = estrdup(tmp);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }

        len = strlen("CODE_FUNC=") + strlen(class_name) + strlen(call_type) + strlen(function_name);
        s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
        strcpy(s, "CODE_FUNC=");
        strcat(s, class_name);
        strcat(s, call_type);
        strcat(s, function_name);
        iov[iov_current].iov_base = s;
        iov[iov_current].iov_len = len;
        iov_current++;
    }

    /* send everything to journald */
    sd_journal_sendv(iov, iov_current);

    /* clean up and exit */
    for (i = 0; i < iov_current; i++) {
        efree(iov[i].iov_base);
    }
    return;
}
/* }}} */

PHPAPI void php_journald_log(const char *message TSRMLS_DC) {
    do_journald_print(JOURNALD_G(priority), message TSRMLS_CC);
}

/* {{{ proto bool sd_journal_print(int priority, string format, ...)
 *    Generate a journal entry */
PHP_FUNCTION(sd_journal_print)
{
    long priority;
    zval *args = NULL;
    int argc = 0;
    char *message;
    zval retval;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l+", &priority,
                              &args, &argc) == FAILURE) {
        return;
    }

    if (argc == 1) {
        convert_to_string_ex(&args[0]);
        message = Z_STRVAL(args[0]);
    } else {
        zval fname;
        /* call PHP's sprintf function to do the formatting */
        ZVAL_STRING(&fname, "sprintf");
        if (call_user_function(EG(function_table), NULL, &fname, &retval, argc, args TSRMLS_CC) != SUCCESS) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to format string");
            zval_dtor(&fname);
            RETURN_FALSE;
        }
        zval_dtor(&fname);
        message = Z_STRVAL(retval);
    }

    do_journald_print(priority, message TSRMLS_CC);

    if (argc > 1) { zval_dtor(&retval); }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool sd_journal_printv(int priority, string format, array args)
 *    Generate a journal entry */
PHP_FUNCTION(sd_journal_printv)
{
    long priority;
    zval *args = NULL;
    int argc = 0;
    char *message;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l+", &priority,
                              &args, &argc) == FAILURE) {
        return;
    }

    if (argc == 1) {
        convert_to_string_ex(&args[0]);
        message = Z_STRVAL(args[0]);
        do_journald_print(priority, message TSRMLS_CC);
    } else {
        int i = 1;
        zval fname, retval;
        zval *newargs, *array, *zv, *z_format;

        /* Convert array to list of args */
        z_format = &args[0];
        array = &args[1];
        SEPARATE_ZVAL(array);
        convert_to_array_ex(array);

        argc = 1 + zend_hash_num_elements(Z_ARRVAL_P(array));
        newargs = (zval *) safe_emalloc(argc, sizeof(zval), 0);
        ZVAL_COPY_VALUE(&newargs[0], z_format);
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(array), zv) {
            ZVAL_COPY_VALUE(&newargs[i], zv);
            i++;
        } ZEND_HASH_FOREACH_END();
        args = newargs;

        /* call PHP's sprintf function to do the formatting */
        ZVAL_STRING(&fname, "sprintf");
        if (call_user_function_ex(EG(function_table), NULL, &fname, &retval, argc, args, 0, NULL TSRMLS_CC) != SUCCESS) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to format string");
            zval_dtor(&fname);
            efree(newargs);
            RETURN_FALSE;
        }
        zval_dtor(&fname);
        message = Z_STRVAL(retval);
        do_journald_print(priority, message TSRMLS_CC);
        zval_dtor(&retval);
        efree(newargs);
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool sd_journal_send(string message, ...)
   Submit log entry to the system journal */
PHP_FUNCTION(sd_journal_send)
{
    zval *args = NULL;
    int argc, current_arg, i, extra;
    uint len, iov_max;
    struct iovec *iov = NULL;
    int iov_current = 0;
    zval fname;
    int priority_seen = 0;
    int facility_seen = 0;
    int ident_seen = 0;
    int location_seen = 0;
    const char *ident = JOURNALD_G(syslog_identifier);
    char *s;
    char tmp[16 + 21 + 1]; /* "SYSLOG_FACILITY=" + str(2**64) + '\0' */

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "+", &args, &argc) == FAILURE) {
        return;
    }

    ZVAL_STRING(&fname, "sprintf");
    iov_max = 16;
    iov = (struct iovec *) safe_emalloc(iov_max, sizeof(struct iovec), 0);

    current_arg = 0;
    while (current_arg < argc) {
        char *format, *f, *field;
        int format_arg_count;
        zval retval;

        /* get the next format string */
        convert_to_string_ex(&args[current_arg]);
        format = Z_STRVAL(args[current_arg]);

        /* count arguments in the format string */
        format_arg_count = 0;
        f = format;
        while (*f != '\0') {
            if (*f == '%') {
                if (*(f+1) == '%') {
                    f++;
                } else {
                    format_arg_count++;
                }
            }
            f++;
        }
        if (current_arg + format_arg_count >= argc) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "not enough arguments: format string in parameter %d specifies %d parameters but only %d follow", current_arg + 1, format_arg_count, argc - current_arg - 1);
            goto failed;
        }

        if (format_arg_count == 0 ) {
            field = format;
        } else {
            /* call PHP's sprintf function to do the formatting */
            if (call_user_function(EG(function_table), NULL, &fname, &retval, format_arg_count + 1, args + current_arg TSRMLS_CC) != SUCCESS) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to format string at parameter %d", current_arg + 1);
                goto failed;
            }
            field = Z_STRVAL(retval);
        }

        /* TODO: warn if the string doesn't have a valid variable name followed by an equals sign? */

        if (!strncmp(field, "PRIORITY=", strlen("PRIORITY="))) {
            priority_seen = 1;
        } else
        if (!strncmp(field, "SYSLOG_FACILITY=", strlen("SYSLOG_FACILITY="))) {
            facility_seen = 1;
        } else
        if (!strncmp(field, "SYSLOG_IDENTIFIER=", strlen("SYSLOG_IDENTIFIER="))) {
            ident_seen = 1;
        } else
        if (!strncmp(field, "CODE_FILE=", strlen("CODE_FILE="))
            || !strncmp(field, "CODE_LINE=", strlen("CODE_LINE="))
            || !strncmp(field, "CODE_FUNC=", strlen("CODE_FUNC="))) {
            location_seen = 1;
        }

        /* add result to iovec */
        if (iov_current >= iov_max) {
            iov_max *= 2;
            iov = (struct iovec *) erealloc(iov, sizeof(struct iovec) * iov_max);
        }
        iov[iov_current].iov_base = estrdup(field);
        iov[iov_current].iov_len = strlen(field);
        iov_current++;

        if (format_arg_count > 0) { zval_dtor(&retval); }
        current_arg += 1 + format_arg_count;
    }

    if (iov_current == 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "nothing to send to journal");
        goto failed;
    }

    extra = 6 - priority_seen - facility_seen - ident_seen - 3*location_seen;
    if (iov_current + extra >= iov_max) {
        iov_max = iov_current + extra;
        iov = (struct iovec *) erealloc(iov, sizeof(struct iovec) * iov_max);
    }
    if (!priority_seen && !JOURNALD_G(suppress_syslog_fields)) {
        len = snprintf(tmp, sizeof(tmp), "PRIORITY=%u", JOURNALD_G(priority));
        if (len > 0) {
            s = estrdup(tmp);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }
    }
    if (!facility_seen && !JOURNALD_G(suppress_syslog_fields)) {
        len = snprintf(tmp, sizeof(tmp), "SYSLOG_FACILITY=%u", JOURNALD_G(syslog_facility));
        if (len > 0) {
            s = estrdup(tmp);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }
    }
    if (!ident_seen && ident && *ident != '\0' && !JOURNALD_G(suppress_syslog_fields)) {
        len = strlen("SYSLOG_IDENTIFIER=") + strlen(ident);
        s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
        strcpy(s, "SYSLOG_IDENTIFIER=");
        strcat(s, ident);
        iov[iov_current].iov_base = s;
        iov[iov_current].iov_len = len;
        iov_current++;
    }

    /* add location information unless it should be suppressed or some is already present */
    if (!location_seen && !JOURNALD_G(suppress_location)) {
        uint lineno = 0;
        const char *filename = "";
        const char *class_name = "";
        const char *call_type = "";
        const char *function_name;
        function_name = php_get_caller_info(&filename, &lineno, &class_name, &call_type TSRMLS_CC);
        len = strlen("CODE_FILE=") + strlen(filename);
        s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
        strcpy(s, "CODE_FILE=");
        strcat(s, filename);
        iov[iov_current].iov_base = s;
        iov[iov_current].iov_len = len;
        iov_current++;

        len = snprintf(tmp, sizeof(tmp), "CODE_LINE=%u", lineno);
        if (len > 0) {
            s = estrdup(tmp);
            iov[iov_current].iov_base = s;
            iov[iov_current].iov_len = len;
            iov_current++;
        }

        len = strlen("CODE_FUNC=") + strlen(class_name) + strlen(call_type) + strlen(function_name);
        s = (char *) emalloc((len+1) * sizeof(char)); /* len+1 for the \0 */
        strcpy(s, "CODE_FUNC=");
        strcat(s, class_name);
        strcat(s, call_type);
        strcat(s, function_name);
        iov[iov_current].iov_base = s;
        iov[iov_current].iov_len = len;
        iov_current++;
    }

    /* send everything to journald */
    sd_journal_sendv(iov, iov_current);

    /* clean up and exit */
    zval_dtor(&fname);
    for (i = 0; i < iov_current; i++) {
        efree(iov[i].iov_base);
    }
    efree(iov);
	RETURN_TRUE;

failed:
    zval_dtor(&fname);
    if (iov) {
        for (i = 0; i < iov_current; i++) {
            efree(iov[i].iov_base);
        }
        efree(iov);
    }
	RETURN_FALSE;

}
/* }}} */

#endif /* HAVE_JOURNALD */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */

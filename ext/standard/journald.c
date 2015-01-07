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

    if (!new_value) {
        return FAILURE;
    }
    if (!strncasecmp(new_value, "LOG_", strlen("LOG_"))) {
        new_value += strlen("LOG_");
    }

    if (!strcasecmp(new_value, "kern")) {
        facility = LOG_FAC(LOG_KERN);
    } else if (!strcasecmp(new_value, "user")) {
        facility = LOG_FAC(LOG_USER);
    } else if (!strcasecmp(new_value, "mail")) {
        facility = LOG_FAC(LOG_MAIL);
    } else if (!strcasecmp(new_value, "daemon")) {
        facility = LOG_FAC(LOG_DAEMON);
    } else if (!strcasecmp(new_value, "auth")) {
        facility = LOG_FAC(LOG_AUTH);
    } else if (!strcasecmp(new_value, "syslog")) {
        facility = LOG_FAC(LOG_SYSLOG);
    } else if (!strcasecmp(new_value, "lpr")) {
        facility = LOG_FAC(LOG_LPR);
#ifdef LOG_NEWS
    } else if (!strcasecmp(new_value, "news")) {
        facility = LOG_FAC(LOG_NEWS);
#endif
#ifdef LOG_UUCP
    } else if (!strcasecmp(new_value, "uucp")) {
        facility = LOG_FAC(LOG_UUCP);
#endif
#ifdef LOG_CRON
    } else if (!strcasecmp(new_value, "cron")) {
        facility = LOG_FAC(LOG_CRON);
#endif
#ifdef LOG_AUTHPRIV
    } else if (!strcasecmp(new_value, "authpriv")) {
        facility = LOG_FAC(LOG_AUTHPRIV);
#endif
#ifdef LOG_FTP
    } else if (!strcasecmp(new_value, "ftp")) {
        facility = LOG_FAC(LOG_FTP);
#endif
#ifndef PHP_WIN32
    } else if (!strcasecmp(new_value, "local0")) {
        facility = LOG_FAC(LOG_LOCAL0);
    } else if (!strcasecmp(new_value, "local1")) {
        facility = LOG_FAC(LOG_LOCAL1);
    } else if (!strcasecmp(new_value, "local2")) {
        facility = LOG_FAC(LOG_LOCAL2);
    } else if (!strcasecmp(new_value, "local3")) {
        facility = LOG_FAC(LOG_LOCAL3);
    } else if (!strcasecmp(new_value, "local4")) {
        facility = LOG_FAC(LOG_LOCAL4);
    } else if (!strcasecmp(new_value, "local5")) {
        facility = LOG_FAC(LOG_LOCAL5);
    } else if (!strcasecmp(new_value, "local6")) {
        facility = LOG_FAC(LOG_LOCAL6);
    } else if (!strcasecmp(new_value, "local7")) {
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

    if (!new_value) {
        return FAILURE;
    }
    if (!strncasecmp(new_value, "LOG_", strlen("LOG_"))) {
        new_value += strlen("LOG_");
    }

    if (!strcasecmp(new_value, "EMERG")) {
        priority = LOG_EMERG;
    } else if (!strcasecmp(new_value, "ALERT")) {
        priority = LOG_ALERT;
    } else if (!strcasecmp(new_value, "CRIT")) {
        priority = LOG_CRIT;
    } else if (!strcasecmp(new_value, "ERR")) {
        priority = LOG_ERR;
    } else if (!strcasecmp(new_value, "WARNING")) {
        priority = LOG_WARNING;
    } else if (!strcasecmp(new_value, "NOTICE")) {
        priority = LOG_NOTICE;
    } else if (!strcasecmp(new_value, "INFO")) {
        priority = LOG_INFO;
    } else if (!strcasecmp(new_value, "DEBUG")) {
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
    zend_class_entry *ce;
    const char *fn;
    const char *function_name = "[unknown function]";
    zend_execute_data *ex = EG(current_execute_data);
    *filename = "[unknown file]";
    *lineno = 0;
    *class_name = "";
    *call_type = "";
    if (!ex || !zend_is_executing(TSRMLS_C)) {
        return function_name;
    }
    if (ex->op_array) {
        *filename = ex->op_array->filename;
        *lineno = ex->opline->lineno;
    }
    /* Usually we'll be called via error_log(), sd_journal_send() or a similar
     * function.  It's not interesting to report these as the function that
     * logged something to the journal, since that is the primiary purpose of
     * these functions.  Instead, skip over the immediate calling PHP function
     * and report the name and line of the function that calls it.
     */
    if (ex->prev_execute_data) {
        ex = ex->prev_execute_data;
    }
    switch (ex->function_state.function->type) {
        case ZEND_USER_FUNCTION: {
            fn = ((zend_op_array *) ex->function_state.function)->function_name;
            if (fn) {
                if (*fn) {
                    ce = ex->function_state.function->common.scope;
                    *class_name = ce ? ce->name : "";
                    *call_type = ce ? "::" : "";
                    function_name = fn;
                }
            } else {
                function_name = "main";
            }
            break;
        }
        case ZEND_INTERNAL_FUNCTION: {
            fn = ((zend_internal_function *) ex->function_state.function)->function_name;
            if (fn && *fn) {
                ce = ex->function_state.function->common.scope;
                *class_name = ce ? ce->name : "";
                *call_type = ce ? "::" : "";
                function_name = fn;
            }
            break;
        }
        default:
            break;
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
    zval ***args = NULL;
    int argc = 0;
    char *message;
    zval retval;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l+", &priority,
                              &args, &argc) == FAILURE) {
        return;
    }

    if (argc == 1) {
        convert_to_string_ex(args[0]);
        message = Z_STRVAL_PP(args[0]);
    } else {
        zval fname;
        /* call PHP's sprintf function to do the formatting */
        INIT_ZVAL(fname);
        ZVAL_STRING(&fname, "sprintf", 1);
        if (call_user_function(EG(function_table), NULL, &fname, &retval, argc, *args TSRMLS_CC) != SUCCESS) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to format string");
            zval_dtor(&fname);
            efree(args);
            RETURN_FALSE;
        }
        zval_dtor(&fname);
        message = Z_STRVAL(retval);
    }

    do_journald_print(priority, message TSRMLS_CC);

    if (argc > 1) { zval_dtor(&retval); }
    if (argc > 0) { efree(args); }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool sd_journal_printv(int priority, string format, array args)
 *    Generate a journal entry */
PHP_FUNCTION(sd_journal_printv)
{
    long priority;
    zval ***args = NULL;
    int argc = 0;
    char *message;
    zval *retval_ptr;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l+", &priority,
                              &args, &argc) == FAILURE) {
        return;
    }

    if (argc == 1) {
        convert_to_string_ex(args[0]);
        message = Z_STRVAL_PP(args[0]);
    } else {
        int i = 1;
        zval fname;
        zval **array, **z_format;
        zval ***newargs;

        /* Convert array to list of args */
        z_format = args[0];
        array = args[1];
        SEPARATE_ZVAL(array);
        convert_to_array_ex(array);

        argc = 1 + zend_hash_num_elements(Z_ARRVAL_PP(array));
        newargs = (zval ***) safe_emalloc(argc, sizeof(zval *), 0);
        newargs[0] = z_format;
        for (zend_hash_internal_pointer_reset(Z_ARRVAL_PP(array));
             zend_hash_get_current_data(Z_ARRVAL_PP(array), (void **)&newargs[i++]) == SUCCESS;
             zend_hash_move_forward(Z_ARRVAL_PP(array)));
        efree(args);
        args = newargs;

        /* call PHP's sprintf function to do the formatting */
        INIT_ZVAL(fname);
        ZVAL_STRING(&fname, "sprintf", 1);
        if (call_user_function_ex(EG(function_table), NULL, &fname, &retval_ptr, argc, args, 0, NULL TSRMLS_CC) != SUCCESS || !retval_ptr) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "unable to format string");
            zval_dtor(&fname);
            efree(args);
            RETURN_FALSE;
        }
        zval_dtor(&fname);
        message = Z_STRVAL_P(retval_ptr);
    }

    do_journald_print(priority, message TSRMLS_CC);

    if (args) { efree(args); }
    if (retval_ptr) { zval_ptr_dtor(&retval_ptr); }
    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool sd_journal_send(string message, ...)
   Submit log entry to the system journal */
PHP_FUNCTION(sd_journal_send)
{
    zval ***args = NULL;
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

    INIT_ZVAL(fname);
    ZVAL_STRING(&fname, "sprintf", 1);
    iov_max = 16;
    iov = (struct iovec *) safe_emalloc(iov_max, sizeof(struct iovec), 0);

    current_arg = 0;
    while (current_arg < argc) {
        char *format, *f, *field;
        int format_arg_count;
        zval retval;

        /* get the next format string */
        convert_to_string_ex(args[current_arg]);
        format = Z_STRVAL_PP(args[current_arg]);

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
            if (call_user_function(EG(function_table), NULL, &fname, &retval, format_arg_count + 1, *args + current_arg TSRMLS_CC) != SUCCESS) {
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
    if (args) { efree(args); }
	RETURN_TRUE;

failed:
    zval_dtor(&fname);
    if (iov) {
        for (i = 0; i < iov_current; i++) {
            efree(iov[i].iov_base);
        }
        efree(iov);
    }
    if (args) { efree(args); }
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

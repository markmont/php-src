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

#ifndef PHP_JOURNALD_H
#define PHP_JOURNALD_H

#ifdef HAVE_JOURNALD

PHP_MINIT_FUNCTION(journald);
PHP_MSHUTDOWN_FUNCTION(journald);
PHP_MINFO_FUNCTION(journald);

PHP_FUNCTION(sd_journal_print);
PHP_FUNCTION(sd_journal_printv);
PHP_FUNCTION(sd_journal_send);

ZEND_BEGIN_MODULE_GLOBALS(journald)
    char *syslog_identifier;
    int syslog_facility;
    int priority;
    long suppress_location;
    long suppress_syslog_fields;
ZEND_END_MODULE_GLOBALS(journald)

ZEND_EXTERN_MODULE_GLOBALS(journald)

#ifdef ZTS
#define JOURNALD_G(v) TSRMG(journald_globals_id, zend_journald_globals *, v)
#else
#define JOURNALD_G(v) (journald_globals.v)
#endif

PHPAPI const char *php_get_caller_info(const char **filename, uint *lineno, const char **classname, const char **space TSRMLS_DC);
PHPAPI void php_journald_log(const char *message TSRMLS_DC);

#endif /* HAVE_JOURNALD */

#endif /* PHP_JOURNALD_H */

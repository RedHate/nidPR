/*
 * PSPLINK
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPLINK root for details.
 *
 * util.c - util functions for psplink
 *
 * Copyright (c) 2005 James F <tyranid@gmail.com>
 * Copyright (c) 2005 Julian T <lovely@crm114.net>
 *
 * $HeadURL: svn://svn.ps2dev.org/psp/trunk/psplink/psplink/util.c $
 * $Id: util.c 1981 2006-07-24 21:51:41Z tyranid $
 */
 
#define MODE_GENERIC 0
#define MODE_EXP 1
#define MODE_FLOAT_ONLY 2

static int is_nan(unsigned int val);
static int is_inf(unsigned int val);
static char get_num(float *val, int *exp);
void f_cvt(unsigned int addr, char *buf, int bufsize, int precision, int mode);

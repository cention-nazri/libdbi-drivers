/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) 2001-2002, David Parker and Mark Tobenkin.
 * http://libdbi.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * dbd_oracle.c: Oracle database support
 * Copyright (C) 2003, Christian M. Stamgren <christian@stamgren.com>
 * http://libdbi-drivers.sourceforge.net
 *
 */

/* 
   This driver is still in an unstable state don't use it for anything besides testing.

   There are ALOT of error checkings missing and not all data types are implemented.
   But hey! It's a start.

   Christian M. Stamgren <Christian@stamgren.com>

*/


#ifdef  HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE /* we need asprintf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>


#include <oci.h>
#include "dbd_oracle.h"


static const dbi_info_t driver_info = {
	"Oracle",
	"Oracle database support (using Oracle Call Interface)",
	"Christian M. Stamgren <christian@stamgren.com>",
	"http://libdbi-drivers.sourceforge.net",
	"dbd_Oracle v0.01",
	__DATE__
};

static const char *custom_functions[] = {NULL}; 
static const char *reserved_words[] = ORACLE_RESERVED_WORDS;

void _translate_oracle_type(int fieldtype, unsigned short *type, unsigned int *attribs);
void _get_field_info(dbi_result_t *result);
void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned long long rowidx);
void checkerr(OCIError * errhp, sword status);

void dbd_register_driver(const dbi_info_t **_driver_info, const char ***_custom_functions, 
			 const char ***_reserved_words) 
{
	*_driver_info = &driver_info;
	*_custom_functions = custom_functions;
	*_reserved_words = reserved_words;
}

int dbd_initialize(dbi_driver_t *driver) 
{
	return OCIInitialize((ub4) OCI_DEFAULT, (dvoid *)0,  
			     (dvoid * (*)(dvoid *, size_t)) 0,
			     (dvoid * (*)(dvoid *, dvoid *, size_t))0,
			     (void (*)(dvoid *, dvoid *)) 0 );
}

int dbd_connect(dbi_conn_t *conn) 
{
	Oraconn *Oconn = malloc( sizeof( Oraconn ));
	
	const char *username =  dbi_conn_get_option(conn, "username");
	const char *password =  dbi_conn_get_option(conn, "password");
	const char *sid      =  dbi_conn_get_option(conn, "dbname");
  
	if(! sid )
		sid = getenv("ORACLE_SID");


	if(OCIEnvCreate ((OCIEnv **) &(Oconn->env), OCI_DEFAULT, (dvoid *)0, 0, 0, 0, (size_t)0, (dvoid **)0)) {
		_dbd_internal_error_handler(conn, "Connect. Unable to initialize enviroment", 0);
		return 1;
	}
	if( (OCIHandleAlloc( (dvoid *) Oconn->env, (dvoid **) &(Oconn->err), OCI_HTYPE_ERROR, 
			     (size_t) 0, (dvoid **) 0))  ||
	    (OCIHandleAlloc( (dvoid *) Oconn->env, (dvoid **) &(Oconn->svc), OCI_HTYPE_SVCCTX,
			     (size_t) 0, (dvoid **) 0))) {
		_dbd_internal_error_handler(conn, "Connect: Unable to allocate handlers.", 0);
		return 1;
	}
	if( OCILogon(Oconn->env, Oconn->err, &(Oconn->svc), username, 
		     strlen(username), password, strlen(password), sid, strlen(sid))) {
		_dbd_internal_error_handler(conn, "Connect: Unable to login to the database.", 0);
		return 1;
	}

	conn->connection = (void *)Oconn;
	if (sid) conn->current_db = strdup(sid); 
  
	return 0;
}

int dbd_disconnect(dbi_conn_t *conn) 
{
	Oraconn *Oconn = conn->connection;
  
	if (Oconn) {
		OCILogoff(Oconn->svc, 
			  Oconn->err); 
		OCIHandleFree((dvoid *) Oconn->svc, OCI_HTYPE_SVCCTX);
		OCIHandleFree((dvoid *) Oconn->err, OCI_HTYPE_ERROR); 
	}
    
	free(conn->connection);
	conn->connection = NULL;
	return 0;
}


int dbd_fetch_row(dbi_result_t *result, unsigned long long rownum) 
{
	dbi_row_t *row = NULL;
  
	if (result->result_state == NOTHING_RETURNED) return -1;
	
	if (result->result_state == ROWS_RETURNED) {
		row = _dbd_row_allocate(result->numfields);
		_get_row_data(result, row, rownum);
		_dbd_row_finalize(result, row, rownum);
	}
  
	return 1; 
}


int dbd_free_query(dbi_result_t *result) 
{
	if (result->result_handle) OCIHandleFree((dvoid *)result->result_handle, OCI_HTYPE_STMT);
	return 0;
}


int dbd_goto_row(dbi_result_t *result, unsigned long long row) {
	/* do nothing OCIStmtFetch2() will fix this by it self */
	return 1;
}


int dbd_get_socket(dbi_conn_t *conn)
{
	return 0; /* Oracle can't do that.. */
}


dbi_result_t *dbd_list_dbs(dbi_conn_t *conn, const char *pattern) 
{
	return NULL; /* Oracle can't do that */
}


dbi_result_t *dbd_list_tables(dbi_conn_t *conn, const char *db, const char *pattern) 
{
	dbi_result_t *res;
	char *sql_cmd;
	
	/* We just ignore the db param, becouse of Oracle can't read from diffrent databases at runtime */
	if (pattern == NULL) {
		asprintf(&sql_cmd, "SELECT table_name FROM user_tables"); 
		res = dbd_query(conn, sql_cmd);
		free(sql_cmd);
		return res;
	}
	else {
		asprintf(&sql_cmd, "SELECT table_name FROM user_tables WHERE table_name LIKE '%s'",pattern);
		res = dbd_query(conn, sql_cmd);
		free(sql_cmd);
		return res;
	}
}


int dbd_quote_string(dbi_driver_t *driver, const char *orig, char *dest) 
{
	unsigned int len;

	strcpy(dest, "'");
	const char *escaped = "\'\"\\";
	len = _dbd_quote_chars(dest, orig, escaped);
	
	strcat(dest, "'");
	
	return len+2;
}


dbi_result_t *dbd_query_null(dbi_conn_t *conn, const char unsigned *statement, unsigned long st_length) 
{
	OCIStmt *stmt;
	OCIDefine *defnp;
	ub2 stmttype = 0;
	ub4 numfields = 0;
	dbi_result_t *result;
	ub4 numrows =  0, affectedrows = 0;
	Oraconn *Oconn = conn->connection;
	sword status,notused;
	ub4 cache_rows = 0;

	OCIHandleAlloc( (dvoid *) Oconn->env, (dvoid **) &stmt,
			OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);

	
	if( OCIStmtPrepare(stmt, Oconn->err, (char  *) statement,
			   (ub4) st_length, (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT)) {
		return NULL;
	}
	

	if ( OCIAttrGet(stmt, OCI_HTYPE_STMT, (dvoid *) &stmttype,
			(ub4 *) 0, (ub4) OCI_ATTR_STMT_TYPE, Oconn->err) != OCI_SUCCESS) {
	}
	
	OCIStmtExecute(Oconn->svc, stmt, Oconn->err, 
		       (ub4) (stmttype == OCI_STMT_SELECT ? 0 : 1), 
		       (ub4) 0, (CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, 
		       OCI_STMT_SCROLLABLE_READONLY);


	
       
	if( stmttype == OCI_STMT_SELECT) { 
                /* get the number of columns */
		OCIAttrGet (stmt, OCI_HTYPE_STMT, (dvoid *) &numfields, 
			    (ub4 *) 0, (ub4) OCI_ATTR_PARAM_COUNT, Oconn->err); 

		
		/* 
		   To find out how many rows there is in a result set we need to call 
		   OCIStmtFetch2() with OCI_FETCH_LAST and then use OCIAttrGet()
		   with OCI_ATTR_CURRENT_POSITION, This is really not that great 
		   becouse it might be very very very slow..... But It's the only way i know
		   It would be really great if libdbi didn't have to know how large a result set is
		   at this early point.
		*/

		/* dummy define .. We need have atleast one define before fetching. Duh!!! */
		OCIDefineByPos(stmt, &defnp, Oconn->err, 1, (dvoid *) &notused,
			       (sword) sizeof(sword), SQLT_INT, (dvoid *) 0, (ub2 *)0,
			       (ub2 *)0, OCI_DEFAULT); 

		
		status = OCIStmtFetch2(stmt, Oconn->err,
			       (ub4)10, OCI_FETCH_LAST, 0, OCI_DEFAULT);
		checkerr(Oconn->err, status);

		status = OCIAttrGet (stmt, OCI_HTYPE_STMT, (dvoid *) &numrows, 
				     (ub4 *) 0, (ub4) OCI_ATTR_CURRENT_POSITION, Oconn->err); 
		checkerr(Oconn->err, status);

		/* cache should be about 20% of all rows. */
		if(dbi_conn_get_option_numeric(conn, "oracle_prefetch_rows")) {
			cache_rows = (ub4)numrows/5;
			OCIAttrSet(stmt, OCI_HTYPE_STMT,
				   &cache_rows, sizeof(cache_rows), OCI_ATTR_PREFETCH_ROWS,
				   Oconn->err);
		}
		//(void) fprintf(stderr, "fields: %d rows: %d cached rows %d\n", numfields, numrows, cache_rows);
		

		/* howto handle affected rows? I don't know .... */
		
	}


	result = _dbd_result_create(conn, (void *)stmt, numrows , affectedrows);
	_dbd_result_set_numfields(result, numfields);
	_get_field_info(result);
	
	return result;
}

dbi_result_t *dbd_query(dbi_conn_t *conn, const char *statement) 
{
	return dbd_query_null(conn, statement, strlen(statement));
}


char *dbd_select_db(dbi_conn_t *conn, const char *db) 
{
	return NULL; /* Oracle can not do that .... */
}

int dbd_geterror(dbi_conn_t *conn, int *errno, char **errstr) 
{
	char errbuf[1024];
	int  errcode = 0;
	Oraconn *Oconn = conn->connection;
	*errno = 0;

	if (!conn->connection) {
		*errstr = strdup("Unable to connect to database");
		return 2;
	} else {
		OCIErrorGet((dvoid *)Oconn->err, (ub4) 1, (text *) NULL, &errcode, errbuf, 
			    (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
		*errstr = strdup(errbuf);
		*errno = errcode;
  }
	
	return 3;
}


unsigned long long dbd_get_seq_last(dbi_conn_t *conn, const char *sequence) 
{  
	OCIStmt *stmt;
	char *sql_cmd;
	sword currval = 0;
	OCIDefine *defnp = (OCIDefine *) 0;
	
	Oraconn *Oconn = conn->connection;
	
	asprintf(&sql_cmd, "SELECT %s.currval FROM dual", sequence);
	
	OCIHandleAlloc( (dvoid *) Oconn->env, (dvoid **) &stmt,
			OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);
	OCIStmtPrepare(stmt, Oconn->err, (char  *) sql_cmd,
		       (ub4) strlen(sql_cmd), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
	free(sql_cmd);
	
	OCIDefineByPos(stmt, &defnp, Oconn->err, 1, (dvoid *) &currval,
		       (sword) sizeof(sword), SQLT_INT, (dvoid *) 0, (ub2 *)0,
		       (ub2 *)0, OCI_DEFAULT);

	OCIStmtExecute(Oconn->svc, stmt, Oconn->err, 
		       (ub4) 1, (ub4) 0, (CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, 
		       OCI_DEFAULT); 
  
	OCIStmtFetch(stmt, Oconn->err, (ub4) 1, (ub4) OCI_FETCH_NEXT,
		     (ub4) OCI_DEFAULT) ;

	(void) OCIHandleFree((dvoid *) stmt, OCI_HTYPE_STMT);
  
	return (currval ? (unsigned long long)currval : 0);
}


unsigned long long dbd_get_seq_next(dbi_conn_t *conn, const char *sequence) 
{	
	OCIStmt *stmt;
	char *sql_cmd;
	sword nextval = 0;
	OCIDefine *defnp = (OCIDefine *) 0;
	
	Oraconn *Oconn = conn->connection;
	
	asprintf(&sql_cmd, "SELECT %s.nextval FROM dual", sequence);
	
	OCIHandleAlloc( (dvoid *) Oconn->env, (dvoid **) &stmt,
			OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0);
	OCIStmtPrepare(stmt, Oconn->err, (char  *) sql_cmd,
		       (ub4) strlen(sql_cmd), (ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT);
	free(sql_cmd);
	
	OCIDefineByPos(stmt, &defnp, Oconn->err, 1, (dvoid *) &nextval,
		       (sword) sizeof(sword), SQLT_INT, (dvoid *) 0, (ub2 *)0,
		       (ub2 *)0, OCI_DEFAULT);
  
	OCIStmtExecute(Oconn->svc, stmt, Oconn->err, 
		       (ub4) 1, (ub4) 0, (CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, 
		       OCI_DEFAULT); 
  
	OCIStmtFetch(stmt, Oconn->err, (ub4) 1, (ub4) OCI_FETCH_NEXT,
		     (ub4) OCI_DEFAULT);
  
	(void) OCIHandleFree((dvoid *) stmt, OCI_HTYPE_STMT);
	
	return (nextval ? (unsigned long long)nextval : 0);
}


int dbd_ping(dbi_conn_t *conn) 
{
	/* select something from dual? */
	return 0;
}

/* CORE ORACLE DATA FETCHING STUFF */



void _translate_oracle_type(int fieldtype, unsigned short *type, unsigned int *attribs) 
{
	unsigned int _type = 0;
	unsigned int _attribs = 0;
	
	//(void) fprintf(stderr, "Got type nr %d", fieldtype);

	switch (fieldtype) {

	case SQLT_INT:
	case SQLT_NUM:
	case SQLT_LNG:
		_type = DBI_TYPE_INTEGER;
		_attribs |= DBI_INTEGER_SIZE8;
		break;
		
	case SQLT_BIN:
	case SQLT_LBI:
		_type = DBI_TYPE_BINARY;
		break;
	  
	case SQLT_FLT:
		_type = DBI_TYPE_DECIMAL;
		break;
	  

		
	case SQLT_AFC:
	case SQLT_STR:
	case SQLT_CHR:

	case SQLT_VNU:
	case SQLT_VCS:
	case SQLT_DAT:

	default:
		_type = DBI_TYPE_STRING;
		break;
	}
	
	*type = _type;
	*attribs = _attribs;
}
 
void _get_field_info(dbi_result_t *result) 
{
	unsigned int idx = 0;
	unsigned short fieldtype;
	unsigned int fieldattribs;
	OCIParam *param;
	ub4 otype;
	text *col_name;
	ub4  col_name_len;
	
	Oraconn *Oconn = (Oraconn *)result->conn->connection;
	
	
	
	while (idx < result->numfields) {
		
		OCIParamGet((dvoid *)result->result_handle, OCI_HTYPE_STMT, Oconn->err, (dvoid **)&param,
			    (ub4) idx+1);
		
		OCIAttrGet((dvoid*) param, (ub4) OCI_DTYPE_PARAM,
			   (dvoid*) &otype,(ub4 *) 0, (ub4) OCI_ATTR_DATA_TYPE,
			   (OCIError *) Oconn->err  );
	 
		OCIAttrGet((dvoid*) param, (ub4) OCI_DTYPE_PARAM,
			   (dvoid**) &col_name,(ub4 *) &col_name_len, (ub4) OCI_ATTR_NAME,
			   (OCIError *) Oconn->err );
	
	  
		_translate_oracle_type(otype, &fieldtype, &fieldattribs);
		_dbd_result_add_field(result, idx, (char *)col_name, fieldtype, fieldattribs);
		idx++;
	}
}



void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned long long rowidx) 
{
	OCIStmt *stmt = result->result_handle;
	OCIDefine *defnp = (OCIDefine *) 0;
	OCIParam *param;
	Oraconn *Oconn = result->conn->connection; 
	int curfield = 0, length = 0;
	unsigned long sizeattrib;
	dbi_data_t *data;
	dword status;
	ub1 nullable;

	while (curfield < result->numfields) {
		
		data = &row->field_values[curfield];
		row->field_sizes[curfield] = 0;
        
		OCIParamGet(stmt, OCI_HTYPE_STMT, Oconn->err, (dvoid **)&param,
			    (ub4) curfield+1);
		
		OCIAttrGet((dvoid*) param, (ub4) OCI_DTYPE_PARAM,
			   (dvoid*) &length,(ub4 *) 0, (ub4) OCI_ATTR_DATA_SIZE,
			   (OCIError *) Oconn->err  );

		OCIAttrGet((dvoid*) param, (ub4) OCI_DTYPE_PARAM,
			   (dvoid*) &nullable,(ub4 *) 0, (ub4) OCI_ATTR_IS_NULL,
			   (OCIError *) Oconn->err  );


		switch (result->field_types[curfield]) {
		case DBI_TYPE_INTEGER:
			sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);
			switch (sizeattrib) {
			case DBI_INTEGER_SIZE1:
			case DBI_INTEGER_SIZE2:
			case DBI_INTEGER_SIZE3:
			case DBI_INTEGER_SIZE4:
			case DBI_INTEGER_SIZE8:
				//(void) fprintf(stderr, "Got a integer\n");
				
				break;
			default:
				break;
			}
			break;
		case DBI_TYPE_DECIMAL:
			sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);
			switch (sizeattrib) {
			case DBI_DECIMAL_SIZE4:
			case DBI_DECIMAL_SIZE8:
				
				break;
			default:
				break;
			}
			break;
		case DBI_TYPE_BINARY:
		case DBI_TYPE_STRING:
			//(void) fprintf(stderr, "we have a string\n");

			data->d_string = (char *)calloc(1, length +1);
			row->field_sizes[curfield] = length;
			
			OCIDefineByPos(stmt, &defnp, Oconn->err, curfield+1, data->d_string,
				       (sword) length, SQLT_CHR, (dvoid *) 0, (ub2 *)0, 
				       (ub2 *)0, OCI_DEFAULT);
			break;

		case DBI_TYPE_DATETIME:
			/* don't know yet, string? */
			break;
      
		default:
			printf("Unsupported type, please implement me..");
			break;
		}
    
		curfield++;
		//(void) fprintf(stderr, "\nOn culumn: %d\n", curfield);
	}

	
	status = OCIStmtFetch2(stmt, Oconn->err,
			       (ub4)1, OCI_FETCH_ABSOLUTE, (sb4)rowidx+1, OCI_DEFAULT);
	checkerr(Oconn->err, status);


	if (dbi_conn_get_option_numeric(result->conn, "oracle_chop_blanks")) {

		unsigned short column = result->numfields;
		char *ptr;
		int slen;
		
		while(column--) {
			slen = row->field_sizes[column];
			ptr = row->field_values[column].d_string;

			if(ptr  && *ptr != '\0') { /* the column is a string type. */
				while(slen && ptr[slen - 1] == ' ')
					--slen;
				
				ptr[slen] = '\0'; /* Chop blanks */
				row->field_sizes[column] = slen; /* alter field length */
			}
		}
	}
}


void checkerr(OCIError * errhp, sword status)
{
	text errbuf[512];
	ub4 buflen;
	ub4 errcode;

	switch (status)
	{
	case OCI_SUCCESS:
		break;
	case OCI_SUCCESS_WITH_INFO:
		(void) fprintf(stderr,"Error - OCI_SUCCESS_WITH_INFO\n");
		break;
	case OCI_NEED_DATA:
		(void) fprintf(stderr, "Error - OCI_NEED_DATA\n");
		break;
	case OCI_NO_DATA:
		(void) fprintf(stderr,"Error - OCI_NODATA\n");
		break;
	case OCI_ERROR:
		(void) OCIErrorGet (errhp, (ub4) 1, (text *) NULL, &errcode,
				    errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);

		(void) fprintf(stderr,"Error - %s\n", errbuf);
		break;
	case OCI_INVALID_HANDLE:
		(void) fprintf(stderr,"Error - OCI_INVALID_HANDLE\n");
		break;
	case OCI_STILL_EXECUTING:
		(void) fprintf(stderr,"Error - OCI_STILL_EXECUTE\n");
		break;
	default:
		break;
	}
}


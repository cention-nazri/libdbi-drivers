/*
 * libdbi-drivers - 3rd-party drivers for libdbi, a database independent
 * abstraction layer for C.
 * Copyright (C) 2002, Markus Hoenicka
 * http://libdbi-drivers.sourceforge.net
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * dbd_sqlite.c: SQLite database support (using libsqlite)
 * Copyright (C) 2002, Markus Hoenicka <mhoenicka@users.sourceforge.net>
 * http://libdbi-drivers.sourceforge.net
 * 
 * $Id: dbd_sqlite.c,v 1.13 2003/05/26 20:04:42 mhoenicka Exp $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE /* we need asprintf */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h> /* defines _POSIX_PATH_MAX */
#include <dirent.h> /* directory listings */
#include <unistd.h> /* stat */
#include <sys/stat.h> /* S_ISXX macros */
#include <sys/types.h> /* directory listings */

#include <dbi/dbi.h>
#include <dbi/dbi-dev.h>
#include <dbi/dbd.h>

#include <sqlite.h>
#include "dbd_sqlite.h"

static const dbi_info_t driver_info = {
  "sqlite",
  "SQLite database support (using libsqlite)",
  "Markus Hoenicka <mhoenicka@users.sourceforge.net>",
  "http://libdbi-drivers.sourceforge.net",
  "dbd_sqlite v" VERSION,
  __DATE__
};

static const char *custom_functions[] = {NULL}; // TODO
static const char *reserved_words[] = SQLITE_RESERVED_WORDS;

/* forward declarations */
int _real_dbd_connect(dbi_conn_t *conn, const char* database);
void _translate_sqlite_type(enum enum_field_types fieldtype, unsigned short *type, unsigned int *attribs);
void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx);
int find_result_field_types(char* field, dbi_conn_t *conn, const char* statement);
char* get_field_type(const char* statement, const char* curr_field_name);
time_t _parse_datetime(const char *raw, unsigned long attribs);
static unsigned long sqlite_escape_string(char *to, const char *from, unsigned long length);
int wild_case_compare(const char *str,const char *str_end,
		      const char *wildstr,const char *wildend,
		      char escape);


/* the real functions */
void dbd_register_driver(const dbi_info_t **_driver_info, const char ***_custom_functions, const char ***_reserved_words) {
  /* this is the first function called after the driver module is loaded into memory */
  *_driver_info = &driver_info;
  *_custom_functions = custom_functions;
  *_reserved_words = reserved_words;
}

int dbd_initialize(dbi_driver_t *driver) {
  /* perform any database-specific server initialization.
   * this is called right after dbd_register_driver().
   * return -1 on error, 0 on success. if -1 is returned, the driver will not
   * be added to the list of available drivers. */
	
  return 0;
}



int dbd_connect(dbi_conn_t *conn) {
    return _real_dbd_connect(conn, "");
  return 0;
}

int _real_dbd_connect(dbi_conn_t *conn, const char* database) {
  sqlite *sqcon;
  char* sq_errmsg = NULL;
  char* db_fullpath = NULL;

/* ToDo: make OS-independent */
  const char dirsep[] = "/";

  const char *dbname;
  const char *dbdir;

  /* initialize error stuff */
  conn->error_number = 0;
  conn->error_message = NULL;

  /* sqlite does not use hostname, username, password, port */
  if (database && *database) {
    dbname = database;
  }
  else {
    dbname = dbi_conn_get_option(conn, "dbname");
  }

  if (!dbname) {
    _dbd_internal_error_handler(conn, "no database specified", 0);
    return -1;
  }

  /* sqlite specific options */
  dbdir = dbi_conn_get_option(conn, "sqlite_dbdir");
	
  if (!dbdir) {
    _dbd_internal_error_handler(conn, "no database directory specified", 0);
    return -1;
  }

  /* the requested database is a file in the given directory. Assemble
     full path of database */
  db_fullpath = malloc(strlen(dbname)+strlen(dbdir)+2); /* leave room
							   for \0 and / */
  if (db_fullpath == NULL) {
    _dbd_internal_error_handler(conn, "out of memory", 0);
    return -1;
  }

  /* start with an empty string */
  db_fullpath[0] = '\0';
	
  if (dbdir && *dbdir) {
    strcpy(db_fullpath, dbdir);
  }
  if (db_fullpath[strlen(db_fullpath)-1] != *dirsep) {
    strcat(db_fullpath, dirsep);
  }
  if (dbname && *dbname) {
    strcat(db_fullpath, dbname);
  }

/*   fprintf(stderr, "try to open %s<<\n", db_fullpath); */
  sqcon = sqlite_open(db_fullpath, 0 /* param not used */, &sq_errmsg);
  free(db_fullpath);
	
  if (!sqcon) {
    if (sq_errmsg) {
      _dbd_internal_error_handler(conn, sq_errmsg, 0);
      free(sq_errmsg);
    }
    else {
      _dbd_internal_error_handler(conn, "could not open database", 0);
    }
    return -1;
  }
  else {
    conn->connection = (void *)sqcon;
    if (dbname) {
      conn->current_db = strdup(dbname);
    }
  }
	
  return 0;
}

int dbd_disconnect(dbi_conn_t *conn) {
  if (conn->connection) {
    sqlite_close((sqlite *)conn->connection);
    if (conn->error_number) {
      conn->error_number = 0;
    }
    if (conn->error_message) {
      free(conn->error_message);
      conn->error_message = NULL;
    }
  }
  return 0;
}

int dbd_fetch_row(dbi_result_t *result, unsigned long long rownum) {
  dbi_row_t *row = NULL;

  if (result->result_state == NOTHING_RETURNED) return -1;
	
  if (result->result_state == ROWS_RETURNED) {
    /* get row here */
    row = _dbd_row_allocate(result->numfields);
    _get_row_data(result, row, rownum);
    _dbd_row_finalize(result, row, rownum);
  }
	
  return 1; /* 0 on error, 1 on successful fetchrow */
}

int dbd_free_query(dbi_result_t *result) {
  if (result->result_handle) {
    sqlite_free_table((char **)result->result_handle);
  }
  return 0;
}

int dbd_goto_row(dbi_result_t *result, unsigned long long row) {
  result->currowidx = row;
  return 1;
}

int dbd_get_socket(dbi_conn_t *conn){
  /* sqlite does not use sockets, so we'll always return 0 */
  return (int)0;
}

dbi_result_t *dbd_list_dbs(dbi_conn_t *conn, const char *pattern) {
  dbi_result_t *res;
  char *sq_errmsg = NULL;
  char old_cwd[_POSIX_PATH_MAX] = "";
  int retval;
  DIR *dp;
  struct dirent *entry;
  struct stat statbuf;

  /* sqlite has no builtin function to list databases. Databases are just
     files in the data directory. We search for matching files and fill a
     temporary table with what we've found. Then we query this table and
     pretend sqlite has done all the work */
  const char *sq_datadir = dbi_conn_get_option(conn, "sqlite_dbdir");

  /* this is not nice but we have to drop the table even if it does not
   exist (sqlite has no way to list *temporary* tables so we can't check
   for it's existence). Then we start over with a fresh table lest we
   want duplicates.
   Update: Now apparently there is a system table that lists
   temporary tables, but the DROP TABLE error doesn't hurt and is
   most likely faster than checking for the existence of the table */
  dbd_query(conn, "DROP TABLE databases");
  dbd_query(conn, "CREATE TEMPORARY TABLE databases (dbname VARCHAR(255))");

  if (sq_datadir && (dp = opendir(sq_datadir)) == NULL) {
    _dbd_internal_error_handler(conn, "could not open data directory", 0);
    return NULL;
  }
  getcwd(old_cwd, _POSIX_PATH_MAX);
  chdir(sq_datadir);

  while ((entry = readdir(dp)) != NULL) {
    stat(entry->d_name, &statbuf);
    if (S_ISREG(statbuf.st_mode)) {
      /* we do a magic number check here to make sure we
	 get only databases, not random files in the current directory.
	 SQLite databases start with the string:
	 
	 ** This file contains an SQLite 2.1 database **

      */
      FILE* fp;

      if ((fp = fopen(entry->d_name, "r")) != NULL) {
	char magic_text[48] = "";

	if (fread(magic_text, 1, 47, fp) < 47) {
	  /* either we can't read at all, or the file is too small
	     for a sqlite database anyway */
	  fclose(fp);
	  continue;
	}

	/* terminate magic text */
	magic_text[47] = '\0';

	if (strcmp(magic_text, "** This file contains an SQLite 2.1 database **")) {
	  /* this file is not meant for us */
	  fclose(fp);
	  continue;
	}

	/* close file again, we're done reading */
	fclose(fp);

	/* match filename to a pattern, or use all found files */
	if (pattern) {
	  if (wild_case_compare(entry->d_name, &entry->d_name[strlen(entry->d_name)], pattern, &pattern[strlen(pattern)], '\\') == 0) {
	    retval = sqlite_exec_printf((sqlite*)(conn->connection), "INSERT INTO databases VALUES ('%s')", NULL, NULL, &sq_errmsg, entry->d_name);
	    if (sq_errmsg) {
	      _dbd_internal_error_handler(conn, sq_errmsg, retval);
	      free(sq_errmsg);
	      break;
	    }
	  }
	}
	else {
	  retval = sqlite_exec_printf((sqlite*)(conn->connection), "INSERT INTO databases  VALUES ('%s')", NULL, NULL, &sq_errmsg, entry->d_name);
	  
	  if (sq_errmsg) {
	    _dbd_internal_error_handler(conn, sq_errmsg, retval);
	    free(sq_errmsg);
	    break;
	  }
	}
      }
      /* else: we can't read it, so forget about it */
    }
  }
  closedir(dp);
  chdir(old_cwd);

  /* now query our temporary table */
  return dbd_query(conn, "SELECT dbname FROM databases");
}

dbi_result_t *dbd_list_tables(dbi_conn_t *conn, const char *db, const char *pattern) {
  /* list tables in a database. The current implementation lists permanent
     tables only, as most applications know about the temporary tables
     they created anyway.
   */
  dbi_result_t *dbi_result;
  dbi_conn_t* tempconn;
  int retval;
  char* sq_errmsg;
  char* sql_cmd;

  /* this function tries to query a specific database, so we need a
   separate connection to that other database, retrieve the table names,
   and feed them to a temporary table in our main connection */
  tempconn = dbi_conn_new("sqlite");

  /* we explicitly cast to (char*) as we discard the "const" thing here */
  dbi_conn_set_option(tempconn, "dbname", (char*)db);
  dbi_conn_set_option(tempconn, "sqlite_dbdir", (char*)dbi_conn_get_option(conn, "sqlite_dbdir"));

  if (dbi_conn_connect(tempconn) < 0) {
    _dbd_internal_error_handler(conn, "could not connect to database", 0);
    return NULL;
  }
  
  /* create temporary table for table names. The DROP command won't hurt
     if the table doesn't exist yet */
  dbd_query(conn, "DROP TABLE tablenames");
  dbd_query(conn, "CREATE TEMPORARY TABLE tablenames (tablename VARCHAR(255))");
/*   fprintf(stderr, "created temporary table\n"); */

  /* sqlite does not support the SHOW command, so we have to extract the
     information from the accessory sqlite_master table */
  if (pattern == NULL) {
    asprintf(&sql_cmd, "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
  }
  else {
    asprintf(&sql_cmd, "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '%s' ORDER BY name", pattern);
  }
  dbi_result = dbd_query(tempconn, sql_cmd);
  free(sql_cmd);
/*   fprintf(stderr, "select from sqlite_master has run\n"); */
  if (dbi_result) {
    while (dbi_result_next_row(dbi_result)) {
      retval = sqlite_exec_printf((sqlite*)(conn->connection), "INSERT INTO tablenames VALUES ('%s')", NULL, NULL, &sq_errmsg, dbi_result_get_string(dbi_result, "name"));
    }
    dbi_result_free(dbi_result);
  }
  else {
    dbi_conn_error(tempconn, (const char**)&sq_errmsg);
    free(sq_errmsg);
  }

  sqlite_close((sqlite*)(tempconn->connection));

  return dbd_query(conn, "SELECT tablename FROM tablenames ORDER BY tablename");
}

dbi_result_t *dbd_query_null(dbi_conn_t *conn, const unsigned char *statement, unsigned long st_length) {
	return NULL;
}

int dbd_quote_string(dbi_driver_t *driver, const char *orig, char *dest) {
  /* foo's -> 'foo\'s' */
  unsigned int len;
	
  strcpy(dest, "'");
  len = sqlite_escape_string(dest+1, orig, strlen(orig));	
  strcat(dest, "'");
	
  return len+2;
}

dbi_result_t *dbd_query(dbi_conn_t *conn, const char *statement) {
  /* allocate a new dbi_result_t and fill its applicable members:
   * 
   * result_handle, numrows_matched, and numrows_changed.
   * everything else will be filled in by DBI */
	
  dbi_result_t *result;
  int query_res;
  int numrows;
  int numcols;
  char** result_table;
  char* errmsg;
  unsigned short idx = 0;
  unsigned short fieldtype;
  unsigned int fieldattribs;
  dbi_error_flag errflag = 0;

  query_res = sqlite_get_table((sqlite*)conn->connection,
			       statement,
			       &result_table,
			       &numrows,
			       &numcols,
			       &errmsg);

  if (query_res) {
    _error_handler(conn, errflag);
    return NULL;
  }
	
  result = _dbd_result_create(conn, (void *)result_table, (unsigned long long)numrows, (unsigned long long)sqlite_changes((sqlite*)conn->connection));
/*   printf("numrows:%d, numcols:%d<<\n", numrows, numcols); */
  _dbd_result_set_numfields(result, (unsigned short)numcols);

  /* assign types to result */
  while (idx < (unsigned short)numcols) {
    int type;
    char *item;
    
    type = find_result_field_types(result_table[idx], conn, statement);
/*     printf("type: %d<<\n", type); */
    _translate_sqlite_type(type, &fieldtype, &fieldattribs);

    /* we need the field name without the table name here */
    item = strchr(result_table[idx], (int)'.');
    if (!item) {
      item = result_table[idx];
    }
    else {
      item++;
    }

    _dbd_result_add_field(result, idx, item, fieldtype, fieldattribs);
    idx++;
  }
  
  /* ToDo: result_table is allocated memory, but we can't free
   it here lest we want segfaults. It is quite unclear to me when, if
   at all, this memory is deallocated */

  return result;
}

int find_result_field_types(char* field, dbi_conn_t *conn, const char* statement) {
  /* 
     field is the name of the field which we want to know the type of
     conn is the connection
     statement is the query string

     returns the type as a FIELD_TYPE_XXX value

     sqlite is typeless (with one exception) on purpose. You can
     declare types with the CREATE TABLE statement, but they serve
     descriptive purposes only. Therefore libsqlite does not provide
     any helper function to find out about the type of a given field.

     However, sqlite stores the CREATE TABLE commands as a string in
     an internal table, so we can try to look up the types in these
     strings. It is a VERY GOOD idea to declare the types if we want
     the following to work

     The code assumes that table and field names do not exceed a given
     length limit. PostgreSQL uses 32 which is a bit low. SQLite
     does not seem to have fixed limits. We use a limit of 128 here.
   */

  char* item;
  char* table;
  char curr_table[128] = "";
  char curr_field_name[128];
  char curr_field_name_up[128];
  char **table_result_table;
  char *curr_type;
  char* errmsg;
  int i;
  int query_res;
  int table_numrows = 0;
  int table_numcols = 0;
  int type;
  dbi_error_flag errflag = 0;

  /* check whether field contains the table info. It does if the
   notation "table.field" is used */
  item = strchr(field, (int)'.');
  if (!item) {
    /* the fields do not contain the table info. This means that
       all fields are from the same table which we have to extract
       from the statement that created the result */

    /* To get started, we use the first item after 'from' or 'FROM'
       as the table name (we currently ignore pathologic cases like
       'FroM' or 'froM'. We could uppercase a copy but we need the
       table name as is, so it is going to get complex) */
    if (!(table = strstr(statement, " from "))) {
      table = strstr(statement, " FROM ");
    }

    if (!table) {
/*       fprintf(stderr, "no from keyword found\n"); */
      return 0;
    }
    
    /* set ptr to possible start of item after 'from' */
    table += 6;

    /* skip spaces */
    while (*table == ' ') {
      table++;
    }

    /* table now points to the table name; find the end of table */
    item = strchr(table, ' ');
    if (!item) {
      strcpy(curr_table, table);
    }
    else {
      strncpy(curr_table, table, item-table);
      curr_table[item-table] = '\0'; /* terminate just in case */
    }

    /* for obvious reasons, the internal tables do not contain the
       commands how they were created themselves. We have to use known
       values for the field types */
    if (!strcmp(curr_table, "sqlite_master") ||
	!strcmp(curr_table, "sqlite_temp_master")) {
      if (!strcmp(field, "rootpage")) {
	return FIELD_TYPE_LONG;
      }
      else {
	return FIELD_TYPE_STRING;
      }
    }

    strcpy(curr_field_name, field);
  }
  else {   /* each field contains table info */
    strncpy(curr_table, field, item-field);
    curr_table[item-field] = '\0';
    strcpy(curr_field_name, item+1);
  }

/*   printf("curr_table went to %s<<\ncurr_field_name went to %s<<\n", curr_table, curr_field_name); */

  /* check for known functions which may appear here instead
     of field names. There is some overlap, i.e. some function work
     both on strings and numbers. These cases would have to be
     analyzed by checking the arguments */
  strcpy(curr_field_name_up, curr_field_name);

  /* uppercase string, reuse item */
  item = curr_field_name_up;
  while (*item) {
    *item = (char)toupper((int)*item);
    item++;
  }

  if (strstr(curr_field_name, "ABS(")
      || strstr(curr_field_name, "LAST_INSERT_ROWID(")
      || strstr(curr_field_name, "LENGTH(")
      || strstr(curr_field_name, "MAX(")
      || strstr(curr_field_name, "MIN(")
      || strstr(curr_field_name, "RANDOM(*)")
      || strstr(curr_field_name, "ROUND(")
      || strstr(curr_field_name, "AVG(")
      || strstr(curr_field_name, "COUNT(")
      || strstr(curr_field_name, "SUM(")) {
    return FIELD_TYPE_LONG;
  }
  else if (strstr(curr_field_name, "COALESCE(")
	   || strstr(curr_field_name, "GLOB(")
	   || strstr(curr_field_name, "LIKE(")
	   || strstr(curr_field_name, "LOWER(")
	   || strstr(curr_field_name, "SUBSTR(")
	   || strstr(curr_field_name, "UPPER(")) {
    return FIELD_TYPE_STRING;
  }
      

  /* curr_table now contains the name of the table that the field
     belongs to. curr_field_name contains the name of the field.
     Look up the field type in the sqlite_master table */

  /* first try in the table containing permanent tables */
  query_res = sqlite_get_table_printf((sqlite*)conn->connection,
				      "SELECT tbl_name, sql FROM sqlite_master where tbl_name='%s'",
				      &table_result_table,
				      &table_numrows,
				      &table_numcols,
				      &errmsg,
				      curr_table);

  if (query_res || !table_numrows) {
    /* now try in the table ocntaining temporary tables */
    query_res = sqlite_get_table_printf((sqlite*)conn->connection,
					"SELECT tbl_name, sql FROM sqlite_temp_master where tbl_name='%s'",
					&table_result_table,
					&table_numrows,
					&table_numcols,
					&errmsg,
					curr_table);
    
    if (query_res || !table_numrows) {
      _error_handler(conn, errflag);
/*       printf("field not found\n"); */
      return 0;
    }
  }
  

  /* table_result_table[3] now contains the sql statement that created
     the table containing the current field */
  /*  parse the sql statement to find the type of the current field */
/*   printf("table_result_table[3]=%s<<\ncurr_field_name=%s<<\n", table_result_table[3], curr_field_name); */
  curr_type = get_field_type(table_result_table[3], curr_field_name);

  /* free memory */
  sqlite_free_table(table_result_table);

  if (!curr_type) {
/*     printf("out of memory\n"); */
    return 0;
  }
  
  /* convert type to uppercase, reuse item */
  item = curr_type;
  while (*item) {
    *item = (char)toupper((int)*item);
    item++;
  }

/*   printf("field type: %s<<\n", curr_type); */
  if (strstr(curr_type, "BLOB") ||
      strstr(curr_type, "CHAR") ||
      strstr(curr_type, "CLOB") ||
      strstr(curr_type, "TEXT")) {
    type = FIELD_TYPE_STRING;
  }
  else if (strstr(curr_type, "SMALLINT")) {
    type = FIELD_TYPE_SHORT;
  }
  else if (strstr(curr_type, "INTEGER")) {
    type = FIELD_TYPE_LONG;
  }
  else if (strstr(curr_type, "BIGINT")) {
    type = FIELD_TYPE_LONGLONG;
  }
  else if (strstr(curr_type, "DECIMAL") ||
	   strstr(curr_type, "NUMERIC")) {
    type = FIELD_TYPE_DECIMAL;
  }
  else if (strstr(curr_type, "TIMESTAMP")) {
    type = FIELD_TYPE_TIMESTAMP;
  }
  else if (strstr(curr_type, "DATE")) {
    type = FIELD_TYPE_DATE;
  }
  else if (strstr(curr_type, "TIME")) {
    type = FIELD_TYPE_TIME;
  }
  else if (strstr(curr_type, "REAL")) {
    type = FIELD_TYPE_FLOAT;
  }
  else if (strstr(curr_type, "DOUBLE PRECISION")) {
    type = FIELD_TYPE_DOUBLE;
  }
  else {
    type = FIELD_TYPE_STRING; /* most reasonable default */
  }
  
  free(curr_type);

  return type;
}

char* get_field_type(const char* statement, const char* curr_field_name) {
  /*
    statement is a ptr to a string with the "create table" statement
    curr_field_name is a ptr to a string containing the name of the
    field we need the type of.

    returns the field type as an allocated string or NULL
    if an error occurred
  */
  char *item;
  char *my_statement;
  char *field_name;
  char *end_field_name;
  char *type;
  char *curr_type = NULL;

  /* make a copy that we may modify */
  if ((my_statement = strdup(statement)) == NULL) {
    return NULL;
  }

  /* the field list of the "create table" statement starts after the 
     first opening bracket */
  item = strchr(my_statement, '(');
  if (!item) {
    free(my_statement);
    return NULL;
  }

  /* make item point to the first item in the comma-separated list */
  item++;

  /* now tokenize the field list */
  for (item = strtok(item, ","); item; item = strtok(NULL, ",")) {
/*     printf("item:%s<<\n", item); */

    /* skip leading whitespace */
    field_name = item;
    while ((*field_name == ' ') || (*field_name == '\n')) {
      field_name++;
    }

    /* terminate field name */
    end_field_name = field_name+1;
    while (*end_field_name != ' ') {
      end_field_name++;
    }
    *end_field_name = '\0';

/*     printf("field_name:%s<<\n", field_name); */

    /* analyze type if the field name is the one we want to check */
    if (!strcmp(field_name, curr_field_name)) {
      /* skip leading whitespace */
      type = end_field_name + 1;
      while (*type == ' ') {
	type++;
      }

      curr_type = strdup(type);
/*       printf("curr_type:%s<<\n"); */
      break;
    }
  }

  free(my_statement);
  return curr_type;
}

char *dbd_select_db(dbi_conn_t *conn, const char *db) {
  /*
    sqlite does not separate connecting to a database server and using
    or opening a database. If we want to switch to a different database,
    we have to drop the current connection and create a new one
    instead, using the new database.
   */

  if (!db || !*db) {
    return NULL;
  }

  if (conn->connection) {
    sqlite_close((sqlite *)conn->connection);
  }

  if (_real_dbd_connect(conn, db)) {
    return NULL;
  }

  return (char *)db;
}

int dbd_geterror(dbi_conn_t *conn, int *errno, char **errstr) {
  /* put error number into errno, error string into errstr
   * return 0 if error, 1 if errno filled, 2 if errstr filled, 3 if both errno and errstr filled */
  int result = 0;

  if (conn->error_number) {
    *errno = conn->error_number;
    result++;
  }
  if (conn->error_message) {
    *errstr = conn->error_message;
    result += 2;
  }
  return result;
}


unsigned long long dbd_get_seq_last(dbi_conn_t *conn, const char *sequence) {
  return (unsigned long long)sqlite_last_insert_rowid((sqlite*)conn->connection);
}

unsigned long long dbd_get_seq_next(dbi_conn_t *conn, const char *sequence) {
  return 0;
}

int dbd_ping(dbi_conn_t *conn) {

	if (dbd_query(conn, "SELECT 1") == NULL) {
	  return 0;
	}
	else {
	  return 1;
	}
}

/* CORE SQLITE DATA FETCHING STUFF */

void _translate_sqlite_type(enum enum_field_types fieldtype, unsigned short *type, unsigned int *attribs) {
  unsigned int _type = 0;
  unsigned int _attribs = 0;

  switch (fieldtype) {
  case FIELD_TYPE_TINY:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE1;
    break;
  case FIELD_TYPE_YEAR:
    _attribs |= DBI_INTEGER_UNSIGNED;
  case FIELD_TYPE_SHORT:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE2;
    break;
  case FIELD_TYPE_INT24:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE3;
    break;
  case FIELD_TYPE_LONG:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE4;
    break;
  case FIELD_TYPE_LONGLONG:
    _type = DBI_TYPE_INTEGER;
    _attribs |= DBI_INTEGER_SIZE8;
    break;
			
  case FIELD_TYPE_FLOAT:
    _type = DBI_TYPE_DECIMAL;
    _attribs |= DBI_DECIMAL_SIZE4;
    break;
  case FIELD_TYPE_DOUBLE:
    _type = DBI_TYPE_DECIMAL;
    _attribs |= DBI_DECIMAL_SIZE8;
    break;
			
  case FIELD_TYPE_DATE: /* TODO parse n stuph to native DBI unixtime type. for now, string */
    _type = DBI_TYPE_DATETIME;
    _attribs |= DBI_DATETIME_DATE;
    break;
  case FIELD_TYPE_TIME:
    _type = DBI_TYPE_DATETIME;
    _attribs |= DBI_DATETIME_TIME;
    break;
  case FIELD_TYPE_DATETIME:
  case FIELD_TYPE_TIMESTAMP:
    _type = DBI_TYPE_DATETIME;
    _attribs |= DBI_DATETIME_DATE;
    _attribs |= DBI_DATETIME_TIME;
    break;
			
  case FIELD_TYPE_DECIMAL: /* decimal is actually a string, has arbitrary precision, no floating point rounding */
  case FIELD_TYPE_ENUM:
  case FIELD_TYPE_SET:
  case FIELD_TYPE_VAR_STRING:
  case FIELD_TYPE_STRING:
    _type = DBI_TYPE_STRING;
    break;
			
  case FIELD_TYPE_TINY_BLOB:
  case FIELD_TYPE_MEDIUM_BLOB:
  case FIELD_TYPE_LONG_BLOB:
  case FIELD_TYPE_BLOB:
    _type = DBI_TYPE_BINARY;
    break;
			
  default:
    _type = DBI_TYPE_STRING;
    break;
  }
	
  *type = _type;
  *attribs = _attribs;
}


void _get_row_data(dbi_result_t *result, dbi_row_t *row, unsigned int rowidx) {
  char **result_table = result->result_handle;
  
  int curfield = 0;
  char *raw = NULL;
  unsigned long long strsize = 0;
  unsigned long sizeattrib;
  dbi_data_t *data;

  while (curfield < result->numfields) {
    /* rowidx appears to be 0-based, but the first row always contains
     the column names */
    raw = result_table[curfield + ((rowidx+1)*result->numfields)];
    data = &row->field_values[curfield];
    
    row->field_sizes[curfield] = 0;
    /* this will be set to the string size later on if the field is indeed a string */

    if (raw == NULL) { /* no data available */
      curfield++;
      continue;
    }
    
    switch (result->field_types[curfield]) {
    case DBI_TYPE_INTEGER:
      sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_INTEGER_SIZE1, DBI_INTEGER_SIZE8);
      switch (sizeattrib) {
      case DBI_INTEGER_SIZE1:
	data->d_char = (char) atol(raw); break;
      case DBI_INTEGER_SIZE2:
	data->d_short = (short) atol(raw); break;
      case DBI_INTEGER_SIZE3:
      case DBI_INTEGER_SIZE4:
	data->d_long = (long) atol(raw); break;
      case DBI_INTEGER_SIZE8:
	data->d_longlong = (long long) atoll(raw); break; /* hah, wonder if that'll work */
      default:
	break;
      }
      break;
    case DBI_TYPE_DECIMAL:
      sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_DECIMAL_SIZE4, DBI_DECIMAL_SIZE8);
      switch (sizeattrib) {
      case DBI_DECIMAL_SIZE4:
	data->d_float = (float) strtod(raw, NULL); break;
      case DBI_DECIMAL_SIZE8:
	data->d_double = (double) strtod(raw, NULL); break;
      default:
	break;
      }
      break;
    case DBI_TYPE_STRING:
      data->d_string = strdup(raw);
      row->field_sizes[curfield] = strlen(raw);
      break;
    case DBI_TYPE_BINARY:
      /* treat as string */
      data->d_string = strdup(raw);
      row->field_sizes[curfield] = strlen(raw);
      break;
    case DBI_TYPE_DATETIME:
      sizeattrib = _isolate_attrib(result->field_attribs[curfield], DBI_DATETIME_DATE, DBI_DATETIME_TIME);
      data->d_datetime = _parse_datetime(raw, sizeattrib);
      break;
      
    default:
      data->d_string = strdup(raw);
      row->field_sizes[curfield] = strlen(raw);
      break;
    }
    
    curfield++;
  }
}

time_t _parse_datetime(const char *raw, unsigned long attribs) {
  struct tm unixtime;
  char *unparsed = strdup(raw);
  char *cur = unparsed;

  unixtime.tm_sec = unixtime.tm_min = unixtime.tm_hour = 0;
  unixtime.tm_mday = unixtime.tm_mon = unixtime.tm_year = 0;
  unixtime.tm_isdst = -1;
	
  if (attribs & DBI_DATETIME_DATE) {
    cur[4] = '\0';
    cur[7] = '\0';
    cur[10] = '\0';
    unixtime.tm_year = atoi(cur)-1900;
    unixtime.tm_mon = atoi(cur+5);
    unixtime.tm_mday = atoi(cur+8);
    if (attribs & DBI_DATETIME_TIME) cur = cur+11;
  }
	
  if (attribs & DBI_DATETIME_TIME) {
    cur[2] = '\0';
    cur[5] = '\0';
    unixtime.tm_hour = atoi(cur);
    unixtime.tm_min = atoi(cur+3);
    unixtime.tm_sec = atoi(cur+6);
  }

  free(unparsed);
  return mktime(&unixtime);
}

/* this function is stolen from MySQL and somewhat simplified for our
   needs */
/* it appears to return 0 on a match, 1 if no match is found, and -1
   in some odd cases */

#define wild_many (char)'%'
#define wild_one (char)'_'
#define INC_PTR(A,B) A++

int wild_case_compare(const char *str,const char *str_end,
		      const char *wildstr,const char *wildend,
		      char escape) {
  int result= -1;				// Not found, using wildcards
  unsigned char cmp;

  while (wildstr != wildend) {
      while (*wildstr != wild_many && *wildstr != wild_one) {
	if (*wildstr == escape && wildstr+1 != wildend) {
	  wildstr++;
	}
	if (str == str_end || *wildstr++ != *str++) {
	  return(1);				// No match
	}
	if (wildstr == wildend) {
	  return (str != str_end);		// Match if both are at end
	}
	result=1;					// Found an anchor char
      }
      if (*wildstr == wild_one)	{
	do {
	  if (str == str_end) {			// Skip one char if possible
	    return (result);
	  }
	  INC_PTR(str,str_end);
	} while (++wildstr < wildend && *wildstr == wild_one);
	if (wildstr == wildend) {
	  break;
	}
      }

      if (*wildstr == wild_many) {		// Found wild_many
	wildstr++;
	/* Remove any '%' and '_' from the wild search string */
	for ( ; wildstr != wildend ; wildstr++) {
	  if (*wildstr == wild_many) {
	    continue;
	  }
	  if (*wildstr == wild_one) {
	    if (str == str_end) {
	      return (-1);
	    }
	    INC_PTR(str,str_end);
	    continue;
	  }
	  break;					// Not a wild character
	}
	if (wildstr == wildend) {
	  return(0);				// Ok if wild_many is last
	}
	if (str == str_end) {
	  return -1;
	}

	if ((cmp= *wildstr) == escape && wildstr+1 != wildend) {
	  cmp= *++wildstr;
	}
	INC_PTR(wildstr,wildend);			// This is compared trough cmp
	  /*        cmp=likeconv(cmp);    */
	do {
	  while (str != str_end && *str != cmp) {
	    str++;
	  }
	  if (str++ == str_end) {
	    return (-1);
	  }

	  {
	    int tmp=wild_case_compare(str,str_end,wildstr,wildend,escape);
	    if (tmp <= 0) {
	      return (tmp);
	    }
	  }
	} while (str != str_end && wildstr[0] != wild_many);
	return(-1);
      }
  }
  return (str != str_end ? 1 : 0);
}

/* this function is stolen from MySQL. The quoting was changed to the
 SQL standard, i.e. single and double quotes are escaped by doubling,
 not by a backslash */
static unsigned long sqlite_escape_string(char *to, const char *from, unsigned long length)
{
  const char *to_start=to;
  const char *end;

  for (end=from+length; from != end ; from++)
    {
      switch (*from) {
      case 0:				/* Must be escaped for 'mysql' */
	*to++= '\\';
	*to++= '0';
	break;
      case '\n':				/* Must be escaped for logs */
	*to++= '\\';
	*to++= 'n';
	break;
      case '\r':
	*to++= '\\';
	*to++= 'r';
	break;
/*       case '\\': */
/* 	*to++= '\\'; */
/* 	*to++= '\\'; */
/* 	break; */
      case '\'':
	*to++= '\''; /* double single quote */
	*to++= '\'';
	break;
      case '"':				/* Better safe than sorry */
	*to++= '"'; /* double double quote */
	*to++= '"';
	break;
      case '\032':			/* This gives problems on Win32 */
	*to++= '\\';
	*to++= 'Z';
	break;
      default:
	*to++= *from;
      }
    }
  *to=0;
  return (unsigned long) (to-to_start);
}

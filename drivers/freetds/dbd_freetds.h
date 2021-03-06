/*
 * libdbi - database independent abstraction layer for C.
 * Copyright (C) Vadym Kononenko
 * http://libdbi.sourceforge.net
 * 
 * dbd_freetds.c: MS SQL database support (using libct of FreeTDS library)
 * Copyright (C) Vadym Kononenko <konan_v@users.sourceforge.net>.
 * http://libdbi.sourceforge.net
 * 
 */

#define TDS_RESERVED_WORDS { \
	"ADD", \
	"ALL", \
	"ALTER", \
	"AND", \
	"ANY", \
	"AS", \
	"ASC", \
	"AUTHORIZATION", \
	"AVG", \
	"BACKUP", \
	"BEGIN", \
	"BETWEEN", \
	"BREAK", \
	"BROWSE", \
	"BULK", \
	"BY", \
	"CASCADE", \
	"CASE", \
	"CHECK", \
	"CHECKPOINT", \
	"CLOSE", \
	"CLUSTERED", \
	"COALESCE", \
	"COLUMN", \
	"COMMIT", \
	"COMMITED", \
	"COMPUTE", \
	"CONFIRM", \
	"CONSTRAINT", \
	"CONTINUE", \
	"CONTROLROW", \
	"CONVERT", \
	"COUNT", \
	"CREATE", \
	"CROSS", \
	"CURRENT", \
	"CURRENT_DATE", \
	"CURRENT_TIMESTAMP", \
	"CURRENT_USER", \
	"CURSOR", \
	"DATABASE", \
	"DBCC", \
	"DEALLOCATE", \
	"DECLARE", \
	"DEFAULT", \
	"DELETE", \
	"DESC", \
	"DISK", \
	"DISTINCT", \
	"DISTRIBUTED", \
	"DOUBLE", \
	"DROP", \
	"DUMMY", \
	"DUMP", \
	"ELSE", \
	"END", \
	"ERRLVL", \
	"ERROREXIT", \
	"ESCAPE", \
	"EXCEPT", \
	"EXECUTE", \
	"EXIT", \
	"FETCH", \
	"FILE", \
	"FILLFACTOR", \
	"FLOPPY", \
	"FOR", \
	"FOREIGN", \
	"FROM", \
	"FULL", \
	"GOTO", \
	"GRANT", \
	"GROUP", \
	"HAVING", \
	"HOLDLOCK", \
	"IDENTITY", \
	"IDENTITY_INSERT", \
	"IDENTITYCOL", \
	"IF", \
	"IN", \
	"INDEX", \
	"INNER", \
	"INSERT", \
	"INSERTSECT", \
	"INTO", \
	"IS", \
	"ISOLATION", \
	"JOIN", \
	"KEY", \
	"KILL", \
	"LEFT", \
	"LEVEL", \
	"LIKE", \
	"LINENO", \
	"MAX", \
	"MIN", \
	"MIRROREXIT", \
	"NOCHECK", \
	"NONCLUSTERED", \
	"NOT", \
	"NULL", \
	"NULLIF", \
	"OF", \
	"OFF", \
	"OFFSETS", \
	"ON", \
	"ONCE", \
	"ONLY", \
	"OPEN", \
	"OPENQUERY", \
	"OPENROWSET", \
	"OPTION", \
	"OR", \
	"ORDER", \
	"OUTER", \
	"OVER", \
	"PERCENT", \
	"PERMANENT", \
	"PIPE", \
	"PLAN", \
	"PRECISION", \
	"PREPARE", \
	"PRINT", \
	"PRIVILEGES", \
	"PROCEDURE", \
	"PROCESSEXIT", \
	"PUBLIC", \
	"RAISERROR", \
	"READ", \
	"READTEXT", \
	"RECONFIGURE", \
	"REFERENCES", \
	"REPEATABLE", \
	"REPLICATION", \
	"RESTRICT", \
	"RETURN", \
	"REVOKE", \
	"RIGTH", \
	"ROLLBACK", \
	"ROWCOUNT", \
	"ROWGUIDCOL", \
	"RULE", \
	"SAVE", \
	"SCHEMA", \
	"SELECT", \
	"SERIALIZABLE", \
	"SESSION_USER", \
	"SET", \
	"SETUSER", \
	"SHUTDOWN", \
	"STATISTICS", \
	"SUM", \
	"SYSTEM_USER", \
	"TABLE", \
	"TAPE", \
	"TEMPORARY", \
	"TEXTSIZE", \
	"THEN", \
	"TO", \
	"TOP", \
	"TRANSACTION", \
	"TRIGGER", \
	"TRUNCATE", \
	"TSEQUAL", \
	"UNCOMMITED", \
	"UNION", \
	"UNIQUE", \
	"UPDATE", \
	"UPDATETEXT", \
	"USE", \
	"USER", \
	"VALUES", \
	"VARYING", \
	"VIEW", \
	"WAITFOR", \
	"WHEN", \
	"WHERE", \
	"WHILE", \
	"WITH", \
	"WORK", \
	"WRITETEXT", \
	NULL }

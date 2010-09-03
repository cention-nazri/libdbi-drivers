#!/bin/sh
#
# test_mysql.sh - runs libdbi test suite for mysql driver using a temporary
# mysql server environment that doesn't distrubs any running MySQL server.
#
# Copyright (C) 2010 Clint Byrum <clint@ubuntu.com>
# Copyright (C) 2010 Thomas Goirand <zigo@debian.org>
#
# This script is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 2.1 of the License, or (at your option)
# any later version.
#
# This script is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to:
# The Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

set -e

MYTEMP_DIR=`mktemp -d`
ME=`whoami`

# --force is needed because buildd's can't resolve their own hostnames to ips
mysql_install_db --no-defaults --datadir=${MYTEMP_DIR} --force --skip-name-resolve --user=${ME}
/usr/sbin/mysqld --no-defaults --skip-grant --user=${ME} --socket=${MYTEMP_DIR}/mysql.sock --datadir=${MYTEMP_DIR} --skip-networking &

# This sets the path of the MySQL socket for any libmysql-client users, which includes
# the ./tests/test_dbi client
export MYSQL_UNIX_PORT=${MYTEMP_DIR}/mysql.sock

echo -n pinging mysqld.
attempts=0
while ! /usr/bin/mysqladmin --socket=${MYTEMP_DIR}/mysql.sock ping ; do
	sleep 3
	attempts=$((attempts+1))
	if [ ${attempts} -gt 10 ] ; then
		echo "skipping test, mysql server could not be contacted after 30 seconds"
		exit 0
	fi
done

( echo ./drivers/mysql/.libs; \
	echo mysql; \
	echo root; \
	echo ""; \
	echo ""; \
	echo "libdbitest"; \
	) | ./tests/test_dbi

ecode=$?

/usr/bin/mysqladmin --socket=${MYTEMP_DIR}/mysql.sock shutdown
rm -rf ${MYTEMP_DIR}
exit ${ecode}

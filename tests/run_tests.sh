#!/bin/sh
#
# run_tests.sh - script to run automatic tests
#

CONFIG="test_dbi.cfg"

CTEST_BIN=/usr/bin/ctest
TEST_DBI_BIN="./test_dbi"


write_engine_string() {
  oIFS="$IFS"
  IFS=:
  TMPFILE=$1
  DBENGINE_STRING=$2
  set - $DBENGINE_STRING
  for i in $DBENGINE_STRING; do  
    echo $i >> ${TMPFILE}
  done
  IFS="$oIFS"
}


while read LINHA; do

 [ "$(echo $LINHA | cut -c1)" = '#' ] && continue

 [ "$LINHA" ] || continue

 set - $LINHA

 chave=$1
 shift
 valor=$*

 case "$chave" in
 
 tag)
  TAG=$valor
  ;;
 type)
  TYPE=$valor
  ;;
 buildname)
  BUILD=$valor
  ;;
 sitename)
  SITE=${valor}-$TAG
  ;;
 hostname)
  HOSTNAME=$valor
  ;;
 os_name)
  OSNAME=$valor
  ;;
 os_version)
  OSVERSION=$valor
  ;;
 os_release)
  OSRELEASE=$valor
  ;;
 os_platform)
  OSPLATFORM=$valor
  ;;
 mysql | pgsql | sqlite | sqlite3 | firebird| db2| oracle| ingress| msql) 
  write_engine_string ${chave}.db "$valor"
  DBENGINE="${chave} ${DBENGINE}"
  ;;
 *)
  echo "Config file error"
  echo "Unknow option '$chave'"
  exit 1
  ;;
esac

done < "$CONFIG"

if [ x"$TYPE" = x"-" -o x"$TYPE" = x"" ]; then
  echo "The test type was not defined. Please change .cfg file"
  exit 1
fi

if ! [ -x $TEST_DBI_BIN ]; then
  echo "I can't find $TEST_DBI_BIN"
  exit 1
fi

if ! [ -x ${CTEST_BIN} ]; then
  echo "I can't find $CTEST_BIN. The tests will run but can´t be submited"
fi

# Change DartConfiguration file
out="sed_tmp"

trap "rm -f ${out}; rm -f DartConfiguration.tcl; rm *.db" QUIT TERM EXIT

for db in $DBENGINE; do
  if [ ! -f ${db}.db ]; then
    echo "File ${db}.db not found. Please check test_dbi.cfg"
    exit 1
  else
    echo "**** Starting ${db} test *****"
    echo "s%@sitename@%${SITE}%g" >> ${out}
    echo "s%@buildname@%${BUILD}-${db}%g" >> ${out}
    sed -f ${out} DartConfiguration.tcl.in > DartConfiguration.tcl

    # runs libdbi test framework with cdash report
    cat ${db}.db | $TEST_DBI_BIN -C -S ${SITE} -B ${BUILD}-${db} -T ${TYPE} \
 -N ${OSNAME} -P ${OSPLATFORM} -R ${OSRELEASE} -V ${OSVERSION} -H ${HOSTNAME}
    # submit to the dashboard 
    $CTEST_BIN -D ${TYPE}Submit
    ret=$?
    if [ $ret -eq 1 ]; then
      echo "Failed to submit the reporter"
    else
      rm -rf Testing  
    fi

    rm -f ${out}; rm -f DartConfiguration.tcl
    sleep 120
    echo "**** Finished ${db} test *****"
  fi

done

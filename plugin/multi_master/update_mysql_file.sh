#!  /bin/sh

CURRENT_DIR=$(dirname $(readlink -f "$0"))

MYSQL_SOURCE_FILES=(multi_master_handler.h multi_master_handler.cc)

SQL_DIR=${CURRENT_DIR}/../../sql/

#ls ${SQL_DIR} | grep multi_master

for var in ${MYSQL_SOURCE_FILES[@]}
do
    if [ -f "${SQL_DIR}/$var" ]; then
        echo "update ${SQL_DIR}$var"
        rm ${SQL_DIR}$var
    fi
    cp ${CURRENT_DIR}/mysql-sql/$var ${SQL_DIR}
done

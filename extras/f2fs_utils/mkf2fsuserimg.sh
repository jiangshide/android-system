#!/bin/bash
#
# To call this script, make sure make_f2fs is somewhere in PATH

function usage() {
cat<<EOT
Usage:
${0##*/} OUTPUT_FILE SIZE
         [-S] [-C FS_CONFIG] [-f SRC_DIR] [-D PRODUCT_OUT]
         [-s FILE_CONTEXTS] [-t MOUNT_POINT] [-T TIMESTAMP]
         [-L LABEL] [--prjquota] [--casefold] [--compression]
         [--sldc <num> [sload compression sub-options]]
<num>: number of the sload compression args, e.g.  -a LZ4 counts as 2
       when sload compression args are not given, <num> must be 0,
       and the default flags will be used.
Note: must conserve the option order
EOT
}

echo "in mkf2fsuserimg.sh PATH=$PATH"

MKFS_OPTS=""
SLOAD_OPTS=""

if [ $# -lt 2 ]; then
  usage
  exit 1
fi

OUTPUT_FILE=$1
SIZE=$2
shift; shift

SPARSE_IMG="false"
if [[ "$1" == "-S" ]]; then
  MKFS_OPTS+=" -S $SIZE"
  SLOAD_OPTS+=" -S"
  SPARSE_IMG="true"
  shift
fi

if [[ "$1" == "-C" ]]; then
  SLOAD_OPTS+=" -C $2"
  shift; shift
fi
if [[ "$1" == "-f" ]]; then
  SLOAD_OPTS+=" -f $2"
  shift; shift
fi
if [[ "$1" == "-D" ]]; then
  SLOAD_OPTS+=" -p $2"
  shift; shift
fi
if [[ "$1" == "-s" ]]; then
  SLOAD_OPTS+=" -s $2"
  shift; shift
fi
if [[ "$1" == "-t" ]]; then
  MOUNT_POINT=$2
  shift; shift
fi

if [ -z $MOUNT_POINT ]; then
  echo "Mount point is required"
  exit 2
fi

if [[ ${MOUNT_POINT:0:1} != "/" ]]; then
  MOUNT_POINT="/"$MOUNT_POINT
fi

SLOAD_OPTS+=" -t $MOUNT_POINT"

if [[ "$1" == "-T" ]]; then
  SLOAD_OPTS+=" -T $2"
  shift; shift
fi

if [[ "$1" == "-L" ]]; then
  MKFS_OPTS+=" -l $2"
  shift; shift
fi

if [[ "$1" == "--prjquota" ]]; then
  MKFS_OPTS+=" -O project_quota,extra_attr"
  shift;
fi
if [[ "$1" == "--casefold" ]]; then
  MKFS_OPTS+=" -O casefold -C utf8"
  shift;
fi

if [[ "$1" == "--compression" ]]; then
  COMPRESS_SUPPORT=1
  MKFS_OPTS+=" -O compression,extra_attr"
  shift;
fi

if [[ "$1" == "--sldc" ]]; then
  if [ -z "$COMPRESS_SUPPORT" ]; then
    echo "--sldc needs --compression flag"
    exit 3
  fi
  SLOAD_OPTS+=" -c"
  shift
  SLDC_NUM_ARGS=$1
  case $SLDC_NUM_ARGS in
    ''|*[!0-9]*)
      echo "--sldc needs a number"
      exit 3 ;;
  esac
  shift
  while [ $SLDC_NUM_ARGS -gt 0 ]; do
    SLOAD_OPTS+=" $1"
    shift
    (( SLDC_NUM_ARGS-- ))
  done
fi

if [ -z $SIZE ]; then
  echo "Need size of filesystem"
  exit 2
fi

if [ "$SPARSE_IMG" = "false" ]; then
  TRUNCATE_CMD="truncate -s $SIZE $OUTPUT_FILE"
  echo $TRUNCATE_CMD
  $TRUNCATE_CMD
  if [ $? -ne 0 ]; then
    exit 3
  fi
fi

MAKE_F2FS_CMD="make_f2fs -g android $MKFS_OPTS $OUTPUT_FILE"
echo $MAKE_F2FS_CMD
$MAKE_F2FS_CMD
if [ $? -ne 0 ]; then
  if [ "$SPARSE_IMG" = "false" ]; then
    rm -f $OUTPUT_FILE
  fi
  exit 4
fi

SLOAD_F2FS_CMD="sload_f2fs $SLOAD_OPTS $OUTPUT_FILE"
echo $SLOAD_F2FS_CMD
$SLOAD_F2FS_CMD
# allow 1: Filesystem errors corrected
ret=$?
if [ $ret -ne 0 ] && [ $ret -ne 1 ]; then
  rm -f $OUTPUT_FILE
  exit 4
fi
exit 0

#! /bin/sh
rc=0
TMPDIR="${TMPDIR-/tmp}"
TEMPFILENAME=`mktemp 2>/dev/null || echo "$TMPDIR/tmp-fig$$.svg"`

fig2dev -L svg "$1" "${TEMPFILENAME}" > /dev/null 2>&1 || rc=1

cat < "${TEMPFILENAME}" || rc=1
rm -f "${TEMPFILENAME}"
exit $rc

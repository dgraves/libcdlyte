#!/bin/sh
(automake --version) < /dev/null > /dev/null 2>&1 ||
{
	echo;
	echo "You must have automake installed";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 ||
{
	echo;
	echo "You must have autoconf installed";
	echo;
	exit;
}

echo "Generating configuration files for fxcd...."
echo;

run_cmd()
{
	echo executing $* ...;
	if ! $*; then
		echo "failed!";
		exit;
	fi
}

run_cmd aclocal;
run_cmd libtoolize --automake --copy
run_cmd autoheader
run_cmd automake --add-missing --copy;
run_cmd autoconf;


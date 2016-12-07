all:
	gcc -o ./commands/pbs/pbs ./commands/pbs/pbs.c ./package/fatSupport.c
	gcc -o ./commands/pfe/pfe ./commands/pfe/pfe.c ./package/fatSupport.c
	gcc -o ./commands/pwd/pwd ./commands/pwd/pwd.c ./package/fatSupport.c
	gcc -o ./commands/cd/cd ./commands/cd/cd.c ./package/fatSupport.c
	gcc -o ./commands/ls/ls ./commands/ls/ls.c ./package/fatSupport.c
	gcc -o ./commands/rm/rm ./commands/rm/rm.c ./package/fatSupport.c
	gcc -o ./commands/rmdir/rmdir ./commands/rmdir/rmdir.c ./package/fatSupport.c
	gcc -o ./commands/mkdir/mkdir ./commands/mkdir/mkdir.c ./package/fatSupport.c
	gcc -o ./commands/touch/touch ./commands/touch/touch.c ./package/fatSupport.c
	gcc -o ./commands/cat/cat ./commands/cat/cat.c ./package/fatSupport.c
	gcc -o ./commands/df/df ./commands/df/df.c ./package/fatSupport.c
	gcc -o shell shell.c

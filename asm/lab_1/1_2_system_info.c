#include <stdio.h>
#include <sys/utsname.h>

int main()
{
	struct utsname info;
    uname(&info);
	printf("Операционная система = %s\n", info.sysname);
	printf("Дистрибутив = %s\n", info.version);
	printf("Архитектура = %s\n", info.machine);
}
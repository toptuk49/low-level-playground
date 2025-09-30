#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swap.h"
#include "revert_string.h"

int main()
{
	char ch1 = 'a';
	char ch2 = 'b';

	Swap(&ch1, &ch2);

	printf("%c %c\n", ch1, ch2);

	char *reverted_str = malloc(sizeof(char) * (strlen("abcdef") + 1));
	strcpy(reverted_str, "abcdef");

	RevertString(reverted_str);

	printf("Reverted: %s\n", reverted_str);
	free(reverted_str);
	return 0;
}

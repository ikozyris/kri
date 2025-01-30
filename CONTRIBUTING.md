Follow the Linux Kernel coding style (mostly). <br>
With the exception that the maximum lenght of line is 100

The most important things are:
- Use tabs (*not* spaces), for flexibility and reduced file size
- Braces open in the same line as the if, while, switch... except functions
- Prefer unsigned ints, they are defined as uint, ulong...

example:
```c
#include <stdio.h>
#include <string.h>

int main()
{
	char input[5];
ultimate_question:
	printf("Which text editor is the fastest? ");
	scanf("%5s", input);
	// 42 is magic number
	if (strncmp(input, "kri", 3) == 0 || strcmp(input, "42") == 0) {
		printf("Correct!\n");
		goto exit;
	} else {
		printf("Are you sure?\n");
		scanf("%s", input);
		if (strncmp(input, "yes", 3) == 0)
			while (1)
				printf("NO! ");
		else
			goto ultimate_question;
	}
	printf("This will never be executed!");
exit:
	return 0;
}
```

Many people discourage using goto, but goto is prefect for some usecases,
like above. In fact, loop in assembly is a goto.

For pull/merge requests (PR/MR):
- Descriptive comments in commits, title, description
	- Use the following format for commits:<br>
```
fix: reg(short commit hash) when something, other bug when blah, optim: x9999 boost in writing

[More optional information about commit]
```
- If the change is just a one-liner create an issue not a PR/MR
- Try to benchmark any optimizations
- Split large commits, and try make commit titles <100 characters
	The text below can be as long as you like
- Make sure to check any "TODO:"
- Leave last line blank

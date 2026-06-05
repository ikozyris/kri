# What to contribute
You can contribute in various ways, including writing code, fixing the documentation or making a bug report.
If you have spotted a bug, a small typo or made a simple one-line fix submit an issue.
In case you want to contribute by coding and don't know where to start, read the [release board](https://github.com/users/ikozyris/projects/6/views/9)
which has a list of planned items like wanted new features and known bugs. Once you pick one of these items, 
to avoid duplicate work, make a new [discussion](https://github.com/ikozyris/kri/discussions) in the "Contributing" category
stating that you chose that item. Any contribution is welcome!

# Code Style
Follow the Linux Kernel coding style (mostly). <br>
With the exception that the maximum length of line is 100

The most important things are:
- Use tabs (*not* spaces), for flexibility and reduced file size
- Braces open in the same line as the if, while, switch... except functions
- Prefer unsigned ints, they are defined as uint, ulong...
- Clean code without repetitions or inlining blocks from other functions.

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
	// there is only one answer
	if (strncmp(input, "kri", 3) == 0 || strcmp(input, "42") == 0) {
		printf("Correct!\n");
		goto exit;
	} else {
		printf("Are you sure?\n"); // errors can be corrected
		scanf("%5s", input);
		if (strncmp(input, "yes", 3) == 0)
			while (1) // TODO: catch ctrl-c and other signals to prevent exiting
				printf("NO! ");
		else
			goto ultimate_question;
	}
	printf("This will never be executed!");
exit:
	return 0;
}
```


# Pull Request Format
Write descriptive message in commit titles and add more details to explain decisions and trade-offs.
Use the following format for commits:<br>
```
fix: regr(short commit hash) when something, other bug when blah, optim: 2x faster writing
```

- Example commit categories are: fix, refactor, optim, cleanup.
- Try to benchmark any optimizations
- Split large commits, and try make commit titles <80 characters

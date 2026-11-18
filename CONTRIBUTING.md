> Kri is made for fun, educational purposes and some utility in narrow tasks.

# What to contribute

You can contribute in various ways, including writing code, fixing the documentation or making a bug report.
If you have spotted a bug, a small typo or made a simple one-line fix, submit an issue.
In case you want to contribute by coding and don't know where to start, read the [release board](https://github.com/users/ikozyris/projects/6/views/9)
which has a list of planned items like wanted new features and known bugs. Once you pick one of these items, 
to avoid duplicate work, make a new [discussion](https://github.com/ikozyris/kri/discussions) in the "Contributing" category
stating that you chose that item. Any contribution is welcome!

# Code Style

- Like Linux kernel
- Max 100 characters per line
- Use tabs (*not* spaces), for flexibility and reduced file size
- Braces open in the same line as the if, while, switch... except functions
- Prefer unsigned ints, they are defined as uint, ulong...
- Streamline and clean code:
  - no repetitions or inlined blocks from other functions
  - no superfluous or unreachable branches; every `if` adds complexity
  - simplicity over functionality

# Pull Request Format

Write descriptive message in commit titles and add more details to explain decisions and trade-offs.
Use the following format for commits:<br>
```
fix: regr(short commit hash) when something, other bug when blah, optim: 2x faster writing
```

- Example commit categories are: fix, refactor, optim, cleanup.
- Try to benchmark any optimizations
- Split large commits, and try make commit titles <80 characters

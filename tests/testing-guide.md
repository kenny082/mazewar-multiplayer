## Testing Instructions

1. Delete or rename `tests/server_tests.c` so it no longer ends in `.c`.

2. Clean and build the project:
```bash
make clean all

```
3. Run the first test suite:
```bash
bin/mazewar_tests --verbose -j1
```

4. Restore `server_tests.c` to its original filename.

5. Delete every other `.c` file inside the `tests/` directory.

6. Clean and rebuild:
```bash
make clean all
```

7. Run the second test suite with concurrency enabled:
```bash
bin/mazewar_tests --verbose -j8
```

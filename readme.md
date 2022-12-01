# Penguini!

## Building and running code
If the code was never built, you must run 
`make build`

After building the app at least once, you can use  
`make run`
to build and run the app with one command.

## Testing
The test library we're using is µnit, https://nemequ.github.io/munit. 

To download it for the first time, run
`git submodule init && git submodule update && make cmake`

then, use `make test` to run all the tests in tests.c

### Rerunning tests with a predetermined randomness seed

`make test && build/penguins-tests --seed 0xbf8782eb`

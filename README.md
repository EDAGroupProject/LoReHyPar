# LoReHyPar

Logic Replication Hyper Partition

## how to compile

you can use the build.sh script to compile the project, or

```bash
mkdir build
cd build
cmake ../
make
```

then you can find the executable file in the build directory named `hypar_shell`.

## how to run

you can use the run.sh script to run the project, or

```bash
./hypar_shell "path/to/input/dir/"
```

for example, if you want to run the sample01 testcase(i.e. ../testcase/sample01/design.*), you can run

```bash
./hypar_shell "../testcase/sample01/"
```

no "path/to/input/dir/" means the default input files are in the same directory as the executable file.

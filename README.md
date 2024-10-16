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

then you can find the executable file in the build directory named `partitioner`.

## how to run

you can use the run.sh script to run the project, or

```bash
./partitioner -t <input/directory> -s <output/file>
```

for example:

```bash
./partitioner -t /home/public/testcase/sample01 -s /home/team01/sample01/design.fpga.out
```

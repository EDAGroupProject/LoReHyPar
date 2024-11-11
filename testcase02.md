# test logic replication on case02

```cpp
void add_logic_replication_og(long long &hop);
void add_logic_replication_pq(long long &hop);
void add_logic_replication_rd(long long &hop);
```

## original

```cpp
eda240317@VM-0-14-ubuntu:~/LoReHyPar$ ./runmt.sh case02
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/eda240317/LoReHyPar/build
[ 14%] Building CXX object CMakeFiles/LoReHyPar.dir/initialpar.cpp.o
[ 28%] Linking CXX static library libLoReHyPar.a
[ 71%] Built target LoReHyPar
[ 85%] Linking CXX executable partitioner
[100%] Built target partitioner
Build succeeded.
Test succeeded for iteration 1.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 682
----------------------------
Resources of FPGA1: [ 746 189 151 185 170 163 117 168 ]
Resources of FPGA2: [ 748 130 174 109 139 164 206 161 ]
Resources of FPGA3: [ 664 144 96 110 114 112 118 136 ]
Resources of FPGA4: [ 48 87 100 80 74 51 63 80 ]
Resources of FPGA5: [ 394 70 93 61 77 71 56 115 ]
Resources of FPGA6: [ 748 114 62 94 79 66 105 121 ]
Resources of FPGA7: [ 729 226 136 166 156 160 223 156 ]
Resources of FPGA8: [ 748 184 114 97 135 190 169 196 ]
Total extern cut of FPGA1: 509
Total extern cut of FPGA2: 734
Total extern cut of FPGA3: 543
Total extern cut of FPGA4: 16
Total extern cut of FPGA5: 210
Total extern cut of FPGA6: 559
Total extern cut of FPGA7: 657
Total extern cut of FPGA8: 796

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3736
----------------------------
Time for iteration 1: 78s
Score for iteration 1: 3736.00
Test succeeded for iteration 2.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 662
----------------------------
Resources of FPGA1: [ 748 115 157 147 159 151 203 156 ]
Resources of FPGA2: [ 723 224 134 151 166 142 191 146 ]
Resources of FPGA3: [ 748 135 97 70 85 94 107 142 ]
Resources of FPGA4: [ 411 95 118 114 101 82 108 126 ]
Resources of FPGA5: [ 647 99 80 84 73 58 95 86 ]
Resources of FPGA6: [ 721 184 108 151 128 153 140 178 ]
Resources of FPGA7: [ 704 163 132 95 156 219 158 180 ]
Resources of FPGA8: [ 55 110 83 62 83 69 39 84 ]
Total extern cut of FPGA1: 740
Total extern cut of FPGA2: 658
Total extern cut of FPGA3: 565
Total extern cut of FPGA4: 208
Total extern cut of FPGA5: 542
Total extern cut of FPGA6: 524
Total extern cut of FPGA7: 814
Total extern cut of FPGA8: 17

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3945
----------------------------
Time for iteration 2: 79s
Score for iteration 2: 3945.00
Test succeeded for iteration 3.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 676
----------------------------
Resources of FPGA1: [ 733 147 126 144 165 101 159 132 ]
Resources of FPGA2: [ 748 193 240 185 223 199 278 209 ]
Resources of FPGA3: [ 502 281 189 236 233 247 244 269 ]
Resources of FPGA4: [ 26 46 45 39 33 35 42 42 ]
Resources of FPGA5: [ 741 93 53 74 34 51 38 77 ]
Resources of FPGA6: [ 736 137 83 75 69 123 100 141 ]
Resources of FPGA7: [ 551 129 80 49 86 120 82 141 ]
Resources of FPGA8: [ 736 125 99 91 96 94 107 113 ]
Total extern cut of FPGA1: 503
Total extern cut of FPGA2: 745
Total extern cut of FPGA3: 387
Total extern cut of FPGA4: 13
Total extern cut of FPGA5: 513
Total extern cut of FPGA6: 699
Total extern cut of FPGA7: 700
Total extern cut of FPGA8: 528

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3925
----------------------------
Time for iteration 3: 77s
Score for iteration 3: 3925.00
Test succeeded for iteration 4.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 660
----------------------------
Resources of FPGA1: [ 37 48 56 21 53 42 27 58 ]
Resources of FPGA2: [ 748 145 113 112 122 115 136 132 ]
Resources of FPGA3: [ 725 195 162 147 125 247 207 231 ]
Resources of FPGA4: [ 646 213 203 211 177 167 236 211 ]
Resources of FPGA5: [ 744 196 130 139 139 133 121 167 ]
Resources of FPGA6: [ 748 93 95 91 121 81 112 126 ]
Resources of FPGA7: [ 310 116 67 54 81 52 55 102 ]
Resources of FPGA8: [ 748 113 99 94 100 93 135 71 ]
Total extern cut of FPGA1: 10
Total extern cut of FPGA2: 531
Total extern cut of FPGA3: 809
Total extern cut of FPGA4: 632
Total extern cut of FPGA5: 402
Total extern cut of FPGA6: 571
Total extern cut of FPGA7: 293
Total extern cut of FPGA8: 689

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3909
----------------------------
Time for iteration 4: 74s
Score for iteration 4: 3909.00
Average Total Hop Length = 3878.75
Average Score = 3878.75
Score Variance = 6955.18
```

## random

```cpp
eda240317@VM-0-14-ubuntu:~/LoReHyPar$ ./runmt.sh case02
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/eda240317/LoReHyPar/build
[ 71%] Built target LoReHyPar
[ 85%] Building CXX object CMakeFiles/partitioner.dir/main.cpp.o
[100%] Linking CXX executable partitioner
[100%] Built target partitioner
Build succeeded.
Test succeeded for iteration 1.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 664
----------------------------
Resources of FPGA1: [ 744 262 183 191 224 211 246 263 ]
Resources of FPGA2: [ 662 235 213 169 222 274 300 268 ]
Resources of FPGA3: [ 439 248 176 171 142 125 142 198 ]
Resources of FPGA4: [ 39 50 74 66 70 59 47 73 ]
Resources of FPGA5: [ 711 80 78 54 64 71 51 110 ]
Resources of FPGA6: [ 723 140 76 94 84 83 87 93 ]
Resources of FPGA7: [ 722 83 50 61 48 68 90 62 ]
Resources of FPGA8: [ 719 72 68 64 87 49 46 79 ]
Total extern cut of FPGA1: 697
Total extern cut of FPGA2: 844
Total extern cut of FPGA3: 316
Total extern cut of FPGA4: 14
Total extern cut of FPGA5: 588
Total extern cut of FPGA6: 488
Total extern cut of FPGA7: 671
Total extern cut of FPGA8: 491

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3850
----------------------------
Time for iteration 1: 75s
Score for iteration 1: 3850.00
Test succeeded for iteration 2.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 689
----------------------------
Resources of FPGA1: [ 437 162 74 138 105 114 103 129 ]
Resources of FPGA2: [ 736 299 202 190 211 233 244 234 ]
Resources of FPGA3: [ 748 111 108 79 88 66 84 94 ]
Resources of FPGA4: [ 30 55 45 80 54 42 56 59 ]
Resources of FPGA5: [ 724 189 123 99 96 167 142 190 ]
Resources of FPGA6: [ 704 133 162 151 160 155 204 202 ]
Resources of FPGA7: [ 694 134 127 100 133 95 79 158 ]
Resources of FPGA8: [ 714 128 90 67 106 96 138 120 ]
Total extern cut of FPGA1: 294
Total extern cut of FPGA2: 762
Total extern cut of FPGA3: 455
Total extern cut of FPGA4: 17
Total extern cut of FPGA5: 703
Total extern cut of FPGA6: 736
Total extern cut of FPGA7: 594
Total extern cut of FPGA8: 630

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3959
----------------------------
Time for iteration 2: 77s
Score for iteration 2: 3959.00
Test succeeded for iteration 3.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 685
----------------------------
Resources of FPGA1: [ 748 101 84 98 80 95 145 56 ]
Resources of FPGA2: [ 736 169 167 183 171 147 214 207 ]
Resources of FPGA3: [ 674 279 188 172 196 192 163 254 ]
Resources of FPGA4: [ 395 84 100 74 68 40 85 84 ]
Resources of FPGA5: [ 48 66 35 51 65 102 98 101 ]
Resources of FPGA6: [ 741 191 120 115 124 127 152 138 ]
Resources of FPGA7: [ 731 184 144 148 155 167 103 183 ]
Resources of FPGA8: [ 740 111 89 94 114 79 79 126 ]
Total extern cut of FPGA1: 688
Total extern cut of FPGA2: 791
Total extern cut of FPGA3: 659
Total extern cut of FPGA4: 206
Total extern cut of FPGA5: 14
Total extern cut of FPGA6: 606
Total extern cut of FPGA7: 521
Total extern cut of FPGA8: 569

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3725
----------------------------
Time for iteration 3: 76s
Score for iteration 3: 3725.00
Test succeeded for iteration 4.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 663
----------------------------
Resources of FPGA1: [ 748 215 150 115 194 141 157 167 ]
Resources of FPGA2: [ 746 113 105 81 143 77 80 127 ]
Resources of FPGA3: [ 518 229 209 230 205 228 294 243 ]
Resources of FPGA4: [ 57 122 88 124 69 91 115 108 ]
Resources of FPGA5: [ 724 147 84 99 81 92 89 98 ]
Resources of FPGA6: [ 708 71 77 62 61 67 56 106 ]
Resources of FPGA7: [ 704 111 64 52 80 144 105 133 ]
Resources of FPGA8: [ 595 138 137 95 102 101 121 152 ]
Total extern cut of FPGA1: 681
Total extern cut of FPGA2: 494
Total extern cut of FPGA3: 374
Total extern cut of FPGA4: 18
Total extern cut of FPGA5: 488
Total extern cut of FPGA6: 590
Total extern cut of FPGA7: 806
Total extern cut of FPGA8: 613

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3724
----------------------------
Time for iteration 4: 71s
Score for iteration 4: 3724.00
Average Total Hop Length = 3814.50
Average Score = 3814.50
Score Variance = 9585.25
```

## priority queue

```cpp
eda240317@VM-0-14-ubuntu:~/LoReHyPar$ ./runmt.sh case02
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/eda240317/LoReHyPar/build
[ 71%] Built target LoReHyPar
[ 85%] Building CXX object CMakeFiles/partitioner.dir/main.cpp.o
[100%] Linking CXX executable partitioner
[100%] Built target partitioner
Build succeeded.
Test succeeded for iteration 1.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 671
----------------------------
Resources of FPGA1: [ 707 209 119 183 190 173 199 194 ]
Resources of FPGA2: [ 449 185 180 120 158 115 189 134 ]
Resources of FPGA3: [ 739 86 53 75 76 64 91 94 ]
Resources of FPGA4: [ 27 34 29 39 48 50 43 36 ]
Resources of FPGA5: [ 689 107 150 139 126 101 160 144 ]
Resources of FPGA6: [ 660 239 172 129 159 200 144 258 ]
Resources of FPGA7: [ 748 102 101 101 101 125 99 131 ]
Resources of FPGA8: [ 747 183 107 126 99 121 107 142 ]
Total extern cut of FPGA1: 557
Total extern cut of FPGA2: 209
Total extern cut of FPGA3: 563
Total extern cut of FPGA4: 15
Total extern cut of FPGA5: 738
Total extern cut of FPGA6: 663
Total extern cut of FPGA7: 734
Total extern cut of FPGA8: 486

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3651
----------------------------
Time for iteration 1: 74s
Score for iteration 1: 3651.00
Test succeeded for iteration 2.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 657
----------------------------
Resources of FPGA1: [ 748 99 112 95 115 138 130 136 ]
Resources of FPGA2: [ 710 172 168 101 158 150 192 181 ]
Resources of FPGA3: [ 660 100 77 92 82 93 135 116 ]
Resources of FPGA4: [ 50 87 66 83 57 39 95 60 ]
Resources of FPGA5: [ 747 154 100 109 117 110 108 163 ]
Resources of FPGA6: [ 732 79 98 69 111 81 95 81 ]
Resources of FPGA7: [ 671 256 195 171 171 224 155 258 ]
Resources of FPGA8: [ 431 162 110 128 83 98 115 119 ]
Total extern cut of FPGA1: 744
Total extern cut of FPGA2: 744
Total extern cut of FPGA3: 556
Total extern cut of FPGA4: 15
Total extern cut of FPGA5: 532
Total extern cut of FPGA6: 498
Total extern cut of FPGA7: 665
Total extern cut of FPGA8: 381

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3946
----------------------------
Time for iteration 2: 79s
Score for iteration 2: 3946.00
Test succeeded for iteration 3.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 651
----------------------------
Resources of FPGA1: [ 716 191 188 173 186 187 182 214 ]
Resources of FPGA2: [ 742 293 250 203 246 276 289 273 ]
Resources of FPGA3: [ 736 240 135 175 195 140 137 174 ]
Resources of FPGA4: [ 415 152 114 133 106 122 132 168 ]
Resources of FPGA5: [ 7 24 13 10 1 6 17 9 ]
Resources of FPGA6: [ 748 86 59 43 46 66 84 73 ]
Resources of FPGA7: [ 663 75 118 76 93 89 127 102 ]
Resources of FPGA8: [ 748 63 39 54 52 28 36 77 ]
Total extern cut of FPGA1: 559
Total extern cut of FPGA2: 667
Total extern cut of FPGA3: 514
Total extern cut of FPGA4: 209
Total extern cut of FPGA5: 7
Total extern cut of FPGA6: 735
Total extern cut of FPGA7: 729
Total extern cut of FPGA8: 554

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3554
----------------------------
Time for iteration 3: 73s
Score for iteration 3: 3554.00
Test succeeded for iteration 4.
Reading FPGA......
Input FPGA Finished: 8
Reading Node......
Input Node Finished: 600
Reading Net......
Input Net Finished: 1239
Reading Solution......
Input Topo Finished: 11
Reading FPGA......
Input Solution Finished: 663
----------------------------
Resources of FPGA1: [ 550 300 262 231 258 244 298 263 ]
Resources of FPGA2: [ 746 276 216 234 247 244 289 276 ]
Resources of FPGA3: [ 723 92 73 71 99 60 62 92 ]
Resources of FPGA4: [ 725 116 99 73 93 89 98 109 ]
Resources of FPGA5: [ 0 0 0 0 0 0 0 0 ]
Resources of FPGA6: [ 725 87 105 82 104 113 123 155 ]
Resources of FPGA7: [ 627 188 114 83 111 157 120 182 ]
Resources of FPGA8: [ 742 93 53 81 34 51 38 77 ]
Total extern cut of FPGA1: 387
Total extern cut of FPGA2: 688
Total extern cut of FPGA3: 495
Total extern cut of FPGA4: 512
Total extern cut of FPGA5: 0
Total extern cut of FPGA6: 781
Total extern cut of FPGA7: 650
Total extern cut of FPGA8: 515

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3908
----------------------------
Time for iteration 4: 73s
Score for iteration 4: 3908.00
Average Total Hop Length = 3764.75
Average Score = 3764.75
Score Variance = 27681.68
```

## priority queue with one more ghg

```cpp
```

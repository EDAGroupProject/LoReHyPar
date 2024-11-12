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
eda240317@VM-0-14-ubuntu:~/lhc/LoReHyPar$ ./runmt.sh case02
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/eda240317/lhc/LoReHyPar/build
[ 14%] Building CXX object CMakeFiles/LoReHyPar.dir/hypar.cpp.o
[ 28%] Building CXX object CMakeFiles/LoReHyPar.dir/coarsen.cpp.o
[ 42%] Building CXX object CMakeFiles/LoReHyPar.dir/initialpar.cpp.o
[ 57%] Building CXX object CMakeFiles/LoReHyPar.dir/refine.cpp.o
[ 71%] Linking CXX static library libLoReHyPar.a
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
Input Solution Finished: 662
----------------------------
Resources of FPGA1: [ 535 229 162 177 192 174 193 228 ]
Resources of FPGA2: [ 628 207 194 130 136 125 178 198 ]
Resources of FPGA3: [ 732 142 87 119 74 133 105 116 ]
Resources of FPGA4: [ 39 60 44 72 63 29 46 72 ]
Resources of FPGA5: [ 733 158 119 129 139 202 183 166 ]
Resources of FPGA6: [ 648 92 65 67 68 78 102 112 ]
Resources of FPGA7: [ 748 155 140 90 125 106 126 127 ]
Resources of FPGA8: [ 735 99 99 82 99 81 90 85 ]
Total extern cut of FPGA1: 511
Total extern cut of FPGA2: 627
Total extern cut of FPGA3: 529
Total extern cut of FPGA4: 14
Total extern cut of FPGA5: 817
Total extern cut of FPGA6: 559
Total extern cut of FPGA7: 540
Total extern cut of FPGA8: 497

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3853
----------------------------
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
Input Solution Finished: 653
----------------------------
Resources of FPGA1: [ 408 267 227 202 250 185 210 238 ]
Resources of FPGA2: [ 724 179 120 150 95 131 137 142 ]
Resources of FPGA3: [ 742 92 63 60 95 109 104 99 ]
Resources of FPGA4: [ 729 120 76 122 84 86 107 99 ]
Resources of FPGA5: [ 29 52 32 25 16 23 56 29 ]
Resources of FPGA6: [ 717 175 120 85 116 158 151 164 ]
Resources of FPGA7: [ 748 105 123 118 120 100 142 169 ]
Resources of FPGA8: [ 734 127 142 104 129 142 101 172 ]
Total extern cut of FPGA1: 293
Total extern cut of FPGA2: 625
Total extern cut of FPGA3: 627
Total extern cut of FPGA4: 530
Total extern cut of FPGA5: 16
Total extern cut of FPGA6: 820
Total extern cut of FPGA7: 790
Total extern cut of FPGA8: 494

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3755
----------------------------
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
Input Solution Finished: 648
----------------------------
Resources of FPGA1: [ 671 157 93 104 103 158 139 162 ]
Resources of FPGA2: [ 706 142 177 146 149 129 162 195 ]
Resources of FPGA3: [ 747 126 143 105 94 69 100 103 ]
Resources of FPGA4: [ 428 125 105 70 104 76 156 106 ]
Resources of FPGA5: [ 39 47 34 56 27 63 55 60 ]
Resources of FPGA6: [ 658 220 147 147 195 166 172 223 ]
Resources of FPGA7: [ 748 155 74 107 107 138 109 134 ]
Resources of FPGA8: [ 741 149 134 122 135 122 105 144 ]
Total extern cut of FPGA1: 556
Total extern cut of FPGA2: 742
Total extern cut of FPGA3: 498
Total extern cut of FPGA4: 388
Total extern cut of FPGA5: 16
Total extern cut of FPGA6: 671
Total extern cut of FPGA7: 689
Total extern cut of FPGA8: 592

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3776
----------------------------
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
Input Solution Finished: 675
----------------------------
Resources of FPGA1: [ 195 283 261 231 220 260 266 255 ]
Resources of FPGA2: [ 728 254 175 226 243 184 257 231 ]
Resources of FPGA3: [ 376 65 72 45 57 24 40 74 ]
Resources of FPGA4: [ 737 47 46 46 38 28 36 70 ]
Resources of FPGA5: [ 695 173 87 111 82 107 89 124 ]
Resources of FPGA6: [ 666 84 118 68 100 98 130 120 ]
Resources of FPGA7: [ 718 129 83 72 91 163 126 161 ]
Resources of FPGA8: [ 728 116 99 81 93 89 98 109 ]
Total extern cut of FPGA1: 17
Total extern cut of FPGA2: 555
Total extern cut of FPGA3: 203
Total extern cut of FPGA4: 563
Total extern cut of FPGA5: 572
Total extern cut of FPGA6: 729
Total extern cut of FPGA7: 796
Total extern cut of FPGA8: 533

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3968
----------------------------
Average Total Hop Length = 3838.00
```

## random first, pq next

```cpp
eda240317@VM-0-14-ubuntu:~/lhc/LoReHyPar$ ./runmt.sh case02
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/eda240317/lhc/LoReHyPar/build
[ 14%] Building CXX object CMakeFiles/LoReHyPar.dir/hypar.cpp.o
[ 28%] Linking CXX static library libLoReHyPar.a
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
Input Solution Finished: 684
----------------------------
Resources of FPGA1: [ 703 247 176 174 171 201 225 221 ]
Resources of FPGA2: [ 748 148 189 145 165 140 209 164 ]
Resources of FPGA3: [ 745 150 118 140 72 89 128 131 ]
Resources of FPGA4: [ 745 109 103 81 123 91 86 105 ]
Resources of FPGA5: [ 308 89 58 59 106 87 59 105 ]
Resources of FPGA6: [ 46 74 71 68 82 81 53 94 ]
Resources of FPGA7: [ 725 201 106 129 117 169 148 203 ]
Resources of FPGA8: [ 748 170 103 100 127 100 132 132 ]
Total extern cut of FPGA1: 761
Total extern cut of FPGA2: 745
Total extern cut of FPGA3: 524
Total extern cut of FPGA4: 495
Total extern cut of FPGA5: 285
Total extern cut of FPGA6: 15
Total extern cut of FPGA7: 707
Total extern cut of FPGA8: 534

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3862
----------------------------
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
Input Solution Finished: 664
----------------------------
Resources of FPGA1: [ 723 122 139 121 174 115 123 142 ]
Resources of FPGA2: [ 747 208 156 114 154 149 221 199 ]
Resources of FPGA3: [ 648 97 69 95 53 64 134 81 ]
Resources of FPGA4: [ 43 57 67 81 60 58 38 65 ]
Resources of FPGA5: [ 748 133 130 158 105 117 100 144 ]
Resources of FPGA6: [ 653 251 140 144 154 199 147 216 ]
Resources of FPGA7: [ 748 103 83 95 115 124 125 139 ]
Resources of FPGA8: [ 437 137 116 98 107 108 133 115 ]
Total extern cut of FPGA1: 509
Total extern cut of FPGA2: 742
Total extern cut of FPGA3: 553
Total extern cut of FPGA4: 15
Total extern cut of FPGA5: 529
Total extern cut of FPGA6: 668
Total extern cut of FPGA7: 748
Total extern cut of FPGA8: 377

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3856
----------------------------
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
Resources of FPGA1: [ 748 125 167 136 152 178 193 208 ]
Resources of FPGA2: [ 718 199 153 134 195 232 218 200 ]
Resources of FPGA3: [ 665 113 81 107 125 109 100 125 ]
Resources of FPGA4: [ 46 94 50 72 70 40 74 94 ]
Resources of FPGA5: [ 423 111 128 127 111 76 83 84 ]
Resources of FPGA6: [ 748 174 115 128 149 106 144 141 ]
Resources of FPGA7: [ 748 219 139 111 98 118 123 139 ]
Resources of FPGA8: [ 720 112 74 113 68 74 84 113 ]
Total extern cut of FPGA1: 739
Total extern cut of FPGA2: 818
Total extern cut of FPGA3: 540
Total extern cut of FPGA4: 14
Total extern cut of FPGA5: 205
Total extern cut of FPGA6: 630
Total extern cut of FPGA7: 662
Total extern cut of FPGA8: 525

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3762
----------------------------
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
Input Solution Finished: 668
----------------------------
Resources of FPGA1: [ 748 182 154 186 192 192 209 179 ]
Resources of FPGA2: [ 690 296 250 214 255 275 265 248 ]
Resources of FPGA3: [ 464 250 161 205 180 189 215 236 ]
Resources of FPGA4: [ 14 24 24 23 18 4 24 32 ]
Resources of FPGA5: [ 746 106 102 72 90 65 76 88 ]
Resources of FPGA6: [ 748 51 32 54 51 34 41 64 ]
Resources of FPGA7: [ 594 138 137 95 102 101 121 149 ]
Resources of FPGA8: [ 748 65 51 44 47 90 84 89 ]
Total extern cut of FPGA1: 514
Total extern cut of FPGA2: 713
Total extern cut of FPGA3: 385
Total extern cut of FPGA4: 5
Total extern cut of FPGA5: 469
Total extern cut of FPGA6: 554
Total extern cut of FPGA7: 615
Total extern cut of FPGA8: 850

Congratulations! Solution is legal.
----------------------------
Total Hop Length = 3918
----------------------------
Average Total Hop Length = 3849.50
```

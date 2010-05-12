./wiinandfuse $1 $2 -p -g0
ls -l -R $2 > $3_0
fusermount -u $2

./wiinandfuse $1 $2 -p -g1
ls -l -R $2 > $3_1
fusermount -u $2

./wiinandfuse $1 $2 -p -g2
ls -l -R $2 > $3_2
fusermount -u $2

./wiinandfuse $1 $2 -p -g3
ls -l -R $2 > $3_3
fusermount -u $2

./wiinandfuse $1 $2 -p -g4
ls -l -R $2 > $3_4
fusermount -u $2

./wiinandfuse $1 $2 -p -g5
ls -l -R $2 > $3_5
fusermount -u $2

./wiinandfuse $1 $2 -p -g6
ls -l -R $2 > $3_6
fusermount -u $2

./wiinandfuse $1 $2 -p -g7
ls -l -R $2 > $3_7
fusermount -u $2

./wiinandfuse $1 $2 -p -g8
ls -l -R $2 > $3_8
fusermount -u $2

./wiinandfuse $1 $2 -p -g9
ls -l -R $2 > $3_9
fusermount -u $2

./wiinandfuse $1 $2 -p -ga
ls -l -R $2 > $3_a
fusermount -u $2

./wiinandfuse $1 $2 -p -gb
ls -l -R $2 > $3_b
fusermount -u $2

./wiinandfuse $1 $2 -p -gc
ls -l -R $2 > $3_c
fusermount -u $2

./wiinandfuse $1 $2 -p -gd
ls -l -R $2 > $3_d
fusermount -u $2

./wiinandfuse $1 $2 -p -ge
ls -l -R $2 > $3_e
fusermount -u $2

./wiinandfuse $1 $2 -p -gf
ls -l -R $2 > $3_f
fusermount -u $2

diff $3_0 $3_1 > $3_diff01
diff $3_1 $3_2 > $3_diff12
diff $3_2 $3_3 > $3_diff23
diff $3_3 $3_4 > $3_diff34
diff $3_4 $3_5 > $3_diff45
diff $3_5 $3_6 > $3_diff56
diff $3_6 $3_7 > $3_diff67
diff $3_7 $3_8 > $3_diff78
diff $3_8 $3_9 > $3_diff89
diff $3_9 $3_a > $3_diff9a
diff $3_a $3_b > $3_diffab
diff $3_b $3_c > $3_diffbc
diff $3_c $3_d > $3_diffcd
diff $3_d $3_e > $3_diffde
diff $3_e $3_f > $3_diffef
diff $3_f $3_0 > $3_difff0

rm $3_0 $3_1 $3_2 $3_3 $3_4 $3_5 $3_6 $3_7 $3_8 $3_9 $3_a $3_b $3_c $3_d $3_e $3_f


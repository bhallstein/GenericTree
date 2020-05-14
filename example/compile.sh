clang++ -std=c++11 \
   example.cpp           \
   ../../../Diatom/Diatom.cpp      \
   ../../../Diatom/Diatom-Storage.cpp  \
   -I ../../../Diatom              \
   -I ../../ChunkVector \
   -o gt \
   -O3

clang++ -std=c++11 \
   example_ref.cpp           \
   ../../../Diatom/Diatom.cpp      \
   ../../../Diatom/Diatom-Storage.cpp  \
   -I ../../../Diatom              \
   -I ../../ChunkVector \
   -o gt_ref \
   -O3

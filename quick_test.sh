#! /bin/bash

echo -e "\nPerforming a compression/decompression test"
make
echo "Compressing file *$1*"
./main -c $1 test_compresso
echo -e "\nCompression done, starting decompression"
./main -d test_compresso test_decompresso
echo "Decompression done, checking diffs"
diff $1 test_decompresso
echo "Test finished, delete compressed and decompressed files (y/n)?"
read answer
if [ "$answer" = "y" ]; then
	rm -f test_compresso 
	rm -f test_decompresso
	echo "Files removed"
fi
make clean
echo -e "\n\n"

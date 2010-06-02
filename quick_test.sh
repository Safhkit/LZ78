#! /bin/bash

echo -e "\nPerforming a compression/decompression test"
make
echo "Compressing file *$1*"
./lz78 -c $1 -o test_compresso
echo -e "\nCompression done, starting decompression"
./lz78 -d test_compresso -o test_decompresso
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

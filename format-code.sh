#/bin/bash

clangf_version=""
for i in $(seq 11 -1 5)
do
	echo "search clang-format-${i}"
	clangf_version=$(which clang-format-${i} 2>/dev/null)
	if [ $? -eq 0 ] ; then
		break;
	fi
done

if [ -z "$clangf_version" ] ; then
	echo "No clang-format found"
	exit 1
fi

clangf_bin=$(which clang-format-${i})

find src -regex '.*\.\(cpp\|hpp\|cc\|cxx\|h\|inl\)' | xargs -I % $clangf_bin -i -style=file %
find tests -regex '.*\.\(cpp\|hpp\|cc\|cxx\|h\|inl\)' | xargs -I % $clangf_bin -i -style=file %
find include -regex '.*\.\(cpp\|hpp\|cc\|cxx\|h\|inl\)' | xargs -I % $clangf_bin -i -style=file %

exit 0

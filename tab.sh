#!/bin/sh

sed -i 's/\t/    /g' $(find -type f | grep -E "\.c$|\.h|\.cpp|.hpp|.json|.html|.css|.js|.conf")

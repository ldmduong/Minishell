function greet
  echo "function=$1"
endfunction

for value in alpha beta
  echo "loop=$value"
endfor

if 2 -gt 1
  echo "condition=pass"
else
  echo "condition=fail"
endif

if -f "$1"
  echo "file-check=pass"
endif

echo "script-output" > "$1"
cat "$1"
call greet "hello world"

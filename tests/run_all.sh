#!/usr/bin/env bash
set -euo pipefail

minishell=$(realpath "$1")
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

output=$(
    cd "$work_dir"
    "$minishell" <<'INPUT'
mkdir -p nested/dir
cd nested/dir
pwd
cd -
cd nested/dir
pwd > location.txt
cat location.txt
touch message.txt
echo "hello world" > message.txt
cat message.txt | wc -c
alias say='echo hi there'
say user
export MINISHELL_TEST=value
printenv MINISHELL_TEST
unset MINISHELL_TEST
ram
cpu
uptime
procs
threads demo 2
rm message.txt
rm location.txt
exit
INPUT
)

[[ "$output" == *"$work_dir/nested/dir"* ]]
[[ "$output" == *"12"* ]]
[[ "$output" == *"hi there user"* ]]
[[ "$output" == *"value"* ]]
[[ "$output" == *"RAM tổng"* ]]
[[ "$output" == *"CPU:"* ]]
[[ "$output" == *"Đã chạy:"* ]]
[[ "$output" == *"TÊN"* ]]
[[ "$output" == *"demo xong"* ]]
[[ ! -e "$work_dir/nested/dir/message.txt" ]]

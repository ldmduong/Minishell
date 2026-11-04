#!/usr/bin/env bash
set -euo pipefail

minishell=$(realpath "$1")
script_dir=$(cd -- "$(dirname -- "$0")" && pwd)
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT
mkdir -p "$work_dir/home"

output=$(
    cd "$work_dir"
    HOME="$work_dir/home" "$minishell" <<INPUT
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
bookmark add demo "$work_dir/nested/dir"
bookmark list
cd /
bookmark go demo
pwd
bookmark remove demo
base FF 16 2
base -10 10 16
color red
color reset
dance 1
run_script "$script_dir/scenarios/features.ms" "$work_dir/script-output.txt"
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
[[ "$output" == *"11111111"* ]]
[[ "$output" == *"-A"* ]]
[[ "$output" == *"\\o/"* ]]
[[ "$output" == *"loop=alpha"* ]]
[[ "$output" == *"loop=beta"* ]]
[[ "$output" == *"condition=pass"* ]]
[[ "$output" == *"function=hello world"* ]]
[[ "$output" == *"script-output"* ]]
[[ ! -e "$work_dir/nested/dir/message.txt" ]]
[[ ! -s "$work_dir/home/.minishell_bookmarks" ]]

schedule_output=$(
    printf 'schedule 1 echo should-not-run | cat\nexit\n' |
        HOME="$work_dir/home" "$minishell" 2>&1
)
[[ "$schedule_output" == *"chỉ dùng như lệnh độc lập"* ]]
[[ "$schedule_output" != *"should-not-run"* ]]

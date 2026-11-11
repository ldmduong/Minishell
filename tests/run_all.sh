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
[[ $(<"$work_dir/script-output.txt") == "script-output" ]]
[[ ! -e "$work_dir/nested/dir/message.txt" ]]
[[ ! -s "$work_dir/home/.minishell_bookmarks" ]]

schedule_output=$(
    printf 'schedule 1 echo should-not-run | cat\nexit\n' |
        HOME="$work_dir/home" "$minishell" 2>&1
)
[[ "$schedule_output" == *"chỉ dùng như lệnh độc lập"* ]]
[[ "$schedule_output" != *"should-not-run"* ]]

cd "$work_dir"
export HOME="$work_dir/home"

check_run() {
    local expected=$1 status=0
    output=$(printf '%s\n' "$2" | "$minishell" 2>&1) || status=$?
    if [[ "$status" != "$expected" ]]; then
        printf 'Expected exit %s, got %s for %s\n%s\n' \
            "$expected" "$status" "$2" "$output" >&2
        return 1
    fi
}

touch -- keep -i
check_run 1 'rm keep -i'
[[ -e keep && -e ./-i ]]
check_run 0 'rm -- -i'
[[ ! -e ./-i ]]

printf 'keep me\n' > source.txt
ln source.txt link.txt
(
    ulimit -f 128
    check_run 1 'cat source.txt >> link.txt'
)
[[ $(<source.txt) == 'keep me' ]]

ln -s "$work_dir/source.txt" "$HOME/.minishell_bookmarks.tmp"
check_run 0 'bookmark add safe .'
[[ $(<source.txt) == 'keep me' ]]
[[ $(<"$HOME/.minishell_bookmarks") == $'safe\t.' ]]

printf '%s\n' 'if 1 -eq 1' 'if invalid' 'endif' 'endif' \
    'echo should-not-run' > syntax.ms
check_run 2 'run_script syntax.ms'
[[ "$output" != *should-not-run* ]]

printf '%s\n' 'run_script recurse.ms' 'echo should-not-run' > recurse.ms
check_run 1 'run_script recurse.ms'
[[ "$output" == *'độ sâu gọi tệp'* && "$output" != *should-not-run* ]]

printf '%s\n' 'function f' 'function f' 'echo updated' 'endfunction' \
    'echo original' 'endfunction' 'call f' 'call f' > redefine.ms
check_run 0 'run_script redefine.ms'
[[ "$output" == $'original\nupdated' ]]

check_run 1 $'false\n  # comment\n  '
[[ -z "$output" ]]
check_run 7 'exit 7'
[[ -z "$output" ]]

check_run 0 $'head -c 4\nabc\necho done'
[[ "$output" == $'abc\ndone' ]]

check_run 1 'echo should-not-run > ""'
[[ "$output" != *should-not-run* ]]
check_run 0 'echo last > first.txt > last.txt'
[[ -f first.txt && ! -s first.txt && $(<last.txt) == last ]]
check_run 1 'echo x > created.txt < missing.txt'
[[ -f created.txt ]]
check_run 1 'pwd > /dev/full'
[[ "$output" == *'lỗi ghi đầu ra'* ]]
check_run 0 $'pwd > /dev/full\npwd'
[[ "$output" == *$'\n'"$work_dir" ]]

check_run 0 $'cd /\nprintenv PWD\nprintenv OLDPWD'
[[ "$output" == '/'$'\n'"$work_dir" ]]
check_run 0 $'bookmark add root /\nbookmark go root\nprintenv PWD'
[[ "$output" == '/' ]]

cat > quotes.ms <<'SCRIPT'
printf '%s\n' '$1' "$1" "\$1" \$1
SCRIPT
check_run 0 'run_script quotes.ms "two words"'
[[ "$output" == $'$1\ntwo words\n$1\n$1' ]]
printf '%s\n' 'echo should-not-run > "$1"' > empty-redirect.ms
check_run 1 'run_script empty-redirect.ms ""'
[[ "$output" != *should-not-run* ]]
printf '%s\n' 'if 1 -eq 1 | cat' 'echo should-not-run' 'endif' > invalid-header.ms
check_run 2 'run_script invalid-header.ms'
[[ "$output" != *should-not-run* ]]

(
    ulimit -n 64
    pipeline='printf x | '
    for ((i = 0; i < 35; ++i)); do
        pipeline+='cat | '
    done
    check_run 0 "$pipeline"'wc -c'
    [[ "$output" == 1 ]]
)

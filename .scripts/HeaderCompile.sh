#!/usr/bin/env bash
set -euo pipefail

ROOT=$(dirname "$(dirname "$0")")
INC="$ROOT/lib"

# ANSI colours: enabled when stdout is a TTY *or* FORCE_COLOR=1
if [[ -t 1 || "${FORCE_COLOR:-0}" == "1" ]]; then
    RED='\033[31m'; GREEN='\033[32m'; BLUE='\033[34m'
    BOLD='\033[1m'; RESET='\033[0m'
else
    RED=''; GREEN=''; BLUE=''; BOLD=''; RESET=''
fi

echo -e "${BOLD}${BLUE}Header check: compiling every .hpp under ptx/${RESET}"

fail=0
while IFS= read -r hdr; do
    printf "${BLUE}> %-60s${RESET}" "$hdr"
    if g++ -std=c++17 -Wno-pragmas \
          -I"$INC" -xc++ -c "$INC/$hdr" -o /dev/null 2>/tmp/err.log; then
        echo -e " ${GREEN}✓${RESET}"
    else
        echo -e " ${RED}✘${RESET}"
        sed 's/^/   /' /tmp/err.log
        fail=1
    fi
done < <(cd "$INC" && find ptx -name '*.hpp' | sort)

[[ $fail -eq 0 ]] && echo -e "${GREEN}${BOLD}All headers compile clean${RESET}" \
                  || echo -e "${RED}${BOLD}Header check failed${RESET}"
exit $fail

---
title: 'RFC: Bash quiz: $1 gleiche Zeichen $2'
date: '2011-09-28T07:48:05+02:00'
layout: post
categories: shell
---

Auf der Suche nach $1 gleichen Zeichen $2 hatte ich ursprünglich folgenden Code:
```bash
ruler () {  # count char
    local i
    for ((i=0;i<$1;i++))
    do
        echo -n "${2:-=}"
    done
}
```

Daraus ist dann irgendwann folgendes entstanden:
```bash
ruler () {  # count char
    local _r="${2:-=}"
    while [ "${#_r}" -lt "$1" ]
    do
        _r="${_r}${_r}"
    done
    _r="${_r//?/${2:-=}}"
    echo "${_r:0:$1}"
}
```

Oder doch gleich Python:
```bash
ruler () {  # count char
    python -c 'import sys;print sys.argv[2] * int(sys.argv[1])' "$1" "${2:-=}"
}
```

Folgendes wollte leider nicht:
```bash
ruler () {  # count char
    seq -f"${2:-=}" "$1"
}
```

Aber folgendes funktioniert:
```bash
ruler () {  # count char
    local t
    printf -v t '%*s' "$1"
    echo "${t// /${2:-=}}"
}
```

Kennt ihr ggf. einfachere/bessere/effizientere Varianten?

PS: RFC steht hier für „Request for Competition“, um mal für zwischendurch oder beim Radfahren eine kleine Herausforderung für das eigene Hirn zu haben 😉

{% include abbreviations.md %}

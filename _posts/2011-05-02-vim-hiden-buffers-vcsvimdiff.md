---
title: 'vim hiden buffers & VCSVimDiff'
date: '2011-05-02T10:29:23+02:00'
layout: post
categories: vim
---

Nach dem Pizza-Abend-Vortrag von Jan-Christoph ist irgendwann mal das `set hidden` in meine `~/.vimrc` gewandert, mit dem Dateien durch ein `:q` nicht sofort geschlossen werden, sondern in den Hintergrund wandern, so daß man ggf. schnell dort weitermachen kann, wenn man die Datei dann doch nochmal öffnen und weiter bearbeiten will.
Nachteilig war das allerdings im Zusammenhang mit VCS-Plugin, weil sich nach einem `VCSVimDiff` dieser Modus nicht mehr ordentlich beendet hat:
Die SVN-Datei blieb auch als Puffer geöffnet, was dazu geführt hat, das im Hintergrund diese weiterhin mit der Arbeitsdatei verglichen wurde.
Nach langer Suche habe ich dann `:bdelete` entdeckt, mit dem man einen Puffer selbst bei gesetzten `hidden` schließen kann.

{% include abbreviations.md %}

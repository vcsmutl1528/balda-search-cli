<?php

$fname = 'dic.dat';
$st = array ();				// массив статистики по длине слова
for ($i = 0; $i < 20; $i++)
	$st [$i] = 0;
$lst = array ();			// массив статистики по длине букв, подходящих от предыдущего слова
for ($i = 0; $i < 20; $i++)
	$lst [$i] = 0;

if ($argc >= 2) {
	$fname = $argv [1];
}
$f = fopen ($fname, 'rb');
if ($f === false) {
	echo "Dictionary file '$fname' open error.\n";
	exit (1);
}

$pw = "";		// предыдущее слово
$pc = 0;		// счетчик начальных букв, подходящих от предыдущего слова
$pc1 = 0;		// счетчик начальных букв без 1-й буквы, подходящих от предыдущего слова
$wc = 0;		// счетчик слов
while (true) {
$w = "";
while (($c = fgetc ($f)) !== false) {
	if ($c == "\0") break;
	$w .= $c;
}
$wc ++;

$wl = strlen ($w);
$lst [$wl] ++;
$l = strlen ($pw);
if ($l < $wl) $wl = $l;
for ($i = 0; $i < $wl; $i++)
	if ($w [$i] != $pw [$i]) break;
$pc += $i;
if ($i > 1) $pc1 += $i - 1;
$st [$i] ++;
if (feof ($f) || $c === false) break;
$pw = $w;
}
$wc --;
fclose ($f);
echo "Word count: $wc\n";
echo "Prev ch count: $pc\n";
echo "Prev ch count (no 1st letter): {$pc1}\n";
echo "Word length stat:\n";
for ($i = 2; $i <= 15; $i++) echo "$i:{$lst[$i]} ";
echo "\n";
echo "Prev ch length stat:\n";
for ($i = 0; $i <= 15; $i++) echo "$i:{$st[$i]} ";
echo "\n";
?>